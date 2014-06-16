#include "StdAfx.h"
#include "lumminance_invarient_descriptor.h"

LmncIvrtDesp1D::~LmncIvrtDesp1D(void)
{
}

float LmncIvrtDesp1D::CmpLmncIvrtDesp1Ds(cvS s)
{
	if(s.val[0] > 1 || s.val[2] > 1 || s.val[3] > 1)
		return CmpLmncIvrtDesp1D((float)s.val[0] / 255.0f, (float)s.val[1] / 255.0f, (float)s.val[2] / 255.0f);
	else
		return CmpLmncIvrtDesp1D((float)s.val[0], (float)s.val[1], (float)s.val[2]);
}

LmncIvrtDesp3D::~LmncIvrtDesp3D(void)
{
}

cvS NormRGBDesp::CmpLmncIvrtDesp3D(cvS s)
{
	//normalize rgb
	cvS s1 = s;
	float nmlzRatio = (float)cvSD(s, cvs(0, 0, 0));
	if(nmlzRatio > 1) nmlzRatio /= 255.0f;
	doF(i, 3) s1.val[i] /= nmlzRatio;

	//c1 c2 c3 ref by [01] shadow identification
	cvS s2 = s;
	s2.val[0] = atan2(s.val[0], max2(s.val[1], s.val[2]));
	s2.val[1] = atan2(s.val[1], max2(s.val[0], s.val[2]));
	s2.val[2] = atan2(s.val[2], max2(s.val[0], s.val[1]));
	doF(i, 3) s2.val[i] = s2.val[i] / CV_PI * 2 * 255;

	//ref by [01pami] color variance
	cvS s3 = s;
	s3.val[0] = 0.06 * s.val[0] + 0.63 * s1.val[0] + 0.27 *s2.val[0];
	s3.val[1] = 0.3 * s.val[0] + 0.04 * s1.val[0] - 0.35 * s2.val[0];
	s3.val[2] = 0.34 * s.val[0] - 0.6 * s1.val[0] + 0.17 * s2.val[0];

	return s1;
}

float ChrmMapDesp::CmpLmncIvrtDesp1DT(float r, float g, float b, float angle)
{
	float theta = angle / 180 * (float)CV_PI;
	g = r * g * b;
	if(g == 0) g = 1e-6f;
	g = pow(g, 1.0f/3.0f);

	float res = cos(theta) * log((float)r/(float)g) 
		+ sin(theta) * log((float)b/(float)g);
	res = clamp(res, -10.0f, 10.0f);
	res = clamp(exp(res) / 2, 0.0f, 1.0f);
	//cout<<res<<" ";
	return res;
}

int test()
{
	cvi* img = cvlic("images//7.png");
	cvi* res = cvci(img);

	cvsi("src.png", img);

	NormRGBDesp desp;
	ChrmMapDesp desp2;

	cvi* img2 = cvci(img);
	cvCvtColor(img2, img2, CV_RGB2HLS);
	cvsi("result2.png", img2);
	//cvSmooth(img2, img2, CV_GAUSSIAN, 5);
	doFcvi(img, i, j)
	{
		cvS v = cvg2(img, i, j);
		//cvS v2 = cvg2(img2, i, j);
		//float high = cvSD(v, v2) * 10;
		// 		cvs2(res, i, j, cvs(high, high, high));

		cvS val = desp.CmpLmncIvrtDesp3D(v);
		cvs2(res, i, j, val);
	}
	cvsi("result.png", res);
	return 0;

	doF(k, 360){
		doFcvi(img, i, j)
		{
			cvS v = cvg2(img, i, j);
			float val = desp2.CmpLmncIvrtDesp1DT((float)v.val[0], 
				(float)v.val[1], (float)v.val[2], (float)k) * 255.f;
			cvs2(res, i, j, cvs(val, val, val));
		}
		cvsi("temp//result"+toStr(k)+".png", res);
	}

	cvri(img);
	cvri(res);
}