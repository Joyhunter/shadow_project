#include "StdAfx.h"
#include "mrf_proc.h"
#include "mrf/include/mrf.h"
#include "mrf/include/GCoptimization.h"
#include "mrf/include/ICM.h"
#include "mrf/include/MaxProdBP.h"

#pragma comment(lib, "mrf/Release/mrf.lib")

MRFProc* MRFProc::s_proc = NULL;

MRFProc::MRFProc(void)
{
}

MRFProc::~MRFProc(void)
{
}

float labelMinV = 0, labelMaxV = 1;
int curCh = 0;
float MRFProc::GetValueFromLabel(int label)
{
	return label / (_f s_proc->m_nLabels - 1) * (labelMaxV - labelMinV) + labelMinV;
}

float MRFProc::GetDataCost(int idx, int label)
{ 
	int x = idx / (s_proc->m_width), y = idx % (s_proc->m_width);

	// dataCost = |res_i - prior_i| * 1;      0 ~ 1
	float v = _f(cvg2(s_proc->m_initMask, x, y).val[curCh]) / 255.0f;
	v = fabs(pow(v - GetValueFromLabel(label), 2.0f));

	//float cfdc = _f cvg2(s_proc->m_initCfdc, x, y).val[0] / 255.0f;
	//float cfdcTerm = _f gaussian(cfdc, 1.0f, 0.5f);

	//if(cvg20(s_proc->m_boundmask, x, y) == 255) return v*0.01f;
	return v;// * cfdcTerm;
}

float MRFProc::GetSmoothCost(int idx1, int idx2, int label1, int label2)
{
	//smoothCost = |res_i - res_j| * 1;

	//global smoothness
	float alpha1 = GetValueFromLabel(label1), alpha2 = GetValueFromLabel(label2);
	float v = fabs(pow(alpha1 - alpha2, 2.0f));

	//affinity
	int x1 = idx1 / (s_proc->m_width), y1 = idx1 % (s_proc->m_width);
	int x2 = idx2 / (s_proc->m_width), y2 = idx2 % (s_proc->m_width);
	cvS s1 = cvg2(s_proc->m_src, x1, y1) / 255.0f;
	cvS s2 = cvg2(s_proc->m_src, x2, y2) / 255.0f;
	float affinity = _f gaussian(_f colorDist(s1, s2), 0.f, 0.2f);

	//shadow-free image values
// 	s1 = s1 / alpha1;
// 	s2 = s2 / alpha2;
// 	float legality = (label1 == label2) ? 0 : (_f colorDist(s1, s2));

	//return v + legality;
	return v * affinity * 2;// + legality * 5;
}

