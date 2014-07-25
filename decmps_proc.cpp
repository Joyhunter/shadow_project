#include "StdAfx.h"
#include "decmps_proc.h"
#include "mrf_proc.h"

DecmpsProc::DecmpsProc(void){}

DecmpsProc::~DecmpsProc(void){}

void DecmpsProc::Analysis(IN cvi* src, IN cvi* maskMRF, OUT cvi* &shdwMask, 
	IN float peekRatio, IN int nLabels)
{
	cout<<"Decomposing...\n";
	shdwMask = cvci(maskMRF);

	//histogram analysis
	Hist hist(64, 0);
	ComputeHist(maskMRF, hist);
	vector<int> peeks(0);
	GetPeeks(hist, peeks, 0.2f, peekRatio);
	//GetPeeksNorm(peeks, 10);
	ShowHistInCvi(hist, peeks, 0.1f, "decmp_hist.png");

	//quantify
	Strt strt;
	Quantify(maskMRF, peeks, strt);
	cout<<"  Quantify using "<<peeks.size()<<" peeks.\n";

	//post-process
	strt.adjacentRadius = src->width / 50; //50 param2 : adjacent radius
	strt.UpdateCellProperty(src);
	strt.Filter();
	ShowStrtImg(strt, "decmp_strt.png");
	vector<Region> regions = strt.GetRegion();
	cout<<"  Get "<<regions.size()<<" regions.\n";
	float maxPixel = 0; int maxRegIdx = -1;
	doFv(i, regions)
	{
		if(regions[i].pixelNum > maxPixel)
		{
			maxPixel = regions[i].pixelNum;
			maxRegIdx = i;
		}
	}
	cout<<"  MaxPixelRegIdx is "<<maxRegIdx<<".\n";
	doFs(i, 0, _i regions.size())
	{
		cout<<"  Solving mrf with the "<<i<<"th region, ";
		regions[i].EstmtAlpha(strt.cells, 0.25f);
		cvi* guidanceAlphaMap = regions[i].GetGidcAlphaMap(strt.strtImg);
		int smoothSize = GetSmoothSize(maskMRF, guidanceAlphaMap, 3); 

		cout<<"boundary width is "<<smoothSize;
		cvsi("decmp_adjReg_" + toStr(i) + ".png", guidanceAlphaMap);

		if(i == maxRegIdx)
		{
			cvi* boundMask = GetBounaryMask(guidanceAlphaMap, smoothSize);

			cvi* resMask;
			MRFProc mProc;
			mProc.SolveWithInitAndGidc(src, maskMRF, guidanceAlphaMap, boundMask, nLabels, resMask, smoothSize);
			cvsic("_mask_4decop.png", resMask, 0);
			cvsi("_ex_mask_decop.png", shdwMask);
			shdwMask = cvlic("_ex_mask_decop.png");

			SmoothBoundary(maskMRF, guidanceAlphaMap, resMask, smoothSize);
			cvCopy(resMask, shdwMask);
			cvri(resMask);
			cvri(boundMask);
		}

		cvri(guidanceAlphaMap);
		cout<<"\r  Solving mrf with the "<<i<<"th region complete.\n";
	}
}

void DecmpsProc::SmoothBoundary(cvi* maskMRF, cvi* gdcMask, cvi* &resMask, int smoothSize)
{
	cvi* temp = cvci81(maskMRF);
	doFcvi(temp, i, j)
	{
		if(cvg20(gdcMask, i, j) < 255)
			cvs20(temp, i, j, 255);
		else
			cvs20(temp, i, j, 0);
	}
	if(smoothSize % 2 == 0) smoothSize++;
	cvSmooth(temp, temp, CV_GAUSSIAN, smoothSize, smoothSize);
	cvi* shdwMask = cvci(resMask);
	cvSmooth(shdwMask, shdwMask, CV_BLUR, smoothSize, smoothSize);
	doFcvi(resMask, i, j)
	{
		float alpha = _f cvg20(temp, i, j) / 255.0f;
		alpha = clamp(alpha*2 - 1, 0.0f, 1.0f);
		cvS s = cvg2(resMask, i, j);
		cvS s2 = cvg2(shdwMask, i, j);
		s = s * alpha + s2 * (1.0f-alpha);
		cvs2(resMask, i, j, s);
	}
	//cvsic("autoS.png", shdwMask2);
	//cvsi(temp);
	cvri(shdwMask);
	cvri(temp);
}

