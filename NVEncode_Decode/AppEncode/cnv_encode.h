/***********************************************************************************************
created: 		2022-12-08

author:			chensong

purpose:		nv_encode
************************************************************************************************/

#ifndef _C_NV_ENCODE_H
#define _C_NV_ENCODE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdbool.h>
 
#include "nvEncodeAPI.h"

typedef NVENCSTATUS(NVENCAPI *NV_CREATE_INSTANCE_FUNC)(NV_ENCODE_API_FUNCTION_LIST *);

extern const char *nv_error_name(NVENCSTATUS err);
extern NV_ENCODE_API_FUNCTION_LIST nv;
extern NV_CREATE_INSTANCE_FUNC nv_create_instance;



/**
 * Encoder interface
 *
 * Encoders have a limited usage with    You are not generally supposed to
 * implement every encoder out there.  Generally, these are limited or specific
 * encoders for h264/aac for streaming and recording.  It doesn't have to be
 * *just* h264 or aac of course, but generally those are the expected encoders.
 *
 * That being said, other encoders will be kept in mind for future use.
 */
struct  cencoder_info {
	/* ----------------------------------------------------------------- */
	/* Required implementation*/

	/** Specifies the named identifier of this encoder */
	const char *id;

	/** Specifies the encoder type (video or audio) */
	//enum encoder_type type;

	/** Specifies the codec */
	const char *codec;

	/**
	 * Gets the full translated name of this encoder
	 *
	 * @param  type_data  The type_data variable of this structure
	 * @return            Translated name of the encoder
	 */
	const char *(*get_name)(void *type_data);

	/**
	 * Creates the encoder with the specified settings
	 *
	 * @param  settings  Settings for the encoder
	 * @param  encoder    encoder context
	 * @return           Data associated with this encoder context, or
	 *                   NULL if initialization failed.
	 */
	void *(*create)( );

	/**
	 * Destroys the encoder data
	 *
	 * @param  data  Data associated with this encoder context
	 */
	void(*destroy)(void *data);

	/**
	 * Encodes frame(s), and outputs encoded packets as they become
	 * available.
	 *
	 * @param       data             Data associated with this encoder
	 *                               context
	 * @param[in]   frame            Raw audio/video data to encode
	 * @param[out]  packet           Encoder packet output, if any
	 * @param[out]  received_packet  Set to true if a packet was received,
	 *                               false otherwise
	 * @return                       true if successful, false otherwise.
	 */
	bool(*encode)(void *data, struct encoder_frame *frame,
		struct encoder_packet *packet, bool *received_packet);

	/** Audio encoder only:  Returns the frame size for this encoder */
	size_t(*get_frame_size)(void *data);

	/* ----------------------------------------------------------------- */
	/* Optional implementation */

	/**
	 * Gets the default settings for this encoder
	 *
	 * @param[out]  settings  Data to assign default settings to
	 */
	void(*get_defaults)();

	/**
	 * Gets the property information of this encoder
	 *
	 * @return         The properties data
	 */
	//properties_t *(*get_properties)(void *data);

	/**
	 * Updates the settings for this encoder (usually used for things like
	 * changing bitrate while active)
	 *
	 * @param  data      Data associated with this encoder context
	 * @param  settings  New settings for this encoder
	 * @return           true if successful, false otherwise
	 */
	bool(*update)(void *data);

	/**
	 * Returns extra data associated with this encoder (usually header)
	 *
	 * @param  data             Data associated with this encoder context
	 * @param[out]  extra_data  Pointer to receive the extra data
	 * @param[out]  size        Pointer to receive the size of the extra
	 *                          data
	 * @return                  true if extra data available, false
	 *                          otherwise
	 */
	bool(*get_extra_data)(void *data, uint8_t **extra_data, size_t *size);

	/**
	 * Gets the SEI data, if any
	 *
	 * @param       data      Data associated with this encoder context
	 * @param[out]  sei_data  Pointer to receive the SEI data
	 * @param[out]  size      Pointer to receive the SEI data size
	 * @return                true if SEI data available, false otherwise
	 */
	bool(*get_sei_data)(void *data, uint8_t **sei_data, size_t *size);

	/**
	 * Returns desired audio format and sample information
	 *
	 * @param          data  Data associated with this encoder context
	 * @param[in/out]  info  Audio format information
	 */
	void(*get_audio_info)(void *data, struct audio_convert_info *info);

	/**
	 * Returns desired video format information
	 *
	 * @param          data  Data associated with this encoder context
	 * @param[in/out]  info  Video format information
	 */
	void(*get_video_info)(void *data, struct video_scale_info *info);

	void *type_data;
	void(*free_type_data)(void *type_data);

	uint32_t caps;

	/**
	 * Gets the default settings for this encoder
	 *
	 * If get_defaults is also defined both will be called, and the first
	 * call will be to get_defaults, then to get_defaults2.
	 *
	 * @param[out]  settings  Data to assign default settings to
	 * @param[in]   typedata  Type Data
	 */
	void(*get_defaults2)( void *type_data);

	/**
	 * Gets the property information of this encoder
	 *
	 * @param[in]   data      Pointer from create (or null)
	 * @param[in]   typedata  Type Data
	 * @return                The properties data
	 */
	//properties_t *(*get_properties2)(void *data, void *type_data);

	bool(*encode_texture)(void *data, uint32_t handle, int64_t pts,
		uint64_t lock_key, uint64_t *next_key,
		struct encoder_packet *packet,
		bool *received_packet);
};
//extern   bool init_nvenc();

  // bool nv_failed(  NVENCSTATUS err, const char *func, const char *call);

#endif // !#define _C_ASYNC_LOG_H