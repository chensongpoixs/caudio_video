# 音视频的基础知识 ^_^ 
   
 
## [一、 H.264视频码流解析](audio_video)

 

H.264原始码流（又称为“裸流”）是由一个一个的NALU组成的。他们的结构如下图所示。

```
|NALU|NALU|NALU|....
```

其中每个NALU之间通过startcode（起始码）进行分隔，起始码分成两种：0x000001（3Byte）或者0x00000001（4Byte）。如果NALU对应的Slice为一帧的开始就用0x00000001，否则就用0x000001。
H.264码流解析的步骤就是首先从码流中搜索0x000001和0x00000001，分离出NALU；然后再分析NALU的各个字段。本文的程序即实现了上述的两个步骤。



### 2、移除H264的NAL防竞争码(0x03)

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

 

## 二、H264码流分层

### 1、NAL层

Network Abstraction Layer,视频数据网络抽样层。

### 2、 VCL层

VIdeo Coding Layer, 视频数据编码层。

### 3、 码流基本概念

#### ①、 SODB（String Of Data Bits）

原始数据比特流，长度不一定是8的倍数，故需要补齐。它是由VCL层产生的。

#### ②、 RBSP（Raw Byte Sequence Payload）

SODB+trailing bits

算法是如果SODB最后一个字节不对齐，则补1和多个0.

#### ③、 NALU 单元

NAL Header(1byte) + RBSP 


![NALUnit](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/NALUnit.png?raw=true)


![h264_slice_block](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/h264_slice_block.png?raw=true)

![h264_slice](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/h264_slice.png?raw=true)

![h264_rtp_annexb](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/h264_rtp_annexb.png?raw=true)


 

1、SPS/PPS/Slice Header 

2、 常见分析工具

3、 ffmpeg视频编码


##  三, SPS中两个重要的参数分别是 Profile 与 Level

### 1、 H264 Profile


 

![h264_profile](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/h264_profile.png?raw=true)


![h264_profile_main_high](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/h264_profile_main_high.png?raw=true)

 

对视频压缩特性的描述，Profile越高，就说明采用了越高级的压缩特性

### 2、 H264 Level

![h264_level](https://github.com/chensongpoixs/caudio_video/blob/master/audio_video/H264/image/h264_level.png?raw=true)


 

Level是对视频的描述，Level越高，视频的码率、分辨率、fps越高


### 3、 分辨率 

|||
|:---:|:---:|
|pic_width_in_mbs_minues1|图像宽度包括的宏块个数-1|
|pic_heigh_in_mbs_minus1| 图像高度包括的宏块个数-1|
|frame_mbs_only_flag| 帧编码还是场编码 （场是隔行扫描、产生两张图）|
|frame_cropping_flag| 图像算法需要裁剪|
|frame_crop_left_offset|减去左侧的偏移量|
|frame_crop_right_offset|减去右侧的偏移量|
|frame_crop_top_offset|减去顶部的偏移量|
|frame_crop_bottom_offset|减去底部的偏移量|

### 4、 帧相关的

#### ①、 帧数 log2_max_frame_num_minus4

#### ②、 参考帧数 max_num_ref_frames

解码设置缓冲区大小的

#### ③、 显示帧序号 pic_order_cnt_type

解码 

### 5、 帧率的计算

```
framerate = (float)(sps->vui.vui_time_scale)/(float)(sps->vui.vui_num_units_in_tick)/2;
```



##  四、 PPS与 Slice Header

|||
|:---:|:---:|
|entropy_coding_mode_flag|熵编码，1表示使用CABAC编码|
|num_slice_groups_minus1|分片数量|
|weighted_pred_flag|在P/SP Slice中开启权重预测|
|weighted_bipred_idc|在B Slice中加权预测的方法|
|pic_init_qp_minus26/pic_init_qs_minus26|初始化量参数,实际参数在Slice header|
|chroma_qp_index_offset|用于计算色度分量的量化参数|
|deblocking_filter_control_present_flag|表示Slice header中是否存在用于去块滤波器控制的信息|
|constrainged_intra_pred_flag|若该标识为1，表示I宏块在进行帧内预测时只能使用来自I和SI类型宏块的信息|
|redundant_pic_cnt_preset_flag|用于表示Slice Header中是否存在redundant_pic_cnt语法元素|

#### Slice Header

#### ①、 帧类型

#### ②、 GOP中解码帧序号

#### ③、 预测权重

#### ④、 滤波


工具

Elecard Stream Eye

CodecVisa

40 * 16 = 640











## [五、NVIDIA之硬件编解码模块NVCODEC研究的demo](NVEncode_Decode) 
 

### [1、AppEncode编码demo](NVEncode_Decode/AppEncode)


### [2、AppDecode解码demo]()

 