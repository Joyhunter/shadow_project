#include "stdafx.h"
#include "gpm_proc.h"
#include "vote_proc.h"
#include "mrf_proc.h"
#include "decmps_proc.h"
#include "img_container.h"

int _tmain(int argc, _TCHAR* argv[])
{
	//12 0.5  16 0.5   9 0.5
	string fileName = "12"; //12(2, 0.5) 9(3, 0.5) 1(3, 0.5) 16(2, 0.5) 
	//14(2, 0.2) 18(3, 0.2) 4(3) 
	int step = 0;
	int MRFLabelsN = 64;

	int downRatio = 2; 
	ImgContainer img("images//" + fileName + ".png", downRatio);

	//------------------- step 1 : patch match --------------------------------
	int gridSize = _i(img.src()->width * 0.28f);//56;//45;//41;//45;
	int gridOffset = _i(img.src()->width * 0.25f);//50;//39;//35;//39;
	int patchSize = 7;
	string corrFileName = "images//corr//" + fileName + "_" + toStr(gridSize) 
		+ "_" + toStr(gridOffset) + "_" + toStr(patchSize) + ".txt";

	LmnIvrtPatchDistMetric metric2;
	GridGPMProc proc(&metric2, gridSize, gridOffset, 1, patchSize, 30);

	if(step <= 0)
	{
		proc.RunGridGPMMultiScale(img, corrFileName, 2, 1);
		if(img.srcR() == NULL) img.GenerateResizedImg(1);
		proc.ShowGPMResUI(img, corrFileName, 80);
	}

	//------------------- step 2 : voting  ------------------------------------
	if(img.srcR() == NULL) img.GenerateResizedImg(1);
	if(step <= 1)
	{
		cvi* initMask = NULL, *initCfdc = NULL; 
		VoteProc vProc;
		vProc.LoadCorrs(corrFileName);
		vProc.Vote(img, initMask, initCfdc, 80); //80/0.5 80/1.0 
		cvsic("_mask_2voting.png", initMask, 0);
		cvsi("_ex_mask_voting.png", initMask);
		cvsi("_ex_cnfdc_voting.png", initCfdc);
		cvri(initMask); cvri(initCfdc);
	}
	return 0;
	//------------------- step 3 : MRF ----------------------------------------
	if(step <= 2)
	{
		cvi* initMask = cvli("_ex_mask_voting.png");
		cvi* initCfdc = cvlig("_ex_cnfdc_voting.png");
		cvi* shdwMaskMRF = NULL;
		MRFProc mProc;
		mProc.SolveWithInitial(img.src(), img.srcHLS(), initMask, initCfdc, MRFLabelsN, shdwMaskMRF);

		cvi* shdwMaskMRFOri = cvci83(img.srcOri());
		cvResize(shdwMaskMRF, shdwMaskMRFOri);

		cvsic("_mask_3mrf.png", shdwMaskMRFOri, 0);
		cvsi("_ex_mask_mrf.png", shdwMaskMRFOri);

		cvri(initMask); cvri(initCfdc);
		cvri(shdwMaskMRFOri);cvri(shdwMaskMRF);
	}

	//------------------- step 4: decompose, get structure --------------------
	if(step <= 3)
	{
		cvi* shdwMaskMRF = cvlic("_ex_mask_mrf.png");
		cvi* shdwMask = NULL;
		DecmpsProc dProc;
		//peeks number: 0.8~0.9~1.0  bigger: less regions
		dProc.Analysis(img.srcOri(), shdwMaskMRF, shdwMask, 0.9f, MRFLabelsN);
		cvsic("_mask_5result.png", shdwMask, 0);
		cvsi("_ex_mask_result.png", shdwMask);
		cvri(shdwMaskMRF);
		cvri(shdwMask);
	}

	//------------------- step 5 : Recover ------------------------------------
	cout<<"Recovery...";
	cvi* shdwMask = cvlic("_ex_mask_result.png");
	cvsi("_src.png", img.srcOri());
	doFcvi(img.srcOri(), i, j)
	{
		auto v = cvg2(img.srcHLSOri(), i, j);
		cvS alpha = cvg2(shdwMask, i, j) / 255.0;
		//alpha.val[1] = 1;
		cvs2(img.srcHLSOri(), i, j, cvs(v.val[0], v.val[1] / alpha.val[0], v.val[2] / alpha.val[1]));
	}
	cvCvtColor(img.srcHLSOri(), img.srcOri(), CV_HLS2BGR_FULL);
	cvsi("_src_rev.png", img.srcOri());
	cvri(shdwMask);
	cout<<"\rRecovery complete."<<endl;

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
