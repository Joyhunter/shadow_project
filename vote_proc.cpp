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

void VoteProc::Vote(cvi* src)
{
	cvi* res = cvci323(m_width, m_height);
	cvZero(res);

	doFcvi(res, i, j)
	{
		if(j == 0) cout<<i<<" ";
		MultiCorr corrs = m_corrs.GetCorrsPerGrid(i, j);
		int nPatch = corrs.size();
		vector<pair<CvPoint, float> > lmncs;
		lmncs.reserve(nPatch+1);

		//get luminance
		Patch patch = PatchDistMetric::GetPatch(src, _f i, _f j, 1.f, 0.f, m_patchOffset);
		float lmnc = PatchLmncProc::GetAvgLmnc(patch);
		lmncs.push_back(make_pair(cvPoint(round(i), round(j)), lmnc));
		doF(i, nPatch)
		{
			Corr& v = corrs[i];
			if(v.dist > 30) continue;
			patch = PatchDistMetric::GetPatch(src, v.x, v.y, v.s, v.r, 
				m_patchOffset);
			lmnc = PatchLmncProc::GetAvgLmnc(patch);
			lmncs.push_back(make_pair(cvPoint(round(v.x), round(v.y)), lmnc));
		}

		//sorting
		sort(lmncs.begin(), lmncs.end(), [](const pair<CvPoint, float>& p1, 
			const pair<CvPoint, float>& p2){
				return p1.second < p2.second;
		});

		//vote
		float highest = lmncs[lmncs.size() - 1].second;
		doFv(i, lmncs)
		{
			CvPoint pos = lmncs[i].first;
			pos.x = clamp(pos.x, 0, m_height - 1);
			pos.y = clamp(pos.y, 0, m_width - 1);
			float ratio = lmncs[i].second / highest;
			cvS v = cvg2(res, pos.x, pos.y);
			v.val[0] += ratio;
			v.val[1] += 1;
			cvs2(res, pos.x, pos.y, v);
			//cout<<pos.x<<" "<<pos.y<<" "<<ratio<<endl;
		}

	}

	cvi* shadowMask = cvci81(m_width, m_height);
	cvSet(shadowMask, cvs(255));
	cvi* shadowCnfdc = cvci81(m_width, m_height);
	cvSet(shadowCnfdc, cvs(0));
	doFcvi(shadowMask, i, j)
	{
		auto v = cvg2(res, i, j);
		if(v.val[1] != 0)
		{
			float alpha = _f v.val[0] / _f v.val[1] * 255;
			cvs2(shadowMask, i, j, cvs(alpha));
		}
		cvs2(shadowCnfdc, i, j, cvs(v.val[1] * 10));
	}
	cvsi("shadowMask.png", shadowMask);
	cvsi("shadowCnfdc.png", shadowCnfdc);

	cvri(shadowMask);
	cvri(shadowCnfdc);
	cvri(res);
}