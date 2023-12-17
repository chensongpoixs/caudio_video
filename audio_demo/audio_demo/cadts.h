
/***********************************************************************************************
created: 		2023-12-17

author:			chensong

purpose:		ADTS

************************************************************************************************/
#ifndef _C_ADTS_H_
#define _C_ADTS_H_
#include <cstdint>

namespace chen {
	struct cadts_header
	{
		unsigned int syncword; // 12bit 同步字  '1111 1111 1111', 说明一个ADTS帧的开始
		unsigned int id;		// 1 bit MPEG  标示符， 0 : MPEG-4, 1 : MPEG-2
		unsigned int layer;     // 2bit 总是 '00'
		unsigned int protection_absent; // 1bit 1:表示没有crc校验, 0:表示有crc校验
		unsigned int profile;   // 1bit 表示使用那个级别的AAC编码
		unsigned int sampling_freq_index;  // 4bit 表示使用的采样频率
		unsigned int private_bit; // 1bit
		unsigned int channel_cfg; // 3bit 表示声道数
		unsigned int original_copy;  // 1bit 
		unsigned int home;           // 1bit

									 // 下面的为改变的参数即每一帧都不同
		unsigned int copyright_identification_bit; // 1bit
		unsigned int copyright_identification_start; // 1bit
		unsigned int aac_frame_length; //13bit 一个ADTS帧的长度包括ADTS头和AAC原始流
		unsigned int adts_buffer_fullness; // 11bit 0X7FF 说明是码率可变的码流

										   // 表示ADTS帧中有number_of_raw_data_blocks_in_frame +1 个AAC原始帧
										   // 所以说number_of_raw_data_block_in_frame == 0
										   // 表示说ADTS帧中有一个AAC数据块并表示说没有(一个AAC原始帧包含一段时间内1024给采样及相关数据)
		unsigned int number_of_raw_data_block_in_frame; // 2bit
	};


	  int parse_adts_header(uint8_t * header_buf, struct cadts_header * adts_header);
	  int convert_adts_header_to_buffer(struct cadts_header * adts_header, uint8_t * adts_header_buffer);

}

#endif // _C_ADTS_H_