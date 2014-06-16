#include "StdAfx.h"
#include "gpm_proc.h"

int nCores = 7;

Interval::Interval(float min, float max):min(min), max(max)
{
}

float Interval::RandValue()
{
	return rand1() * (max - min) + min;
}

DenseCorr::DenseCorr(int w, int h, int knn, int patchOffset):m_width(w), m_height(h), 
	m_knn(knn), m_patchOffset(patchOffset)
{
	m_values.resize(m_width * m_height * m_knn);
}

void DenseCorr::RandomInitialize(Interval wItvl, Interval hItvl, 
	Interval sItvl, Interval rItrl)
{
	int idx = 0;

	randInit();
	doF(i, m_height) doF(j, m_width) doF(k, m_knn)
	{
		m_values[idx].x = hItvl.RandValue();
		m_values[idx].y = wItvl.RandValue();
		m_values[idx].s = sItvl.RandValue();
		m_values[idx].r = rItrl.RandValue();
		idx++;
	}
// 	idx = GetCorrIdx(3, 3);
// 	m_values[idx].x = 3; m_values[idx].y = 3; 
// 	m_values[idx].s = 1; m_values[idx].r = 0; 
}

void DenseCorr::ShowCorr(string imgStr)
{
	cvi* result = cvci(cvSize(m_width, m_height), 8, 3);
	doFcvi(result, i, j)
	{
		Corr& v = m_values[(i*m_width + j) * m_knn];
		cvs2(result, i, j, cvs(0, v.y / m_width * 255.0, v.x / m_height * 255.0));
	}
	cvsi(imgStr, result);
	cvri(result);
}

void DenseCorr::ShowCorrDist(string imgStr)
{
	cvi* result = cvci(cvSize(m_width, m_height), 8, 1);
	doFcvi(result, i, j)
	{
		cvs2(result, i, j, cvs(m_values[(i*m_width + j) * m_knn].dist));
	}
	cvsi(imgStr, result);
	cvri(result);
}

void DenseCorr::UpdatePatchDistance(cvi* dst, cvi* src)
{
	int idx = 0;
	RegularPatchDistMetric metric;

	omp_set_num_threads(nCores);
#pragma omp parallel for
	doF(i, m_height) doF(j, m_width)
	{
		doF(k, m_knn)
		{
			Corr& v = m_values[(i*m_width + j) * m_knn + k];
			v.dist = metric.ComputePatchDist(dst, _f(i), _f(j), 
				src, v.x, v.y, v.s, v.r, m_patchOffset);
			idx++;
		}
		sort(m_values.begin() + (i*m_width + j) * m_knn, 
			m_values.begin() + (i*m_width + j + 1) * m_knn);
	}
}

void DenseCorr::AddCoor(int r, int c, float cx, float cy, float cs, float cr, 
	float cdist)
{
	int sIdx = GetCorrIdx(r, c);
	int eIdx = sIdx + m_knn;
	while(m_values[sIdx].dist < cdist && sIdx < eIdx) sIdx++;
	if(sIdx >= eIdx) return;
	eIdx--;
	while(eIdx > sIdx)
	{
		m_values[eIdx] = m_values[eIdx - 1];
		eIdx--;
	}
	Corr& v = m_values[eIdx];
	v.x = cx; v.y = cy; v.s = cs; v.r = cr;
	v.dist = cdist;
}

float PatchDistMetric::ComputePatchDist(cvi* dst, float dpRow, float dpCol, 
	cvi* src, float spRow, float spCol, float spScale, float spRotate, 
	int patchOffset)
{
	int nPixels = sqr(2*patchOffset + 1);
	vector<cvS> vd(nPixels), vs(nPixels);
	int idx = 0;
	doFs(pr, -patchOffset, patchOffset+1) doFs(pc, -patchOffset, patchOffset+1)
	{
		float dpRowCur = dpRow + pr, dpColCur = dpCol + pc;
		float preAngle = atan2(_f(pr), _f(pc));
		float length = sqrt(sqr(_f(pr)) + sqr(_f(pc)));
		float spRowCur = spRow + length * sin(preAngle - spRotate) * spScale;
		float spColCur = spCol + length * cos(preAngle - spRotate) * spScale;
		vd[idx] = cvg2(dst, dpRowCur, dpColCur);
		vs[idx] = cvg2(src, spRowCur, spColCur);
		idx++;
	}
	return ComputeVectorDist(vd, vs);
}

float RegularPatchDistMetric::ComputeVectorDist(vector<cvS> vDst, vector<cvS> vSrc)
{
	float sum = 0;
	doFv(i, vDst)
	{
		sum += _f cvSDSqr(vDst[i], vSrc[i]);
	}
	return sqrt(sum / vDst.size()); 
}

GPMProc::GPMProc(int knn, int patchSize, int nItr):m_knn(knn), m_patchSize(patchSize),
	m_nItr(nItr)
{
	m_patchOffset = (m_patchSize - 1) / 2;

	m_minScale = 0.5f;
	m_maxScale = 2.0f;
	m_minRotate = -1.05f * (float)CV_PI;
	m_maxRotate = 1.05f * (float)CV_PI;

// 	m_minScale = 1.0f;
// 	m_maxScale = 1.0f;
// 	m_minRotate = -0.f * (float)CV_PI;
// 	m_maxRotate = 0.f * (float)CV_PI;
}

GPMProc::~GPMProc(void)
{
}

