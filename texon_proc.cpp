#include "StdAfx.h"
#include "texon_proc.h"

TexonAnysProc::TexonAnysProc(void)
{
}

TexonAnysProc::~TexonAnysProc(void)
{
}


float Filter::Conv(cvi* img, int i_, int j_, int ch)
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
		res += _f cvg2(img, _r, _c).val[ch] * weights[i-s-i_][j-s-j_];
	}
	return res;
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

//use omp
cvi* TexonAnysProc::GetDensityImg(cvi* image, int ch)
{
	int nScales = 4;

	vector<float> r(nScales), log_r(nScales);
	doFv(i, r) r[i] = _f i + 2;
	doFv(i, r) log_r[i] = log(r[i]) / log(10.f);

	float log_r_sum = 0;
	doF(k, nScales) log_r_sum += log_r[k];
	log_r_sum /= nScales;
	doF(k, nScales) log_r[k] -= log_r_sum;
	float len = 0;
	doF(k, nScales) len += sqr(log_r[k]);

	cvi* res = cvci321(image);

	omp_set_num_threads(6);
#pragma omp parallel for
	doFcvi(image, i, j)
	{
		vector<float> log_mus(nScales);
		doF(k, nScales)
		{
			float mu = 0;

			bool useGau = false;
			int rk = _i(_f r[k]*(useGau?2.f:1.0f));
			doFs(oi, -rk, rk) doFs(oj, -rk, rk)
			{
				if(useGau)
					mu += max2(_f cvg2(image, i+oi, j+oj).val[ch], 1.0f) * _f gaussianNormalize(distEulerL1(oi, oj, 0, 0), r[k], r[k] / 2) * sqr(r[k]);
				else
					mu += max2(_f cvg2(image, i+oi, j+oj).val[ch], 1.0f);
			}

			float log_mu = log(mu) / log(10.f);
			log_mus[k] = log_mu;
		}

		float log_mu_sum = 0;
		doF(k, nScales) log_mu_sum += log_mus[k];
		log_mu_sum /= nScales;
		doF(k, nScales) log_mus[k] -= log_mu_sum;

		float dd = 0;
		doF(k, nScales) dd += log_r[k] * log_mus[k];

		float slope = dd / len;
		cvs20(res, i, j, slope);
	}

	return res;
}

//use omp
void TexonAnysProc::GetTexonImg(IN cvi* image, IN int clusterN, OUT vector<TexDscrptor>& texValues, OUT cvi* &texIdx, int ch)
{
	FilterBank fb = FbCreate(8, 0.5, 1, sqrt(2.f), 3);
	int N = image->width * image->height;

	CvMat* data = cvCreateMat(N, fb.values.size(), CV_32FC1);

	omp_set_num_threads(6);
#pragma omp parallel for
	doFcvi(image, i, j)
	{
		doFv(k, fb.values)
		{
			float res = fb.values[k].Conv(image, i, j, ch);
			data->data.fl[(i*image->width+j)*fb.values.size() + k] = res;
		}
	}
	CvMat* label = cvCreateMat(N, 1, CV_32SC1);
	cvZero(label);

	cvKMeans2(data, clusterN, label, cvTermCriteria(CV_TERMCRIT_ITER, 10, 1e-30), 3);

	texValues.resize(clusterN);
	doF(i, clusterN) texValues[i].resize(fb.values.size(), 0);
	vector<int> pts(clusterN, 0);

	texIdx = cvci81(image);
	doFcvi(texIdx, i, j)
	{
		int pIdx = i*image->width+j;
		int idx = label->data.i[pIdx];
		cvs20(texIdx, i, j, idx);

		TexDscrptor& dsp = texValues[idx];
		pts[idx]++;
		doF(k, _i fb.values.size())
			dsp[k] += data->data.fl[pIdx*fb.values.size() + k];
	}
	doF(i, clusterN) doF(k, _i fb.values.size())
		texValues[i][k] /= max2(pts[i], 1);

	cvReleaseMat(&data);
	cvReleaseMat(&label);
}

