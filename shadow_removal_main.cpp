#include "stdafx.h"
#include "gpm_proc.h"
#include "vote_proc.h"
#include "mrf_proc.h"
#include "decmps_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	string fileName = "1"; //1_small 7_test
	int step = 3;

	cvi* srcOri = cvlic(("images//" + fileName + ".png").c_str()); // 600*450
	int downRatio = 3; 
	cvi* src = cvci83(srcOri->width / downRatio, srcOri->height / downRatio); //200*150
	cvResize(srcOri, src);
	cvi* src2Ori = cvci(srcOri);
	cvCvtColor(srcOri, src2Ori, CV_BGR2HLS_FULL);
	cvi* src2 = cvci(src);
	cvCvtColor(src, src2, CV_BGR2HLS_FULL);

	//------------------- step 1 : patch match --------------------------------
	int gridSize = _i(src->width * 0.28f);//56;//45;//41;//45;
	int gridOffset = _i(src->width * 0.25f);//50;//39;//35;//39;
	int patchSize = 7;
	string corrFileName = "images//corr//" + fileName + "_" + toStr(gridSize) 
		+ "_" + toStr(gridOffset) + "_" + toStr(patchSize) + "_full_test2.txt";

	LmnIvrtPatchDistMetric metric2;
	GridGPMProc proc(&metric2, gridSize, gridOffset, 1, patchSize, 30);
	proc.SetROI(cvRect(0, 0, src->width, src->height));

	if(step <= 0)
	{
		proc.RunGridGPMMultiScale(src2, corrFileName, 2, 1);
		//proc.ShowGPMResUI(src, src2, corrFileName, 80);
	}

	//------------------- step 2 : voting  ------------------------------------
	if(step <= 1)
	{
		cvi* initMask = NULL, *initCfdc = NULL; 
		VoteProc vProc;
		vProc.LoadCorrs(corrFileName);
		vProc.Vote(src2, initMask, initCfdc, 80, 0.5f); //80/0.5 80/1.0 
		cvsic("_mask_2voting.png", initMask, 0);
		cvsi("_ex_mask_voting.png", initMask);
		cvsi("_ex_cnfdc_voting.png", initCfdc);
		cvri(initMask); cvri(initCfdc);
	}

	//------------------- step 3 : MRF ----------------------------------------
	if(step <= 2)
	{
		cvi* initMask = cvli("_ex_mask_voting.png");
		cvi* initCfdc = cvlig("_ex_cnfdc_voting.png");
		cvi* shdwMaskMRF = NULL;
		MRFProc mProc;
		mProc.SolveWithInitial(src, src, initMask, initCfdc, 64, shdwMaskMRF);

		cvi* shdwMaskMRFOri = cvci83(srcOri);
		cvResize(shdwMaskMRF, shdwMaskMRFOri);

		cvsic("_mask_3mrf.png", shdwMaskMRFOri, 0);
		cvsi("_ex_mask_mrf.png", shdwMaskMRFOri);

		cvri(initMask); cvri(initCfdc);
		cvri(shdwMaskMRFOri);cvri(shdwMaskMRF);
	}

	//------------------- step 4: decompose, get structure --------------------
	cvi* shdwMaskMRF = cvlic("_ex_mask_mrf.png");
	cvi* shdwMask = NULL;
	DecmpsProc dProc;
	//peeks number: 0.8~0.9~1.0  bigger: less regions
	//soft boundary radius: 0.05~0.1~0.15  radius * src->width
	dProc.Analysis(srcOri, shdwMaskMRF, shdwMask, 0.9f, 0.1f, 64);
	cvsic("_mask_4result.png", shdwMask, 0);
	cvri(shdwMaskMRF);

	//------------------- step 5 : Recover ------------------------------------
	cout<<"Recovery...";
	cvsi("_src.png", srcOri);
	doFcvi(srcOri, i, j)
	{
		auto v = cvg2(src2Ori, i, j);
		cvS alpha = cvg2(shdwMask, i, j) / 255.0;
		alpha.val[1] = 1;
		cvs2(src2Ori, i, j, cvs(v.val[0], v.val[1] / alpha.val[0], v.val[2] / alpha.val[1]));
	}
	cvCvtColor(src2Ori, srcOri, CV_HLS2BGR_FULL);
	cvsi("_src_rev.png", srcOri);
	cvri(shdwMask);
	cout<<"\rRecovery complete."<<endl;

	cvri(src); cvri(src2);
	cvri(srcOri); cvri(src2Ori);
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
