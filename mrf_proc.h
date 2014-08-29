#pragma once

class MRFProc
{
public:
	MRFProc(void);
	~MRFProc(void);

	//ori
	void SolveWithInitial(IN cvi* src, IN cvi* srcHLS, IN cvi* initMask, IN cvi* initCfdc,
		IN int nLabels, OUT cvi* &shdwMask);
	void SolveWithInitAndGidc(IN cvi* src, IN cvi* initMask, IN cvi* gdcMask, IN cvi* boundMask, IN int nLabels, 
		OUT cvi* &shdwMask, IN int smoothSize);

	//current
	void SolveWithInitialAllCh(IN cvi* src, IN cvi* initParam, IN int nLabels, OUT cvi* &shdwMask);

private:

	static float GetValueFromLabel(int label);

	static float GetDataCost(int idx, int label);
	static float GetSmoothCost(int idx1, int idx2, int label1, int label2);

	static float GetDataCostGdc(int idx, int label);
	static float GetSmoothCostGdc(int idx1, int idx2, int label1, int label2);

private:

	static MRFProc *s_proc;

	int m_width, m_height;
	int m_nLabels;

	cvi* m_src, *m_srcHLS, *m_initMask, *m_initCfdc;
	cvi* m_gdcMask, *m_boundmask;
	

};

