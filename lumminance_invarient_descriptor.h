#pragma once

class LmncIvrtDesp1D
{
public:
	virtual ~LmncIvrtDesp1D(void) = 0;
	virtual float CmpLmncIvrtDesp1D(float r, float g, float b) = 0;
	virtual float CmpLmncIvrtDesp1Ds(cvS s);
};

class LmncIvrtDesp3D
{
public:
	virtual ~LmncIvrtDesp3D(void) = 0;
	virtual cvS CmpLmncIvrtDesp3D(cvS s) = 0;
};

class NormRGBDesp : public LmncIvrtDesp3D
{
public:
	cvS CmpLmncIvrtDesp3D(cvS s);
};

class ChrmMapDesp// : public LmncIvrtDesp1D
{
public:
	float CmpLmncIvrtDesp1DT(float r, float g, float b, float angle = 0);
};