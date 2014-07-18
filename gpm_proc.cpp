#include "StdAfx.h"
#include "gpm_proc.h"

int nCores = 7;

Interval::Interval(float min, float max):min(min), max(max)
{
}

float Interval::RandValue()
{
	return rand1() * (max - min) + min;
}

DenseCorr::DenseCorr(int w, int h, int knn, int patchOffset):m_width(w), m_height(h), 
	m_knn(knn), m_patchOffset(patchOffset)
{
	m_values.resize(m_width * m_height * m_knn);
}

void DenseCorr::RandomInitialize(Interval wItvl, Interval hItvl, 
	Interval sItvl, Interval rItrl)
{
	int idx = 0;

	doF(i, m_height) doF(j, m_width) doF(k, m_knn)
	{
		m_values[idx].x = hItvl.RandValue();
		m_values[idx].y = wItvl.RandValue();
		m_values[idx].s = sItvl.RandValue();
		m_values[idx].r = rItrl.RandValue();
		idx++;
	}
// 	idx = GetCorrIdx(3, 3);
// 	m_values[idx].x = 3; m_values[idx].y = 3; 
// 	m_values[idx].s = 1; m_values[idx].r = 0; 
}

void DenseCorr::ShowCorr(string imgStr)
{
	cvi* result = cvci(cvSize(m_width, m_height), 8, 3);
	doFcvi(result, i, j)
	{
		Corr& v = m_values[(i*m_width + j) * m_knn];
		cvs2(result, i, j, cvs(0, v.y / m_width * 255.0, v.x / m_height * 255.0));
	}
	cvsi(imgStr, result);
	cvri(result);
}

void DenseCorr::ShowCorrDist(string imgStr)
{
	cvi* result = cvci(cvSize(m_width, m_height), 8, 1);
	doFcvi(result, i, j)
	{
		cvs2(result, i, j, cvs(m_values[(i*m_width + j) * m_knn].dist));
	}
	cvsi(imgStr, result);
	cvri(result);
}

void DenseCorr::UpdatePatchDistance(cvi* dst, cvi* src, PatchDistMetric* metric)
{
	int idx = 0;

	doF(i, m_height) doF(j, m_width)
	{
		doF(k, m_knn)
		{
			Corr& v = m_values[(i*m_width + j) * m_knn + k];
			v.dist = metric->ComputePatchDist(dst, _f(i), _f(j), 
				src, v.x, v.y, v.s, v.r, m_patchOffset);
			idx++;
		}
		sort(m_values.begin() + (i*m_width + j) * m_knn, 
			m_values.begin() + (i*m_width + j + 1) * m_knn);
	}
}

void DenseCorr::AddCoor(int r, int c, float cx, float cy, float cs, float cr, 
	float cdist)
{
	int sIdx = GetCorrIdx(r, c);
	int eIdx = sIdx + m_knn;
	while(m_values[sIdx].dist < cdist && sIdx < eIdx) sIdx++;
	if(sIdx >= eIdx) return;
	eIdx--;
	while(eIdx > sIdx)
	{
		m_values[eIdx] = m_values[eIdx - 1];
		eIdx--;
	}
	Corr& v = m_values[eIdx];
	v.x = cx; v.y = cy; v.s = cs; v.r = cr;
	v.dist = cdist;
}

Patch PatchDistMetric::GetPatch(cvi* src, float x, float y, float s, float r, int patchOffset)
{
	int nPixels = sqr(2*patchOffset + 1);
	Patch vs(nPixels);
	int idx = 0;
	doFs(pr, -patchOffset, patchOffset+1) doFs(pc, -patchOffset, patchOffset+1)
	{
		float preAngle = atan2(_f(pr), _f(pc));
		float length = sqrt(sqr(_f(pr)) + sqr(_f(pc)));
		float spRowCur = x + length * sin(preAngle - r) * s;
		float spColCur = y + length * cos(preAngle - r) * s;
		vs[idx] = cvg2(src, spRowCur, spColCur);
		idx++;
	}
	return vs;
}