bool operator < (const CvPoint& v1, const CvPoint& v2)
{
	if(v1.x < v2.x) return true;
	else return (v1.y < v2.y);
}
bool operator == (const CvPoint& v1, const CvPoint& v2)
{
	return (v1.x == v2.x) && (v1.y == v2.y);
}

int DecmpsProc::GetSmoothSize(cvi* maskMRF, cvi* &guidanceAlphaMap, int pixelStep)
{
	set<CvPoint> bps;
	int alpha = 0;
	int radius = pixelStep;
	doFcvi(guidanceAlphaMap, i, j)
	{
		if(cvg20(guidanceAlphaMap, i, j) == 255) continue;
		alpha = _i cvg20(guidanceAlphaMap, i, j);
		doFs(oi, -radius, radius) doFs(oj, -radius, radius)
		{
			if(cvIn(i+oi, j+oj, guidanceAlphaMap) && cvg20(guidanceAlphaMap, i+oi, j+oj) == 255) 
				bps.insert(cvPoint(i+oi, j+oj));
		}
	}
	if(bps.size() == 0) return 0;

	float avgShdwBak = 0;
	int res = 0;
	while(1)
	{
		float avgShdw = 0;
		for(auto p = bps.begin(); p != bps.end(); p++)
		{
			avgShdw += _f cvg20(maskMRF, p->x, p->y);
			cvs20(guidanceAlphaMap, p->x, p->y, alpha);
		}
		avgShdw /= bps.size();
		//cout<<avgShdw<<endl;

		set<CvPoint> bps2 = bps;
		bps.clear();
		for(auto p = bps2.begin(); p != bps2.end(); p++)
		{
			int i = p->x, j = p->y;
			doFs(oi, -radius, radius) doFs(oj, -radius, radius)
			{
				if(cvIn(i+oi, j+oj, guidanceAlphaMap) && cvg20(guidanceAlphaMap, i+oi, j+oj) == 255) 
					bps.insert(cvPoint(i+oi, j+oj));
			}
		}

		if(bps.size() == 0) return res;
		if(avgShdw <= avgShdwBak) return res;
		res += radius;
		avgShdwBak = avgShdw;
	}
}

cvi* DecmpsProc::GetBounaryMask(cvi* mask, int r)
{
	cvi* res = cvci(mask); 
	cvZero(res);
	doFcvi(mask, i, j)
	{
		if(cvg20(mask, i, j) != 255) 
			cvs20(res, i, j, 255);
	}
	cvi* res2 = cvci(res);
	//cvErode(res, res, 0, r);
	cvDilate(res2, res2, 0, r);
	doFcvi(res, i, j)
	{
		if(cvg20(res2, i, j) > 0 && cvg20(res, i, j) == 0)
			cvs20(res, i, j, 255);
		else
			cvs20(res, i, j, 0);
	}
	cvri(res2);
	return res;
}

void Region::EstmtAlpha(vector<Cell>& cells, float ratio)
{
	vector<pair<int, float> > vs;
	pixelNum = 0;
	doFv(i, cellIdx)
	{
		Cell& c = cells[cellIdx[i]];
		vs.push_back(make_pair(c.avgV, c.pixelNum));
		pixelNum += c.pixelNum;
	}
	sort(vs.begin(), vs.end(), [](const pair<int, float>& v1, const pair<int, float>& v2){
		return v1.first > v2.first;
	});

	float pixelNumEst = pixelNum * ratio;
	doFv(i, vs)
	{
		pixelNumEst -= vs[i].second;
		if(pixelNumEst < 0)
		{
			estmtedAlpha = _f vs[i].first;
			return;
		}
	}
}

cvi* Region::GetGidcAlphaMap(cvi* strtImg)
{
	cvi* res = cvci81(strtImg);
	cvZero(res);
	unordered_set<int> sets;
	doFv(i, cellIdx) sets.insert(cellIdx[i]);
	doFcvi(strtImg, i, j)
	{
		int idx = CV_IMAGE_ELEM(strtImg, int, i, j);
		if(sets.count(idx) > 0) cvs20(res, i, j, estmtedAlpha);
		else cvs20(res, i, j, 255);
	}
	return res;
}

