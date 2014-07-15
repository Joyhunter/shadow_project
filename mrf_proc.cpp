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
	v = fabs(pow(v - GetValueFromLabel(label), 1.0f));

	float cfdc = _f cvg2(s_proc->m_initCfdc, x, y).val[0] / 255.0f;
	float cfdcTerm = _f gaussian(cfdc, 1.0f, 0.5f);
	
	return v;// * cfdcTerm;
}

float MRFProc::GetSmoothCost(int idx1, int idx2, int label1, int label2)
{
	//smoothCost = |res_i - res_j| * 1;

	//global smoothness
	float alpha1 = GetValueFromLabel(label1), alpha2 = GetValueFromLabel(label2);
	float v = fabs(pow(alpha1 - alpha2, 1.0f));

	//affinity
	int x1 = idx1 / (s_proc->m_width), y1 = idx1 % (s_proc->m_width);
	int x2 = idx2 / (s_proc->m_width), y2 = idx2 % (s_proc->m_width);
	cvS s1 = cvg2(s_proc->m_src, x1, y1) / 255.0f;
	cvS s2 = cvg2(s_proc->m_src, x2, y2) / 255.0f;
	float affinity = _f gaussian(_f colorDist(s1, s2), 0.f, 0.2f);

	//shadow-free image values
	s1 = s1 / alpha1;
	s2 = s2 / alpha2;
	float legality = (label1 == label2) ? 0 : (_f colorDist(s1, s2));

	//return v + legality;
	return v * affinity * 2;// + legality * 5;
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
	MRF* mrf = new Expansion(m_width, m_height, nLabels, eng); // Expansion Swap MaxProdBP
	mrf->initialize();  
	mrf->clearAnswer();
	mrf->optimize(5, timeCost); 

	MRF::EnergyVal E_smooth = mrf->smoothnessEnergy();
	MRF::EnergyVal E_data = mrf->dataEnergy();
	printf("Total Energy = %d (Smoothness energy %d, Data Energy %d)\n", 
		E_smooth+E_data,E_smooth,E_data);
	
	shdwMask = cvci(initMask);
	doFcvi(shdwMask, i, j)
	{
		cvS s = cvg2(shdwMask, i, j);
		s.val[0] = mrf->getLabel(i * m_width + j) * 255 / nLabels;
		cvs2(shdwMask, i, j, s);
	}
	
	delete mrf;

}