float PatchDistMetric::ComputePatchDist(cvi* dst, float dpRow, float dpCol, 
	cvi* src, float spRow, float spCol, float spScale, float spRotate, 
	int patchOffset)
{
	Patch vd = GetPatch(dst, dpRow, dpCol, 1.f, 0.f, patchOffset);
	Patch vs = GetPatch(src, spRow, spCol, spScale, spRotate, patchOffset);
	return ComputeVectorDist(vd, vs);
}

float RegularPatchDistMetric::ComputeVectorDist(Patch& vDst, Patch& vSrc)
{
	float sum = 0;
	doFv(i, vDst)
	{
		sum += _f cvSDSqr(vDst[i], vSrc[i]);
	}
	return sqrt(sum / vDst.size()); 
}

float LmnIvrtPatchDistMetric::ComputeVectorDist(Patch& vDst, Patch& vSrc)
{
	//hls: dst(l)*a = src(l), D(dst(h, l*a), src(h, l))
	int useAlpha[] = {-1, 1, -1};
	float channelWeight[] = {2, 1, 0.9f};
	bool hasHue = true;

	//hsv: dst(v)*a = src(v), D(dst(h, v*a), src(h, v))
	//int useAlpha[] = {-1, -1, 1};
	//float channelWeight[] = {1, 0, 1};
	//bool hasHue = true;

	//rgb: dst(rgb)*a = src(rgb), D(dst(r*a, g*a, b*a), src(r, g, b))
	//int useAlpha[] = {1, 1, 1};
	//float channelWeight[] = {1, 1, 1};
	//bool hasHue = false;

	cvS sumS = cvs(0, 0, 0), sumD = cvs(0, 0, 0);
	doFv(i, vSrc)
	{
		sumS += vSrc[i];
		sumD += vDst[i];
	}
	cvS alpha = cvs(1, 1, 1);
	doF(k, 3)
	{
		float sumS2 = 0, sumD2 = 0;
		doF(i, 3)
		{
			if(useAlpha[i] != k) continue;
			sumS2 +=  _f sumS.val[i];
			sumD2 += _f sumD.val[i];
		}
		if(sumD2 > 0) alpha.val[k] = clamp(sumS2 / sumD2, 0.1f, 10.0f);
	}

	float sum = 0;
	doFv(i, vDst)
	{
		cvS dstV = vDst[i], srcV = vSrc[i];
		doF(k, 3)
		{
			if(useAlpha[k] > 0) dstV.val[k] *= alpha.val[useAlpha[k]];
		}

		if(hasHue)
		{
			float hueChange = fabs(_f (dstV.val[0] - srcV.val[0]));
			if(hueChange > 128) hueChange = 255 - hueChange;
			sum += sqr(hueChange) * channelWeight[0];
		}
		else
		{
			sum += _f sqr(dstV.val[0] - srcV.val[0]) * channelWeight[0];
		}

		sum += _f sqr(dstV.val[1] - srcV.val[1]) * channelWeight[1];
		sum += _f sqr(dstV.val[2] - srcV.val[2]) * channelWeight[2];
	}
	return sqrt(sum / vDst.size());

	
}

GPMProc::GPMProc(PatchDistMetric* metric, int knn, int patchSize, int nItr):
	m_metric(metric), m_knn(knn), m_patchSize(patchSize), m_nItr(nItr)
{
	m_patchOffset = (m_patchSize - 1) / 2;

	m_minScale = 0.67f;
	m_maxScale = 1.5f;
	m_minRotate = -1.05f * (float)CV_PI;
	m_maxRotate = 1.05f * (float)CV_PI;

// 	m_minScale = 1.0f;
// 	m_maxScale = 1.0f;
// 	m_minRotate = -0.f * (float)CV_PI;
// 	m_maxRotate = 0.f * (float)CV_PI;

	randInit();
}

GPMProc::~GPMProc(void)
{
}

