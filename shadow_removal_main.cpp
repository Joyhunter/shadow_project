#include "stdafx.h"
#include "gpm_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	cvi* src = cvlic("images//1_test2.png");
	cvi* hsv = cvci(src);

	cvCvtColor(src, hsv, CV_BGR2HSV);
// 	cvi* hue = cvci81(src);
// 	cvi* saturation = cvci81(src);
// 	cvi* value = cvci81(src);
// 	cvSplit(src, hue, 0, 0, 0 ); 
// 	cvSplit(src, 0, saturation, 0, 0 );  
// 	cvSplit(src, 0, 0, value, 0 );  
// 	cvsi("1.png", hue); 
// 	cvsi("2.png", saturation); 
// 	cvsi("3.png", value); 
// 	pause;

	RegularPatchDistMetric metric;
	LmnIvrtPatchDistMetric metric2;

	GridGPMProc proc(&metric2, 50, 40, 1, 7, 20);
	//proc.SetROI(cvRect(0, 0, src->width, src->height));
	proc.SetROI(cvRect(30, 70, 30, 30));
	//proc.SetROI(cvRect(129, 33, 30, 30));
	//proc.SetROI(cvRect(129, 63, 50, 50));
	
	proc.RunGridGPM(hsv, "corr.txt");
	proc.ShowGPMResUI(src, "corr.txt");

	//GPMProc proc(&metric);
	//proc.RunGPM(src, dst);

	cvri(src);

	return 0;
}

