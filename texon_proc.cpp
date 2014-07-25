#include "StdAfx.h"
#include "texon_proc.h"

TexonAnysProc::TexonAnysProc(void)
{
}

TexonAnysProc::~TexonAnysProc(void)
{
}


float Filter::Conv(cvi* img, int i_, int j_)
{
	float res = 0;
	int s = -(r-1)/2;
	
	doFs(i, i_+s, i_+s+r) doFs(j, j_+s, j_+s+r)
	{
		int _r = i, _c = j; 
		if(_r < 0) _r = -_r;
		else if(_r > img->height - 1) _r = img->height*2-2-_r;
		if(_c < 0) _c = -_c;
		else if(_c > img->width - 1) _c = img->width*2-2-_c; //cout<<_r<<" "<<_c<<" "<<i-s<<" "<<j-s<<endl;
		res += _f cvg20(img, _r, _c) * weights[i-s-i_][j-s-j_];
	}
	return res;
}

cvi* TexonAnysProc::TexonAnalysis(cvi* image)
{
	FilterBank fb = FbCreate(8, 0.5, 1, sqrt(2.f), 3);
	int N = image->width * image->height;

	CvMat* data = cvCreateMat(N, fb.values.size(), CV_32FC1);
	doFcvi(image, i, j)
	{
		doFv(k, fb.values)
		{
			float res = fb.values[k].Conv(image, i, j);
			data->data.fl[(i*image->width+j)*fb.values.size() + k] = res;
		}
	}
	CvMat* label = cvCreateMat(N, 1, CV_32SC1);

	int clusterN = 128;
	cvKMeans2(data, clusterN, label, cvTermCriteria(CV_TERMCRIT_ITER, 10, 1e-30), 3);

	vector<cvS> ranColors(clusterN);
	randInit();
	doF(k, clusterN) ranColors[k] = cvs(rand1()*255.0, rand1()*255.0, rand1()*255.0);
	cvi* res = cvci83(image);
	doFcvi(res, i, j)
	{
		int idx = label->data.i[i*image->width+j]; //cout<<idx<<" ";
		cvs2(res, i, j, ranColors[idx]);
	}
	cvsi(res);
	cvri(res);

	cvReleaseMat(&data);
	cvReleaseMat(&label);

	return NULL;
}

FilterBank TexonAnysProc::FbCreate(int numOrient, float startSigma, int numScales, float scaling, float elong)
{
	FilterBank fb;
	fb.oritN = numOrient; fb.scaleN = numScales;

	fb.values.clear();

	doF(i, fb.scaleN)
	{
		float sigma = startSigma * pow(scaling, i);
		vector<float> sigmaVec(2, sigma);
		sigmaVec[0] *= elong;
		doF(j, fb.oritN)
		{
			float theta = j * _f CV_PI / numOrient;
			fb.values.push_back(OeFilter(sigmaVec, 3, theta, 2, 0));
			fb.values.push_back(OeFilter(sigmaVec, 3, theta, 2, 1));
		}
	}

	return fb;
}

void Hilbert(vector<float>& vs)
{
	int N = vs.size();
	cvi* a = cvci(cvSize(N, 1), 32, 2);
	doFv(i, vs) cvs2(a, 0, i, cvs(vs[i], 0));
	cvi* b = cvci(cvSize(N, 1), 32, 2);
	//doF(k, 10) cout<<vs[k]<<" "; cout<<endl;
	cvDFT(a, b, CV_DXT_FORWARD);

	int k = N/2;
	doFs(i, 1, k)
	{
		auto v = cvg2(b, 0, i);
		cvs2(b, 0, i, v*2);
	}
	doFs(i, k+1, N)
	{
		cvs2(b, 0, i, cvs(0, 0));
	}

	cvDFT(b, a, CV_DXT_INVERSE_SCALE);
	doFv(i, vs) vs[i] = _f cvg2(a, 0, i).val[1];
	//doF(k, 10) cout<<vs[k]<<" "; cout<<endl;

	cvri(a); cvri(b);
}

Filter TexonAnysProc::OeFilter(vector<float>& sigma, float support, float theta, int deriv, bool hil)
{
	int hsz = _i ceil(sigma[0] * support);
	int sz = 2*hsz + 1;

	int maxsamples = 1000;
	int maxrate = 10;
	int frate = 10;

	float rate = min2(_f maxrate, _f max2(1.f,  floor(_f maxsamples/sz)));
	int samples = round(sz*rate);

	float r = floor(_f sz/2.f) + 0.5f * (1.f - 1.f/rate);
	vector<float> dom(samples);
	doF(i, samples) dom[i] = -r + 2*r/(samples-1)*i;

	float R = r*sqrt(2.f)*1.01f;
	int fsamples = _i ceil(R*rate*frate);
	fsamples = fsamples + (fsamples+1)%2;
	float gap = 2*R/(fsamples-1);

	vector<float> fdom(fsamples), fx(fsamples), fy(fsamples);
	doF(i, fsamples)
	{
		fdom[i] = -R + 2*R/(fsamples-1)*i;
		fx[i] = exp(-sqr(fdom[i])/(2*sqr(sigma[0])));
		fy[i] = exp(-sqr(fdom[i])/(2*sqr(sigma[1])));
		if(deriv == 1)
			fy[i] = fy[i] * (-fdom[i]/(sqr(sigma[1])));
		else if(deriv == 2)
			fy[i] = fy[i] * (sqr(fdom[i])/sqr(sigma[1]) - 1);
	}
	if(hil)
	{
		Hilbert(fy);
	}

	vector<float> f(sz*sz, 0);
	doF(i, samples) doF(j, samples)
	{
		float sx = dom[i], sy = dom[j];
		int mx = round(sx), my = round(sy);
		int membership = (mx+hsz) + (my+hsz)*sz;
		float su = sx*sin(theta) + sy*cos(theta);
		float sv = sx*cos(theta) - sy*sin(theta);

		float xi = round(su/gap) + floor(_f fsamples/2);
		float yi = round(sv/gap) + floor(_f fsamples/2);
		float f_ = fx[_i xi] * fy[_i yi];
		if(membership < 1 || membership > sz*sz) continue;
		f[membership] += f_;
	}

	float sum = 0;
	doFv(i, f) sum += f[i];
	sum /= sz*sz;
	doFv(i, f) f[i] -= sum;
	sum = 0;
	doFv(i, f) sum += fabs(f[i]);
	if(sum > 0) doFv(i, f) f[i] /= sum;

	Filter filter;
	filter.r = sz;
	filter.weights.resize(sz);
	doF(i, sz)
	{
		filter.weights[i].resize(sz);
		doF(j, sz) filter.weights[i][j] = f[i*sz+j];
	}
	//filter.Visualize(); pause;

	return filter;
}

void Filter::Visualize()
{
	int a = 10;
	cvi* res = cvci81(r*a, r*a);
	float minV = 1e10, maxV = -1e10;
	doF(i, r) doF(j, r)
	{
		if(weights[i][j] < minV) minV = weights[i][j];
		if(weights[i][j] > maxV) maxV = weights[i][j];
	}

	doFcvi(res, i, j)
	{
		int v = _i((weights[i/a][j/a] - minV) / (maxV - minV) * 255);
		cvs20(res, i, j, v);
	}
	cvsi(res);
	cvri(res);
}
