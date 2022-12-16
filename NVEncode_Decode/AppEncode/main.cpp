#define  _CRT_SECURE_NO_WARNINGS
//#include <iostream>
#include "cnv_encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cdxgi_output_duplication.h"
int main(int argc, char *argv[])
{

	//nvenc_info.get_name(NULL);
	 nvenc_create();
	//helloworld();

	chen::cdxgi_output_duplication dxgi;

	dxgi.init();

	while (true)
	{
		dxgi.Capture(30);
		Sleep(1);
	}


	return EXIT_SUCCESS;
}

