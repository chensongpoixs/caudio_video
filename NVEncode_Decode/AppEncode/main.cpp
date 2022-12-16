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
	void * p =  nvenc_create();
	//helloworld();

	chen::cdxgi_output_duplication dxgi;

	dxgi.init();

	while (true)
	{
		dxgi.Capture(30);
		uint64_t next_key = 0;
		++next_key;
		int64_t pts = 1;
		++pts;
		nvenc_encode_tex(p, (uint32_t)dxgi.get_shared_handle(), pts, next_key -1, &next_key, 0, 0);
		Sleep(1);
	}


	return EXIT_SUCCESS;
}

