/***********************************************************************************************
created: 		2022-12-08

author:			chensong

purpose:		nv_encode
************************************************************************************************/

//#include "jim-nvenc.h"
//#include <util/circlebuf.h>
//#include <util/darray.h>
//#include <util/dstr.h>
//#include <chen-avc.h>
//#include <libavutil/rational.h>
#define  _CRT_SECURE_NO_WARNINGS
#include "cnv_encode.h"
#include "cnv_encode_helpers.h"
#include "circlebuf.h"
#define INITGUID
#include <dxgi.h>
#include <d3d11.h>
#include <d3d11_1.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include "darray.h"
#include "cavc.h"
#include "clog.h"
/* ========================================================================= */

//static const std::string  g_nv_encode_file_name = "./nv_encode/";// +::time(NULL) + "_nv_encode.log";




 



#define EXTRA_BUFFERS 5
//void blog(int log_level, const char *format, ...) 
//{
//
//}
#define do_log  LOG       

#define error(format, ...) do_log(/*LOG_ERROR, */format, ##__VA_ARGS__)
#define warn(format, ...) do_log(/*LOG_WARNING, */format, ##__VA_ARGS__)
#define info(format, ...) do_log(/*LOG_INFO,*/ format, ##__VA_ARGS__)
#define debug(format, ...) do_log(/*LOG_DEBUG,*/ format, ##__VA_ARGS__)

#define error_hr(msg) error("%s: %s: 0x%08lX", __FUNCTION__, msg, (uint32_t)hr);



static const char *astrblank = "";
static const wchar_t *wstrblank = L"";


int astrcmpi(const char *str1, const char *str2)
{
	if (!str1)
	{
		str1 = astrblank;
	}
	if (!str2)
	{
		str2 = astrblank;
	}

	do {
		char ch1 = (char)toupper(*str1);
		char ch2 = (char)toupper(*str2);

		if (ch1 < ch2)
		{
			return -1;
		}
		else if (ch1 > ch2)
		{
			return 1;
		}
	} while (*str1++ && *str2++);

	return 0;
}



static const char * get_picture_type(NV_ENC_PIC_TYPE type)
{
	switch (type)
	{
	case NV_ENC_PIC_TYPE_P:
	{
		return "NV_ENC_PIC_TYPE_P";
		break;
	}

	case NV_ENC_PIC_TYPE_B:
	{
		return "NV_ENC_PIC_TYPE_B";
		break;
	}
	case NV_ENC_PIC_TYPE_I:
	{
		return "NV_ENC_PIC_TYPE_I";
		break;
	}
	case NV_ENC_PIC_TYPE_IDR:
	{
		return "NV_ENC_PIC_TYPE_IDR";
		break;
	}
	case NV_ENC_PIC_TYPE_BI:
	{
		return "NV_ENC_PIC_TYPE_BI";
		break;
	}

	case NV_ENC_PIC_TYPE_SKIPPED:
	{
		return "NV_ENC_PIC_TYPE_SKIPPED";
		break;
	}
	case NV_ENC_PIC_TYPE_INTRA_REFRESH:
	{
		return "NV_ENC_PIC_TYPE_INTRA_REFRESH";
		break;
	}
	default:
		return "NV_ENC_PIC_TYPE_UNKNOWN";
		break;
	}
	return "NV_ENC_PIC_TYPE_UNKNOWN";
}
 
struct nv_bitstream;
struct nv_texture;

struct handle_tex {
	uint32_t handle;
	ID3D11Texture2D *tex;
	IDXGIKeyedMutex *km;
};

/* ------------------------------------------------------------------------- */
/* Main Implementation Structure                                             */

struct nvenc_data {
	//chen_encoder_t *encoder;

	void *session;
	NV_ENC_INITIALIZE_PARAMS params;
	NV_ENC_CONFIG config;
	int rc_lookahead;
	int buf_count;
	int output_delay;
	int buffers_queued;
	size_t next_bitstream;
	size_t cur_bitstream;
	bool encode_started;
	bool first_packet;
	bool can_change_bitrate;
	int32_t bframes;

	DARRAY(struct nv_bitstream) bitstreams;
	DARRAY(struct nv_texture) textures;
	DARRAY(struct handle_tex) input_textures;
	struct circlebuf dts_list;

	DARRAY(uint8_t) packet_data;
	int64_t packet_pts;
	bool packet_keyframe;

	ID3D11Device *device;
	ID3D11DeviceContext *context;

	uint32_t cx;
	uint32_t cy;

	uint8_t *header;
	size_t header_size;

	uint8_t *sei;
	size_t sei_size;
};

/* ------------------------------------------------------------------------- */
/* Bitstream Buffer                                                          */

struct nv_bitstream {
	void *ptr;
	HANDLE event;
};

#define NV_FAILED(x) nv_failed( x, __FUNCTION__, #x)

