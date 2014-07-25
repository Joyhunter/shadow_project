#pragma once

struct Filter
{
	vector<vector<float> > weights;
	int r;

	void Visualize();
	float Conv(cvi* img, int i, int j);
};

struct FilterBank
{
	vector<Filter > values;
	int oritN, scaleN;
};

class TexonAnysProc
{
public:
	TexonAnysProc(void);
	~TexonAnysProc(void);

	cvi* TexonAnalysis(cvi* image);

private:

	FilterBank FbCreate(int numOrient = 8, float startSigma = 0.5f, int numScales = 1, float scaling = sqrt(2.0f), float elong = 3.f);

	Filter OeFilter(vector<float>& sigma, float support, float theta, int deriv, bool hil);

};

