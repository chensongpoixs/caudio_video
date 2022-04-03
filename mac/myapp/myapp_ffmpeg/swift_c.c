//
//  swift_c.c
//  myapp_ffmpeg
//
//  Created by chensong on 2022/3/22.
//

#include "swift_c.h"
#include "libavutil/avutil.h"


/**
 ffmpeg   要编译成动态库哈 
 */

void swift_call_c()
{
    
    printf("swift call c  =====> []!!!\n");
    
    av_log_set_level(AV_LOG_DEBUG);
    av_log(NULL, AV_LOG_DEBUG, "hello world --->  ffmpeg  !!!\n");
    return;
}
