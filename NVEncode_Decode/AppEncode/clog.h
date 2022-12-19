/***********************************************************************************************
created: 		2022-12-19

author:			chensong

purpose:		log
************************************************************************************************/

#ifndef _C_LOG_H
#define _C_LOG_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
 


bool LOG_init();


void LOG_destroy();
void LOG(const char* format, ...);

#define WARNING_EX_LOG(format, ...)	LOG("[%s][%s][%d][warn]" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define DEBUG_EX_LOG(format, ...)   LOG("[%s][%s][%d][debug]" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#define ERROR_EX_LOG(format, ...)   LOG("[%s][%s][%d][error]" format, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif // _C_AVC_H