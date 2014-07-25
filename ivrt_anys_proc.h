#pragma once

class IvrtAnysProc
{
public:
	IvrtAnysProc(void);
	~IvrtAnysProc(void);

	void IvrtAnalysis(cvi* image);
	cvi* IvrtAnalysis2(cvi* image);
};

