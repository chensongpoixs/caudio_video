# ffmpeg 命令

## ffmpeg 格式转换命令

### ffmpeg支持的YUV格式

ffmpeg -pix_fmts

### ffplay播放YUV格式

ffplay -pix_fmt yuv420p -s 480x360 a.yuv

### ffmpeg YUV格式转换

ffmpeg -pix_fmt yuv420p -s 176x144 -i carphone_qcif.yuv -pix_fmt nv12 carphone_qcif_nv12.yuv

### ffmpeg YUV缩放加格式转换

ffmpeg -s:v 1920x1080 -r 25 -i input.yuv -vf scale=960:540 -c:v rawvideo -pix_fmt yuv420p out.yuv 

### ffmpeg解码命令

ffmpeg -i 720P.264 -s 1280x720 -pix_fmt yuv422p 720P-out.yuv

### 音视频分离

ffmpeg -i 123.mp4 -codec copy -bsf:v h264_mp4toannexb -f h264 20180521.264

### YUV420_8bit->H264

ffmpeg.exe -s 1920x1080 -pix_fmt yuv420p -i Demo_1920_1080_HD.yuv -vcodec libx264 -x264-params fps=25:bframes=7:keyint=24:bitrate=2000:preset=fast Demo_1920_1080_2M.h264

### YUV420_8bit->H265

ffmpeg.exe -s 720x576 -pix_fmt yuv420p -i demo_720_576.yuv -vcodec libx265 -x265-params fps=25:bframes=7:keyint=24:bitrate=2000:preset=fast demo_720_576_2M.h265

### H264->TS

ffmpeg.exe -use_wallclock_as_timestamps true -i demo_720_576_2M.h264 -vcodec copy demo_2M_H264.ts

### H265->TS

ffmpeg.exe -i demo_720_576_2M.h265 -vcodec hevc -y demo_720_576_2M_H265.ts