/***********************************************************************************************
created: 		2022-12-08

author:			chensong

purpose:		nv_encode
************************************************************************************************/

//#include "jim-nvenc.h"
//#include <util/platform.h>
//#include <util/threading.h>
//#include <util/dstr.h>
#include "cnv_encode.h"
#include "cnv_encode_helpers.h"
#include "clog.h"
//#include <thread>
//#include <mutex>
static void *nvenc_lib = NULL;
//static pthread_mutex_t init_mutex = PTHREAD_MUTEX_INITIALIZER;
//static std::mutex		init_mutex;

static CRITICAL_SECTION   init_mutex;// crit_section_;
NV_ENCODE_API_FUNCTION_LIST nv = {NV_ENCODE_API_FUNCTION_LIST_VER};
NV_CREATE_INSTANCE_FUNC nv_create_instance = NULL;

#define error  ERROR_EX_LOG
//blog(LOG_ERROR, "[jim-nvenc] " format, ##__VA_ARGS__)
#define debug  DEBUG_EX_LOG
//blog(LOG_DEBUG, "[jim-nvenc] " format, ##__VA_ARGS__)

bool nv_failed(  NVENCSTATUS err, const char *func, const char *call)
{
	//struct dstr error_message = {0};

	switch (err) {
	case NV_ENC_SUCCESS:
		return false;

	case NV_ENC_ERR_OUT_OF_MEMORY:
		error("NVENC.TooManySessions");
		break;

	case NV_ENC_ERR_UNSUPPORTED_DEVICE:
		error("NVENC.UnsupportedDevice");
		break;

	case NV_ENC_ERR_INVALID_VERSION:
		error("NVENC.OutdatedDriver");
		break;

	default:
		error("NVENC Error: %s: %s failed: %d (%s)", func, call, (int)err, nv_error_name(err));
		//chen_encoder_set_last_error(encoder, error_message.array);
		//dstr_free(&error_message);
		break;
	}

	error("%s: %s failed: %d (%s)", func, call, (int)err, nv_error_name(err));
	return true;
}

#define NV_FAILED(  x)  nv_failed(  x, __FUNCTION__, #x)

bool load_nvenc_lib(void)
{
	if (sizeof(void *) == 8) {
		nvenc_lib = LoadLibrary("nvEncodeAPI64.dll");
	} else {
		nvenc_lib = LoadLibrary("nvEncodeAPI.dll");
	}

	return !!nvenc_lib;
}
void *os_dlsym(void *module, const char *func)
{
	void *handle;

	handle = (void *)GetProcAddress((HMODULE)module, func);

	return handle;
}
static void *load_nv_func(const char *func)
{
	void *func_ptr = os_dlsym(nvenc_lib, func);
	if (!func_ptr) 
	{
		error("Could not load function: %s", func);
	}
	return func_ptr;
}

typedef NVENCSTATUS(NVENCAPI *NV_MAX_VER_FUNC)(uint32_t *);

const char *nv_error_name(NVENCSTATUS err)
{
#define RETURN_CASE(x) \
	case x:        \
		return #x

	switch (err) {
		RETURN_CASE(NV_ENC_SUCCESS);
		RETURN_CASE(NV_ENC_ERR_NO_ENCODE_DEVICE);
		RETURN_CASE(NV_ENC_ERR_UNSUPPORTED_DEVICE);
		RETURN_CASE(NV_ENC_ERR_INVALID_ENCODERDEVICE);
		RETURN_CASE(NV_ENC_ERR_INVALID_DEVICE);
		RETURN_CASE(NV_ENC_ERR_DEVICE_NOT_EXIST);
		RETURN_CASE(NV_ENC_ERR_INVALID_PTR);
		RETURN_CASE(NV_ENC_ERR_INVALID_EVENT);
		RETURN_CASE(NV_ENC_ERR_INVALID_PARAM);
		RETURN_CASE(NV_ENC_ERR_INVALID_CALL);
		RETURN_CASE(NV_ENC_ERR_OUT_OF_MEMORY);
		RETURN_CASE(NV_ENC_ERR_ENCODER_NOT_INITIALIZED);
		RETURN_CASE(NV_ENC_ERR_UNSUPPORTED_PARAM);
		RETURN_CASE(NV_ENC_ERR_LOCK_BUSY);
		RETURN_CASE(NV_ENC_ERR_NOT_ENOUGH_BUFFER);
		RETURN_CASE(NV_ENC_ERR_INVALID_VERSION);
		RETURN_CASE(NV_ENC_ERR_MAP_FAILED);
		RETURN_CASE(NV_ENC_ERR_NEED_MORE_INPUT);
		RETURN_CASE(NV_ENC_ERR_ENCODER_BUSY);
		RETURN_CASE(NV_ENC_ERR_EVENT_NOT_REGISTERD);
		RETURN_CASE(NV_ENC_ERR_GENERIC);
		RETURN_CASE(NV_ENC_ERR_INCOMPATIBLE_CLIENT_KEY);
		RETURN_CASE(NV_ENC_ERR_UNIMPLEMENTED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_REGISTER_FAILED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_NOT_REGISTERED);
		RETURN_CASE(NV_ENC_ERR_RESOURCE_NOT_MAPPED);
	}
#undef RETURN_CASE

	return "Unknown Error";
}

static inline bool init_nvenc_internal()
{
	static bool initialized = false;
	static bool success = false;

	if (initialized)
	{
		return success;
	}
	 
	initialized = load_nvenc_lib();
	if (!initialized)
	{
		WARNING_EX_LOG("nvEncodeAPI load failed !!!");
		return false;
	}
	NV_MAX_VER_FUNC nv_max_ver = (NV_MAX_VER_FUNC)load_nv_func("NvEncodeAPIGetMaxSupportedVersion");
	if (!nv_max_ver) 
	{
		error("Missing NvEncodeAPIGetMaxSupportedVersion, check your video card drivers are up to date.");
		return false;
	}

	uint32_t ver = 0;
	if (NV_FAILED(  nv_max_ver(&ver)))
	{
		return false;
	}

	uint32_t cur_ver = (NVENCAPI_MAJOR_VERSION << 4) | NVENCAPI_MINOR_VERSION;
	if (cur_ver > ver) {
		error("NVENC.OutdatedDriver");

		error("Current driver version does not support this NVENC "
		      "version, please upgrade your driver");
		return false;
	}

	nv_create_instance = (NV_CREATE_INSTANCE_FUNC)load_nv_func( "NvEncodeAPICreateInstance");
	if (!nv_create_instance) {
		error(  "Missing NvEncodeAPICreateInstance, check "
				 "your video card drivers are up to date.");
		return false;
	}

	if (NV_FAILED(  nv_create_instance(&nv))) {
		return false;
	}

	success = true;
	return true;
}

bool init_nvenc( )
{
	//std::lock_guard<std::mutex> lock(init_mutex);
	bool success;
	//EnterCriticalSection(&init_mutex);
	//pthread_mutex_lock(&init_mutex);
	success = init_nvenc_internal(/*encoder*/);
	//pthread_mutex_unlock(&init_mutex);
	//LeaveCriticalSection(&init_mutex);

	// ::DeleteCriticalSection(&init_mutex);
	return success;
}


void jim_nvenc_load(void)
{
	//pthread_mutex_init(&init_mutex, NULL);
	//std::lock_guard<std::mutex> lock(init_mutex);
	//chen_register_encoder(&nvenc_info);
}

void jim_nvenc_unload(void)
{
	//pthread_mutex_destroy(&init_mutex);
}
