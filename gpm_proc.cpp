#include "StdAfx.h"
#include "gpm_proc.h"

Interval::Interval(float min, float max):min(min), max(max)
{
}

float Interval::RandValue()
{
	return (float)rand()/RAND_MAX * (max - min) + min;
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

	srand((unsigned int)time(0));
	doF(i, m_height) doF(j, m_width) doF(k, m_knn)
	{
		m_values[idx].x = hItvl.RandValue();
		m_values[idx].y = wItvl.RandValue();
		m_values[idx].s = sItvl.RandValue();
		m_values[idx].r = rItrl.RandValue();
		idx++;
	}
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

	omp_set_num_threads(7);
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
}

GPMProc::~GPMProc(void)
{
}

void GPMProc::RunGPM(cvi* src, cvi* dst)
{
	if(src == NULL || dst == NULL) return;

	int w = dst->width, h = dst->height; 
	int hStart = m_patchOffset, hEnd = h - 1 - m_patchOffset;
	int wStart = m_patchOffset, wEnd = w - 1 -m_patchOffset;

	int sw = src->width, sh = src->height;
	int shStart = m_patchOffset, shEnd = sh - 1 - m_patchOffset;
	int swStart = m_patchOffset, swEnd = sw - 1 - m_patchOffset;

	//random initialize
	DenseCorr dsCor(w, h, m_knn, m_patchOffset);
	dsCor.RandomInitialize(Interval(_f(swStart), _f(swEnd)), 
		Interval(_f(shStart), _f(shEnd)),
		Interval(m_minScale, m_maxScale), 
		Interval(m_minRotate, m_maxRotate));
	dsCor.UpdatePatchDistance(dst, src);
	dsCor.ShowCorr("init.png");
	dsCor.ShowCorrDist("initDist.png");

	//iteration
	doF(kItr, m_nItr)
	{
		cout<<"\rIteration "<<kItr<<"...";
	}

}