static bool nv_bitstream_init(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	NV_ENC_CREATE_BITSTREAM_BUFFER buf = { NV_ENC_CREATE_BITSTREAM_BUFFER_VER};
	NV_ENC_EVENT_PARAMS params = {NV_ENC_EVENT_PARAMS_VER};
	HANDLE event = NULL;

	if (NV_FAILED(nv.nvEncCreateBitstreamBuffer(enc->session, &buf))) 
	{
		return false;
	}

	event = CreateEvent(NULL, true, true, NULL);
	if (!event) 
	{
		error("%s: %s", __FUNCTION__, "Failed to create event");
		goto fail;
	}

	params.completionEvent = event;
	if (NV_FAILED(nv.nvEncRegisterAsyncEvent(enc->session, &params))) 
	{
		goto fail;
	}

	bs->ptr = buf.bitstreamBuffer;
	bs->event = event;
	return true;

fail:
	if (event) 
	{
		CloseHandle(event);
	}
	if (buf.bitstreamBuffer) 
	{
		nv.nvEncDestroyBitstreamBuffer(enc->session, buf.bitstreamBuffer);
	}
	return false;
}

static void nv_bitstream_free(struct nvenc_data *enc, struct nv_bitstream *bs)
{
	if (bs->ptr) 
	{
		nv.nvEncDestroyBitstreamBuffer(enc->session, bs->ptr);

		NV_ENC_EVENT_PARAMS params = {NV_ENC_EVENT_PARAMS_VER};
		params.completionEvent = bs->event;
		nv.nvEncUnregisterAsyncEvent(enc->session, &params);
		CloseHandle(bs->event);
	}
}

/* ------------------------------------------------------------------------- */
/* Texture Resource                                                          */

struct nv_texture {
	void *res;
	ID3D11Texture2D *tex;
	void *mapped_res;
};

static bool nv_texture_init(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	ID3D11Device *device = enc->device;
	ID3D11Texture2D *tex;
	HRESULT hr;

	D3D11_TEXTURE2D_DESC desc = {0};
	desc.Width = enc->cx;
	desc.Height = enc->cy;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;// DXGI_FORMAT_NV12;
	desc.SampleDesc.Count = 1;
	desc.BindFlags = D3D11_BIND_RENDER_TARGET;

	hr = device->lpVtbl->CreateTexture2D(device, &desc, NULL, &tex);
	if (FAILED(hr)) {
		error_hr("Failed to create texture");
		return false;
	}

	tex->lpVtbl->SetEvictionPriority(tex, DXGI_RESOURCE_PRIORITY_MAXIMUM);

	NV_ENC_REGISTER_RESOURCE res = {NV_ENC_REGISTER_RESOURCE_VER};
	res.resourceType = NV_ENC_INPUT_RESOURCE_TYPE_DIRECTX;
	res.resourceToRegister = tex;
	res.width = enc->cx;
	res.height = enc->cy;
	res.bufferFormat = NV_ENC_BUFFER_FORMAT_ARGB;// NV_ENC_BUFFER_FORMAT_NV12;

	if (NV_FAILED(nv.nvEncRegisterResource(enc->session, &res))) 
	{
		tex->lpVtbl->Release(tex);
		return false;
	}

	nvtex->res = res.registeredResource;
	nvtex->tex = tex;
	return true;
}

static void nv_texture_free(struct nvenc_data *enc, struct nv_texture *nvtex)
{
	if (nvtex->res) {
		if (nvtex->mapped_res) {
			nv.nvEncUnmapInputResource(enc->session,
						   nvtex->mapped_res);
		}
		nv.nvEncUnregisterResource(enc->session, nvtex->res);
		nvtex->tex->lpVtbl->Release(nvtex->tex);
	}
}

/* ------------------------------------------------------------------------- */
/* Implementation                                                            */

static const char *nvenc_get_name(void *type_data)
{
	//UNUSED_PARAMETER(type_data);
	return "NVIDIA NVENC H.264 (new)";
}

static inline int nv_get_cap(struct nvenc_data *enc, NV_ENC_CAPS cap)
{
	if (!enc->session)
		return 0;

	NV_ENC_CAPS_PARAM param = {NV_ENC_CAPS_PARAM_VER};
	int v;

	param.capsToQuery = cap;
	nv.nvEncGetEncodeCaps(enc->session, NV_ENC_CODEC_H264_GUID, &param, &v);
	return v;
}

/*static*/ bool nvenc_update(void *data, int bitrate)
{
	struct nvenc_data *enc = data;

	/* Only support reconfiguration of CBR bitrate */
	if (enc->can_change_bitrate) 
	{
		//int bitrate = (int)chen_data_get_int(settings, "bitrate");

		enc->config.rcParams.averageBitRate = bitrate * 1000;
		enc->config.rcParams.maxBitRate = bitrate * 1000;

		NV_ENC_RECONFIGURE_PARAMS params = {0};
		params.version = NV_ENC_RECONFIGURE_PARAMS_VER;
		params.reInitEncodeParams = enc->params;
		params.resetEncoder = 1;
		params.forceIDR = 1;

		if (NV_FAILED(nv.nvEncReconfigureEncoder(enc->session, &params)))
		{
			return false;
		}
	}

	return true;
}

static HANDLE get_lib(struct nvenc_data *enc, const char *lib)
{
	HMODULE mod = GetModuleHandleA(lib);
	if (mod)
	{
		return mod;
	}

	mod = LoadLibraryA(lib);
	if (!mod)
	{
		error("Failed to load %s", lib);
	}
	return mod;
}

typedef HRESULT(WINAPI *CREATEDXGIFACTORY1PROC)(REFIID, void **);

