# 音频


## AAC音频格式有两种（ADIF、ADTS）


1. ADIF(Audio Data Interachange Format), 音频数据交换格式； 只有一个统一的头，必须得到所有数据后解码，适用于本地文件
2. ADTS(Audio Data Transport Stream), 音频数据传输流； 每一帧都有头信息，任意帧解码，适用于传输流。


好处是 ADTS每帧都可以解码播放




ffmpeg 命令行 从mp4视频文件提取aac音频文件

```

ffmpeg -i input.mp4  -vn -acodec aac input.aac 


备注: -vm disable video  丢掉视频
     -acodec  设置音频编码格式
```


ffmpeg从acc音频文件解码为pcm音频文件

```
ffmpeg -i input.aac -f s16le input.pcm

备注:  -f 表示输出格式
```


ffplay播放 .pcm 音频文件

```
ffplay -ar 44100 -ac 2 -f s16le -i input.pcm 
```