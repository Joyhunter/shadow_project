/*************************************************
  Copyright (C), 2014, Joy Artworks. Tsinghua Uni.
  All rights reserved.
  
  File name: opencv_header.h  v1.0
  Description: This file  gathers  frequently used 
               [OPENCV] header, abbre., functions. 
			   Directly include  this file in your 
			   pre-compiled header.
  Other: Ad OPENCV_INCLUDE, OPENCV_LIB, OPENCV_BIN
         directory to your project. OPENCV version
		 used is 2.2.

  Header List:
	1. cv/cvaux/cxcore/highgui/ml

  Abbreviation List:
	1. IplImage: cvi cvli(c/g) cvsi(c) cvri
	             cvci(83/323/81/321)
				 cvS cvs cvg2(0)(i/f) cvs2(0/3)
	2. for: doFcvi doFcvm

  Function List: 
    1. cvSD/cvSDSqr/colorDist (cvS, cvS)
	2. cvS: + - * / (cvS cvS/T) 
	        += -= *= /= (cvS/T)
			clamp (cvS, T, T)/(cvS, cvS, cvS)
	3. CvSize: == !=
	4. CV_Assert_ (expr, info)
	5. cvIn (r, c, cvi/CvSize/h,w/CvRect/
	         hLow,wLow,hHigh,wHigh)

  History:
    1. Date: 2014.5.14
       Author: Li-Qian Ma
       Modification: Version 1.0.
*************************************************/

#pragma once

/***************** header ************************/
#include "stl_header.h"

#pragma warning(disable: 4996) //fopen unsafe warning 
#include <opencv\cv.h>
#include <opencv\cvaux.h>
#include <opencv\cxcore.h>
#include <opencv\highgui.h>
#include <opencv2/ml/ml.hpp>
using namespace cv;
#pragma warning(default: 4996) //fopen unsafe warning 

#ifdef _DEBUG
#pragma comment(lib, "opencv_imgproc220d.lib")
#pragma comment(lib, "opencv_highgui220d.lib")
#pragma comment(lib, "opencv_core220d.lib")
#pragma comment(lib, "opencv_ml220d.lib")
#else
#pragma comment(lib, "opencv_imgproc220.lib")
#pragma comment(lib, "opencv_highgui220.lib")
#pragma comment(lib, "opencv_core220.lib")
#pragma comment(lib, "opencv_ml220.lib")
#endif

/***************** abbreviation ******************/
#define cvi IplImage
#define cvli cvLoadImage
#define cvlic(str) cvLoadImage(str, 1)
#define cvlig(str) cvLoadImage(str, 0)
#define cvS CvScalar
#define cvs cvScalar
#define cvg20(src, i, j) cvGet2D(src, i, j).val[0]
#define cvs2 cvSet2D
#define cvs20(src, i, j, v) cvSet2D(src, i, j, cvScalar(v))
#define cvs23(src, i, j, v) cvSet2D(src, i, j, cvScalar(v, v, v))

inline cvi* cvci(CvSize size, int depth = 8, int channels = 3)
{
	return cvCreateImage(size, depth, channels);
}

inline cvi* cvci(cvi* img)
{
	return cvCloneImage(img);
}

inline int cvsi(const char* filename, const CvArr* image)
{
	return cvSaveImage(filename, image);
}

inline int cvsi(string filename, const CvArr* image)
{
	return cvSaveImage(filename.c_str(), image);
}

inline int cvsi(const CvArr* image)
{
	return cvSaveImage("autoSave.png", image);
}

inline void cvri(cvi* image)
{
	cvReleaseImage(&image);
}

inline void cvri(cvi** image)
{
	cvReleaseImage(image);
}

inline cvi* cvci83(const cvi* img)
{
	return cvCreateImage(cvGetSize(img), 8, 3);
}

inline cvi* cvci83(CvSize size)
{
	return cvCreateImage(size, 8, 3);
}

inline cvi* cvci83(int width, int height)
{
	return cvci83(cvSize(width, height));
}

inline cvi* cvci323(const cvi* img)
{
	return cvCreateImage(cvGetSize(img), 32, 3);
}

inline cvi* cvci323(CvSize size)
{
	return cvCreateImage(size, 32, 3);
}

