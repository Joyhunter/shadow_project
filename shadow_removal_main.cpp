#include "stdafx.h"
#include "gpm_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	string fileName = "7_small";
	int gridSize = 50;
	int gridOffset = 40;
	int patchSize = 7;

	cvi* src = cvlic(("images//" + fileName + ".png").c_str());
	cvi* src2 = cvci(src);
	cvCvtColor(src, src2, CV_BGR2HLS);

// 	cvi* hue = cvci81(src);
// 	cvi* saturation = cvci81(src);
// 	cvi* value = cvci81(src);
// 	cvSplit(hsv, hue, 0, 0, 0 ); 
// 	cvSplit(hsv, 0, saturation, 0, 0 );  
// 	cvSplit(hsv, 0, 0, value, 0 );  
// 	cvsi("1.png", hue); 
// 	cvsi("2.png", saturation); 
// 	cvsi("3.png", value); 
// 	pause;

	RegularPatchDistMetric metric;
	LmnIvrtPatchDistMetric metric2;

	GridGPMProc proc(&metric2, gridSize, gridOffset, 1, patchSize, 20);
	proc.SetROI(cvRect(0, 0, src->width, src->height));
	//proc.SetROI(cvRect(30, 70, 50, 50));
	//proc.SetROI(cvRect(109, 33, 80, 80));
	//proc.SetROI(cvRect(129, 63, 50, 50));
	
	string corrFileName = fileName + "_" + toStr(gridSize) + "_" + toStr(gridOffset)
		+ "_" + toStr(patchSize) + "_full.txt";
	//cout<<corrFileName<<endl;

	proc.RunGridGPM(src2, corrFileName);
	proc.ShowGPMResUI(src, src2, corrFileName, 30);


	//proc.ShowGPMResUI(src, src2, "1_100_90_7.txt", 30);
	//proc.ShowGPMResUI(src, src2, "7_test_50_40_7_full.txt");

	//GPMProc proc(&metric);
	//proc.RunGPM(src, dst);

	cvri(src);
	cvri(src2);

	return 0;
}

