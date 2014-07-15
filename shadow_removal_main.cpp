#include "stdafx.h"
#include "gpm_proc.h"
#include "vote_proc.h"
#include "mrf_proc.h"
#include "decmps_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	string fileName = "1_small"; //1_small 7_test

	cvi* src = cvlic(("images//" + fileName + ".png").c_str());
	cvi* src2 = cvci(src);
	cvCvtColor(src, src2, CV_BGR2HLS_FULL);

	//------------------- step 1 : patch match --------------------------------
	int gridSize = 56;//45;//41;//45;
	int gridOffset = 50;//39;//35;//39;
	int patchSize = 7;
	string corrFileName = "images//corr//" + fileName + "_" + toStr(gridSize) 
		+ "_" + toStr(gridOffset) + "_" + toStr(patchSize) + "_full_test2.txt";

	LmnIvrtPatchDistMetric metric2;
	GridGPMProc proc(&metric2, gridSize, gridOffset, 1, patchSize, 30);
	proc.SetROI(cvRect(0, 0, src->width, src->height));

	if(0)
	{
		proc.RunGridGPMMultiScale(src2, corrFileName, 2, 1);
		proc.ShowGPMResUI(src, src2, corrFileName, 80);
	}

	//------------------- step 2 : voting  ------------------------------------
	if(0)
	{
		cvi* initMask = NULL, *initCfdc = NULL; 
		VoteProc vProc;
		vProc.LoadCorrs(corrFileName);
		vProc.Vote(src2, initMask, initCfdc, 80, 0.5f); //80/0.5 80/1.0 
		cvsic("_mask_2postprocess.png", initMask, 0);
		cvsi("_mask_init.png", initMask);
		cvsi("_cnfdc_init.png", initCfdc);
		cvri(initMask); cvri(initCfdc);
	}

	//------------------- step 3 : MRF ----------------------------------------
	if(0)
	{
		cvi* initMask = cvli("_mask_init.png");
		cvi* initCfdc = cvli("_cnfdc_init.png");
		//cvi* shdwMask = cvli("_mask_init.png");
		cvi* shdwMaskMRF = NULL;
		MRFProc mProc;
		mProc.SolveWithInitial(src, src2, initMask, initCfdc, 256, shdwMaskMRF);
		cvsic("_mask_3res.png", shdwMaskMRF, 0);
		cvsi("_mask_mrf.png", shdwMaskMRF);
		cvri(initMask); cvri(initCfdc); cvri(shdwMaskMRF);
	}

	//------------------- step 4: decompose, get structure --------------------
	cvi* shdwMaskMRF = cvlic("_mask_mrf.png");
	cvi* shdwMask = NULL;
	DecmpsProc dProc;
	dProc.Analysis(src, shdwMaskMRF, shdwMask);
	cvsic("_mask_4res.png", shdwMask, 0);
	cvri(shdwMaskMRF);

	//------------------- step 4 : Recover ------------------------------------
	cvsi("_src.png", src);
	doFcvi(src, i, j)
	{
		auto v = cvg2(src2, i, j);
		cvS alpha = cvg2(shdwMask, i, j) / 255.0;
		cvs2(src2, i, j, cvs(v.val[0], v.val[1] / alpha.val[0], v.val[2] / alpha.val[1]));
	}
	cvCvtColor(src2, src, CV_HLS2BGR_FULL);
	cvsi("_src_rev.png", src);
	cvri(shdwMask);

	cvri(src);
	cvri(src2);
	return 0;
}


// 	cvi* hue = cvci81(src);
// 	cvi* saturation = cvci81(src);
// 	cvi* value = cvci81(src);
// 	cvSplit(src2, hue, 0, 0, 0 ); 
// 	cvSplit(src2, 0, saturation, 0, 0 );  
// 	cvSplit(src2, 0, 0, value, 0 );  
// 	cvsi("1.png", hue); 
// 	cvsi("2.png", saturation); 
// 	cvsi("3.png", value); 
// 	pause;
//RegularPatchDistMetric metric;
//proc.SetROI(cvRect(30, 70, 50, 50));
//proc.SetROI(cvRect(109, 33, 80, 80));
//proc.SetROI(cvRect(149, 63, 50, 50));
//cout<<corrFileName<<endl;


//proc.RunGridGPM(src2, "corr.txt");
//proc.ShowGPMResUI(src, src2, "corr.txt", 30);