inline cvi* cvci323(int width, int height)
{
	return cvci323(cvSize(width, height));
}

inline cvi* cvci81(const cvi* img)
{
	return cvCreateImage(cvGetSize(img), 8, 1);
}

inline cvi* cvci81(CvSize size)
{
	return cvCreateImage(size, 8, 1);
}

inline cvi* cvci81(int width, int height)
{
	return cvci81(cvSize(width, height));
}

inline cvi* cvci321(const cvi* img)
{
	return cvCreateImage(cvGetSize(img), 32, 1);
}

inline cvi* cvci321(CvSize size)
{
	return cvCreateImage(size, 32, 1);
}

inline cvi* cvci321(int width, int height)
{
	return cvci321(cvSize(width, height));
}

inline cvS cvg2(const cvi* img, int r, int c)
{
	return cvGet2D(img, r, c);
}

#define doFcvi(img, i, j) for (int i = 0;i<img->height;i++) for (int j = 0;j<img->width;j++)
#define doFcvm(img, i, j) for (int i = 0;i<img.rows;i++) for (int j = 0;j<img.cols;j++)
#define doForPixel(img, i, j) for (int i = 0;i<img->height;i++) for (int j = 0;j<img->width;j++)
#define doForPixel2(img, i, j) for (int i = 0;i<img.rows;i++) for (int j = 0;j<img.cols;j++)

inline int cvsic(string filename, const cvi* image, int ch = 0)
{
	cvi* res = cvci81(image->width, image->height);
	doFcvi(res, i, j) cvs20(res, i, j, cvg2(image, i, j).val[ch]);
	int v = cvSaveImage(filename.c_str(), res);
	cvri(res);
	return v;
}

inline int cvsic(const cvi* image, int ch = 0)
{
	return cvsic("autoSave.png", image, ch);
}

/***************** function **********************/
inline double cvSDSqr(const cvS& c1, const cvS& c2)
{
	return sqr(c1.val[0]-c2.val[0])+sqr(c1.val[1]-c2.val[1])+sqr(c1.val[2]-c2.val[2]);
}

inline double cvSD(const cvS& c1, const cvS& c2)
{
	return sqrt(sqr(c1.val[0]-c2.val[0])+sqr(c1.val[1]-c2.val[1])+sqr(c1.val[2]-c2.val[2]));
}

#define colorDist(c1, c2) cvSD(c1, c2)

template <class T>
inline cvS clamp(const cvS& c, T min, T max)
{
	cvS res = c;
	for (int i = 0; i < 3; i++)
	{
		if(res.val[i] > max) res.val[i] = max;
		if(res.val[i] < min) res.val[i] = min;
	}
	return res;
}

inline cvS clamp(const cvS& c, const cvS& min, const cvS& max)
{
	cvS res = c;
	for (int i = 0; i < 3; i++)
	{
		if(res.val[i] > max.val[i]) res.val[i] = max.val[i];
		if(res.val[i] < min.val[i]) res.val[i] = min.val[i];
	}
	return res;
}

template <class T>
inline cvS operator + (const cvS& c, const T v)
{
	return cvs(c.val[0] + v, c.val[1] + v, c.val[2] + v);
}

template <class T>
inline cvS operator - (const cvS& c, const T v)
{
	return cvs(c.val[0] - v, c.val[1] - v, c.val[2] - v);
}

template <class T>
inline cvS operator * (const cvS& c, const T v)
{
	return cvs(c.val[0] * v, c.val[1] * v, c.val[2] * v);
}

template <class T>
inline cvS operator / (const cvS& c, const T v)
{
	return cvs(c.val[0] / v, c.val[1] / v, c.val[2] / v);
}

inline cvS operator + (const cvS& c1, const cvS& c2)
{
	return cvs(c1.val[0] + c2.val[0], c1.val[1] + c2.val[1], c1.val[2] + c2.val[2]);
}

inline cvS operator - (const cvS& c1, const cvS& c2)
{
	return cvs(c1.val[0] - c2.val[0], c1.val[1] - c2.val[1], c1.val[2] - c2.val[2]);
}

inline cvS operator * (const cvS& c1, const cvS& c2)
{
	return cvs(c1.val[0] * c2.val[0], c1.val[1] * c2.val[1], c1.val[2] * c2.val[2]);
}