DenseCorr* GPMProc::RunGPM(cvi* src, cvi* dst)
{
	if(src == NULL || dst == NULL) return NULL;

	int w = dst->width, h = dst->height;
	Interval hItvl(_f m_patchOffset, _f h - 1 - m_patchOffset);
	Interval wItvl(_f m_patchOffset, _f w - 1 - m_patchOffset);

	int sw = src->width, sh = src->height;
	Interval shItvl(_f m_patchOffset, _f sh - 1 - m_patchOffset);
	Interval swItvl(_f m_patchOffset, _f sw - 1 - m_patchOffset);

	//random initialize
	DenseCorr* dsCor = new DenseCorr(w, h, m_knn, m_patchOffset);
	dsCor->RandomInitialize(swItvl, shItvl, Interval(m_minScale, m_maxScale), 
		Interval(m_minRotate, m_maxRotate));
	dsCor->UpdatePatchDistance(dst, src, m_metric);
	//dsCor->ShowCorr("init.png");
	//dsCor->ShowCorrDist("initDist.png");

	RunGPMWithInitial(src, dst, dsCor);

	return dsCor;
}

void GPMProc::RunGPMWithInitial(cvi* src, cvi* dst, DenseCorr* dsCor)
{

	if(src == NULL || dst == NULL) return;

	int w = dst->width, h = dst->height;
	Interval hItvl(_f m_patchOffset, _f h - 1 - m_patchOffset);
	Interval wItvl(_f m_patchOffset, _f w - 1 - m_patchOffset);

	int sw = src->width, sh = src->height;
	Interval shItvl(_f m_patchOffset, _f sh - 1 - m_patchOffset);
	Interval swItvl(_f m_patchOffset, _f sw - 1 - m_patchOffset);

	//iteration
	doF(kItr, m_nItr)
	{
		nCores = 1;
		doF(k, nCores)
		{
			float hStart = hItvl.min, hEnd = hItvl.max;
			float wStart = wItvl.min, wEnd = wItvl.max;
			if(m_nItr % 4 == 0 || m_nItr % 4 == 1)
			{
				float hD = (hEnd - hStart) / nCores;
				hStart = hD * k + hStart;
				hEnd = hStart + hD;
			}
			else
			{
				float wD = (wEnd - wStart) / nCores;
				wStart = wD * k + wStart;
				wEnd = wStart + wD;
			}

			doFs(_x, _i hStart, _i hEnd) doFs(_y, _i wStart, _i wEnd)
			{
				int x = _x, y = _y;
				if(kItr % 2 == 1){
					x = _i hItvl.min + _i hItvl.max - x;
					y = _i wItvl.min + _i wItvl.max - y;
				}
				Propagate(src, dst, x, y, (kItr % 2 == 1), *dsCor, wItvl, hItvl, 
					swItvl, shItvl);
				RandomSearch(src, dst, x, y, *dsCor, swItvl, shItvl);
			}
		}
	}
}

//todo: patch pre generate
void GPMProc::Propagate(cvi* src, cvi* dst, int x, int y, bool direction, 
	DenseCorr& dsCor, Interval wItvl, Interval hItvl, 
	Interval swItvl, Interval shItvl)
{
	int offset[4][2] = {{1, 0}, {0, 1}, {0, -1}, {-1, 0}};
	int offsetIdxStart = 0;
	int offsetIdxEnd = 2;
	if(!direction){
		offsetIdxStart = 2;
		offsetIdxEnd = 4;
	}

	Patch dstPatch = m_metric->GetPatch(dst, _f x, _f y, 1.f, 0.f, m_patchOffset);
	float distThres = dsCor.GetDistThres(x, y);
	doFs(i, offsetIdxStart, offsetIdxEnd)
	{
		int dx = offset[i][0], dy = offset[i][1];
		if(!cvIn(x + dx, y + dy, _i hItvl.min, _i hItvl.max, 
			_i wItvl.min, _i wItvl.max))
			continue;

		int corrOffIdx = dsCor.GetCorrIdx(x + dx, y + dy);
		doF(k, m_knn)
		{
			Corr& v = dsCor.Get(corrOffIdx + k);
			float angle = atan2(-(float)dx, -(float)dy);
			float newX = clamp(v.x + sin(angle - v.r) * v.s, shItvl.min, shItvl.max);
			float newY = clamp(v.y + cos(angle - v.r) * v.s, swItvl.min, swItvl.max);
			Patch srcPatch = m_metric->GetPatch(src, newX, newY, v.s, v.r, m_patchOffset);
			float patchDist = m_metric->ComputeVectorDist(dstPatch, srcPatch);
			//float patchDist = ComputePatchDist(src, dst, x, y, newX, newY, v.s, v.r);
			if(patchDist < distThres){
				dsCor.AddCoor(x, y, newX, newY, v.s, v.r, patchDist);
				distThres = dsCor.GetDistThres(x, y);
			}
		}
	}
}