cvi* TexonAnysProc::TexonAnalysis(cvi* image, int& clusterN, int ch)
{

	//get density map
	cout<<"Texon: density computing..";
	cvi* densityMap = GetDensityImg(image, ch);

// 	cvi* densityVisual = VisualizeDensity(densityMap);
// 	cvsi("_texon_density_map.png", densityVisual);
// 	cvri(densityVisual);

	//compute texon
	cout<<"\rTexon: texton computing..";
	vector<TexDscrptor> texDsps;
	cvi* texIdxs = NULL;
	GetTexonImg(densityMap, clusterN, texDsps, texIdxs, 0);

	//TODO: decrease cluster number
	//modify clusterN
	clusterN = SimplyTexIdx(texIdxs, clusterN, texDsps);


// 	GetTexonImg(image, clusterN, texDsps, texIdxs, ch);
// 	texonVisual = VisualizeTexon(texIdxs, clusterN);
// 	cvsi("_texon_map.png", texonVisual);
// 	cvri(texonVisual);

	cvri(densityMap);
	cout<<"\rTexon: texton computing done, with clusters = "<<clusterN<<".\n";
	return texIdxs;
}

int TexonAnysProc::SimplyTexIdx(IN OUT cvi* texIdxs, int clusterN, IN vector<TexDscrptor>& vectexDsps)
{
	vecI nPixels(clusterN, 0);
	doFcvi(texIdxs, i, j)
	{
		int idx = _i cvg20(texIdxs, i, j);
		nPixels[idx]++;
	}

	int thres = texIdxs->height * texIdxs->width / clusterN / 20;
	map<int, int> mapping;
	int idx = 0;
	while(mapping.size() < 10)
	{
		mapping.clear();
		idx = 0;
		doF(i, clusterN)
		{
			if(nPixels[i] > thres)
			{
				mapping[i] = idx;
				idx++;
			}
		}
		thres = thres/2;
		break;
	}

	doF(i, clusterN)
	{
		if(nPixels[i] <= thres)
		{
			int minIdx = -1; float minDist = 1e30f;
			for(auto p = mapping.begin(); p != mapping.end(); p++)
			{
				int j = p->first;
				float dist = 0;
				doFv(k, vectexDsps[j]) dist += sqr(vectexDsps[i][k] - vectexDsps[j][k]);
				if(dist < minDist)
				{
					minDist = dist;
					minIdx = p->second;
				}
			}
			mapping[i] = minIdx;
		}
	}

	doFcvi(texIdxs, i, j)
	{
		int v = _i cvg20(texIdxs, i, j);
		cvs20(texIdxs, i, j, mapping[v]);
	}
	return idx;
}

cvi* TexonAnysProc::VisualizeDensity(cvi* densityMap)
{
	cvi* res = cvci81(densityMap);
	float maxV = -1e20f, minV = 1e20f;
	doFcvi(densityMap, i, j)
	{
		float v = _f cvg20(densityMap, i, j);
		if(v > maxV) maxV = v;
		if(v < minV) minV = v;
	}
	doFcvi(res, i, j)
	{
		float v = _f cvg20(densityMap, i, j);
		cvs20(res, i, j, (v-minV)*255/(maxV-minV));
	}
	return res;
}

cvi* TexonAnysProc::VisualizeTexon(IN cvi* texIdx)
{
	randInit();
	map<int, cvS> mapping;
	cvi* res = cvci83(texIdx);
	doFcvi(res, i, j)
	{
		int idx = _i cvg20(texIdx, i, j);
		if(mapping.count(idx) == 0) mapping[idx] = cvs(rand1()*255.0, rand1()*255.0, rand1()*255.0);
		cvs2(res, i, j, mapping[idx]);
	}
	return res;
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

