#include "stdafx.h"
#include "gpm_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	cvi* src = cvlic("images//1_test2.png");
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

	GridGPMProc proc(&metric2, 50, 40, 1, 7, 20);
	proc.SetROI(cvRect(0, 0, src->width, src->height));
	//proc.SetROI(cvRect(30, 70, 50, 50));
	//proc.SetROI(cvRect(109, 33, 80, 80));
	//proc.SetROI(cvRect(129, 63, 50, 50));
	
	//proc.RunGridGPM(src2, "corr.txt");
	proc.ShowGPMResUI(src, src2, "1_test2_50_40_7_full.txt");
	//proc.ShowGPMResUI(src, src2, "7_test_50_40_7_full.txt");

	//GPMProc proc(&metric);
	//proc.RunGPM(src, dst);

	cvri(src);

	return 0;
}