static bool init_d3d11(struct nvenc_data *enc, const struct cencoder_config * settings)
{
	HMODULE dxgi = get_lib(enc, "DXGI.dll");
	HMODULE d3d11 = get_lib(enc, "D3D11.dll");
	CREATEDXGIFACTORY1PROC create_dxgi;
	PFN_D3D11_CREATE_DEVICE create_device;
	IDXGIFactory1 *factory= NULL;
	IDXGIAdapter *adapter = NULL;
	ID3D11Device *device = NULL;
	ID3D11DeviceContext *context = NULL;
	HRESULT hr;

	if (!dxgi || !d3d11) 
	{
		return false;
	}

	create_dxgi = (CREATEDXGIFACTORY1PROC)GetProcAddress( dxgi, "CreateDXGIFactory1");
	create_device = (PFN_D3D11_CREATE_DEVICE)GetProcAddress( d3d11, "D3D11CreateDevice");

	if (!create_dxgi || !create_device) 
	{
		error("Failed to load D3D11/DXGI procedures");
		return false;
	}

	hr = create_dxgi(&IID_IDXGIFactory1, &factory);
	if (FAILED(hr)) 
	{
		error_hr("CreateDXGIFactory1 failed");
		return false;
	}

	// 选择GPU类型NVIDIA
	for (int gpuIndex = settings->gpu_index; gpuIndex <= 5; ++gpuIndex)
	{
		hr = factory->lpVtbl->EnumAdapters(factory, gpuIndex, &adapter);

		//
		char desc[128] = { 0 };
		DXGI_ADAPTER_DESC adapterDesc;
		adapter->lpVtbl->GetDesc(adapter, &adapterDesc);
		wcstombs(desc, adapterDesc.Description, sizeof(desc));
		info("[adapter name  = %s]", desc);

		factory->lpVtbl->Release(factory);
		if (FAILED(hr))
		{
			error_hr("EnumAdapters failed");
			return false;
		}

		if (strstr(desc, "NVIDIA") == NULL)
		{
			continue;
		}
		break;
	}
	

	hr = create_device(adapter, D3D_DRIVER_TYPE_UNKNOWN, NULL, 0, NULL, 0,
			   D3D11_SDK_VERSION, &device, NULL, &context);
	adapter->lpVtbl->Release(adapter);
	if (FAILED(hr)) 
	{
		error_hr("D3D11CreateDevice failed");
		return false;
	}

	enc->device = device;
	enc->context = context;
	return true;
}

static bool init_session(struct nvenc_data *enc)
{
	NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS params = {NV_ENC_OPEN_ENCODE_SESSION_EX_PARAMS_VER};
	params.device = enc->device;
	params.deviceType = NV_ENC_DEVICE_TYPE_DIRECTX;
	params.apiVersion = NVENCAPI_VERSION;

	if (NV_FAILED(nv.nvEncOpenEncodeSessionEx(&params, &enc->session))) {
		return false;
	}
	return true;
}