cvi* GetBounaryMask(cvi* mask, int r)
{
	cvi* res = cvci81(mask); 
	cvZero(res);
	doFcvi(mask, i, j)
	{
		if(cvg20(mask, i, j) < 0.6) 
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

void MRFProc::SolveWithInitialAllCh(IN cvi* src, IN cvi* initParam, IN int nLabels, OUT cvi* &shdwMask)
{
	s_proc = this;
	m_width = src->width;
	m_height = src->height;
	m_nLabels = nLabels;
	m_src = cvci(src);
	m_initMask = initParam;

	m_boundmask = GetBounaryMask(initParam, src->width / 30);
	//cvsi(m_boundmask);

	//cvi* temp = cvci(m_src);
	//cvSmooth(temp, m_src, CV_BILATERAL, 20, 20, 200, 200); cvri(temp);
	//cvsi(m_src);

	//find min, max
	cvS minParam = cvs(1e6, 1e6, 1e6), maxParam = cvs(-1e6, -1e6, -1e6);
	doFcvi(initParam, i, j)
	{
		auto v = cvg2(initParam, i, j);
		doF(k, 3)
		{
			minParam.val[k] = min2(minParam.val[k], v.val[k]);
			maxParam.val[k] = max2(maxParam.val[k], v.val[k]);
		}
	}

	shdwMask = cvci(initParam);
	cvSmooth(shdwMask, shdwMask, 1, 10, 10);
	doF(k, 3)
	{
		//cout<<minParam.val[k]<<" "<<maxParam.val[k]<<endl;
		if(k == 0)
		{
			labelMinV = 0; labelMaxV = 1; curCh = 0; 
		}
		else
		{
			labelMinV = _f minParam.val[k]; labelMaxV = _f maxParam.val[k]; curCh = k; continue;
			labelMinV = 0; labelMaxV = 3; curCh = k;
		}

		DataCost *data = new DataCost(MRFProc::GetDataCost);
		SmoothnessCost *smooth = new SmoothnessCost(MRFProc::GetSmoothCost);
		EnergyFunction *eng = new EnergyFunction(data, smooth);
		cout<<"\r"<<k<<" Data Ready.";

		float timeCost;
		MRF* mrf = new Expansion(m_width, m_height, nLabels, eng); // Expansion Swap MaxProdBP
		mrf->initialize();  
		mrf->clearAnswer();
		mrf->optimize(5, timeCost); 

		MRF::EnergyVal E_smooth = mrf->smoothnessEnergy();
		MRF::EnergyVal E_data = mrf->dataEnergy();
		cout<<"\r"<<k<<" Solving MRF complete ( E_Smooth = "<<E_smooth<<", E_data = "<<E_data<<" ).\n";

		doFcvi(shdwMask, i, j)
		{
			cvS s = cvg2(shdwMask, i, j);
			s.val[k] = GetValueFromLabel(mrf->getLabel(i * m_width + j));
			cvs2(shdwMask, i, j, s);
		}

		delete data, smooth, eng;
		delete mrf;
	}
	labelMinV = 0; labelMaxV = 1; curCh = 0; 

	cvri(m_src); cvri(m_boundmask);
}

void MRFProc::SolveWithInitial(IN cvi* src, IN cvi* srcHLS, IN cvi* initMask, 
	IN cvi* initCfdc, IN int nLabels, OUT cvi* &shdwMask)
{
	s_proc = this;
	m_width = src->width;
	m_height = src->height;
	m_nLabels = nLabels;
	m_src = src;
	m_srcHLS = srcHLS;
	m_initMask = initMask;
	m_initCfdc = initCfdc;

	cout<<"Solving MRF system...";

	DataCost *data = new DataCost(MRFProc::GetDataCost);
	SmoothnessCost *smooth = new SmoothnessCost(MRFProc::GetSmoothCost);
	EnergyFunction *eng = new EnergyFunction(data,smooth);

	float timeCost;
	MRF* mrf = new Expansion(m_width, m_height, nLabels, eng); // Expansion Swap MaxProdBP
	mrf->initialize();  
	mrf->clearAnswer();
	mrf->optimize(5, timeCost); 

	MRF::EnergyVal E_smooth = mrf->smoothnessEnergy();
	MRF::EnergyVal E_data = mrf->dataEnergy();
	cout<<"\rSolving MRF complete ( E_Smooth = "<<E_smooth<<", E_data = "<<E_data<<" ).\n";
	
	shdwMask = cvci(initMask);
	doFcvi(shdwMask, i, j)
	{
		cvS s = cvg2(shdwMask, i, j);
		s.val[0] = GetValueFromLabel(mrf->getLabel(i * m_width + j)) * 255.0;
		cvs2(shdwMask, i, j, s);
	}
	
	delete data, smooth, eng;
	delete mrf;

}


float MRFProc::GetDataCostGdc(int idx, int label)
{
	int x = idx / (s_proc->m_width), y = idx % (s_proc->m_width);

	float v = _f cvg20(s_proc->m_gdcMask, x, y) / 255.0f;
	float vI = _f cvg20(s_proc->m_initMask, x, y) / 255.0f;
	float vP = GetValueFromLabel(label);
	float res1 = fabs(pow(v - vP, 2.0f)), res2 = fabs(pow(vI - vP, 2.0f));

	float weight = _f gaussian(v / vI, 1.2, 0.05);
	if(v / vI > 1.2) weight = 1;
	if(v >= 0.99f) return res1 * 10;
	
	//weight = 0;
	//if(cvg20(s_proc->m_boundmask, x, y) == 0) weight = 1;

	float weight2 = _f gaussian(v, 1.0f, 0.1f);
	//if(v > 0.95) weight2 = 1;
	//weight = clamp(weight * (weight2 + 1), 0.f, 1.f);

	return res1*weight + res2*(1-weight);
}

float MRFProc::GetSmoothCostGdc(int idx1, int idx2, int label1, int label2)
{
	//global smoothness
	float alpha1 = GetValueFromLabel(label1), alpha2 = GetValueFromLabel(label2);
	float v = fabs(pow(alpha1 - alpha2, 2.0f));

	//affinity
// 	int x1 = idx1 / (s_proc->m_width), y1 = idx1 % (s_proc->m_width);
// 	int x2 = idx2 / (s_proc->m_width), y2 = idx2 % (s_proc->m_width);
// 	cvS s1 = cvg2(s_proc->m_src, x1, y1) / 255.0f;
// 	cvS s2 = cvg2(s_proc->m_src, x2, y2) / 255.0f;
// 	float affinity = _f gaussian(_f colorDist(s1, s2), 0.f, 0.2f);

	return v * 10; //10
}

void MRFProc::SolveWithInitAndGidc(IN cvi* src, IN cvi* initMask, IN cvi* gdcMask, IN cvi* boundMask, 
	IN int nLabels, OUT cvi* &shdwMask, IN int smoothSize)
{
	s_proc = this;
	m_width = src->width;
	m_height = src->height;
	m_nLabels = nLabels;
	m_src = src;
	m_initMask = initMask;
	m_gdcMask = gdcMask;
	m_boundmask = boundMask;

	//cout<<"Solving MRF system...";

	DataCost *data = new DataCost(MRFProc::GetDataCostGdc);
	SmoothnessCost *smooth = new SmoothnessCost(MRFProc::GetSmoothCostGdc);
	EnergyFunction *eng = new EnergyFunction(data,smooth);

	float timeCost;
	MRF* mrf = new Expansion(m_width, m_height, nLabels, eng); // Expansion Swap MaxProdBP
	mrf->initialize();  
	mrf->clearAnswer();

	mrf->optimize(5, timeCost); 
	shdwMask = cvci(initMask);
	doFcvi(shdwMask, i, j)
	{
		cvS s = cvg2(shdwMask, i, j);
		s.val[0] = GetValueFromLabel(mrf->getLabel(i * m_width + j)) * 255;
		s.val[2] = s.val[0];
		cvs2(shdwMask, i, j, s);
	}

	delete mrf;

}