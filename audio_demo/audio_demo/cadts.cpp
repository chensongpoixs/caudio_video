
/***********************************************************************************************
created: 		2023-12-17

author:			chensong

purpose:		ADTS

************************************************************************************************/

#include "cadts.h"
#include <iostream>
namespace chen {
	int parse_adts_header(uint8_t * header_buffer, cadts_header * adts_header)
	{

		memset(adts_header, 0, sizeof(*adts_header));

		if ((header_buffer[0] == 0xFF) && ((header_buffer[1] & 0xF0) == 0xF0)) {

			adts_header->id = ((unsigned int)header_buffer[1] & 0x08) >> 3;
			adts_header->layer = ((unsigned int)header_buffer[1] & 0x06) >> 1;
			adts_header->protection_absent = (unsigned int)header_buffer[1] & 0x01;
			adts_header->profile = ((unsigned int)header_buffer[2] & 0xc0) >> 6;
			adts_header->sampling_freq_index = ((unsigned int)header_buffer[2] & 0x3c) >> 2;
			adts_header->private_bit = ((unsigned int)header_buffer[2] & 0x02) >> 1;
			adts_header->channel_cfg = ((((unsigned int)header_buffer[2] & 0x01) << 2) | (((unsigned int)header_buffer[3] & 0xc0) >> 6));
			adts_header->original_copy = ((unsigned int)header_buffer[3] & 0x20) >> 5;
			adts_header->home = ((unsigned int)header_buffer[3] & 0x10) >> 4;
			adts_header->copyright_identification_bit = ((unsigned int)header_buffer[3] & 0x08) >> 3;
			adts_header->copyright_identification_start = (unsigned int)header_buffer[3] & 0x04 >> 2;
			adts_header->aac_frame_length = (((((unsigned int)header_buffer[3]) & 0x03) << 11) |
				(((unsigned int)header_buffer[4] & 0xFF) << 3) |
				((unsigned int)header_buffer[5] & 0xE0) >> 5);
			adts_header->adts_buffer_fullness = (((unsigned int)header_buffer[5] & 0x1f) << 6 |
				((unsigned int)header_buffer[6] & 0xfc) >> 2);
			adts_header->number_of_raw_data_block_in_frame = ((unsigned int)header_buffer[6] & 0x03);

			return 0;
		}
		else 
		{
			printf("failed to parse adts header\n");
			return -1;
		}
		return -1;
	}
	int convert_adts_header_to_buffer(cadts_header * adts_header, uint8_t * adts_header_buffer)
	{
		adts_header_buffer[0] = 0xFF;
		adts_header_buffer[1] = 0xF1;
		adts_header_buffer[2] = ((adts_header->profile) << 6) + (adts_header->sampling_freq_index << 2) + (adts_header->channel_cfg >> 2);
		adts_header_buffer[3] = (((adts_header->channel_cfg & 3) << 6) + (adts_header->aac_frame_length >> 11));
		adts_header_buffer[4] = ((adts_header->aac_frame_length & 0x7FF) >> 3);
		adts_header_buffer[5] = (((adts_header->aac_frame_length & 7) << 5) + 0x1F);
		adts_header_buffer[6] = 0xFC;
		return 0;
	}
}