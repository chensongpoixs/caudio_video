//
//  H264ParseSPS.c
//
//  Created by lzj<lizhijian_21@163.com> on 2018/7/6.
//  Copyright © 2018年 LZJ. All rights reserved.
//
//  See https://www.itu.int/rec/T-REC-H.264-201610-S
//

#include "cbs_h2645.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
typedef unsigned char BYTE;
typedef int INT;
typedef unsigned int UINT;

typedef struct
{
	const BYTE *data;   //sps数据
	UINT size;          //sps数据大小
	UINT index;         //当前计算位所在的位置标记
} sps_bit_stream;

/**


H264附录B指明一种简单高效的方案解决解码器分辨每个NAL的起始位置和终止位置-当数据流存储在介质中时，在每个NAL前添加：0x000001

在某些类型的存储介质中，为了寻址方便，要求数据流在长度上对齐，或必须是某个常数的倍数。考虑到这种情况，H264建议在起始码前添加若干字节的0来填充，直到该NAL的长度符合要求。

在这样的机制下，解码器在码流中检测起始码，作为一个NAL的起始标志，当检测到下一个起始码时当前NAL结束。H264规定当检测到0x000000时也可以表征当前NAL的结束，这是因为连着的3个字节的0中任何一个字节的0要么属于起始码，要么是起始码前面的0。

但是如果NAL内部出现了0x000001或0x000000的序列怎么办？解决办法如下：

0x000000------------->0x00000300

0x000001-------------->0x00000301

0x000002-------------->0x00000302

0x000003-------------->0x00000303

就是在0000后面加上03；

有人问：如果原序列中本来就有个0x00000303呢？会不会导致歧义？

答案肯定是不会的，假设原序列：0x00 00 03 03 ....

那么经过转换之后变成： 0x00 00 03 03 03.....

解码之后：0x00 00 03 03

解码之后的数据和原序列一样。所以不用担心产生歧义。
移除H264的NAL防竞争码(0x03)

@param data sps数据
@param dataSize sps数据大小
*/
static void del_emulation_prevention(BYTE *data, UINT *dataSize)
{
	UINT dataSizeTemp = *dataSize;
	for (UINT i = 0, j = 0; i<(dataSizeTemp - 2); i++) {
		int val = (data[i] ^ 0x0) + (data[i + 1] ^ 0x0) + (data[i + 2] ^ 0x3);    //检测是否是竞争码
		if (val == 0) {
			for (j = i + 2; j<dataSizeTemp - 1; j++) {    //移除竞争码
				data[j] = data[j + 1];
			}

			(*dataSize)--;      //data size 减1
		}
	}
}

static void sps_bs_init(sps_bit_stream *bs, const BYTE *data, UINT size)
{
	if (bs) 
	{
		bs->data = data;
		bs->size = size;
		bs->index = 0;
	}
}

/**
是否已经到数据流最后

@param bs sps_bit_stream数据
@return 1：yes，0：no
*/
static INT eof(sps_bit_stream *bs)
{
	return (bs->index >= bs->size * 8);    //位偏移已经超出数据
}

/**
读取从起始位开始的BitCount个位所表示的值

@param bs sps_bit_stream数据
@param bitCount bit位个数(从低到高)
@return value
*/
static UINT u(sps_bit_stream *bs, BYTE bitCount)
{
	UINT val = 0;
	for (BYTE i = 0; i<bitCount; i++) {
		val <<= 1;
		if (eof(bs)) {
			val = 0;
			break;
		}
		else if (bs->data[bs->index / 8] & (0x80 >> (bs->index % 8))) {     //计算index所在的位是否为1
			val |= 1;
		}
		bs->index++;  //递增当前起始位(表示该位已经被计算，在后面的计算过程中不需要再次去计算所在的起始位索引，缺点：后面每个bit位都需要去位移)
	}

	return val;
}

/**

指数哥伦布码
在计算机中，一般数字的编码都为二进制，但是由于以相等长度来记录不同数字，因此会出现很多的冗余信息，如下：
|十进制   | 5        |  4       | 255     |2        |   1       |
|二进制   | 00000101 | 00000100 |11111111 |00000010 |00000001   |
|有效字节 | 3        |3         |  8      |   2     |    1      |
如数字1，原本只需要1个bit就能表示的数据，如今需要8个bit来表示，那么其余7个bit就可以看做是冗余数据，在网络传输时，如果以原本等长的编码方式来传输数据，则会出现很大的冗余量，加重网络负担但是如果只用有效字节来传输上述码流，则会是：10110011111111101，这样根本不能分离出原本的数据。哥伦布编码则是作为一种压缩编码算法，能很有效地对原本的数据进行压缩，并且能很容易地把编码后的码流分离成码字。


用来表示非负整数的k阶指数哥伦布码可用如下步骤生成：
1. 将数字以二进制形式写出，去掉最低的k个比特，之后加1
2. 计算留下的比特数，将此数减一，即是需要增加的前导零个数
3. 将第一步中去掉的最低k个比特位补回比特串尾部


读取无符号哥伦布编码值(UE)
#2^LeadingZeroBits - 1 + (xxx)
 

 
@param bs sps_bit_stream数据
@return value
*/
static UINT ue(sps_bit_stream *bs)
{
	UINT zeroNum = 0;
	while (u(bs, 1) == 0 && !eof(bs) && zeroNum < 32) {
		zeroNum++;
	}

	return (UINT)((1 << zeroNum) - 1 + u(bs, zeroNum));
}