static bool init_encoder(struct nvenc_data *enc, const struct cencoder_config * settings/*, chen_data_t *settings*//*, bool psycho_aq*/)
{
	//const char *rc = "CBR"; //可控制 chen_data_get_string(settings, "rate_control");
	uint32_t bitrate = settings->bitrate;//(int)chen_data_get_int(settings, "bitrate");
	uint32_t max_bitrate = settings->max_bitrate;//5000; // (int)chen_data_get_int(settings, "max_bitrate"); // 
	uint32_t cqp = settings->cqp;// default 20 // (int)chen_data_get_int(settings, "cqp");
	//int keyint_sec = 0; // (int)chen_data_get_int(settings, "keyint_sec");
	//const char *preset = "ll"; // chen_data_get_string(settings, "preset");
	///*
	//preset值	释义
	//ultrafast	极快
	//superfast	超快
	//veryfast	非常快
	//faster	更快
	//fast	快
	//medium	中
	//slow	慢
	//slower	更慢
	//veryslow	非常慢
	//placebo	超慢
	//*/
	//const char *profile = "high";// chen_data_get_string(settings, "profile");
	bool lookahead = settings->lookahead; // chen_data_get_bool(settings, "lookahead");
	//int bf = 2; // (int)chen_data_get_int(settings, "bf");
	//bool vbr = false;// astrcmpi(rc, "VBR") == 0;
	NVENCSTATUS err;

	//video_t *video = chen_encoder_video(enc->encoder);
	//const struct video_output_info *voi = video_output_get_info(video);

	enc->cx = settings->width;
	enc->cy = settings->height;

	/* -------------------------- */
	/* get preset                 */

	GUID nv_preset = NV_ENC_PRESET_DEFAULT_GUID;
	bool twopass = false;
	bool hp = false;
	bool ll = false;

	if (astrcmpi(settings->preset, "hq") == 0) {
		nv_preset = NV_ENC_PRESET_HQ_GUID;

	} else if (astrcmpi(settings->preset, "mq") == 0) {
		nv_preset = NV_ENC_PRESET_HQ_GUID;
		twopass = true;

	} else if (astrcmpi(settings->preset, "hp") == 0) {
		nv_preset = NV_ENC_PRESET_HP_GUID;
		hp = true;

	} else if (astrcmpi(settings->preset, "ll") == 0) {
		nv_preset = NV_ENC_PRESET_LOW_LATENCY_DEFAULT_GUID;
		ll = true;

	} else if (astrcmpi(settings->preset, "llhq") == 0) {
		nv_preset = NV_ENC_PRESET_LOW_LATENCY_HQ_GUID;
		ll = true;

	} else if (astrcmpi(settings->preset, "llhp") == 0) {
		nv_preset = NV_ENC_PRESET_LOW_LATENCY_HP_GUID;
		hp = true;
		ll = true;
	}

	const bool rc_lossless = astrcmpi(settings->rc, "lossless") == 0;
	bool lossless = rc_lossless;
	if (rc_lossless) {
		lossless = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOSSLESS_ENCODE);
		if (lossless) {
			nv_preset = hp ? NV_ENC_PRESET_LOSSLESS_HP_GUID
				       : NV_ENC_PRESET_LOSSLESS_DEFAULT_GUID;
		} else {
			warn("lossless encode is not supported, ignoring");
		}
	}

	/* -------------------------- */
	/* get preset default config  */

	NV_ENC_PRESET_CONFIG preset_config = {NV_ENC_PRESET_CONFIG_VER, {NV_ENC_CONFIG_VER}};

	err = nv.nvEncGetEncodePresetConfig(enc->session, NV_ENC_CODEC_H264_GUID, nv_preset, &preset_config);
	if (nv_failed( err, __FUNCTION__, "nvEncGetEncodePresetConfig")) 
	{
		return false;
	}

	/* -------------------------- */
	/* main configuration         */

	enc->config = preset_config.presetCfg;

	uint32_t gop_size = settings->gop_size; // (keyint_sec) ? keyint_sec * voi->fps_num / voi->fps_den : 250;

	NV_ENC_INITIALIZE_PARAMS *params = &enc->params;
	NV_ENC_CONFIG *config = &enc->config;
	NV_ENC_CONFIG_H264 *h264_config = &config->encodeCodecConfig.h264Config;
	NV_ENC_CONFIG_H264_VUI_PARAMETERS *vui_params = &h264_config->h264VUIParameters;

	//int darWidth, darHeight;
	//av_reduce(&darWidth, &darHeight, voi->width, voi->height, 1024 * 1024);

	memset(params, 0, sizeof(*params));
	params->version = NV_ENC_INITIALIZE_PARAMS_VER;
	params->encodeGUID = NV_ENC_CODEC_H264_GUID;
	params->presetGUID = nv_preset;
	params->encodeWidth = enc->cx;
	params->encodeHeight = enc->cy;
	params->darWidth = enc->cx;
	params->darHeight = enc->cy;
	params->frameRateNum = settings->frameRateDen;
	params->frameRateDen = settings->frameRateNum;
	params->enableEncodeAsync = 1;
	params->enablePTD = 1;
	params->encodeConfig = &enc->config;
	config->gopLength = gop_size;
	config->frameIntervalP = 1 + settings->bf;
	h264_config->idrPeriod = gop_size;

	//bool repeat_headers = false; // chen_data_get_bool(settings, "repeat_headers");
	//重复SPS/PPS 放到关键帧前面 
	if (settings->repeat_headers)
	{
		h264_config->repeatSPSPPS = 1;
		h264_config->disableSPSPPS = 0;
		h264_config->outputAUD = 1;
	}

	h264_config->sliceMode = 3;
	h264_config->sliceModeData = 1;

	h264_config->useBFramesAsRef = NV_ENC_BFRAME_REF_MODE_DISABLED;

	vui_params->videoSignalTypePresentFlag = 1;
	vui_params->videoFullRangeFlag = false; // (voi->range == VIDEO_RANGE_FULL);
	vui_params->colourDescriptionPresentFlag = 1;

	/*switch (VIDEO_CS_DEFAULT) {
	case VIDEO_CS_601:
		vui_params->colourPrimaries = 6;
		vui_params->transferCharacteristics = 6;
		vui_params->colourMatrix = 6;
		break;
	case VIDEO_CS_DEFAULT:
	case VIDEO_CS_709:*/
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 1;
		vui_params->colourMatrix = 1;
	/*	break;
	case VIDEO_CS_SRGB:
		vui_params->colourPrimaries = 1;
		vui_params->transferCharacteristics = 13;
		vui_params->colourMatrix = 1;
		break;
	}
*/
	enc->bframes = settings-> bf;

	/* lookahead */
	const bool use_profile_lookahead = config->rcParams.enableLookahead;
	lookahead = nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_LOOKAHEAD) && (settings->lookahead || use_profile_lookahead);
	if (lookahead) 
	{
		enc->rc_lookahead = use_profile_lookahead
					    ? config->rcParams.lookaheadDepth
					    : 8;
	}

	int buf_count = max(4, config->frameIntervalP * 2 * 2);
	if (lookahead) 
	{
		buf_count = max(buf_count, config->frameIntervalP +  enc->rc_lookahead + EXTRA_BUFFERS);
	}

	buf_count = min(64, buf_count);
	enc->buf_count = buf_count;

	const int output_delay = buf_count - 1;
	enc->output_delay = output_delay;

	if (lookahead) 
	{
		const int lkd_bound = output_delay - config->frameIntervalP - 4;
		if (lkd_bound >= 0) 
		{
			config->rcParams.enableLookahead = 1;
			config->rcParams.lookaheadDepth =
				max(enc->rc_lookahead, lkd_bound);
			config->rcParams.disableIadapt = 0;
			config->rcParams.disableBadapt = 0;
		} 
		else 
		{
			lookahead = false;
		}
	}

	/* psycho aq */
	if (nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_TEMPORAL_AQ)) 
	{
		config->rcParams.enableAQ = settings-> psycho_aq;
		config->rcParams.aqStrength = 8;
		config->rcParams.enableTemporalAQ = settings->psycho_aq;
	} 
	else if (settings->psycho_aq)
	{
		warn("Ignoring Psycho Visual Tuning request since GPU is not capable");
	}

	/* -------------------------- */
	/* rate control               */

	enc->can_change_bitrate =
		nv_get_cap(enc, NV_ENC_CAPS_SUPPORT_DYN_BITRATE_CHANGE) && !lookahead;

	config->rcParams.rateControlMode = twopass ? NV_ENC_PARAMS_RC_VBR_HQ : NV_ENC_PARAMS_RC_VBR;

	if (astrcmpi(settings->rc, "cqp") == 0 || rc_lossless)
	{
		if (lossless) 
		{
			h264_config->qpPrimeYZeroTransformBypassFlag = 1;
			 cqp = 0;
		}

		config->rcParams.rateControlMode = NV_ENC_PARAMS_RC_CONSTQP;
		config->rcParams.constQP.qpInterP = cqp;
		config->rcParams.constQP.qpInterB = cqp;
		config->rcParams.constQP.qpIntra = cqp;
		enc->can_change_bitrate = false;

		bitrate = 0;
		max_bitrate = 0;

	} 
	else if (astrcmpi(settings->rc, "vbr") != 0)
	{ /* CBR by default */
		h264_config->outputBufferingPeriodSEI = 1;
		config->rcParams.rateControlMode =
			twopass ? NV_ENC_PARAMS_RC_2_PASS_QUALITY : NV_ENC_PARAMS_RC_CBR;
	}

	h264_config->outputPictureTimingSEI = 1;
	config->rcParams.averageBitRate = bitrate * 1000;
	config->rcParams.maxBitRate = false/*vbr*/ ? max_bitrate * 1000 : bitrate * 1000;
	config->rcParams.vbvBufferSize = bitrate * 1000;


	uint32_t const MinQP = 1;
	uint32_t const MaxQP =  51 ;
	//config->rcParams.multiPass = NV_ENC_TWO_PASS_FULL_RESOLUTION;
	NV_ENC_QP minQP = { MinQP, MinQP, MinQP };
	NV_ENC_QP maxQP = { MaxQP, MaxQP, MaxQP };
	config->rcParams.minQP = minQP;
	config->rcParams.maxQP = maxQP;
	config->rcParams.enableMinQP = 1;
	config->rcParams.enableMaxQP = 1;

	/* -------------------------- */
	/* profile                    */

	if (astrcmpi(settings->profile, "main") == 0)
	{
		config->profileGUID = NV_ENC_H264_PROFILE_MAIN_GUID;
	}
	else if (astrcmpi(settings->profile, "baseline") == 0)
	{
		config->profileGUID = NV_ENC_H264_PROFILE_BASELINE_GUID;
	}
	else if (!lossless) 
	{
		config->profileGUID = NV_ENC_H264_PROFILE_HIGH_GUID;
	}

	/* -------------------------- */
	/* initialize                 */

	if (NV_FAILED(nv.nvEncInitializeEncoder(enc->session, params))) 
	{
		return false;
	}

	info("settings:\n"
	     "\trate_control: %s\n"
	     "\tbitrate:      %d\n"
	     "\tcqp:          %d\n"
	     "\tkeyint:       %d\n"
	     "\tpreset:       %s\n"
	     "\tprofile:      %s\n"
	     "\twidth:        %d\n"
	     "\theight:       %d\n"
	     "\t2-pass:       %s\n"
	     "\tb-frames:     %d\n"
	     "\tlookahead:    %s\n"
	     "\tpsycho_aq:    %s\n",
		settings->rc, bitrate, cqp, gop_size, settings->preset, settings->profile, enc->cx, enc->cy,
	     twopass ? "true" : "false", settings->bf, lookahead ? "true" : "false",
		settings->psycho_aq ? "true" : "false");

	return true;
}