void GPMProc::RandomSearch(cvi* src, cvi* dst, int x, int y, 
	DenseCorr& dsCor, Interval wItvl, Interval hItvl)
{
	float distThres = dsCor.GetDistThres(x, y);
	int idx = dsCor.GetCorrIdx(x, y);

	vector<Corr> corrs(m_knn);
	doF(k, m_knn) corrs[k] = dsCor.Get(idx + k);

	Patch dstPatch = m_metric->GetPatch(dst, _f x, _f y, 1.f, 0.f, m_patchOffset);

	float scaleFactor = 0.5f;
	doF(k, m_knn)
	{
		Corr& v = corrs[k];
		float hSpace = hItvl.max - hItvl.min;
		float wSpace = wItvl.max - wItvl.min;
		float sSpace = m_maxScale - m_minScale;
		float rSpace = m_maxRotate - m_minRotate;
		while(hSpace > 1 && wSpace > 1)
		{
			float newx = clamp(hSpace * (2 * rand1() - 1) + v.x, hItvl.min, hItvl.max);
			float newy = clamp(wSpace * (2 * rand1() - 1) + v.y, wItvl.min, wItvl.max);
			float news = clamp(sSpace * (2 * rand1() - 1) + v.s, m_minScale, m_maxScale);
			float newr = clamp(rSpace * (2 * rand1() - 1) + v.r, m_minRotate, m_maxRotate);
			
			Patch srcPatch = m_metric->GetPatch(src, newx, newy, news, newr, m_patchOffset);
			float patchDist = m_metric->ComputeVectorDist(dstPatch, srcPatch);
			//float patchDist = ComputePatchDist(src, dst, x, y, newx, newy, news, newr);
			
			if(patchDist < distThres){
				dsCor.AddCoor(x, y, newx, newy, news, newr, patchDist);
				distThres = dsCor.GetDistThres(x, y);
			}
			hSpace *= scaleFactor; wSpace *= scaleFactor;
			sSpace *= scaleFactor; rSpace *= scaleFactor;
		}
	}
}

float GPMProc::ComputePatchDist(cvi* src, cvi* dst, int x, int y, float sx, float sy, float sScale, float sRot)
{
	return m_metric->ComputePatchDist(dst, _f x, _f y, src, sx, sy, sScale, sRot, m_patchOffset);
}

DenseCorrBox2D::DenseCorrBox2D(int nWidth, int nHeight)
{
	SetSize(nWidth, nHeight);
}

void DenseCorrBox2D::SetSize(int nWidth, int nHeight)
{
	m_nHeight = nHeight; m_nWidth = nWidth;
	m_box.resize(m_nHeight * m_nWidth, NULL);
	m_srcH = m_srcW = 100;
}

DenseCorrBox2D::~DenseCorrBox2D()
{
	doFv(i, m_box)
	{
		if(m_box[i] != NULL) delete m_box[i];
	}
}

void DenseCorrBox2D::ShowCorr(string imgStr)
{
	int w = GetValue(0, 0)->m_width * m_nWidth, 
		h = GetValue(0, 0)->m_height * m_nHeight;
	
	cvi* res = cvci83(w, h);
	DenseCorr* corr;
	doF(i, m_nHeight) doF(j, m_nWidth)
	{
		corr = GetValue(i, j);
		doF(r, corr->m_height) doF(c, corr->m_width)
		{
			Corr& v = corr->m_values[(r*corr->m_width + c) * corr->m_knn];
			cvs2(res, r + i * corr->m_height, c + j * corr->m_width, 
				cvs(0, v.y / m_srcW * 255.0, v.x / m_srcH * 255.0));
		}
	}

	cvsi(imgStr, res);
	cvri(res);
}