/**
读取有符号哥伦布编码值(SE)
#(-1)^(k+1) * Ceil(k/2)

@param bs sps_bit_stream数据
@return value
*/
INT se(sps_bit_stream *bs)
{
	INT ueVal = (INT)ue(bs);
	double k = ueVal;

	INT seVal = (INT)ceil(k / 2);     //ceil:返回大于或者等于指定表达式的最小整数
	if (ueVal % 2 == 0) {       //偶数取反，即(-1)^(k+1)
		seVal = -seVal;
	}

	return seVal;
}

/**
视频可用性信息(Video usability information)解析

@param bs sps_bit_stream数据
@param info sps解析之后的信息数据及结构体
@see E.1.1 VUI parameters syntax
*/
void vui_para_parse(sps_bit_stream *bs, sps_info_struct *info)
{
	UINT aspect_ratio_info_present_flag = u(bs, 1);
	if (aspect_ratio_info_present_flag) {
		UINT aspect_ratio_idc = u(bs, 8);
		if (aspect_ratio_idc == 255) {      //Extended_SAR
			u(bs, 16);      //sar_width
			u(bs, 16);      //sar_height
		}
	}

	UINT overscan_info_present_flag = u(bs, 1);
	if (overscan_info_present_flag) {
		u(bs, 1);       //overscan_appropriate_flag
	}

	UINT video_signal_type_present_flag = u(bs, 1);
	if (video_signal_type_present_flag) {
		u(bs, 3);       //video_format
		u(bs, 1);       //video_full_range_flag
		UINT colour_description_present_flag = u(bs, 1);
		if (colour_description_present_flag) {
			u(bs, 8);       //colour_primaries
			u(bs, 8);       //transfer_characteristics
			u(bs, 8);       //matrix_coefficients
		}
	}

	UINT chroma_loc_info_present_flag = u(bs, 1);
	if (chroma_loc_info_present_flag) {
		ue(bs);     //chroma_sample_loc_type_top_field
		ue(bs);     //chroma_sample_loc_type_bottom_field
	}

	UINT timing_info_present_flag = u(bs, 1);
	if (timing_info_present_flag) {
		UINT num_units_in_tick = u(bs, 32);
		UINT time_scale = u(bs, 32);
		UINT fixed_frame_rate_flag = u(bs, 1);

		info->fps = (UINT)((float)time_scale / (float)num_units_in_tick);
		if (fixed_frame_rate_flag) {
			info->fps = info->fps / 2;
		}
	}

	UINT nal_hrd_parameters_present_flag = u(bs, 1);
	if (nal_hrd_parameters_present_flag) {
		//hrd_parameters()  //see E.1.2 HRD parameters syntax
	}

	//后面代码需要hrd_parameters()函数接口实现才有用
	UINT vcl_hrd_parameters_present_flag = u(bs, 1);
	if (vcl_hrd_parameters_present_flag) {
		//hrd_parameters()
	}
	if (nal_hrd_parameters_present_flag || vcl_hrd_parameters_present_flag) {
		u(bs, 1);   //low_delay_hrd_flag
	}

	u(bs, 1);       //pic_struct_present_flag
	UINT bitstream_restriction_flag = u(bs, 1);
	if (bitstream_restriction_flag) {
		u(bs, 1);   //motion_vectors_over_pic_boundaries_flag
		ue(bs);     //max_bytes_per_pic_denom
		ue(bs);     //max_bits_per_mb_denom
		ue(bs);     //log2_max_mv_length_horizontal
		ue(bs);     //log2_max_mv_length_vertical
		ue(bs);     //max_num_reorder_frames
		ue(bs);     //max_dec_frame_buffering
	}
}

