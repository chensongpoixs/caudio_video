#define  _CRT_SECURE_NO_WARNINGS
//#include <iostream>
#include "cnv_encode.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "cdxgi_output_duplication.h"
#include "clog.h"
int main(int argc, char *argv[])
{

	LOG_init();
	/*
	uint32_t frame_rate;
	uint32_t bitrate;
	uint32_t max_bitrate;
	uint32_t gop_size;
	const char * codec_name; // default H264;
	uint32_t gpu_index;
	DXGI_FORMAT format;
	const char * rc; // CBR;
	uint32_t cqp;
	uint32_t keyint_sec;
	const char *profile; // "high"
	const char * preset; // "ll";
	bool lookahead; // false;
	uint32_t bf; // 2
	uint32_t frameRateNum  ;
	uint32_t frameRateDen ;
	bool repeat_headers; // pps //sps info
	bool psycho_aq; //
	*/
	//nvenc_info.get_name(NULL);
	chen::cdxgi_output_duplication dxgi;

	dxgi.init();
	cencoder_config encoderconfig = {
	  dxgi.width(),
	  dxgi.height(),
	  60,
	  300,
	  500 ,
	  250,
	  "h264",
	  0/*GPU index*/,
	  DXGI_FORMAT_B8G8R8A8_UNORM,
	  "CBR",
	  20,
	  0 /*keyint_sec*/,
	  "high" /*profile*/,
	  "ll" /*preset*/,
	  false /*lookahead*/,
	  5/*bf*/,
	  60 /*frameRateNum*/,
	  1 /*frameRateDen*/,
	  true /*repeat_headers*/,
	  true /*psycho_aq*/

	};
	void * p =  nvenc_create(&encoderconfig);
	 
	

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

	LOG_destroy();
	return EXIT_SUCCESS;
}

