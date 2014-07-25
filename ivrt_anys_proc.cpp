#include "StdAfx.h"
#include "ivrt_anys_proc.h"

IvrtAnysProc::IvrtAnysProc(void){}

IvrtAnysProc::~IvrtAnysProc(void){}

void IvrtAnysProc::IvrtAnalysis(cvi* _image)
{
	float ratio = 1.0f;
	//cvi* image = cvci83(_i (_image->width * ratio), _i (_image->height * ratio));
	cvi* image = cvci83(200, 200);
	cvResize(_image, image);

	double epsilon = 1e-3;
	cvi* data = cvci323(image);
	doFcvi(image, i, j)
	{
		cvS v = cvg2(image, i, j);
		v = clamp(v, cvs(epsilon, epsilon, epsilon), cvs(255, 255, 255));
		cvs2(data, i, j, cvs(log(v.val[2] / v.val[0]), log(v.val[1] / v.val[0])));
	}

	double minEn = 1e30;
	int minDeg = -1;
	int N = data->width * data->height;
	doFs(deg, 0, 180)
	{
		double angle = CV_PI / 180 * deg;
		double sinA = sin(angle), cosA = cos(angle);

		vector<double> vals(N);
		doFcvi(data, i, j)
		{
			auto v = cvg2(data, i, j);
			double val = 255*(v.val[0] * cosA - v.val[1] * sinA);
			vals[i*data->width+j] = val;
		}

		double maxVal = -1e10, minVal = 1e10;
		doFv(i, vals)
		{
			double v = vals[i];
			if(v > maxVal) maxVal = v;
			if(v < minVal) minVal = v;
		}
		int binN = 64;
		vector<double> hist(binN, 0);
		doFv(i, vals)
		{
			int idx = round((vals[i]-minVal)/(maxVal-minVal)*255)/4;
			if(idx == binN) idx = binN - 1;
			hist[idx]++;
		}
		double en = 0;
		doF(k, binN)
		{
			if(hist[k] == 0) hist[k] = 1e-3;
			else hist[k] /= N;
			en += hist[k] * log(hist[k]);
		}
		en = -en;

		if(en < minEn)
		{
			minEn = en;
			minDeg = deg;
		}
		cout<<en<<" ";
	}
	cout<<minDeg<<endl;

	double angle = CV_PI / 180 * minDeg;
	double sinA = sin(angle), cosA = cos(angle);
	double maxVal = -1e10, minVal = 1e10;
	doFcvi(data, i, j)
	{
		auto v = cvg2(data, i, j);
		double val = exp(v.val[0] * cosA - v.val[1] * sinA);
		if(val > maxVal) maxVal = val;
		if(val < minVal) minVal = val;
	}
	cvi* res = cvci81(image);
	doFcvi(data, i, j)
	{
		auto v = cvg2(data, i, j);
		double val = exp(v.val[0] * cosA - v.val[1] * sinA);
		int pixelVal = _i((val - minVal) / (maxVal - minVal) * 255);
		cvs20(res, i, j, pixelVal);
	}
	cvsi(res);
	cvri(res);

	cvri(image);
	cvri(data);
}

cvi* IvrtAnysProc::IvrtAnalysis2(cvi* image)
{

	//map points to 2D plane
	cvi* data = cvci323(image);
	double XX1[3] = {1.0 / sqrt(2.0), -1.0 / sqrt(2.0), 0};
	double XX2[3] = {-1.0 / sqrt(6.0), -1.0 / sqrt(6.0), 2.0 / sqrt(6.0)};
	doFcvi(image, i, j)
	{
		cvS v = cvg2(image, i, j);
		doF(k, 3)
		{
			if(v.val[k] == 0) v.val[k] = 1;
		}
		double pro = pow(v.val[0]*v.val[1]*v.val[2], 1.0/3.0);

		doF(k, 3) v.val[k] = log(v.val[k] / pro);

		double x1 = 0, x2 = 0;
		doF(k, 3)
		{
			x1 += v.val[k] * XX1[k];
			x2 += v.val[k] * XX2[k];
		}
		cvs2(data, i, j, cvs(_f x1, _f x2, 0));
	}

	//iterate all angle
	int N = data->width * data->height;
	double maxEntropy = -1e30, maxIdx = -1;
	int angleN = 180;
	doFs(theta, 0, angleN)
	{
		double angle = CV_PI / angleN * theta;
		double sinA = sin(angle), cosA = cos(angle);

		//compute mapping to a line
		vector<double> vals(N);
		doFcvi(data, i, j)
		{
			auto v = cvg2(data, i, j);
			double val = v.val[0] * cosA + v.val[1] * sinA;
			vals[i*data->width+j] = val;
		}
		
		//throw high and low 5%
		sort(vals.begin(), vals.end());
		double throwRatio = 0.05;
		int start = _i(_f N*throwRatio), end = N - start, N2 = end - start;

		//get max, min 
		double maxData = -1e10, minData = 1e10, avgData = 0;
		doFs(k, start, end)
		{
			double v = vals[k];
			if(v > maxData) maxData = v;
			if(v < minData) minData = v;
			avgData += v;
		}
		avgData /= N2;

		//map into 64 bins
		double binWid = (maxData - minData) / 64;
		map<int, int> values;
		doFs(k, start, end)
		{
			double v = vals[k];
			int idx = _i floor((v - minData) / binWid);
			if(values.find(idx) == values.end()) values[idx] = 1;
			else values[idx]++;
		}

		//compute entropy
		double entropy = 0;
		for(auto p = values.begin(); p != values.end(); p++)
		{
			double per = (double)p->second / N;
			entropy += per * log(per);
		}
		if(entropy > maxEntropy)
		{
			maxEntropy = entropy;
			maxIdx = theta;
		}
		//cout<<-entropy<<" ";
	}
	cout<<"Min entropy appears at "<<_f maxIdx * 180 / angleN<<" degrees."<<endl;

	//output invariant image
	cvi* temp = cvci83(data);
	double angle = CV_PI / angleN * maxIdx;
	double sinA = sin(angle), cosA = cos(angle);

	doFcvi(data, i, j)
	{
		auto v = cvg2(data, i, j);
		double val = v.val[0] * cosA + v.val[1] * sinA;
		//to 2d
		double x2d = val * cosA, y2d = val * sinA;
		//to 3d
		cvS val3D;
		doF(k, 3) val3D.val[k] = exp(x2d * XX1[k] + y2d * XX2[k]);
		double sum3D = val3D.val[0] + val3D.val[1] + val3D.val[2];
		doF(k, 3) val3D.val[k] /= sum3D;
		cvs2(temp, i, j, val3D * 255.0);
	}

	cvri(data);
	return temp;
}

//cvi* forShow = cvci83(data);
//cvNamedWindow("123");
//
// 		double sigma = 0;
// 		doFs(k, start, end)
// 		{
// 			double v = vals[k];
// 			sigma += pow(v - avgData, 2.0);
// 		}
// 		sigma = pow(sigma / (N2-1), 0.5);
// 		double binWid = 3.5 * sigma / pow(N2, 1.0/3.0);
// 		if(binWid == 0)
// 		{
// 			cout<<"Error!"<<endl;
// 		}


// 		doFcvi(data, i, j)
// 		{
// 			auto v = cvg2(data, i, j);
// 			double val = v.val[0] * cosA + v.val[1] * sinA;
// 			int idx = _i((val - minData)*255 / (maxData-minData));
// 			cvs2(forShow, i, j, cvs(idx, idx, idx));
// 		}
//cvShowImage("123", forShow);
//cvWaitKey(0);