//See 7.3.1 NAL unit syntax
//See 7.3.2.1.1 Sequence parameter set data syntax
INT h264_parse_sps(const BYTE *data, UINT dataSize, sps_info_struct *info)
{
	if (!data || dataSize <= 0 || !info) return 0;
	INT ret = 0;

	BYTE *dataBuf = (BYTE *)malloc(dataSize);
	memcpy(dataBuf, data, dataSize);        //重新拷贝一份数据，防止移除竞争码时对原数据造成影响
	del_emulation_prevention(dataBuf, &dataSize);

	sps_bit_stream bs = { 0 };
	sps_bs_init(&bs, dataBuf, dataSize);   //初始化SPS数据流结构体

	u(&bs, 1);      //forbidden_zero_bit
	u(&bs, 2);      //nal_ref_idc
	UINT nal_unit_type = u(&bs, 5);

	if (nal_unit_type == 0x7) {     //Nal SPS Flag
		info->profile_idc = u(&bs, 8);
		u(&bs, 1);      //constraint_set0_flag
		u(&bs, 1);      //constraint_set1_flag
		u(&bs, 1);      //constraint_set2_flag
		u(&bs, 1);      //constraint_set3_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 1);      //constraint_set4_flag
		u(&bs, 2);      //reserved_zero_2bits
		info->level_idc = u(&bs, 8);

		ue(&bs);    //seq_parameter_set_id

		UINT chroma_format_idc = 1;     //摄像机出图大部分格式是4:2:0
		if (info->profile_idc == 100 || info->profile_idc == 110 || info->profile_idc == 122 ||
			info->profile_idc == 244 || info->profile_idc == 44 || info->profile_idc == 83 ||
			info->profile_idc == 86 || info->profile_idc == 118 || info->profile_idc == 128 ||
			info->profile_idc == 138 || info->profile_idc == 139 || info->profile_idc == 134 || info->profile_idc == 135) {
			chroma_format_idc = ue(&bs);
			if (chroma_format_idc == 3) {
				u(&bs, 1);      //separate_colour_plane_flag
			}

			ue(&bs);        //bit_depth_luma_minus8
			ue(&bs);        //bit_depth_chroma_minus8
			u(&bs, 1);      //qpprime_y_zero_transform_bypass_flag
			UINT seq_scaling_matrix_present_flag = u(&bs, 1);
			if (seq_scaling_matrix_present_flag) {
				UINT seq_scaling_list_present_flag[8] = { 0 };
				for (INT i = 0; i<((chroma_format_idc != 3) ? 8 : 12); i++) {
					seq_scaling_list_present_flag[i] = u(&bs, 1);
					if (seq_scaling_list_present_flag[i]) {
						if (i < 6) {    //scaling_list(ScalingList4x4[i], 16, UseDefaultScalingMatrix4x4Flag[I])
						}
						else {    //scaling_list(ScalingList8x8[i − 6], 64, UseDefaultScalingMatrix8x8Flag[i − 6] )
						}
					}
				}
			}
		}

		ue(&bs);        //log2_max_frame_num_minus4
		UINT pic_order_cnt_type = ue(&bs);
		if (pic_order_cnt_type == 0) 
		{
			ue(&bs);        //log2_max_pic_order_cnt_lsb_minus4
		}
		else if (pic_order_cnt_type == 1) {
			u(&bs, 1);      //delta_pic_order_always_zero_flag
			se(&bs);        //offset_for_non_ref_pic
			se(&bs);        //offset_for_top_to_bottom_field

			UINT num_ref_frames_in_pic_order_cnt_cycle = ue(&bs);
			INT *offset_for_ref_frame = (INT *)malloc((UINT)num_ref_frames_in_pic_order_cnt_cycle * sizeof(INT));
			for (INT i = 0; i<num_ref_frames_in_pic_order_cnt_cycle; i++) {
				offset_for_ref_frame[i] = se(&bs);
			}
			free(offset_for_ref_frame);
		}

		ue(&bs);      //max_num_ref_frames
		u(&bs, 1);      //gaps_in_frame_num_value_allowed_flag

		UINT pic_width_in_mbs_minus1 = ue(&bs);     //第36位开始
		UINT pic_height_in_map_units_minus1 = ue(&bs);      //47
		UINT frame_mbs_only_flag = u(&bs, 1);

		info->width = (INT)(pic_width_in_mbs_minus1 + 1) * 16;
		info->height = (INT)(2 - frame_mbs_only_flag) * (pic_height_in_map_units_minus1 + 1) * 16;

		if (!frame_mbs_only_flag) {
			u(&bs, 1);      //mb_adaptive_frame_field_flag
		}

		u(&bs, 1);     //direct_8x8_inference_flag
		UINT frame_cropping_flag = u(&bs, 1);
		if (frame_cropping_flag) {
			UINT frame_crop_left_offset = ue(&bs);
			UINT frame_crop_right_offset = ue(&bs);
			UINT frame_crop_top_offset = ue(&bs);
			UINT frame_crop_bottom_offset = ue(&bs);

			//See 6.2 Source, decoded, and output picture formats
			INT crop_unit_x = 1;
			INT crop_unit_y = 2 - frame_mbs_only_flag;      //monochrome or 4:4:4
			if (chroma_format_idc == 1) {   //4:2:0
				crop_unit_x = 2;
				crop_unit_y = 2 * (2 - frame_mbs_only_flag);
			}
			else if (chroma_format_idc == 2) {    //4:2:2
				crop_unit_x = 2;
				crop_unit_y = 2 - frame_mbs_only_flag;
			}

			info->width -= crop_unit_x * (frame_crop_left_offset + frame_crop_right_offset);
			info->height -= crop_unit_y * (frame_crop_top_offset + frame_crop_bottom_offset);
		}

		UINT vui_parameters_present_flag = u(&bs, 1);
		if (vui_parameters_present_flag) 
		{
			vui_para_parse(&bs, info);
		}

		ret = 1;
	}
	free(dataBuf);

	return ret;
}