static bool init_bitstreams(struct nvenc_data *enc)
{
	da_reserve(enc->bitstreams, enc->buf_count);
	for (int i = 0; i < enc->buf_count; i++) {
		struct nv_bitstream bitstream;
		if (!nv_bitstream_init(enc, &bitstream)) {
			return false;
		}

		da_push_back(enc->bitstreams, &bitstream);
	}

	return true;
}

static bool init_textures(struct nvenc_data *enc)
{
	da_reserve(enc->bitstreams, enc->buf_count);
	for (int i = 0; i < enc->buf_count; i++) {
		struct nv_texture texture;
		if (!nv_texture_init(enc, &texture)) {
			return false;
		}

		da_push_back(enc->textures, &texture);
	}

	return true;
}

///static void nvenc_destroy(void *data);

static void *nvenc_create_internal(const struct cencoder_config * settings/*,chen_data_t *settings, chen_encoder_t *encoder,*/ /*bool psycho_aq*/)
{
	NV_ENCODE_API_FUNCTION_LIST init = {NV_ENCODE_API_FUNCTION_LIST_VER};
	struct nvenc_data *enc = bmalloc(sizeof(*enc));
	memset(enc, 0, sizeof(*enc));
	
	enc->first_packet = true;

	if (!init_nvenc(/*encoder*/)) {
		goto fail;
	}
	if (NV_FAILED(nv_create_instance(&init))) {
		goto fail;
	}
	if (!init_d3d11(enc, settings/*, settings*/)) {
		goto fail;
	}
	if (!init_session(enc)) {
		goto fail;
	}
	if (!init_encoder(enc, settings/*,settings,*/ /*psycho_aq*/)) {
		goto fail;
	}
	if (!init_bitstreams(enc)) {
		goto fail;
	}
	if (!init_textures(enc)) {
		goto fail;
	}

	return enc;

fail:
	nvenc_destroy(enc);
	return NULL;
}
 
