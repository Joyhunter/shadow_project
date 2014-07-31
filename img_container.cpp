#include "StdAfx.h"
#include "img_container.h"

ImgContainer::ImgContainer():m_srcOri(NULL), m_srcHLS(NULL), m_src(NULL),
	m_srcHLSOri(NULL), m_tex(NULL), m_srcR(NULL), m_srcHLSR(NULL), m_texR(NULL)
{
}

ImgContainer::ImgContainer(string fileDir, int downRatio):m_srcOri(NULL), m_srcHLS(NULL), m_src(NULL),
	m_srcHLSOri(NULL), m_tex(NULL), m_srcR(NULL), m_srcHLSR(NULL), m_texR(NULL)
{
	m_downRatio = downRatio;
	m_srcOri = cvlic(fileDir.c_str()); // 600*450
	m_src = cvci83(m_srcOri->width / downRatio, m_srcOri->height / downRatio); //200*150
	cvResize(m_srcOri, m_src);

	//IvrtAnysProc iProc;
	//cvi* ivrtSrc = iProc.IvrtAnalysis2(src);
	//cvsi("_ivrt.png", ivrtSrc);


	m_srcHLSOri = cvci(m_srcOri);
	cvCvtColor(m_srcOri, m_srcHLSOri, CV_BGR2HLS_FULL);
	m_srcHLS = cvci(m_src);
	cvCvtColor(m_src, m_srcHLS, CV_BGR2HLS_FULL);

	// 	cvsic("__h.png", m_srcHLSOri, 0);
	// 	cvsic("__l.png", m_srcHLSOri, 1);
	// 	cvsic("__s.png", m_srcHLSOri, 2);


	m_texClusterN = 64;

// 	TexonAnysProc tproc;
// 	m_tex = tproc.TexonAnalysis(m_srcOri, m_texClusterN, 0);
// 	cvsi("preComp.png", m_tex);

	m_tex = cvlig("preComp.png");

}

ImgContainer::~ImgContainer(void)
{
	cvri(m_srcOri); cvri(m_srcHLSOri);
	cvri(m_src); cvri(m_srcHLS);
	cvri(m_tex);
	cvri(m_srcR); cvri(m_srcHLSR); cvri(m_texR);
}

void ImgContainer::GenerateResizedImg(int ratio)
{
	cvri(m_srcR); cvri(m_srcHLSR); cvri(m_texR);

	m_srcR = cvci83(m_src->width / ratio, m_src->height / ratio);
	cvResize(m_src, m_srcR);

	m_srcHLSR = cvci(m_srcR);
	cvCvtColor(m_srcR, m_srcHLSR, CV_BGR2HLS_FULL);

	m_texR = cvci81(m_srcR);
	ResizeTex(m_tex, m_texR);

	cvi* texonVisual = TexonAnysProc::VisualizeTexon(m_texR);
	cvsi("_texon_after_density_map.png", texonVisual);
	cvri(texonVisual);
}

void ImgContainer::ResizeTex(IN cvi* tex, OUT cvi* texR)
{
	int ratio = tex->width / texR->width;
	doFcvi(texR, i, j)
	{
		map<int, int> mapping;
		doF(oi, ratio) doF(oj, ratio)
		{
			int v = _i cvg20(tex, i*ratio+oi, j*ratio+oj);
			if(mapping.count(v) == 0) mapping[v] = 1;
			else mapping[v] ++;
		}
		int maxV = mapping.rbegin()->first;
		cvs20(texR, i, j, maxV);
	}
}

// thread safe
ImgContainer* ImgContainer::GetCropedInstance(CvRect& rect)
{
	ImgContainer* res = new ImgContainer();
	if(m_srcR != NULL)
	{
		cvi* temp = cvci(m_srcR);
		cvSetImageROI(temp, rect);
		res->m_srcR = cvci83(rect.width, rect.height);
		cvCopy(temp, res->m_srcR);
		cvri(temp);
	}
	if(m_srcHLSR != NULL)
	{
		cvi* temp = cvci(m_srcHLSR);
		cvSetImageROI(temp, rect);
		res->m_srcHLSR = cvci83(rect.width, rect.height);
		cvCopy(temp, res->m_srcHLSR);
		cvResetImageROI(m_srcHLSR);
		cvri(temp);
	}
	if(m_texR != NULL)
	{
		cvi* temp = cvci(m_texR);
		cvSetImageROI(temp, rect);
		res->m_texR = cvci81(rect.width, rect.height);
		cvCopy(temp, res->m_texR);
		cvResetImageROI(m_texR);
		cvri(temp);
	}
	return res;
}