MultiCorr DenseCorrBox2D::GetCorrsPerGrid(int r, int c)
{	
	MultiCorr result(0);
	if(!cvIn(r, c, 0, GetValue(0, 0)->m_height, 0, GetValue(0, 0)->m_width)) return result;

	result.resize(m_nWidth * m_nHeight); 
	doF(i, m_nHeight) doF(j, m_nWidth)
	{
		int idx = GetValue(i, j)->GetCorrIdx(r, c);
		Corr& v = GetValue(i, j)->Get(idx);
		Corr v2 = v;
		v2.x += hInts[i].min; v2.y += wInts[j].min;
		result[i*m_nWidth+j] = v2;
	}
	return result;
}

void DrawRectToCvi(cvi* img, float x, float y, float radius = 3.f, 
	cvS color = cvs(0, 0, 255), float s = 1.0f, float r = 0.0f, int w = 1)
{
	vector<CvPoint> points(4);
	doF(i, 4)
	{
		float preAngle = _f CV_PI / 2 * (_f i  + 0.5f);
		float spRowCur = x + radius * sin(preAngle - r) * s;
		float spColCur = y + radius * cos(preAngle - r) * s;
		points[i] = cvPoint(round(spColCur), round(spRowCur));
	}
	doF(i, 4)
	{
		cvLine(img, points[i], points[(i+1)%4], color, w);
	}
}

void DenseCorrBox2D::ShowGridCorrs(CvRect& roi, int r, int c, int radius, cvi* src, string imgStr)
{
	cvi* res = cvci(src);
	ShowGridCorrs(roi, r, c, radius, src, res);
	cvsi(imgStr, res);
	cvri(res);
}

void DenseCorrBox2D::ShowGridCorrs(CvRect& roi, int r, int c, int radius, cvi* src, cvi* res)
{
	MultiCorr corrs = GetCorrsPerGrid(r, c);
	int basicRadius = radius;
	doFv(i, corrs)
	{
		float dist = clamp(corrs[i].dist / 200.0f, 0.f, 1.f);
		float alpha = (1 - dist); 
		DrawRectToCvi(res, corrs[i].x, corrs[i].y, _f basicRadius, 
			cvs(0, 0, 255*alpha), corrs[i].s, corrs[i].r);
	}
	DrawRectToCvi(res, _f (roi.y + r), _f (roi.x + c), _f basicRadius, 
		cvs(255, 0, 0));
}

GridGPMProc::GridGPMProc(PatchDistMetric* metric, int gridSize, int gridStep,
	int knn, int patchSize, int nItr):m_gridSize(gridSize), m_gridStep(gridStep), 
	m_patchSize(patchSize)
{
	m_roi = cvRect(0, 0, gridSize, gridSize);
	m_proc = new GPMProc(metric, knn, patchSize, nItr);
}

GridGPMProc::~GridGPMProc(void)
{
	delete m_proc;
}

void GridGPMProc::SetROI(CvRect roi)
{
	m_roi = roi;
}

void GridGPMProc::RunGridGPM(cvi* src, string saveFile)
{	
	DenseCorrBox2D box;
	InitDenseBox2D(src->width, src->height, box);

	RunGridGPM(src, box);

	if(saveFile != "") box.Save(saveFile);

	//box.ShowCorr("temp.png");
	//box.ShowGridCorrs(m_roi, 10, 10, GetPatchRadius(), src, "temp2.png");
	//cout<<box.GetCorrsPerGrid(10, 10);
}

void GridGPMProc::InitDenseBox2D(int srcW, int srcH, DenseCorrBox2D& box)
{
	vector<Interval> wInts, hInts;
	int temp = 0;
	while(temp + m_gridSize <= srcW){
		wInts.push_back(Interval(_f temp, _f temp + m_gridSize));
		temp += m_gridStep;
	}
	if(temp - m_gridStep + m_gridSize< srcW) 
		wInts.push_back(Interval(_f temp, _f srcW));
	temp = 0;
	while(temp + m_gridSize <= srcH){
		hInts.push_back(Interval(_f temp, _f temp + m_gridSize));
		temp += m_gridStep;
	}
	if(temp - m_gridStep + m_gridSize < srcH) 
		hInts.push_back(Interval(_f temp, _f srcH));
	int nGridw = wInts.size(), nGridh = hInts.size();

	box.SetSize(nGridw, nGridh);
	box.SetSrcSize(m_gridSize, m_gridSize);
	box.SetIntevals(wInts, hInts);
}

