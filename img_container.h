#pragma once
#include "texon_proc.h"
#include "ivrt_anys_proc.h"

class ImgContainer
{

public:
	
	ImgContainer();
	ImgContainer(string fileDir, int downRatio = 1);
	~ImgContainer(void);

	cvi* src(){return m_src;};
	cvi* srcHLS(){return m_srcHLS;};
	cvi* srcOri(){return m_srcOri;};
	cvi* srcHLSOri(){return m_srcHLSOri;};
	cvi* srcR(){return m_srcR;};
	cvi* srcHLSR(){return m_srcHLSR;};
	cvi* texR(){return m_texR;};
	int texClusterN(){return m_texClusterN;};

	void GenerateResizedImg(int ratio);

	ImgContainer* GetCropedInstance(CvRect& rect);

private:

	void ResizeTex(IN cvi* m_tex, OUT cvi* m_texR);

private:

	int m_downRatio;

	cvi* m_srcOri;
	cvi* m_srcHLSOri;

	cvi* m_tex;

	cvi* m_src;
	cvi* m_srcHLS;

	//resized
	cvi* m_srcR;
	cvi* m_srcHLSR;
	cvi* m_texR;
	int m_texClusterN;

};