/*static*/ void *nvenc_create(const struct cencoder_config * settings/*chen_data_t *settings, chen_encoder_t *encoder*/)
{
	/* this encoder requires shared textures, this cannot be used on a
	 * gpu other than the one chen is currently running on. */
	//const int gpu = 0;// (int)chen_data_get_int(settings, "gpu");
	//if (gpu != 0) {
	//	WARNING_EX_LOG(  "[jim-nvenc] different GPU selected by user, falling back to ffmpeg");
	//	goto reroute;
	//}

	 

	

	//const bool psycho_aq = true; // chen_data_get_bool(settings, "psycho_aq");
	struct nvenc_data *enc = nvenc_create_internal(/*settings, encoder,*/settings/*,  psycho_aq*/);
	if ((enc == NULL) && settings-> psycho_aq)
	{
		WARNING_EX_LOG( "[jim-nvenc] nvenc_create_internal failed, trying again without Psycho Visual Tuning");
		enc = nvenc_create_internal(/*settings, encoder,*/ settings, false);
	}

	if (enc) {
		return enc;
	}

reroute:
	return NULL;// chen_encoder_create_rerouted(encoder, "ffmpeg_nvenc");
}

static bool get_encoded_packet(struct nvenc_data *enc, bool finalize);

/*static*/ void nvenc_destroy(void *data)
{
	struct nvenc_data *enc = data;

	if (enc->encode_started) {
		size_t next_bitstream = enc->next_bitstream;
		HANDLE next_event = enc->bitstreams.array[next_bitstream].event;

		NV_ENC_PIC_PARAMS params = {NV_ENC_PIC_PARAMS_VER};
		params.encodePicFlags = NV_ENC_PIC_FLAG_EOS;
		params.completionEvent = next_event;
		nv.nvEncEncodePicture(enc->session, &params);
		get_encoded_packet(enc, true);
	}
	for (size_t i = 0; i < enc->textures.num; i++) {
		nv_texture_free(enc, &enc->textures.array[i]);
	}
	for (size_t i = 0; i < enc->bitstreams.num; i++) {
		nv_bitstream_free(enc, &enc->bitstreams.array[i]);
	}
	if (enc->session) {
		nv.nvEncDestroyEncoder(enc->session);
	}
	for (size_t i = 0; i < enc->input_textures.num; i++) {
		ID3D11Texture2D *tex = enc->input_textures.array[i].tex;
		IDXGIKeyedMutex *km = enc->input_textures.array[i].km;
		tex->lpVtbl->Release(tex);
		km->lpVtbl->Release(km);
	}
	if (enc->context) {
		enc->context->lpVtbl->Release(enc->context);
	}
	if (enc->device) {
		enc->device->lpVtbl->Release(enc->device);
	}

	bfree(enc->header);
	bfree(enc->sei);
	//circlebuf_free(&enc->dts_list);
	da_free(enc->textures);
	da_free(enc->bitstreams);
	da_free(enc->input_textures);
	da_free(enc->packet_data);
	bfree(enc);
}

static ID3D11Texture2D *get_tex_from_handle(struct nvenc_data *enc,
					    uint32_t handle,
					    IDXGIKeyedMutex **km_out)
{
	ID3D11Device *device = enc->device;
	IDXGIKeyedMutex *km = NULL;
	ID3D11Texture2D *input_tex;
	HRESULT hr;

	for (size_t i = 0; i < enc->input_textures.num; i++) {
		struct handle_tex *ht = &enc->input_textures.array[i];
		if (ht->handle == handle) {
			*km_out = ht->km;
			return ht->tex;
		}
	}

	hr = device->lpVtbl->OpenSharedResource(device,
						(HANDLE)(uintptr_t)handle,
						&IID_ID3D11Texture2D,
						&input_tex);
	if (FAILED(hr)) {
		error_hr("OpenSharedResource failed");
		return NULL;
	}

	/*hr = input_tex->lpVtbl->QueryInterface(input_tex, &IID_IDXGIKeyedMutex,
		&km);
	if (FAILED(hr)) {
		error_hr("QueryInterface(IDXGIKeyedMutex) failed");
		input_tex->lpVtbl->Release(input_tex);
		return NULL;
	}*/

	/*input_tex->lpVtbl->SetEvictionPriority(input_tex,
		DXGI_RESOURCE_PRIORITY_MAXIMUM);*/

	*km_out = km;

	struct handle_tex new_ht = {handle, input_tex, km};
	da_push_back(enc->input_textures, &new_ht);
	return input_tex;
}

