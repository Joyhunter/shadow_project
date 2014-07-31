#pragma once

struct Filter
{
	vector<vector<float> > weights;
	int r;

	void Visualize();
	float Conv(cvi* img, int i, int j, int ch = 0);
};

struct FilterBank
{
	vector<Filter> values;
	int oritN, scaleN;
};

typedef vector<float> TexDscrptor;

class TexonAnysProc
{
public:
	TexonAnysProc(void);
	~TexonAnysProc(void);

	cvi* TexonAnalysis(cvi* image, int& clusterN, int ch = 0);

	static cvi* VisualizeTexon(IN cvi* texIdx);

private:

	cvi* GetDensityImg(cvi* image, int ch = 0);

	void GetTexonImg(IN cvi* image, IN int clusterN, OUT vector<TexDscrptor>& texValues, OUT cvi* &texIdx, int ch = 0);


	cvi* VisualizeDensity(cvi* densityMap);

	FilterBank FbCreate(int numOrient = 8, float startSigma = 0.5f, int numScales = 1, float scaling = sqrt(2.0f), float elong = 3.f);

	Filter OeFilter(vector<float>& sigma, float support, float theta, int deriv, bool hil);

	int SimplyTexIdx(IN OUT cvi* texIdxs, int clusterN, IN vector<TexDscrptor>& vectexDsps);

};

