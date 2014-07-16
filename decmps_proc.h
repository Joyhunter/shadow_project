#pragma once

typedef vector<float> Hist;

struct Cell
{
	int avgV;

	float pixelNum; // 0 ~ 1 ~ 5? pixelN / width / height / cellN

	int minX, maxX, minY, maxY;
	int width, height;
	float ratio;

	set<int> adjacents;

	//src edge info
	int edgePixelNum;
	float edgeWeight;

	bool legal;
	Cell():legal(true){};
};

struct Region
{
	vector<int> cellIdx;
	float estmtedAlpha;
	float pixelNum;

	void EstmtAlpha(vector<Cell>& cells, float ratio = 0.25f);
	cvi* GetGidcAlphaMap(cvi* strtImg);
};

struct Strt
{
	cvi* strtImg;
	vector<Cell> cells;
	int adjacentRadius;

	Strt():strtImg(NULL), cells(0){};
	void UpdateCellProperty(cvi* src);
	void Filter();
	vector<Region> GetRegion();
};

class DecmpsProc
{

public:

	DecmpsProc(void);
	~DecmpsProc(void);

	void Analysis(IN cvi* src, IN cvi* maskMRF, OUT cvi* &shdwMask);

private:

	void ComputeHist(IN cvi* mask, OUT Hist& hist);
	void GetPeeks(Hist& hist, vector<int>& peeks, float minValue = 0.2f, float minGap = 5.0f);
	void ShowHistInCvi(IN Hist& hist, IN vector<int>& peeks, float heightRatio, string fileDir);

	void Quantify(cvi* maskMRF, vector<int>& peeks, Strt& strt);
	void ShowStrtImg(Strt& strt, string fileDir);
};