static bool get_encoded_packet(struct nvenc_data *enc, bool finalize)
{
	void *s = enc->session;

	da_resize(enc->packet_data, 0);

	if (!enc->buffers_queued)
		return true;
	if (!finalize && enc->buffers_queued < enc->output_delay)
		return true;

	size_t count = finalize ? enc->buffers_queued : 1;

	for (size_t i = 0; i < count; i++) {
		size_t cur_bs_idx = enc->cur_bitstream;
		struct nv_bitstream *bs = &enc->bitstreams.array[cur_bs_idx];
		struct nv_texture *nvtex = &enc->textures.array[cur_bs_idx];

		/* ---------------- */

		NV_ENC_LOCK_BITSTREAM lock = {NV_ENC_LOCK_BITSTREAM_VER};
		lock.outputBitstream = bs->ptr;
		lock.doNotWait = false;

		if (NV_FAILED(nv.nvEncLockBitstream(s, &lock))) {
			return false;
		}

		/*if (enc->first_packet) 
		{
			uint8_t *new_packet;
			size_t size;

			enc->first_packet = false;
			extract_avc_headers(lock.bitstreamBufferPtr,
						lock.bitstreamSizeInBytes,
						&new_packet, &size,
						&enc->header, &enc->header_size,
						&enc->sei, &enc->sei_size);

			da_copy_array(enc->packet_data, new_packet, size);
			bfree(new_packet);
		} else*/
		{
			da_copy_array(enc->packet_data, lock.bitstreamBufferPtr,
				      lock.bitstreamSizeInBytes);
		}

		enc->packet_pts = (int64_t)lock.outputTimeStamp;
		enc->packet_keyframe = lock.pictureType == NV_ENC_PIC_TYPE_IDR;
		DEBUG_EX_LOG("[pictureType = %s]", get_picture_type(lock.pictureType));
		if (NV_FAILED(nv.nvEncUnlockBitstream(s, bs->ptr))) {
			return false;
		}

		/* ---------------- */

		if (nvtex->mapped_res) {
			NVENCSTATUS err;
			err = nv.nvEncUnmapInputResource(s, nvtex->mapped_res);
			if (nv_failed(/*enc->encoder,*/ err, __FUNCTION__,
				      "unmap")) {
				return false;
			}
			nvtex->mapped_res = NULL;
		}

		/* ---------------- */

		if (++enc->cur_bitstream == enc->buf_count)
			enc->cur_bitstream = 0;

		enc->buffers_queued--;
	}

	return true;
}

