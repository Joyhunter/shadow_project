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

float PatchDistMetric::ComputePatchDist(cvi* dst, float dpRow, float dpCol, 
	cvi* src, float spRow, float spCol, float spScale, float spRotate, 
	int patchOffset)
{
	int nPixels = sqr(2*patchOffset + 1);
	vector<cvS> vd(nPixels), vs(nPixels);
	int idx = 0;
	doFs(pr, -patchOffset, patchOffset+1) doFs(pc, -patchOffset, patchOffset+1)
	{
		float dpRowCur = dpRow + pr, dpColCur = dpCol + pc;
		float preAngle = atan2(_f(pr), _f(pc));
		float length = sqrt(sqr(_f(pr)) + sqr(_f(pc)));
		float spRowCur = spRow + length * sin(preAngle - spRotate) * spScale;
		float spColCur = spCol + length * cos(preAngle - spRotate) * spScale;
		vd[idx] = cvg2(dst, dpRowCur, dpColCur);
		vs[idx] = cvg2(src, spRowCur, spColCur);
		idx++;
	}
	return ComputeVectorDist(vd, vs);
}

float RegularPatchDistMetric::ComputeVectorDist(vector<cvS>& vDst, vector<cvS>& vSrc)
{
	float sum = 0;
	doFv(i, vDst)
	{
		sum += _f cvSDSqr(vDst[i], vSrc[i]);
	}
	return sqrt(sum / vDst.size()); 
}

float LmnIvrtPatchDistMetric::ComputeVectorDist(vector<cvS>& vDst, vector<cvS>& vSrc)
{
	//hsv: dst(v)*alpha = src
	int useAlpha[] = {-1, -1, 1};
	float useChannels[] = {1, 0, 1};
	//rgb: dst(rgb)*alpha = src
	//int useAlpha[] = {1, 1, 1};
	//float useChannels[] = {1, 1, 1};

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
			dstV.val[k] *= useChannels[k];
			srcV.val[k] *= useChannels[k];
		}
		sum += _f cvSDSqr(dstV, srcV);
	}
	return sqrt(sum / vDst.size());

	
}

GPMProc::GPMProc(PatchDistMetric* metric, int knn, int patchSize, int nItr):
	m_metric(metric), m_knn(knn), m_patchSize(patchSize), m_nItr(nItr)
{
	m_patchOffset = (m_patchSize - 1) / 2;

	m_minScale = 0.8f;
	m_maxScale = 1.25f;
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

	//iteration
	doF(kItr, m_nItr)
	{
		//cout<<"\rIteration "<<kItr<<"...";

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
		//dsCor->ShowCorr("res.png");
		//dsCor->ShowCorrDist("resDist.png");
	}
	return dsCor;
}

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
			float patchDist = ComputePatchDist(src, dst, x, y, newX, newY, v.s, v.r);
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
			float patchDist = ComputePatchDist(src, dst, x, y, newx, newy, news, newr);
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

DenseCorrBox2D::DenseCorrBox2D(int nWidth, int nHeight):m_nWidth(nWidth), m_nHeight(nHeight)
{
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

vector<Corr> DenseCorrBox2D::GetCorrsPerGrid(int r, int c)
{	
	vector<Corr> result(0);
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
	vector<Corr> corrs = GetCorrsPerGrid(r, c);
	int basicRadius = radius;
	doFv(i, corrs)
	{
		float dist = clamp(corrs[i].dist / 50.0f, 0.f, 1.f);
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
	if(!cvIn(m_roi.y, m_roi.x, src) || 
		!cvIn(m_roi.y + m_roi.height - 1, m_roi.x + m_roi.width - 1, src))
	{
		cout<<"ROI wrong error.\n";
		return;
	}

	cvi* gpmDst = cvci83(m_roi.width, m_roi.height);
	cvSetImageROI(src, m_roi);
	cvCopy(src, gpmDst);

	vector<Interval> wInts, hInts;
	int temp = 0;
	while(temp + m_gridSize <= src->width){
		wInts.push_back(Interval(_f temp, _f temp + m_gridSize));
		temp += m_gridStep;
	}
	if(temp - m_gridStep + m_gridSize< src->width) 
		wInts.push_back(Interval(_f temp, _f src->width));
	temp = 0;
	while(temp + m_gridSize <= src->height){
		hInts.push_back(Interval(_f temp, _f temp + m_gridSize));
		temp += m_gridStep;
	}
	if(temp - m_gridStep + m_gridSize < src->height) 
		hInts.push_back(Interval(_f temp, _f src->height));
	int nGridw = wInts.size(), nGridh = hInts.size();
	
	DenseCorrBox2D box(nGridw, nGridh);
	box.SetSrcSize(m_gridSize, m_gridSize);
	box.SetIntevals(wInts, hInts);

	omp_set_num_threads(nCores);
#pragma omp parallel for
	doF(i, nGridh) doF(j, nGridw)
	{
		cout<<"\rProcessing grid ("<<i<<", "<<j<<")";
		int gridW = _i wInts[j].max - _i wInts[j].min, gridH = _i hInts[i].max - _i hInts[i].min;

		cvi* gpmSrc = cvci83(gridW, gridH);
		cvSetImageROI(src, cvRect(_i wInts[j].min, _i hInts[i].min, gridW, gridH));
		cvCopy(src, gpmSrc);

		DenseCorr* corr = m_proc->RunGPM(gpmSrc, gpmDst);

		box.SetValue(i, j, corr);
		cvri(gpmSrc);
	}

	cvResetImageROI(src);
	if(saveFile != "") box.Save(saveFile);
	
	box.ShowCorr("temp.png");
	box.ShowGridCorrs(m_roi, 10, 10, GetPatchRadius(), src, "temp2.png");
	cout<<box.GetCorrsPerGrid(10, 10);


	cvri(gpmDst);
}

string ui_winTitle = "Click to select patch";
cvi* ui_src;
GridGPMProc* ui_proc;
DenseCorrBox2D* ui_box;
void UIMouseClick(int event,int x,int y,int flags,void *param)
{
	if(event != CV_EVENT_LBUTTONDOWN) return;
	cout<<x<<" "<<y<<endl;

	cvi* show = cvci(ui_src);
	CvRect& rect = ui_proc->GetROI();
	ui_box->ShowGridCorrs(rect, y - rect.y, x - rect.x, ui_proc->GetPatchRadius(), 
		ui_src, show);
	cvShowImage(ui_winTitle.c_str(), show);
	cvri(show);
}
void GridGPMProc::ShowGPMResUI(cvi* src, string fileStr)
{
	DenseCorrBox2D box;
	ui_proc = this;
	ui_src = src;
	box.Load(fileStr);
	ui_box = &box;

	cvNamedWindow(ui_winTitle.c_str(), WINDOW_AUTOSIZE);
	cvShowImage(ui_winTitle.c_str(), src);

	setMouseCallback(ui_winTitle.c_str(), UIMouseClick, 0);
	cvWaitKey(0);
	cvDestroyWindow(ui_winTitle.c_str());

// 	box.ShowCorr("temp.png");
// 	box.ShowGridCorrs(m_roi, 30, 30, GetPatchRadius(), src, "temp2.png");
// 	cout<<box.GetCorrsPerGrid(10, 10);
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

