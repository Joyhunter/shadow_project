#include "stdafx.h"
#include "gpm_proc.h"

int _tmain(int argc, _TCHAR* argv[])
{
	cvi* src = cvlic("images//1.png");
	cvi* dst = cvlic("images//1.png");

	GPMProc proc;
	proc.RunGPM(src, dst);

	return 0;
}