void GridGPMProc::RunGridGPM(cvi* src, DenseCorrBox2D& box, bool initValue)
{
	if(!cvIn(m_roi.y, m_roi.x, src) || 
		!cvIn(m_roi.y + m_roi.height - 1, m_roi.x + m_roi.width - 1, src))
	{
		cout<<"ROI wrong error.\n";
		return;
	}

	cvi* gpmDst = cvci83(m_roi.width, m_roi.height);
	cvSetImageROI(src, m_roi);
	cvCopy(src, gpmDst);

	int nGridw = box.wInts.size(), nGridh = box.hInts.size();
	vector<Interval>& wInts = box.wInts;
	vector<Interval>& hInts = box.hInts;

	omp_set_num_threads(nCores);
#pragma omp parallel for
	doF(k, nGridh * nGridw)
	{
		int i = k / nGridw;
		int j = k % nGridw;
		cout<<"\rPatch-match processing grid ("<<i<<", "<<j<<")";
		int gridW = _i wInts[j].max - _i wInts[j].min, gridH = _i hInts[i].max - _i hInts[i].min;

		cvi* gpmSrc = cvci83(gridW, gridH);
		cvSetImageROI(src, cvRect(_i wInts[j].min, _i hInts[i].min, gridW, gridH));
		cvCopy(src, gpmSrc);
		
		if(!initValue)
		{
			DenseCorr* corr = m_proc->RunGPM(gpmSrc, gpmDst);
			box.SetValue(i, j, corr);
		}
		else
		{
			m_proc->RunGPMWithInitial(gpmSrc, gpmDst, box.GetValue(i, j));
		}

		cvri(gpmSrc);
	}

	cvResetImageROI(src);
	cvri(gpmDst);
}

void GridGPMProc::RunGridGPMMultiScale(cvi* src, string saveFile, int rat, int levels)
{
	int ratio = round(pow(_f rat, _f levels));
	int downRatio = rat;

	//init
	DenseCorrBox2D box;
	m_gridSize /= ratio;
	m_gridStep /= ratio;
	InitDenseBox2D(src->width / ratio, src->height / ratio, box);

	bool first = false;
	while(ratio >= 1)
	{
		cvi* srcResize = cvci83(src->width / ratio, src->height / ratio);
		cvResize(src, srcResize);
		SetROI(cvRect(0, 0, srcResize->width, srcResize->height));

		RunGridGPM(srcResize, box, first);
		first = true;

		cvri(srcResize);

		if(ratio == 1) break;
		ratio /= downRatio;
		//box.ShowCorr("temp.png");

		box.LevelUp(downRatio);
		//box.ShowCorr("temp2.png");
	}
	cout<<"\rPatch match complete.\n";

	box.Save(saveFile);

}