static FILE *out_rgba_ptr = NULL;
static FILE *out_h264_file_ptr =   NULL;
/*
void *data, uint32_t handle, int64_t pts,
		uint64_t lock_key, uint64_t *next_key,
		struct encoder_packet *packet,
		bool *received_packet
*/
/*static*/ bool nvenc_encode_tex(void *data, uint32_t handle, int64_t pts,
			     uint64_t lock_key, uint64_t *next_key,
			     struct encoder_packet *packet,
			     bool *received_packet)
{
	struct nvenc_data *enc = data;
	ID3D11Device *device = enc->device;
	ID3D11DeviceContext *context = enc->context;
	ID3D11Texture2D *input_tex;
	ID3D11Texture2D *output_tex;
	//static ID3D11Texture2D *new_tex;
	IDXGIKeyedMutex *km;
	struct nv_texture *nvtex;
	struct nv_bitstream *bs;
	NVENCSTATUS err;
#define INVALID_HANDLE (uint32_t) -1
	if (handle == INVALID_HANDLE) {
		error("Encode failed: bad texture handle");
		*next_key = lock_key;
		return false;
	}

	bs = &enc->bitstreams.array[enc->next_bitstream];
	nvtex = &enc->textures.array[enc->next_bitstream];

	input_tex = get_tex_from_handle(enc, handle, &km);
	output_tex = nvtex->tex;

	/*if (!input_tex) {
		*next_key = lock_key;
		return false;
	}*/

	circlebuf_push_back(&enc->dts_list, &pts, sizeof(pts));

	/* ------------------------------------ */
	/* wait for output bitstream/tex        */

	WaitForSingleObject(bs->event, INFINITE);

	/* ------------------------------------ */
	/* copy to output tex                   */

	//km->lpVtbl->AcquireSync(km, lock_key, INFINITE);

	context->lpVtbl->CopyResource(context, (ID3D11Resource *)output_tex, (ID3D11Resource *)input_tex);

	//if (!new_tex)
	//{
	//	D3D11_TEXTURE2D_DESC bufferTextureDesc = { 0 };
	//	bufferTextureDesc.Width = 1920;
	//	bufferTextureDesc.Height = 1080;
	//	bufferTextureDesc.MipLevels = 1;
	//	bufferTextureDesc.ArraySize = 1;

	//	bufferTextureDesc.SampleDesc.Count = 1;

	//	bufferTextureDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	//	bufferTextureDesc.BindFlags = 0;
	//	bufferTextureDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	//	bufferTextureDesc.MiscFlags = 0;
	//	bufferTextureDesc.Usage = D3D11_USAGE_STAGING;
	//	device->lpVtbl->CreateTexture2D(device, &bufferTextureDesc, NULL, &new_tex);
	//}
	//else
	//{
	//	context->lpVtbl->CopyResource(context, (ID3D11Resource *)new_tex, (ID3D11Resource *)input_tex);

	//	D3D11_MAPPED_SUBRESOURCE mapd;
	//	UINT subResource = 0;
	//	//D3D11CalcSubresource(0, 0, 1);
	//	HRESULT hr = context->lpVtbl->Map (context, (ID3D11Resource*)new_tex, subResource, D3D11_MAP_READ, 0, &mapd);
	//	if (FAILED(hr))
	//	{
	//		ERROR_EX_LOG("[%s][%d][ID3D11DeviceContext_Map][ERROR]\n", __FUNCTION__, __LINE__);

	//		return;
	//	}
	//	//size_t rgba_size = data.cx * data.cy * 4;

	//	// filp
	//	if (!out_rgba_ptr)
	//	{
	//		out_rgba_ptr = fopen("rgba.rgba", "wb+");
	//	  }

	//	fwrite(mapd.pData, 1, 1920 * 1080 * 4, out_rgba_ptr);
	//	fflush(out_rgba_ptr);
	//	 

	//	context->lpVtbl->Unmap(context, (ID3D11Resource*)new_tex, subResource);
	//	{
	//		SYSTEMTIME t1;
	//		GetSystemTime(&t1);
	//		//DEBUG_EX_LOG("cpu --> mem end  cur = %u", t1.wMilliseconds);
	//	}
	//}
	


	//km->lpVtbl->ReleaseSync(km, *next_key);

	/* ------------------------------------ */
	/* map output tex so nvenc can use it   */

	NV_ENC_MAP_INPUT_RESOURCE map = {NV_ENC_MAP_INPUT_RESOURCE_VER};
	map.registeredResource = nvtex->res;
	if (NV_FAILED(nv.nvEncMapInputResource(enc->session, &map))) {
		return false;
	}

	nvtex->mapped_res = map.mappedResource;

	/* ------------------------------------ */
	/* do actual encode call                */

	NV_ENC_PIC_PARAMS params = {0};
	params.version = NV_ENC_PIC_PARAMS_VER;
	params.pictureStruct = NV_ENC_PIC_STRUCT_FRAME;
	params.inputBuffer = nvtex->mapped_res;
	params.bufferFmt = NV_ENC_BUFFER_FORMAT_ARGB;// NV_ENC_BUFFER_FORMAT_NV12;
	params.inputTimeStamp = (uint64_t)pts;
	params.inputWidth = enc->cx;
	params.inputHeight = enc->cy;
	params.outputBitstream = bs->ptr;
	params.completionEvent = bs->event;

	err = nv.nvEncEncodePicture(enc->session, &params);
	if (err != NV_ENC_SUCCESS && err != NV_ENC_ERR_NEED_MORE_INPUT) {
		nv_failed(/*enc->encoder,*/ err, __FUNCTION__,
			  "nvEncEncodePicture");
		return false;
	}

	enc->encode_started = true;
	enc->buffers_queued++;

	if (++enc->next_bitstream == enc->buf_count) {
		enc->next_bitstream = 0;
	}

	/* ------------------------------------ */
	/* check for encoded packet and parse   */

	if (!get_encoded_packet(enc, false)) {
		return false;
	}

	/* ------------------------------------ */
	/* output encoded packet                */

	if (enc->packet_data.num) {
		int64_t dts;
		 circlebuf_pop_front(&enc->dts_list, &dts, sizeof(dts));

		///* subtract bframe delay from dts */
		//dts -= (int64_t)enc->bframes * packet->timebase_num;

		//*received_packet = true;
		//packet->data = enc->packet_data.array;
		//packet->size = enc->packet_data.num;
		static uint64_t count = 0;
		DEBUG_EX_LOG("[count = %llu][packet->size = %llu]\n", ++count, enc->packet_data.num);


		if (!out_h264_file_ptr)
		{
			out_h264_file_ptr = fopen("./h264.h264", "wb+");;
		}

		if (enc->packet_data.array)
		{
			fwrite(enc->packet_data.array, 1, enc->packet_data.num, out_h264_file_ptr);
			fflush(out_h264_file_ptr);
		}
		//packet->type = chen_ENCODER_VIDEO;
		//packet->pts = enc->packet_pts;
		//packet->dts = dts;
		//packet->keyframe = enc->packet_keyframe;
	} else {
		//*received_packet = false;
	}

	return true;
}

void nvenc_defaults(/*chen_data_t *settings*/)
{}
//extern chen_properties_t *nvenc_properties(void *unused);

static bool nvenc_extra_data(void *data, uint8_t **header, size_t *size)
{
	struct nvenc_data *enc = data;

	if (!enc->header) {
		return false;
	}

	*header = enc->header;
	*size = enc->header_size;
	return true;
}

static bool nvenc_sei_data(void *data, uint8_t **sei, size_t *size)
{
	struct nvenc_data *enc = data;

	if (!enc->sei) {
		return false;
	}

	*sei = enc->sei;
	*size = enc->sei_size;
	return true;
}
bool init_nv_encode(struct cencoder_info * info)
{

	return true;
}
//  struct cencoder_info nvenc_info = {
//	.id = "jim_nvenc",
//	.codec = "h264", 
//	.get_name = nvenc_get_name,
//	.create = nvenc_create,
//	.destroy = nvenc_destroy,
//	.update = nvenc_update,
//	.encode_texture = nvenc_encode_tex,
//	.get_defaults = nvenc_defaults,
//	//.get_properties = nvenc_properties,
//	.get_extra_data = nvenc_extra_data,
//	.get_sei_data = nvenc_sei_data,
//};
//