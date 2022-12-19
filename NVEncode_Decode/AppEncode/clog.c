/***********************************************************************************************
created: 		2022-12-19

author:			chensong

purpose:		log
************************************************************************************************/
#include "clog.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

//static const char * g_nv_encode_file_name = "./nv_encode/encode.log";
static FILE* g_out_file_log_ptr = NULL;

static  bool log_init = false;

bool LOG_init()
{
	printf("[%s][%d]LOG init ... \n", __FUNCTION__, __LINE__);
	if (log_init)
	{
		printf("[%s][%d]LOG init  failed    !!!\n", __FUNCTION__, __LINE__);
		return false;
	}
	char buffer[1024] = {0};


	sprintf(&buffer[0], "./log/nvcodec_%llu.log", time(NULL));

	 
	g_out_file_log_ptr = fopen(buffer, "wb+");;;
 
	if (!g_out_file_log_ptr)
	{
		printf("[%s][%d]LOG  not write file log !!! (path = %s)   \n", __FUNCTION__, __LINE__, buffer);
	}

	log_init = true;
	printf("[%s][%d]LOG OK   \n", __FUNCTION__, __LINE__);
	return !!g_out_file_log_ptr;
}


void LOG_destroy()
{
	printf("[%s][%d]LOG destroy ... \n", __FUNCTION__, __LINE__);
	if (!log_init)
	{
		printf("[%s][%d]LOG destroy  failed  not init !!!\n", __FUNCTION__, __LINE__);
		return false;
	}

	log_init = false;
	if (g_out_file_log_ptr)
	{
		fflush(g_out_file_log_ptr);
		fclose(g_out_file_log_ptr);
		g_out_file_log_ptr = NULL;
	}

}
static inline void SHOW(const char* format, va_list args)
{
	
 

	char message[10240] = { 0 };

	int num = _vsprintf_p(message, 1024, format, args);
	if (g_out_file_log_ptr)
	{
		fprintf(g_out_file_log_ptr, "%s\n", message);
		fflush(g_out_file_log_ptr);
	}
	printf("%s\n", message);
	fflush(stdout);

}
void LOG(const char* format, ...)
{
	 
	va_list args;
	va_start(args, format);

	SHOW(format, args);
	va_end(args);

}