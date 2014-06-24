#pragma once
#include "gpm_proc.h"

class VoteProc
{
public:
	VoteProc(void);
	~VoteProc(void);

	void LoadCorrs(string fileStr);
	void Vote(cvi* src);

private:
	DenseCorrBox2D m_corrs;
	int m_width, m_height;
	int m_patchOffset;
};

