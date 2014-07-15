#include "StdAfx.h"
#include "decmps_proc.h"

DecmpsProc::DecmpsProc(void){}

DecmpsProc::~DecmpsProc(void){}

void DecmpsProc::Analysis(IN cvi* src, IN cvi* maskMRF, OUT cvi* &shdwMask)
{
	shdwMask = cvci(maskMRF);

	//histogram analysis
	Hist hist(128, 0);
	ComputeHist(maskMRF, hist);
	vector<int> peeks(0);
	GetPeeks(hist, peeks, 0.2f, 15.0f);
	ShowHistInCvi(hist, peeks, 0.1f, "decmp_hist.png");

	//quantify
	Strt strt;
	Quantify(maskMRF, peeks, strt);

	//post-process
	int smoothSize = src->width / 200;
	cvi* edgeImg = cvci81(src);
	cvCvtColor(src, edgeImg, CV_BGR2GRAY);
	cvCanny(edgeImg, edgeImg, 100, 255, 3);
	//cvSmooth(edgeImg, edgeImg, 2, smoothSize, smoothSize);
	cvDilate(edgeImg, edgeImg, 0, smoothSize);
	cvsi("decmp_edge.png", edgeImg);
	strt.UpdateCellProperty(edgeImg);
	cvri(edgeImg);
	strt.Filter();

	ShowStrtImg(strt, "decmp_strt.png");

}

void Strt::UpdateCellProperty(cvi* edgeImg)
{
	doFv(i, cells)
	{
		cells[i].pixelNum = 0;
		cells[i].edgePixelNum = 0;
		cells[i].edgeWeight = 0;
	}

	doFcvi(strtImg, i, j)
	{
		int idx = CV_IMAGE_ELEM(strtImg, int, i, j);
		cells[idx].pixelNum++;

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
		cells[i].pixelNum /= strtImg->width * strtImg->height / cells.size();
		cells[i].edgeWeight /= cells[i].edgePixelNum;
	}
}

void Strt::Filter()
{
	//avgLuminance > 230
	doFv(i, cells)
	{
		if(cells[i].avgV > 230) cells[i].legal = false;
		if(cells[i].pixelNum < 0.4) cells[i].legal = false;
		if(cells[i].edgeWeight > 0.6) cells[i].legal = false;
	}

}

void DecmpsProc::ShowStrtImg(Strt& strt, string fileDir)
{
	int cellN = strt.cells.size();
	vector<cvS> rdmClrs(cellN, cvs(0, 0, 0));
	randInit();
	doF(i, cellN)
	{
		int v = _i (strt.cells[i].avgV * 1);
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
	peeks.push_back(0);
	int histN = hist.size();
	doF(i, histN)
	{
		if(i > 0 && i < histN - 1 && hist[i] > hist[i+1] && hist[i] > hist[i-1])
		{
			if(hist[i] < minValue) continue;
			float peek = 256 * (_f i + 0.5f) / histN;
			if(peek - peeks[peeks.size() - 1] < minGap)
				peeks[peeks.size() - 1] = _i ((peeks[peeks.size() - 1] + peek) / 2);
			else
				peeks.push_back(_i peek);
		}
	}
	if(255 - peeks[peeks.size() - 1] < minGap)
		peeks[peeks.size() - 1] = 255;
	else
		peeks.push_back(255);
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