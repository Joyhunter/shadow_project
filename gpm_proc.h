#pragma once

typedef vector<cvS> Patch;

class PatchDistMetric
{
public:
	virtual float ComputeVectorDist(Patch& vDst, Patch& vSrc) = 0;
	float ComputePatchDist(cvi* dst, float dpRow, float dpCol, 
		cvi* src, float spRow, float spCol, float spScale, float spRotate, 
		int patchOffset);
	static Patch GetPatch(cvi* src, float x, float y, float s, float r, 
		int patchOffset);
};

class RegularPatchDistMetric : public PatchDistMetric
{
public:
	float ComputeVectorDist(vector<cvS>& vDst, vector<cvS>& vSrc);
};

class LmnIvrtPatchDistMetric : public PatchDistMetric
{
public:
	float ComputeVectorDist(vector<cvS>& vDst, vector<cvS>& vSrc);
};

class Interval
{
public:
	float min, max;
	Interval(float min = 0, float max = 0);
	float RandValue();
	friend ostream& operator << (ostream& out, Interval& val){
		out<<val.min<<" "<<val.max;
		return out;
	}
};

class Corr
{
public:
	float x, y, s, r;
	float dist;
	bool operator < (const Corr &b) const{
		return this->dist < b.dist;
	}
	friend ostream& operator << (ostream& out, const Corr& val){
		out<<"(x="<<val.x<<", y="<<val.y<<", s="<<val.s<<", r="<<val.r<<", dist="
			<<val.dist<<")";
		return out;
	}
	void Save(ostream& fout);
	void Load(istream& fin);
};

typedef vector<Corr> MultiCorr;

class DenseCorr
{
public:
	DenseCorr(){};
	DenseCorr(int w, int h, int knn, int patchOffset = 1);
	void RandomInitialize(Interval wItvl, Interval hItvl, 
		Interval sItvl, Interval rItrl);
	void UpdatePatchDistance(cvi* dst, cvi* src, PatchDistMetric* metric);
	void ShowCorr(string imgStr);
	void ShowCorrDist(string imgStr);
	int GetCorrIdx(int r, int c){
		return (r * m_width + c) * m_knn;
	}
	Corr& Get(int idx){
		return m_values[idx];
	}
	float GetDistThres(int r, int c){
		return m_values[GetCorrIdx(r, c+1) - 1].dist;
	}
	void AddCoor(int r, int c, float cx, float cy, float cs, float cr, float cdist);
	void Save(ostream& fout);
	void Load(istream& fin);
public:
	int m_width, m_height, m_knn;
	int m_patchOffset;
	vector<Corr> m_values; 
};

class GPMProc
{

public:

	GPMProc(PatchDistMetric* metric, int knn = 1, int patchSize = 7, int nItr = 10);
	~GPMProc(void);

	DenseCorr* RunGPM(cvi* src, cvi* dst);

private:

	void Propagate(cvi* src, cvi* dst, int x, int y, bool direction, 
		DenseCorr& dsCor, Interval wItvl, Interval hItvl, 
		Interval swItvl, Interval shItvl);
	void RandomSearch(cvi* src, cvi* dst, int x, int y, 
		DenseCorr& dsCor, Interval wItvl, Interval hItvl);

	float ComputePatchDist(cvi* src, cvi* dst, int x, int y, 
		float sx, float sy, float sScale, float sRot);

private:

	PatchDistMetric* m_metric;

	//arguments
	int m_knn;
	int m_patchSize, m_patchOffset;
	int m_nItr;
	
	float m_minScale, m_maxScale;
	float m_minRotate, m_maxRotate;

};

class DenseCorrBox2D
{
public:
	DenseCorrBox2D(){};
	DenseCorrBox2D(int nWidth, int nHeight);
	~DenseCorrBox2D();
	void SetValue(int r, int c, DenseCorr* corr){
		m_box[r*m_nWidth+c] = corr;
	}
	void SetSrcSize(int w, int h){
		m_srcW = w; m_srcH = h;
	}
	void ShowCorr(string imgStr);
	DenseCorr* GetValue(int r, int c){
		return m_box[r*m_nWidth+c];
	}
	void SetIntevals(vector<Interval>& ws, vector<Interval>& hs){
		wInts = ws; hInts = hs;
	}
	MultiCorr GetCorrsPerGrid(int r, int c);
	void ShowGridCorrs(CvRect& roi, int r, int c, int radius, cvi* src, string imgStr);
	void ShowGridCorrs(CvRect& roi, int r, int c, int radius, cvi* src, cvi* res);
	void Save(string file);
	void Load(string file);
private:
	int m_nWidth, m_nHeight;
	int m_srcW, m_srcH;
	vector<DenseCorr*> m_box;
	vector<Interval> wInts, hInts;
};

class GridGPMProc
{
public:

	GridGPMProc(PatchDistMetric* metric, int gridSize = 50, int gridStep = 40, 
		int knn = 1, int patchSize = 7, int nItr = 10);
	~GridGPMProc(void);

	void SetROI(CvRect roi);
	void RunGridGPM(cvi* src, string saveFile = "");
	void ShowGPMResUI(cvi* src, cvi* srcHLS, string fileStr, float distThres = 255);

	CvRect GetROI(){
		return m_roi;
	}
	int GetPatchSize(){
		return m_patchSize;
	}
	int GetPatchRadius(){
		return (m_patchSize-1)/2;
	}

private:

	GPMProc* m_proc;

	//arguments
	int m_gridSize;
	int m_gridStep;
	int m_patchSize;
	CvRect m_roi;

};

//.first = patch luminance  .second = patch similarity / confidence 0~255
typedef vector<pair<float, float>> LmncVec;
//.first = all patch numbers in each hist  .second = confident patch numbers
typedef vector<pair<int, int>> LmncHist;

class PatchLmncProc
{
public:
	static int histN;
	static float GetAvgLmnc(Patch& vs);
	static float GetAvgSatu(Patch& vs);
	static int GetHistIdx(float lmnc);
	static LmncHist GetLmncHist(LmncVec& lmnc, float distThres = 255);
	static cvi* ShowHistInCvi(LmncHist& hist, int focusIdx = -1);
};