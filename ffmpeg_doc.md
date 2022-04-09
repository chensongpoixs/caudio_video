# ffmpeg 在2019年进行编码量化参数添加 对感兴趣区域进行编码量化 


## 可变区域的roi encoding


这种情况下，每个frame中的roi区域都可能会发生变化，包括区域个数、区域位置和大小、还有qoffset的值。所以，每次都需要重新申请side_data空间，并且填好数值。主要代码如下所示，省略了错误返回值的处理。

```
while () 
{
	// 首先解码得到
	AVFrameAVFrame *frame = decode();
	// 申请side_data空间，假设有2个区域。
	// 不需要显式释放申请到的空间，
	// 在AVFrame的清理函数中会自动处理。
	AVFrameSideData *sd =           av_frame_new_side_data(frame,AV_FRAME_DATA_REGIONS_OF_INTEREST,2*sizeof(AVRegionOfInterest));
	// 获取刚申请到的内存地址
	AVRegionOfInterest* roi =                 (AVRegionOfInterest*)sd->data;
	// 设置第一个区域    
	roi[0].self_size = sizeof(*roi);
    roi[0].top       = ...;   
	 roi[0].bottom    = ...;    
	 roi[0].left      = ...;    
	 roi[0].right     = ...;   
	 roi[0].qoffset   = ...;
	 // 设置第二个区域   
	 roi[1].self_size = sizeof(*roi);    
	 roi[1].top       = ...;   
	 roi[1].bottom    = ...;    
	 roi[1].left      = ...;   
	 roi[1].right     = ...;    
	 roi[1].qoffset   = ...;
	 // 然后调用视频编码器进行编码    
	 encode(frame);
 }
```

## 固定区域的roi encoding

这种情况下，因为roi区域不变，所以，我们没有必要每次都重新申请side_data并且填入数值，只要在一开始的时候准备好，后续继续使用即可，可以用下面的代码来实现。


```
// 首先准备好存放roi数据的buffer，假设只有1个roi区域
AVBufferRef *roi_buf_ref =       av_buffer_alloc(sizeof(AVRegionOfInterest));
// 获取刚申请到的内存地址
AVRegionOfInterest* roi =          (AVRegionOfInterest*)roi_buf_ref->data;
// 填入数值
*roi = (AVRegionOfInterest) {        .self_size = sizeof(*roi),        .top       = ...,  
      .bottom    = ...,        .left      = ...,        .right     = ...,      
	  .qoffset   = ...,        };while () 
	  {
	  // 首先解码得到
	  AVFrameAVFrame *frame = decode();
	  // 将之前准备好的roi数据(指针)放入frame的side_data中
	  AVBufferRef *ref = av_buffer_ref(roi_buf_ref);    
	  av_frame_new_side_data_from_buf(frame,AV_FRAME_DATA_REGIONS_OF_INTEREST,                  ref);
	  // 然后调用视频编码器进行编码    
	  encode(frame);
	  }
	  // 最后，释放一开始申请的内存
	  av_buffer_unref(&roi_buf_ref);

```