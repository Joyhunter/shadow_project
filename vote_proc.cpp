#include "StdAfx.h"
#include "vote_proc.h"

VoteProc::VoteProc(void)
{
}

VoteProc::~VoteProc(void)
{
}

void VoteProc::LoadCorrs(string fileStr)
{
	m_corrs.Load(fileStr);
	DenseCorr* v = m_corrs.GetValue(0, 0);
	if(v != NULL)
	{
		m_width = v->m_width;
		m_height = v->m_height;
		m_patchOffset = v->m_patchOffset;
	}
	//cout<<m_width<<" "<<m_height<<endl;
}

void VoteProc::Vote(cvi* src, OUT cvi* &mask, OUT cvi* &cfdcMap, 
	float distThres, float distThresRatio)
{
	mask = cvci323(m_width, m_height);
	cvZero(mask);

	cvi* _test = cvci81(m_width, m_height);
	float distThresBak = distThres;
	doFcvi(mask, i, j)
	{
		if(j == 0) cout<<"\rVoting: "<<i*100/mask->height<<"%.";
		MultiCorr corrs = m_corrs.GetCorrsPerGrid(i, j);
		int nPatch = corrs.size();
		vector<pair<CvPoint, pair<float, float> > > lmncs;
		lmncs.reserve(nPatch+1);

		//get distance thres
		vecF dists(corrs.size());
		doFv(k, dists) dists[k] = corrs[k].dist;
		sort(dists.begin(), dists.end());
		float distThresSpatial = dists[clamp(_i(_f dists.size() * distThresRatio), 
			0, _i dists.size()-1)];
		distThres = min2(distThresBak, distThresSpatial);
		cvs20(_test, i, j, distThresSpatial);

		//get luminance
		Patch patch = PatchDistMetric::GetPatch(src, _f i, _f j, 1.f, 0.f, m_patchOffset);
		float lmnc = PatchLmncProc::GetAvgLmnc(patch);
		float satu = PatchLmncProc::GetAvgSatu(patch);
		lmncs.push_back(make_pair(cvPoint(round(i), round(j)), make_pair(lmnc, satu) ));
		doF(k, nPatch)
		{
			Corr& v = corrs[k];
			if(v.dist > distThres) continue;
			patch = PatchDistMetric::GetPatch(src, v.x, v.y, v.s, v.r, 
				m_patchOffset);
			lmnc = PatchLmncProc::GetAvgLmnc(patch);
			satu = PatchLmncProc::GetAvgSatu(patch);
			lmncs.push_back(make_pair(cvPoint(round(v.x), round(v.y)), make_pair(lmnc, satu)));
		}

		//sorting
		sort(lmncs.begin(), lmncs.end(), [](const pair<CvPoint, pair<float, float> >& p1, 
			const pair<CvPoint, pair<float, float>>& p2){
				return p1.second.first < p2.second.first;
		});

		//vote TODO: confidence = f(coor.dist)
		pair<float, float> highest = lmncs[lmncs.size() - 1].second;
		doFv(i, lmncs)
		{
			CvPoint pos = lmncs[i].first;
			pos.x = clamp(pos.x, 0, m_height - 1);
			pos.y = clamp(pos.y, 0, m_width - 1);
			float ratio = lmncs[i].second.first / highest.first;
			float ratio2 = lmncs[i].second.second / highest.second;
			cvS v = cvg2(mask, pos.x, pos.y);
			v.val[0] += ratio;
			v.val[1] += ratio2;
			v.val[2] += 1;
			cvs2(mask, pos.x, pos.y, v);
			//cout<<pos.x<<" "<<pos.y<<" "<<ratio<<endl;
		}

	}
	cout<<"\rVoting complete.\n";

	//cvsi("_test.png", _test);
	
	cfdcMap = cvci81(m_width, m_height);
	cvSet(cfdcMap, cvs(0));
	doFcvi(mask, i, j)
	{
		auto v = cvg2(mask, i, j);
		cvs2(cfdcMap, i, j, cvs(v.val[2] * 10));
		if(v.val[1] != 0)
		{
			float alpha = _f v.val[0] / _f v.val[2] * 255;
			float alpha2 = _f v.val[1] / _f v.val[2] * 255;
			cvs2(mask, i, j, cvs(alpha, alpha2, alpha));
		}
	}

	cvsic("_mask_1init.png", mask, 0);
	PostProcess(mask, cfdcMap);

	cvri(_test);
}

void VoteProc::PostProcess(cvi* &mask, cvi* &cfdcMap)
{
	//border
	doFcvi(mask, i, j)
	{
		if(i < m_patchOffset || i > m_height - 1 - m_patchOffset 
			|| j < m_patchOffset || j > m_width - 1 - m_patchOffset)
		{
			cvs2(mask, i, j, cvg2(mask, 
				clamp(i, m_patchOffset, m_height - 1 - m_patchOffset), 
				clamp(j, m_patchOffset, m_width - 1 - m_patchOffset)));
		}
	}

	//average using confidence
	int winSize = 5;
	cvi* mask2 = cvci(mask);
	doFcvi(mask, i, j)
	{
		if(cvg20(cfdcMap, i, j) > 50) continue;
		if(cvg20(mask, i, j) < 150) continue;
		cvS v1 = cvs(0, 0, 0); float v2 = 0;
		doFs(oi, -winSize, winSize + 1) doFs(oj, -winSize, winSize + 1)
		{
			if(cvIn(i + oi, j+ oj, mask))
			{
				float weight = 1;//_f cvg20(cfdcMap, i+oi, j+oj) / 10;
				v1 += cvg2(mask, i+oi, j+oj) * weight;
				v2 += weight;
			}
		}
		cvs2(mask2, i, j, v1 / v2);
	}
	cvCopy(mask2, mask);

}