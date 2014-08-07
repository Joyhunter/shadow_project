#pragma once
#include "img_container.h"

struct Patch
{
	vector<cvS> hls;
	vector<cvS> ori;
	vector<int> tex;
	int texBinN;
};

class PatchDistMetric
{
public:
	virtual float ComputeVectorDist(Patch& vDst, Patch& vSrc, cvS bias, cvS gain) = 0;
	float ComputePatchDist(ImgContainer& dst, float dpRow, float dpCol, 
		ImgContainer& src, float spRow, float spCol, float spScale, float spRotate, cvS spBias, cvS spGain,
		int patchOffset);
	static void GetPatch(ImgContainer& src, float x, float y, float s, float r, 
		int patchOffset, Patch& patch);

	float CptDistAlphaWeight(vector<cvS>& vDst, vector<cvS>& vSrc, 
		IN cvS useAlpha, IN cvS weights, IN bool firstIsHue, IN float maxRatio, OUT cvS& alpha);
	float CptDistDirectWithBiasAndGain(vector<cvS>& vDst, vector<cvS>& vSrc, IN cvS bias, IN cvS gain);
	float CptDistDstrbt(vecI& vDst, vecI& vSrc, int binN);
};

class RegularPatchDistMetric : public PatchDistMetric
{
public:
	float ComputeVectorDist(Patch& vDst, Patch& vSrc, cvS bias, cvS gain);
};

class LmnIvrtPatchDistMetric : public PatchDistMetric
{
public:
	float ComputeVectorDist(Patch& vDst, Patch& vSrc, cvS bias, cvS gain);
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
	cvS bias, gain;
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
		Interval sItvl, Interval rItrl, vector<Interval>& biasItrl, vector<Interval>& gainItrl);
	void UpdatePatchDistance(ImgContainer& dst, ImgContainer& src, PatchDistMetric* metric);
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
	void AddCoor(int r, int c, float cx, float cy, float cs, float cr, cvS cBias, cvS cGain, float cdist);
	void Save(ostream& fout);
	void Load(istream& fin);
	void LevelUp(int ratio = 2);
public:
	int m_width, m_height, m_knn;
	int m_patchOffset;
	vector<Corr> m_values; 
};

class DenseCorrBox2D
{
public:
	DenseCorrBox2D(){};
	DenseCorrBox2D(int nWidth, int nHeight);
	~DenseCorrBox2D();
	void SetSize(int nWidth, int nHeight);
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
	void LevelUp(int ratio = 2);
private:
	int m_nWidth, m_nHeight;
	int m_srcW, m_srcH;
	vector<DenseCorr*> m_box;
public:
	vector<Interval> wInts, hInts;
};

struct GPMRange
{
	Interval m_scaleItrl, m_rotateItrl;
	vector<Interval> m_biasItrl, m_gainItrl;

	void setScale(float minV, float maxV)
	{ m_scaleItrl.min = minV; m_scaleItrl.max = maxV;};
	void setRotate(float minV, float maxV)
	{ m_rotateItrl.min = minV; m_rotateItrl.max = maxV;};
	void setBias(cvS minV, cvS maxV)
	{
		m_biasItrl.resize(3);
		doF(k, 3){m_biasItrl[k].min = _f minV.val[k]; m_biasItrl[k].max = _f maxV.val[k];}
	};
	void setGain(cvS minV, cvS maxV)
	{
		m_gainItrl.resize(3);
		doF(k, 3){m_gainItrl[k].min = _f minV.val[k]; m_gainItrl[k].max = _f maxV.val[k];}
	};
};

class GPMProc
{

public:

	GPMProc(PatchDistMetric* metric, int knn = 1, int patchSize = 7, int nItr = 10, GPMRange* range = NULL);
	~GPMProc(void);

	DenseCorr* RunGPM(ImgContainer& src, ImgContainer& dst);
	void RunGPMWithInitial(ImgContainer& src, ImgContainer& dst, DenseCorr* );

private:

	void Propagate(ImgContainer& src, ImgContainer& dst, int x, int y, bool direction, 
		DenseCorr& dsCor, Interval wItvl, Interval hItvl, 
		Interval swItvl, Interval shItvl);
	void RandomSearch(ImgContainer& src, ImgContainer& dst, int x, int y, 
		DenseCorr& dsCor, Interval wItvl, Interval hItvl);

	float ComputePatchDist(ImgContainer& src, ImgContainer& dst, int x, int y, 
		float sx, float sy, float sScale, float sRot, cvS bias, cvS gain);

private:

	PatchDistMetric* m_metric;

	//arguments
	int m_knn;
	int m_patchSize, m_patchOffset;
	int m_nItr;
	
	GPMRange m_range;
	//Interval m_scaleItrl, m_rotateItrl;
	//vector<Interval> m_biasItrl, m_gainItrl;

};

class GridGPMProc
{
public:

	GridGPMProc(PatchDistMetric* metric, int gridSize = 50, int gridStep = 40, 
		int knn = 1, int patchSize = 7, int nItr = 10, GPMRange* range = NULL);
	~GridGPMProc(void);

	void SetROI(CvRect roi);

	void RunGridGPM(ImgContainer& img, string saveFile = ""); //used
	void RunGridGPMMultiScale(ImgContainer& img, string saveFile = "", int ratio = 2, int levels = 1); // used

	void ShowGPMResUI(ImgContainer& img, string fileStr, float distThres = 255);

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

	void InitDenseBox2D(int srcW, int srcH, DenseCorrBox2D& box);
	void RunGridGPMSingleScale(ImgContainer& img, DenseCorrBox2D& box, bool initValue = false); // used

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