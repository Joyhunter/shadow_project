#pragma once
#include "gpm_proc.h"

class VoteProc
{
public:
	VoteProc(void);
	~VoteProc(void);

	void LoadCorrs(string fileStr);
	void Vote(IN cvi* src, OUT cvi* &mask, OUT cvi* &cfdcMap, 
		float distThres = 30.f);

private:

	void PostProcess(cvi* &mask, cvi* &cfdcMap);
	float EstmtShdwRatio(cvi* src, float distThres);

private:
	DenseCorrBox2D m_corrs;
	int m_width, m_height;
	int m_patchOffset;
};