void GPMProc::RunGPM(cvi* src, cvi* dst)
{
	if(src == NULL || dst == NULL) return;

	int w = dst->width, h = dst->height;
	Interval hItvl(_f m_patchOffset, _f h - 1 - m_patchOffset);
	Interval wItvl(_f m_patchOffset, _f w - 1 - m_patchOffset);

	int sw = src->width, sh = src->height;
	Interval shItvl(_f m_patchOffset, _f sh - 1 - m_patchOffset);
	Interval swItvl(_f m_patchOffset, _f sw - 1 - m_patchOffset);

	//random initialize
	DenseCorr dsCor(w, h, m_knn, m_patchOffset);
	dsCor.RandomInitialize(swItvl, shItvl, Interval(m_minScale, m_maxScale), 
		Interval(m_minRotate, m_maxRotate));
	dsCor.UpdatePatchDistance(dst, src);
	dsCor.ShowCorr("init.png");
	dsCor.ShowCorrDist("initDist.png");

	//iteration
	doF(kItr, m_nItr)
	{
		cout<<"\rIteration "<<kItr<<"...";

		nCores = 4;
		omp_set_num_threads(nCores);
#pragma omp parallel for
		doF(k, nCores)
		{
			float hStart = hItvl.min, hEnd = hItvl.max;
			float wStart = wItvl.min, wEnd = wItvl.max;
			if(m_nItr % 4 == 0 || m_nItr % 4 == 1)
			{
				float hD = (hEnd - hStart) / nCores;
				hStart = hD * k + hStart;
				hEnd = hStart + hD;
			}
			else
			{
				float wD = (wEnd - wStart) / nCores;
				wStart = wD * k + wStart;
				wEnd = wStart + wD;
			}

			doFs(_x, _i hStart, _i hEnd) doFs(_y, _i wStart, _i wEnd)
			{
				int x = _x, y = _y;
				if(kItr % 2 == 1){
					x = _i hItvl.min + _i hItvl.max - x;
					y = _i wItvl.min + _i wItvl.max - y;
				}
				Propagate(src, dst, x, y, (kItr % 2 == 1), dsCor, wItvl, hItvl);
				RandomSearch(src, dst, x, y, dsCor, wItvl, hItvl);
			}
		}
		dsCor.ShowCorr("res.png");
		dsCor.ShowCorrDist("resDist.png");
	}
}

void GPMProc::Propagate(cvi* src, cvi* dst, int x, int y, bool direction, 
	DenseCorr& dsCor, Interval wItvl, Interval hItvl)
{
	int offset[4][2] = {{1, 0}, {0, 1}, {0, -1}, {-1, 0}};
	int offsetIdxStart = 0;
	int offsetIdxEnd = 2;
	if(!direction){
		offsetIdxStart = 2;
		offsetIdxEnd = 4;
	}

	float distThres = dsCor.GetDistThres(x, y);
	doFs(i, offsetIdxStart, offsetIdxEnd)
	{
		int dx = offset[i][0], dy = offset[i][1];
		if(!cvIn(x + dx, y + dy, _i hItvl.min, _i hItvl.max, 
			_i wItvl.min, _i wItvl.max))
			continue;

		int corrOffIdx = dsCor.GetCorrIdx(x + dx, y + dy);
		doF(k, m_knn)
		{
			Corr& v = dsCor.Get(corrOffIdx + k);
			float angle = atan2(-(float)dx, -(float)dy);
			float newX = v.x + sin(angle - v.r) * v.s;
			float newY = v.y + cos(angle - v.r) * v.s;
			float patchDist = ComputePatchDist(src, dst, x, y, newX, newY, v.s, v.r);
			if(patchDist < distThres){
				dsCor.AddCoor(x, y, newX, newY, v.s, v.r, patchDist);
				distThres = dsCor.GetDistThres(x, y);
			}
		}
	}
}

void GPMProc::RandomSearch(cvi* src, cvi* dst, int x, int y, 
	DenseCorr& dsCor, Interval wItvl, Interval hItvl)
{
	float distThres = dsCor.GetDistThres(x, y);
	int idx = dsCor.GetCorrIdx(x, y);

	vector<Corr> corrs(m_knn);
	doF(k, m_knn) corrs[k] = dsCor.Get(idx + k);

	float scaleFactor = 0.5f;
	randInit();
	doF(k, m_knn)
	{
		Corr& v = corrs[k];
		float hSpace = hItvl.max - hItvl.min;
		float wSpace = wItvl.max - wItvl.min;
		float sSpace = m_maxScale - m_minScale;
		float rSpace = m_maxRotate - m_minRotate;
		while(hSpace > 1 && wSpace > 1)
		{
			float newx = clamp(hSpace * (2 * rand1() - 1) + v.x, hItvl.min, hItvl.max);
			float newy = clamp(wSpace * (2 * rand1() - 1) + v.y, wItvl.min, wItvl.max);
			float news = clamp(sSpace * (2 * rand1() - 1) + v.s, m_minScale, m_maxScale);
			float newr = clamp(rSpace * (2 * rand1() - 1) + v.r, m_minRotate, m_maxRotate);
			float patchDist = ComputePatchDist(src, dst, x, y, newx, newy, news, newr);
			if(patchDist < distThres){
				dsCor.AddCoor(x, y, newx, newy, news, newr, patchDist);
				distThres = dsCor.GetDistThres(x, y);
			}
			hSpace *= scaleFactor; wSpace *= scaleFactor;
			sSpace *= scaleFactor; rSpace *= scaleFactor;
		}
	}
}

float GPMProc::ComputePatchDist(cvi* src, cvi* dst, int x, int y, float sx, float sy, float sScale, float sRot)
{
	return RegularPatchDistMetric().ComputePatchDist(dst, _f x, _f y, src, sx, sy, sScale, sRot, m_patchOffset);
}