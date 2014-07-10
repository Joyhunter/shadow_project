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

float MRFProc::GetValueFromLabel(int label)
{
	return label / (_f s_proc->m_nLabels - 1);
}

float MRFProc::GetDataCost(int idx, int label)
{

	int x = idx / (s_proc->m_width), y = idx % (s_proc->m_width);

	// dataCost = |res_i - prior_i| * 1;      0 ~ 1
	float v = _f cvg20(s_proc->m_initMask, x, y) / 255.0f;
	v = fabs(v - GetValueFromLabel(label));

	float cfdc = _f cvg2(s_proc->m_initCfdc, x, y).val[0] / 255.0f;

	return v * pow(cfdc, 0.3f);
}

float MRFProc::GetSmoothCost(int idx1, int idx2, int label1, int label2)
{
	//smoothCost = |res_i - res_j| * 1;
	float alpha1 = GetValueFromLabel(label1), alpha2 = GetValueFromLabel(label2);
	float v = fabs(alpha1 - alpha2);

	cvS s1 = cvg2(s_proc->m_srcHLS, idx1 / (s_proc->m_width), 
		idx1 % (s_proc->m_width)) / 255.0f;
	cvS s2 = cvg2(s_proc->m_srcHLS, idx2 / (s_proc->m_width), 
		idx2 % (s_proc->m_width)) / 255.0f;
	float v2 = fabs(s1.val[1] / (alpha1 + 0.00001f) - s2.val[1] / (alpha1 + 0.00001f));

	return v * 1.f;
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

	DataCost *data = new DataCost(MRFProc::GetDataCost);
	SmoothnessCost *smooth = new SmoothnessCost(MRFProc::GetSmoothCost);
	EnergyFunction *eng = new EnergyFunction(data,smooth);

	float timeCost;
	MRF* mrf = new Swap(m_width, m_height, nLabels, eng); // Expansion Swap MaxProdBP
	mrf->initialize();  
	mrf->clearAnswer();
	mrf->optimize(5, timeCost); 

	MRF::EnergyVal E_smooth = mrf->smoothnessEnergy();
	MRF::EnergyVal E_data = mrf->dataEnergy();
	printf("Total Energy = %d (Smoothness energy %d, Data Energy %d)\n", 
		E_smooth+E_data,E_smooth,E_data);
	
	shdwMask = cvci81(src);
	doFcvi(shdwMask, i, j)
	{
		cvs20(shdwMask, i, j, mrf->getLabel(i * m_width + j) * 255 / nLabels);
	}
	
	delete mrf;

}