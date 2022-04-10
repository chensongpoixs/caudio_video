// H.264 NALU
// SPS , PPS 、 IDR





#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "get_bits.h"
#include "cbs_h2645.h"


int main(int argc, char *argv[])
{
	

	uint8_t  buffer_ptr[1024] = {0};


 

	FILE *in_file_ptr = fopen("sintel.h264", "rb+");

	if (!in_file_ptr)
	{
		return -1;
	}

	fread(&buffer_ptr[0], 1, 1024, in_file_ptr);
	sps_info_struct info;
	h264_parse_sps(&buffer_ptr[0]+4, 1020, &info);


	return 0;
}
 