inline cvS operator / (const cvS& c1, const cvS& c2)
{
	return cvs(c1.val[0] / c2.val[0], c1.val[1] / c2.val[1], c1.val[2] / c2.val[2]);
}

inline void operator += (cvS& c1, const cvS& c2)
{
	c1.val[0] += c2.val[0];
	c1.val[1] += c2.val[1];
	c1.val[2] += c2.val[2];
}

inline void operator -= (cvS& c1, const cvS& c2)
{
	c1.val[0] -= c2.val[0];
	c1.val[1] -= c2.val[1];
	c1.val[2] -= c2.val[2];
}

inline void operator *= (cvS& c1, const cvS& c2)
{
	c1.val[0] *= c2.val[0];
	c1.val[1] *= c2.val[1];
	c1.val[2] *= c2.val[2];
}

inline void operator /= (cvS& c1, const cvS& c2)
{
	c1.val[0] /= c2.val[0];
	c1.val[1] /= c2.val[1];
	c1.val[2] /= c2.val[2];
}

template <class T>
inline void operator += (cvS& c, const T val)
{
	c.val[0] += val;
	c.val[1] += val;
	c.val[2] += val;
}

template <class T>
inline void operator -= (cvS& c, const T val)
{
	c.val[0] -= val;
	c.val[1] -= val;
	c.val[2] -= val;
}

template <class T>
inline void operator *= (cvS& c, const T val)
{
	c.val[0] *= val;
	c.val[1] *= val;
	c.val[2] *= val;
}

template <class T>
inline void operator /= (cvS& c, const T val)
{
	c.val[0] /= val;
	c.val[1] /= val;
	c.val[2] /= val;
}

inline bool operator != (CvSize& c1, CvSize& c2)
{
	return (c1.height != c2.height) || (c1.width != c2.width);
}

inline bool operator == (CvSize& c1, CvSize& c2)
{
	return (c1.height == c2.height) && (c1.width == c2.width);
}

#define CV_Assert_(expr, args) \
{\
	if(!(expr)) {\
	string msg = cv::format args; \
	CmLog::LogError("%s in %s:%d\n", msg.c_str(), __FILE__, __LINE__); \
	cv::error(cv::Exception(CV_StsAssert, msg, __FUNCTION__, __FILE__, __LINE__) ); }\
}

template <class T>
inline bool cvIn(T r, T c, cvi* img)
{
	return (r >= 0) && (c >= 0) && (r < img->height) && (c < img->width);
}

template <class T>
inline bool cvIn(T r, T c, CvSize size)
{
	return (r >= 0) && (c >= 0) && (r < size.height) && (c < size.width);
}

template <class T>
inline bool cvIn(T r, T c, T height, T width)
{
	return (r >= 0) && (c >= 0) && (r < height) && (c < width);
}

template <class T>
inline bool cvIn(T r, T c, const CvRect& rect)
{
	return (r >= rect.y) && (c >= rect.x) && (r < rect.y + rect.height) && (c < rect.x + rect.width);
}

template <class T>
inline bool cvIn(T r, T c, T rLow, T rHigh, T cLow, T cHigh)
{
	return (r >= rLow) && (c >= cLow) && (r < rHigh) && (c < cHigh);
}

inline cvS cvg2(const cvi* img, float x, float y)
{
	x = clamp(x, 0.f, _f img->height-1);
	y = clamp(y, 0.f, _f img->width-1);
	if(fequal(x, _f round(x)) && fequal(y, _f round(y)))
	{
		return cvGet2D(img, round(x), round(y));
	}

	int x1 = _i(floor(x)), x2 = x1 + 1;
	int y1 = _i(floor(y)), y2 = y1 + 1;
	float alpha1 = x - x1, alpha2 = y - y1;
	x1 = clamp(x1, 0, img->height-1);
	x2 = clamp(x2, 0, img->height-1);
	y1 = clamp(y1, 0, img->width-1);
	y2 = clamp(y2, 0, img->width-1);
	return (cvGet2D(img, x1, y1) * (1 - alpha1) + cvGet2D(img, x2, y1) * alpha1) * (1 - alpha2)
		+ (cvGet2D(img, x1, y2) * (1 - alpha1) + cvGet2D(img, x2, y2) * alpha1) * alpha2;
}