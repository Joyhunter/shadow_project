/*************************************************
  Copyright (C), 2014, Joy Artworks. Tsinghua Uni.
  All rights reserved.
  
  File name: win_file_header.h  v1.0
  Description: This file  gathers  frequently used 
               header, functions to handle files
			   in windows system.
			   Directly include  this file in your 
			   pre-compiled header.
  Other: 

  Header List:
	1. 

  Function List: 
    1. wGetDirFiles(dir, (type,) vec<str>&)

  History:
    1. Date: 2014.7.31
       Author: Li-Qian Ma
       Modification: Version 1.0.
*************************************************/

#pragma once

#include "stl_header.h"

/***************** header ************************/
#include <stdio.h>
#include <io.h>

inline void wGetDirFiles(string fileDir, vector<string>& fileNames)
{
	fileNames.clear();

	_finddata_t finder;
	long lfDir;

	if((lfDir = _findfirst(fileDir.c_str(), &finder)) == -1l)
		return;
	else
	{
		do
		{
			fileNames.push_back(string(finder.name));
		}
		while(_findnext(lfDir, &finder) == 0);
	}
	_findclose(lfDir);
}

inline void wGetDirFiles(string fileDir, string fileType, vector<string>& fileNames)
{
	return wGetDirFiles(fileDir + fileType, fileNames);
}