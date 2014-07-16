#include "StdAfx.h"
#include "decmps_proc.h"
#include "mrf_proc.h"

DecmpsProc::DecmpsProc(void){}

DecmpsProc::~DecmpsProc(void){}

void DecmpsProc::Analysis(IN cvi* src, IN cvi* maskMRF, OUT cvi* &shdwMask)
{
	shdwMask = cvci(maskMRF);

	//histogram analysis
	Hist hist(128, 0);
	ComputeHist(maskMRF, hist);
	vector<int> peeks(0);
	GetPeeks(hist, peeks, 0.2f, 0.0f);
	ShowHistInCvi(hist, peeks, 0.1f, "decmp_hist.png");

	//quantify
	Strt strt;
	Quantify(maskMRF, peeks, strt);

	//post-process
	strt.adjacentRadius = src->width / 50;
	strt.UpdateCellProperty(src);
	strt.Filter();
	ShowStrtImg(strt, "decmp_strt.png");
	vector<Region> regions = strt.GetRegion();
	doFs(i, 1, _i regions.size())
	{
		regions[i].EstmtAlpha(strt.cells, 0.25f);
		cvi* guidanceAlphaMap = regions[i].GetGidcAlphaMap(strt.strtImg);

		int smoothSize = src->width / 10;
		if(smoothSize % 2 == 0) smoothSize++;
		cvSmooth(guidanceAlphaMap, guidanceAlphaMap, 2, smoothSize, smoothSize);
		cvsi("decmp_adjReg_" + toStr(i) + ".png", guidanceAlphaMap);

		cvi* resMask;
		MRFProc mProc;
		mProc.SolveWithInitAndGidc(src, maskMRF, guidanceAlphaMap, 64, resMask);

		cvCopy(resMask, shdwMask);
		cvri(guidanceAlphaMap);
		cvri(resMask);
		return;
	}
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
		queue<int> idxs;
		idxs.push(i);
		r.cellIdx.push_back(i);
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
		if(cells[i].avgV > 200) cells[i].legal = false;
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

void DecmpsProc::GetPeeks(Hist& hist, vector<int>& peeks, float minValue, float minGap)
{
	int histN = hist.size();

	vector<pair<float, int> > peekOri;
	float minRatio = 1.0f;
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

	minRatio = 0.85f;
	doFv(i, peekOri)
	{
		if((i == 0 || peekOri[i].first > peekOri[i-1].first * minRatio)
			&& (i == peekOri.size() -1 || peekOri[i].first >= peekOri[i+1].first * minRatio))
			peeks.push_back(peekOri[i].second);
	}

}

void DecmpsProc::ComputeHist(IN cvi* mask, OUT Hist& hist)
{
	int histN = hist.size();
	doFv(i, hist) hist[i] = 0;
	doFcvi(mask, i, j)
	{
		double v = cvg20(mask, i, j);
		int idx = _i floor(v * histN / 255);
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