vector<Region> Strt::GetRegion()
{
	vector<Region> res(0);

	vector<bool> used(cells.size(), false);
	doFv(i, cells)
	{
		if(cells[i].legal == false || used[i] == true) continue;
		
		Region r;
		r.pixelNum = 0;
		queue<int> idxs;
		idxs.push(i);
		r.cellIdx.push_back(i);
		r.pixelNum += cells[i].pixelNum;
		used[i] = true;
		while(!idxs.empty())
		{
			int idx = idxs.front();
			idxs.pop();
			for(auto p = cells[idx].adjacents.begin(); p != cells[idx].adjacents.end(); p++)
			{
				int idxAd = *p;
				if(used[idxAd] == true) continue;
				//add adjacent darker regions (no matter they are legal or not)
				if(cells[idxAd].legal == false && cells[idxAd].avgV > cells[idx].avgV) continue;
				idxs.push(idxAd);
				r.cellIdx.push_back(idxAd);
				r.pixelNum += cells[idxAd].pixelNum;
				used[idxAd] = true;
			}
		}
		res.push_back(r);
	}

	return res;
}

void Strt::UpdateCellProperty(cvi* src)
{

	int smoothSize = src->width / 200;
	cvi* edgeImg = cvci81(src);
	cvCvtColor(src, edgeImg, CV_BGR2GRAY);
	cvCanny(edgeImg, edgeImg, 100, 255, 3);
	//cvSmooth(edgeImg, edgeImg, 2, smoothSize, smoothSize);
	cvDilate(edgeImg, edgeImg, 0, smoothSize);
	//cvsi("decmp_edge.png", edgeImg);

	doFv(i, cells)
	{
		cells[i].pixelNum = 0;
		cells[i].edgePixelNum = 0;
		cells[i].edgeWeight = 0;
		cells[i].minX = edgeImg->height;
		cells[i].maxX = 0;
		cells[i].minY = edgeImg->width;
		cells[i].maxY = 0;
		cells[i].adjacents.clear();
	}

	doFcvi(strtImg, i, j)
	{
		int idx = CV_IMAGE_ELEM(strtImg, int, i, j);
		cells[idx].pixelNum++;

		cells[idx].maxX = max2(cells[idx].maxX, i);
		cells[idx].minX = min2(cells[idx].minX, i);
		cells[idx].maxY = max2(cells[idx].maxY, j);
		cells[idx].minY = min2(cells[idx].minY, j);

		set<int>& vs = cells[idx].adjacents;
		doFs(oi, -adjacentRadius, adjacentRadius) doFs(oj, -adjacentRadius, adjacentRadius)
		{
			if(cvIn(i+oi, j+oj, src) && CV_IMAGE_ELEM(strtImg, int, i+oi, j+oj) != idx)
			{
				int idx2 = CV_IMAGE_ELEM(strtImg, int, i+oi, j+oj);
				if(vs.find(idx2) == vs.end()) vs.insert(idx2);
			}
		}

		if((i > 0 && CV_IMAGE_ELEM(strtImg, int, i-1, j) != idx)
			|| (i < strtImg->height - 1 && CV_IMAGE_ELEM(strtImg, int, i+1, j) != idx)
			|| (j > 0 && CV_IMAGE_ELEM(strtImg, int, i, j-1) != idx)
			|| (j < strtImg->width - 1 && CV_IMAGE_ELEM(strtImg, int, i, j+1) != idx))
		{
			cells[idx].edgePixelNum ++;
			cells[idx].edgeWeight += _f cvg20(edgeImg, i, j) / 255.0f;
		}
	}

	doFv(i, cells)
	{
		cells[i].width  = cells[i].maxY - cells[i].minY + 1;
		cells[i].height = cells[i].maxX - cells[i].minX + 1;
		cells[i].ratio = _f cells[i].pixelNum / cells[i].width / cells[i].height;
		cells[i].pixelNum /= strtImg->width * strtImg->height / cells.size();
		cells[i].edgeWeight /= cells[i].edgePixelNum;
	}
	cvri(edgeImg);
}

void Strt::Filter()
{
	//avgLuminance > 230
	doFv(i, cells)
	{
		if(cells[i].avgV > 200) cells[i].legal = false; // 200
		if(cells[i].pixelNum < 0.4) cells[i].legal = false;
		if(cells[i].ratio < 0.2) cells[i].legal = false;
		//if(cells[i].edgeWeight > 0.6) cells[i].legal = false;
	}

}

void DecmpsProc::ShowStrtImg(Strt& strt, string fileDir)
{
	int cellN = strt.cells.size();
	vector<cvS> rdmClrs(cellN, cvs(0, 0, 0));
	randInit();
	doF(i, cellN)
	{
		int v = _i (strt.cells[i].ratio * 100);
		rdmClrs[i] = cvs(v, v, v); //continue;

		rdmClrs[i] = cvs(rand1()*128+127, rand1()*128+127, rand1()*128+127);
		if(!strt.cells[i].legal)
		{
			rdmClrs[i] = rdmClrs[i] * 0.2f;
			continue;
		}
	}

	cvi* forShow = cvci83(strt.strtImg);
	doFcvi(forShow, i, j)
		cvs2(forShow, i, j, rdmClrs[CV_IMAGE_ELEM(strt.strtImg, int, i, j)]);

	cvsi(fileDir, forShow);
	cvri(forShow);
}