//test ui function
struct UI_param
{
	string ui_winTitle, ui_winTitle2;
	cvi* ui_src, *ui_srcHLS;
	GridGPMProc* ui_proc;
	DenseCorrBox2D* ui_box;
	float distThres;
};
bool LDown = false;
void UIMouseClick(int event, int x, int y, int flags, void *param)
{
	if(event == CV_EVENT_LBUTTONDOWN) LDown = true;
	else if(event == CV_EVENT_LBUTTONUP) LDown = false;
	if(!LDown) return;
	UI_param* uiParam = (UI_param*)param;
	cvi* ui_src = uiParam->ui_src, *ui_srcHLS = uiParam->ui_srcHLS;
	GridGPMProc* ui_proc = uiParam->ui_proc;
	DenseCorrBox2D* ui_box = uiParam->ui_box;

	//show rects in src img
	cvi* show = cvci(ui_src);
	CvRect& rect = ui_proc->GetROI();
	ui_box->ShowGridCorrs(rect, y - rect.y, x - rect.x, ui_proc->GetPatchRadius(), 
		ui_src, show);
	cvShowImage(uiParam->ui_winTitle.c_str(), show);
	//cvsi("_1.png", show);
	cvri(show);

	//show luminance hists of the patch group
	float dstLmnc = PatchLmncProc::GetAvgLmnc(PatchDistMetric::GetPatch(ui_srcHLS, 
		_f y, _f x, 1.f, 0.f, ui_proc->GetPatchRadius()));
	MultiCorr corrs = ui_box->GetCorrsPerGrid(y - rect.y, x - rect.x);
	LmncVec srcLmncVec(corrs.size());
	doFv(i, srcLmncVec)
	{
		Patch srcPatch = PatchDistMetric::GetPatch(ui_srcHLS, corrs[i].x, 
			corrs[i].y, corrs[i].s, corrs[i].r, ui_proc->GetPatchRadius());
		srcLmncVec[i].first = PatchLmncProc::GetAvgLmnc(srcPatch);
		srcLmncVec[i].second = corrs[i].dist;
	}
	LmncHist hist = PatchLmncProc::GetLmncHist(srcLmncVec, uiParam->distThres);
	cvi* histImg = PatchLmncProc::ShowHistInCvi(hist, PatchLmncProc::GetHistIdx(dstLmnc));
	cvShowImage(uiParam->ui_winTitle2.c_str(), histImg);
	//cvsi("_2.png", histImg);
	cvri(histImg);
}
void GridGPMProc::ShowGPMResUI(cvi* src, cvi* srcHLS, string fileStr, float distThres)
{
	DenseCorrBox2D box;
	UI_param param;
	param.ui_proc = this;
	param.ui_src = src;
	param.ui_srcHLS = srcHLS;
	box.Load(fileStr);
	param.ui_box = &box;
	param.ui_winTitle = "Image";
	param.ui_winTitle2 = "Hist";
	param.distThres = distThres;

	cvNamedWindow(param.ui_winTitle.c_str(), WINDOW_AUTOSIZE);
	cvNamedWindow(param.ui_winTitle2.c_str(), WINDOW_AUTOSIZE);
	cvShowImage(param.ui_winTitle.c_str(), src);

	setMouseCallback(param.ui_winTitle.c_str(), UIMouseClick, &param);
	cvWaitKey(0);
	cvDestroyWindow(param.ui_winTitle.c_str());
}

int PatchLmncProc::histN = 25;

float PatchLmncProc::GetAvgLmnc(Patch& vs)
{
	//hls
	float sum = 0;
	doFv(i, vs) sum += _f vs[i].val[1];
	return sum / vs.size();
}

float PatchLmncProc::GetAvgSatu(Patch& vs)
{
	//hls
	float sum = 0;
	doFv(i, vs) sum += _f vs[i].val[2];
	return sum / vs.size();
}

int PatchLmncProc::GetHistIdx(float lmnc)
{
	return _i floor(lmnc * histN / 255);
}

LmncHist PatchLmncProc::GetLmncHist(LmncVec& lmncVec, float distThres)
{
	LmncHist hist(histN);
	doFv(i, lmncVec)
	{
		int idx = GetHistIdx(lmncVec[i].first);
		hist[idx].first++;
		if(lmncVec[i].second < distThres)
			hist[idx].second++;
	}
	return hist;
}

cvi* PatchLmncProc::ShowHistInCvi(LmncHist& hist, int focusIdx)
{
	int histWidth = 10;
	int showHeight = 100;
	int showHRatio = 10;
	int gap = 1;
	cvi* img = cvci83(histWidth * hist.size(), 100);
	cvZero(img);
	doFv(i, hist)
	{
		cvS color = cvs(0, 0, 255);
		if(i == focusIdx) color = cvs(255, 0, 0);
		int histHeight = showHRatio * hist[i].first;
		int histHeight2 = showHRatio * hist[i].second;
		cvRectangle(img, cvPoint(i*histWidth + gap, showHeight-histHeight),
			cvPoint((i+1)*histWidth - gap, showHeight), color, -1);
		cvRectangle(img, cvPoint(i*histWidth + gap, showHeight-histHeight),
			cvPoint((i+1)*histWidth - gap, showHeight-histHeight2), color / 3, -1);
	}
	return img;
}

