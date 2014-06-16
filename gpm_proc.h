#pragma once

class Interval
{
public:
	float min, max;
	Interval(float min = 0, float max = 0);
	float RandValue();
};

class Corr
{
public:
	float x, y, s, r;
	float dist;
	bool operator < (const Corr &b) const{
		return this->dist < b.dist;
	}
};

class DenseCorr
{
public:
	DenseCorr(int w, int h, int knn, int patchOffset = 1);
	void RandomInitialize(Interval wItvl, Interval hItvl, 
		Interval sItvl, Interval rItrl);
	void UpdatePatchDistance(cvi* dst, cvi* src);
	void ShowCorr(string imgStr);
	void ShowCorrDist(string imgStr);
private:
	int m_width, m_height, m_knn;
	int m_patchOffset;
	vector<Corr> m_values; 
};

class PatchDistMetric
{
	virtual float ComputeVectorDist(vector<cvS> vDst, vector<cvS> vSrc) = 0;
public:
	float ComputePatchDist(cvi* dst, float dpRow, float dpCol, 
		cvi* src, float spRow, float spCol, float spScale, float spRotate, 
		int patchOffset);
};

class RegularPatchDistMetric : public PatchDistMetric
{
	float ComputeVectorDist(vector<cvS> vDst, vector<cvS> vSrc);
};

class GPMProc
{

public:

	GPMProc(int knn = 1, int patchSize = 7, int nItr = 20);
	~GPMProc(void);

	void RunGPM(cvi* src, cvi* dst);

private:

	//arguments
	int m_knn;
	int m_patchSize, m_patchOffset;
	int m_nItr;
	
	float m_minScale, m_maxScale;
	float m_minRotate, m_maxRotate;


};

