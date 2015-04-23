// simpletest.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "voeapp.h"

int _tmain(int argc, _TCHAR* argv[])
{
	CVOETestApp voeapp;
	voeapp.Init();
	voeapp.Start();
	while(1)
	{

	}
	return 0;
}

