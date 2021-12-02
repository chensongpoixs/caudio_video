#include <iostream>

#include <cstdio>
#include <cstdlib>
#define _CRT_SECURE_NO_WARNINGS
extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/log.h>#include <libavcodec/codec.h>

#include <libavformat/avformat.h>
#include <libavdevice/avdevice.h>
}

 AVFormatContext * open_dev()
{
	int ret = 0;
	char errors[1024] = {0};

	AVFormatContext *fmt_ctx = NULL;
	AVDictionary * options = NULL;

	/// [video device]:[audio device]
	char *devicename = "desktop";

	avdevice_register_all();
	

	AVInputFormat * iformat = av_find_input_format("gdigrab");


	av_dict_set(&options, "video_size", "640X480", 0);
	av_dict_set(&options, "framerate", "30", 0);
	av_dict_set(&options, "pixel_formt", "nv12", 0);


	if ((ret = avformat_open_input(&fmt_ctx, devicename, iformat, &options)) < 0)
	{
		av_strerror(ret, errors, 1024);
		printf("Failed to open video device [%d]%s\n", ret, errors);
		return NULL;
	}
	return fmt_ctx;
}
int main(int argc, char *argv[])
{

	int ret = 0;

	AVFormatContext *fmt_ctx = NULL;

	AVPacket pkt;

	av_log_set_level(AV_LOG_DEBUG);


	FILE *out_file_ptr = ::fopen("video.yuv", "wb+");


	fmt_ctx = open_dev();

	while ((ret = av_read_frame(fmt_ctx, &pkt)) == 0)
	{
		av_log(NULL, AV_LOG_INFO, "packet size is %d(%p)\n", pkt.size, pkt.data);
		// width * height * 3;
		fwrite(pkt.data, 1, 640 *480 * 3, out_file_ptr);
		::fflush(out_file_ptr);
		av_packet_unref(&pkt);
	}

	fclose(out_file_ptr);
	return EXIT_SUCCESS;
}