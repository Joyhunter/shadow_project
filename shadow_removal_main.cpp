#include "stdafx.h"
#include "gpm_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	cvi* src = cvlic("images//1_test2.png");
	//cvCvtColor(src, src, CV_RGB2HSV);

	RegularPatchDistMetric metric;
	LmnIvrtPatchDistMetric metric2;

	GridGPMProc proc(&metric2, 50, 40, 1, 13, 20);
	//proc.SetROI(cvRect(0, 0, src->width, src->height));
	//proc.SetROI(cvRect(30, 70, 50, 50));
	proc.SetROI(cvRect(129, 33, 50, 50));
	
	//proc.RunGridGPM(src, "corr.txt");
	proc.ShowGPMResUI(src, "corr.txt");

	//GPMProc proc(&metric);
	//proc.RunGPM(src, dst);

	cvri(src);

	return 0;
}

