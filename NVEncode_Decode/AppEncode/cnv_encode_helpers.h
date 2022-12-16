/***********************************************************************************************
created: 		2022-12-16

author:			chensong

purpose:		nv_encode
************************************************************************************************/

#ifndef _C_NV_ENCODE_HELPERS_H
#define _C_NV_ENCODE_HELPERS_H


#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>

#include "nvEncodeAPI.h"

typedef NVENCSTATUS(NVENCAPI *NV_CREATE_INSTANCE_FUNC)(NV_ENCODE_API_FUNCTION_LIST *);

 
extern NV_ENCODE_API_FUNCTION_LIST nv;
extern NV_CREATE_INSTANCE_FUNC nv_create_instance;

#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
extern bool init_nvenc( );
bool nv_failed(  NVENCSTATUS err, const char *func, const char *call);

#ifdef __cplusplus
}
#endif
 
#endif // !#define _C_NV_ENCODE_HELPERS_H