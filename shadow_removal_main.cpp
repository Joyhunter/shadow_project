#include "stdafx.h"
#include "gpm_proc.h"
#include "vote_proc.h"
#include "mrf_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	string fileName = "1_small";

	cvi* src = cvlic(("images//" + fileName + ".png").c_str());
	cvi* src2 = cvci(src);
	cvCvtColor(src, src2, CV_BGR2HLS_FULL);

	//------------------- step 1 : patch match --------------------------------
	int gridSize = 45;
	int gridOffset = 39;
	int patchSize = 7;
	string corrFileName = "images//corr//" + fileName + "_" + toStr(gridSize) 
		+ "_" + toStr(gridOffset) + "_" + toStr(patchSize) + "_full_test.txt";

	LmnIvrtPatchDistMetric metric2;
	GridGPMProc proc(&metric2, gridSize, gridOffset, 1, patchSize, 30);
	proc.SetROI(cvRect(0, 0, src->width, src->height));

	if(0)
	{
		proc.RunGridGPM(src2, corrFileName);
		proc.ShowGPMResUI(src, src2, corrFileName, 30);
	}

	//------------------- step 2 : voting  ------------------------------------
	if(0)
	{
		cvi* initMask = NULL, *initCfdc = NULL; 
		VoteProc vProc;
		vProc.LoadCorrs(corrFileName);
		vProc.Vote(src2, initMask, initCfdc, 30, 1.0f);
		cvsi("initMask.png", initMask);
		cvsi("initCnfdc.png", initCfdc);
	}

	//------------------- step 3 : MRF ----------------------------------------
	cvi* initMask = cvli("initMask.png");
	cvi* initCfdc = cvli("initCnfdc.png");
	cvi* shdwMask = NULL;
	MRFProc mProc;
	mProc.SolveWithInitial(src, src2, initMask, initCfdc, 256, shdwMask);
	cvsi("shdwMask.png", shdwMask);
	cvri(initMask); cvri(initCfdc);

	//------------------- step 4 : Recover ------------------------------------
	doFcvi(src, i, j)
	{
		auto v = cvg2(src2, i, j);
		double alpha = cvg20(shdwMask, i, j) / 255.0;
		cvs2(src2, i, j, cvs(v.val[0], v.val[1] / alpha, v.val[2]));
	}
	cvCvtColor(src2, src, CV_HLS2BGR_FULL);
	cvsi("result.png", src);

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