void Corr::Save(ostream& fout)
{
	fout<<x<<" "<<y<<" "<<s<<" "<<r<<" "<<dist<<endl;
}

void Corr::Load(istream& fin)
{
	fin>>x>>y>>s>>r>>dist;
}

void DenseCorr::Save(ostream& fout)
{
	fout<<m_width<<" "<<m_height<<" "<<m_knn<<" "<<m_patchOffset<<" ";
	fout<<m_values.size()<<endl;
	doFv(i, m_values) m_values[i].Save(fout);
}

void DenseCorr::Load(istream& fin)
{
	fin>>m_width>>m_height>>m_knn>>m_patchOffset;
	int size; fin>>size;
	m_values.resize(size);
	doFv(i, m_values) m_values[i].Load(fin);
}

void DenseCorrBox2D::Save(string file)
{
	ofstream fout(file.c_str());
	fout<<m_nWidth<<" "<<m_nHeight<<" "<<m_srcW<<" "<<m_srcH<<endl;
	fout<<m_box.size()<<endl;
	doFv(i, m_box) m_box[i]->Save(fout);
	fout<<wInts.size()<<endl;
	doFv(i, wInts) fout<<wInts[i].min<<" "<<wInts[i].max<<endl;
	fout<<hInts.size()<<endl;
	doFv(i, hInts) fout<<hInts[i].min<<" "<<hInts[i].max<<endl;
	fout.close();
}

void DenseCorrBox2D::Load(string file)
{
	ifstream fin(file.c_str());
	if(!fin)
	{
		cout<<file<<" not exist error!\n";
		return;
	}
	fin>>m_nWidth>>m_nHeight>>m_srcW>>m_srcH;
	int size; fin>>size; m_box.resize(size);
	doFv(i, m_box)
	{
		m_box[i] = new DenseCorr();
		m_box[i]->Load(fin);
	}
	fin>>size; wInts.resize(size);
	doFv(i, wInts) fin>>wInts[i].min>>wInts[i].max;
	fin>>size; hInts.resize(size);
	doFv(i, hInts) fin>>hInts[i].min>>hInts[i].max;
	fin.close();
}

void DenseCorrBox2D::LevelUp(int ratio)
{
	m_srcH *= ratio; m_srcW *= ratio;
	doFv(i, m_box)
		m_box[i]->LevelUp(ratio);
	doFv(i, wInts)
	{
		wInts[i].min *= ratio;
		wInts[i].max *= ratio;
	}
	doFv(i, hInts)
	{
		hInts[i].min *= ratio;
		hInts[i].max *= ratio;
	}
}

void DenseCorr::LevelUp(int ratio)
{
	int newWidth = m_width * ratio, newHeight = m_height * ratio;

	vector<Corr> newValues(newWidth * newHeight * m_knn);

	doF(i, m_height) doF(j, m_width) doF(k, m_knn)
	{
		int oldIdx = GetCorrIdx(i, j) + k;
		Corr& oldV = m_values[oldIdx];

		doF(oi, ratio) doF(oj, ratio)
		{
			int idx = (i * ratio + oi) * newWidth + (j * ratio + oj) + k;
			Corr& newV = newValues[idx];

			newV.s = oldV.s;
			newV.r = oldV.s;
			newV.dist = oldV.dist;

			float dx = (1.5f - _f ratio) + oi;
			float dy = (1.5f - _f ratio) + oj;
			float angle = atan2((float)dx, (float)dy);
			float len = pow(dx*dx+dy*dy, 0.5f);
			newV.x = ratio * oldV.x + sin(angle - oldV.r) * oldV.s * len;
			newV.y = ratio * oldV.y + cos(angle - oldV.r) * oldV.s * len;

		}

	}

	m_values = newValues;
	m_width = newWidth;
	m_height = newHeight;
}