void DecmpsProc::Quantify(cvi* maskMRF, vector<int>& peeks, Strt& strt)
{
	strt.strtImg = cvci321(maskMRF);
	
	vector<int> pixelMapping(255, 0);
	int idx = 0;
	doF(i, 256)
	{
		if(idx == peeks.size() - 1)
		{
			pixelMapping[i] = peeks.size() - 1;
			continue;
		}
		if(i - peeks[idx] <= peeks[idx+1] - i)
			pixelMapping[i] = idx;
		else
		{
			pixelMapping[i] = idx + 1;
			idx++;
		}
	}

	doFcvi(maskMRF, i, j)
	{
		int v = _i cvg20(maskMRF, i, j);
		CV_IMAGE_ELEM(strt.strtImg, int, i, j) = pixelMapping[v];
	}

	//flood fill
	int sIdx = peeks.size() * 2;
	strt.cells.resize(0);
	idx = 0;
	doFcvi(strt.strtImg, i, j)
	{
		int v = CV_IMAGE_ELEM(strt.strtImg, int, i, j);
		if(v >= sIdx) continue;
		cvFloodFill(strt.strtImg, cvPoint(j, i), cvs(sIdx + idx));
		strt.cells.resize(strt.cells.size() + 1);
		Cell& cell = strt.cells[strt.cells.size() - 1];;
		cell.avgV = peeks[v];
		idx++;
	}

	doFcvi(strt.strtImg, i, j)
	{
		int v = _i cvg20(strt.strtImg, i, j);
		CV_IMAGE_ELEM(strt.strtImg, int, i, j) = v - sIdx;
	}

}

void DecmpsProc::GetPeeks(Hist& hist, vector<int>& peeks, float minValue, float minRatio)
{
	int histN = hist.size();

	vector<pair<float, int> > peekOri;
	doF(i, histN)
	{
		if((i == 0 || hist[i] > hist[i-1] * minRatio)
			&& (i == histN -1 || hist[i] >= hist[i+1] * minRatio))
		{
			if(hist[i] < minValue) continue;
			float peek = 256 * (_f i + 0.5f) / histN;
			peekOri.push_back(make_pair(hist[i], _i peek));
		}
	}
	if(peekOri.size() <= 3)
	{
		doFv(i, peekOri) peeks.push_back(peekOri[i].second);
		return;
	}

	//minRatio = 0.85f;
	doFv(i, peekOri)
	{
		if((i == 0 || peekOri[i].first > peekOri[i-1].first * minRatio)
			&& (i == peekOri.size() -1 || peekOri[i].first >= peekOri[i+1].first * minRatio))
			peeks.push_back(peekOri[i].second);
	}

}

void DecmpsProc::GetPeeksNorm(vector<int>& peeks, int N)
{
	peeks.resize(N);
	doF(i, N)
	{
		peeks[i] = 255 * i / (N-1);
	}
}

void DecmpsProc::ComputeHist(IN cvi* mask, OUT Hist& hist)
{
	int histN = hist.size();
	doFv(i, hist) hist[i] = 0;
	doFcvi(mask, i, j)
	{
		double v = cvg20(mask, i, j);
		int idx = _i floor(v * (histN-1) / 255);
		hist[idx] += 1;
	}
	doFv(i, hist) hist[i] /= mask->width * mask->height / histN;
}

void DecmpsProc::ShowHistInCvi(IN Hist& hist, IN vector<int>& peeks, float heightRatio, 
	string fileDir)
{
	int histWidth = 5;
	int showHeight = 100;
	int gap = 1;
	cvi* img = cvci83(histWidth * hist.size(), showHeight);
	cvZero(img);
	int idx = 0;
	doFv(i, hist)
	{
		cvS color = cvs(255, 255, 255);
		if(idx < _i hist.size() 
			&& fabs(_f i + 0.5f - _f peeks[idx] * _f hist.size() / 256.f) <= 0.5f )
		{
			color = cvs(0, 255, 255);
			idx++;
		}
		int histHeight = _i (heightRatio * showHeight * hist[i]);
		cvRectangle(img, cvPoint(i*histWidth + gap, showHeight-histHeight),
			cvPoint((i+1)*histWidth - gap, showHeight), color, -1);
	}
	cvsi(fileDir, img);
	cvri(img);
}
