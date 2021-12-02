录制屏幕
为了实现对于Window桌面录制，有两种方式进行采集：

DirectShow 设备
gdigrab设备
其基本命令行如下：

//Dshow设备
ffmpeg -f dshow -i video="screen-capture-recorder" output.mkv

//gdigrab设备 采集整个桌面
ffmpeg -f gdigrab -framerate 30 -i desktop output.mkv
1
2
3
4
5
这两个命令默认都是采用x264 进行编码，在本地CPU不是足够高的情况下，录制的视频画面根本看不清楚，这是因为编码效率太低导致；

为了提高录制效果，我们可以采用无损编码+提高编码速度方式进行录制，具体命令如下：

ffmpeg -framerate 30 -f gdigrab -i desktop -c:v libx264rgb -crf 0 -preset ultrafast output.mkv
或者
ffmpeg -framerate 30 -f gdigrab -i desktop -c:v libx264rgb -preset:v ultrafast -tune:v zerolatency output.mkv
1
2
3
关于FFmpeg屏幕采集可以参考 https://trac.ffmpeg.org/wiki/Capture/Desktop文章
关于H264编码方面的知识可以参考https://trac.ffmpeg.org/wiki/Encode/H.264文章

录制声音
在上面提到过录制屏幕除了采用gdigrab外，还可以采用dshow方式；它们的区别就是：gdigrab设置仅支持截取屏幕信息，对声音的录制是不支持的，而show方式可以支持录制屏幕和声音。

这个dshow软件的下载信息如下：

编译好的下载地址是:
http://sourceforge.net/projects/screencapturer/

源码地址是:
https://github.com/rdp/screen-capture-recorder-to-video-windows-free

为了使系统能识别出dshow设备，我们首先需要进行注册，为了去掉不必要的文件，我们只提取四个dll：

screen-capture-recorder.dll
screen-capture-recorder-x64.dll
audio_sniffer-x64.dll
audio_sniffer.dll
注册命令行如下：

//注册屏幕录制设备（我们采用32位的ffmpeg，可以不用注册带x64的dll）
regsvr32 /s  screen-capture-recorder.dll
//注册虚拟音频设备
regsvr32 /s audio_sniffer.dll
1
2
3
4
注册成功后，可以采用以下命令进行检查是否注册成功

ffmpeg -list_devices true -f dshow -i dummy  
1
系统输出大致如下:

“screen-capture-recorder” 这个就是桌面捕获设备,用于录制屏幕
“virtual-audio-capturer” 这个是音频捕获设备，用于录制声音

-f dshow -i audio="virtual-audio-capturer" 这代表声音从“virtual-audio-capturer”音频设备获取
1
为了能够同时录制声音和画面，我们可以使用以下命令进行采集：

ffmpeg -framerate 30 -f gdigrab -i desktop -f dshow -i audio="virtual-audio-capturer" -c:v libx264rgb -preset:v ultrafast -tune:v zerolatency output.mp4
1
用vlc打开录制文件，可以看书画面显示正常以及声音正常被播放处理，截图如下：


以上就是关于ffmpeg录制window桌面的全部过程了，欢迎大家交流~
