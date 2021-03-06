/*************************************************
  Copyright (C), 2014, Joy Artworks. Tsinghua Uni.
  All rights reserved.
  
  File name: stl_header.h  v1.0
  Description: This file gathers  frequently  used 
			   [STL]   header, type, abbre., func. 
			   Directly include  this file in your
			   pre-compiled header.

  Header List:
	1. iostream/cmath/string/vector/map/functional
	   /queue/deque/ctime/fstream/sstream

  Type List:
	1. vectors: vecS vecI vecF vecD vecB

  Abbreviation List:
	1. JREAL(double)
	2. for: doF doFs doFv doFvs
	3. type: _i _f
	4. pause

  Function List: 
    1. round/sqr/toStr (T) 
	2. min2/max2 (T, T)
	3. clamp/min3/max3/mid3 (T, T, T)
	4. distEulerL1/2 (x1, y1, x2, y2)
	5. gaussian(Normalize) (val, center, sigma)
	6. randInit rand1
	7. cout (vector)
	8. fequal

  History:
    1. Date: 2014.5.14
       Author: Li-Qian Ma
       Modification: Version 1.0.
*************************************************/

#pragma once

/***************** header ************************/
#include <string>
#include <vector>
#include <iostream>
#include <functional>
#include <map>
#include <unordered_map>
#include <set>
#include <unordered_set>
#include <cmath>
#include <queue>
#include <deque>
#include <ctime>
#include <fstream>
#include <sstream>
using namespace std;

#define PI 3.141592653

/******************* type ************************/
typedef vector<string> vecS;
typedef vector<bool> vecB;
typedef vector<int> vecI;
typedef vector<float> vecF;
typedef vector<double> vecD;

/***************** abbreviation ******************/
#define JREAL double
#define doF(i, N) for (int i = 0; i < N; i++)
#define doFs(i, j, N) for (int i = j; i < N; i++)
#define doFv(i, v) for (size_t i = 0; i < v.size(); i++)
#define doFsv(i, j, v) for (size_t i = j; i < v.size(); i++)

#define _f (float)
#define _d (double)
#define _i (int)
#define pause system("pause")

/***************** function **********************/
template <class T>
int round(T x)
{
	if ((JREAL)x-int(x)+1e-8>0.5)
		return int(x)+1;
	else
		return int(x);
}

template <class T> 
inline T sqr(T v)
{
	return v*v;
}

template <class T>
inline string toStr(T t)
{
	ostringstream oos;
	oos<<t;
	return oos.str();
}

template <typename T>
inline T min2(T x, T y)
{
	if (x <= y) return x;
	else return y;
}

template <typename T>
inline T max2(T x, T y)
{
	if (x >= y) return x;
	else return y;
}

template <typename T> 
inline T clamp(T x, T min, T max)
{
	if (x < min) return min; 
	if (x > max) return max; 
	return x;
}

template <typename T> 
inline T max3(T r, T g, T b)
{
	if (r >= g && r >= b) return r;
	if (g >= r && g >= b) return g;
	return b;
}

template <typename T> 
inline T min3(T r, T g, T b)
{
	if (r <= g && r <= b) return r;
	if (g <= r && g <= b) return g; 
	return b;
}

template <typename T> 
inline T mid3(T r, T g, T b)
{
	return (r + g + b) - max3(r, g, b) - min3(r, g, b);
}

template <class T1, class T2, class T3>
inline JREAL gaussian(T1 val, T2 center, T3 sigma)
{
	return exp(-sqr(((JREAL)val - (JREAL)center)/(JREAL)sigma) / 2);
}

template <class T1, class T2, class T3>
inline JREAL gaussianNormalize(T1 val, T2 center, T3 sigma)
{
	return 1.0/(JREAL)sigma/sqrt(2.0*PI) * gaussian(val, center, sigma);
}

template <typename T>
inline JREAL distEulerL1(T x1, T y1, T x2, T y2)
{
	return abs((JREAL)(x1-x2)) + abs((JREAL)(y1-y2));
}

template <typename T>
inline JREAL distEulerL2(T x1, T y1, T x2, T y2)
{
	return sqrt((JREAL)(sqr(x1-x2) + sqr(y1-y2)));
}

inline void randInit()
{
	srand((unsigned int)time(0));
}

inline float rand1()
{
	return (float)rand()/RAND_MAX;
}

template <class T>
inline istream& operator >> (istream& in, vector<T>& vals)
{
	int s; 
	in>>s;
	val.resize(s);
	doFv(i, vals) in>>vals[i];
	return in;
}

template <class T>
inline ostream& operator << (ostream& out, vector<T>& vals)
{
	out<<vals.size()<<endl;
	doFv(i, vals) out<<vals[i]<<endl;
	return out;
}

inline bool fequal(float v1, float v2)
{
	return (fabs(v1 - v2) < 1e-6);
}

inline void oswrite(ostream& fout, int v)
{
	fout.write((char*)(&v), 4);
}
inline void oswrite(ostream& fout, float v)
{
	fout.write((char*)(&v), 4);
}
inline void oswrite(ostream& fout, double v)
{
	fout.write((char*)(&v), 8);
}
inline void osread(istream& fin, int& v)
{
	fin.read((char*)(&v), 4);
}
inline void osread(istream& fin, float& v)
{
	fin.read((char*)(&v), 4);
}
inline void osread(istream& fin, double& v)
{
	fin.read((char*)(&v), 8);
}