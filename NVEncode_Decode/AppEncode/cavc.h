/***********************************************************************************************
created: 		2022-12-16

author:			chensong

purpose:		avc 
************************************************************************************************/

#ifndef _C_AVC_H
#define _C_AVC_H
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#else
#if defined(_MSC_VER) && !defined(inline)
#define inline __inline
#endif
#endif
 
struct encoder_packet;

enum {
	 NAL_UNKNOWN = 0,
	 NAL_SLICE = 1,
	 NAL_SLICE_DPA = 2,
	 NAL_SLICE_DPB = 3,
	 NAL_SLICE_DPC = 4,
	 NAL_SLICE_IDR = 5,
	 NAL_SEI = 6,
	 NAL_SPS = 7,
	 NAL_PPS = 8,
	 NAL_AUD = 9,
	 NAL_FILLER = 12,
};

enum {
	NAL_PRIORITY_DISPOSABLE = 0,
	NAL_PRIORITY_LOW = 1,
	NAL_PRIORITY_HIGH = 2,
	NAL_PRIORITY_HIGHEST = 3,
};

/* Helpers for parsing AVC NAL units.  */

  bool avc_keyframe(const uint8_t *data, size_t size);
  const uint8_t *avc_find_startcode(const uint8_t *p, const uint8_t *end);
  //void parse_avc_packet(struct encoder_packet *avc_packet, const struct encoder_packet *src);
  //size_t parse_avc_header(uint8_t **header, const uint8_t *data, size_t size);
  void extract_avc_headers(const uint8_t *packet, size_t size, uint8_t **new_packet_data, size_t *new_packet_size,
	uint8_t **header_data, size_t *header_size,
	uint8_t **sei_data, size_t *sei_size);

#ifdef __cplusplus
}
#endif

#endif // _C_AVC_H