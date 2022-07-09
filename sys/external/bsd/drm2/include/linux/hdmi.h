/*	$NetBSD: hdmi.h,v 1.21 2022/07/09 18:18:36 riastradh Exp $	*/

/*-
 * Copyright (c) 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef	_LINUX_HDMI_H_
#define	_LINUX_HDMI_H_

#include <sys/types.h>

struct device;

/* namespace */
#define	hdmi_audio_infoframe_init	linux_hdmi_audio_infoframe_init
#define	hdmi_audio_infoframe_pack	linux_hdmi_audio_infoframe_pack
#define	hdmi_avi_infoframe_check	linux_hdmi_avi_infoframe_check
#define	hdmi_avi_infoframe_init		linux_hdmi_avi_infoframe_init
#define	hdmi_avi_infoframe_pack		linux_hdmi_avi_infoframe_pack
#define	hdmi_drm_infoframe_check	linux_hdmi_drm_infoframe_check
#define	hdmi_drm_infoframe_init		linux_hdmi_drm_infoframe_init
#define	hdmi_drm_infoframe_pack		linux_hdmi_drm_infoframe_pack
#define	hdmi_drm_infoframe_pack_only	linux_hdmi_drm_infoframe_pack_only
#define	hdmi_infoframe_checksum		linux_hdmi_infoframe_checksum
#define	hdmi_infoframe_log		linux_hdmi_infoframe_log
#define	hdmi_infoframe_pack		linux_hdmi_infoframe_pack
#define	hdmi_infoframe_pack_only	linux_hdmi_infoframe_pack_only
#define	hdmi_infoframe_set_checksum	linux_hdmi_infoframe_set_checksum
#define	hdmi_infoframe_unpack		linux_hdmi_infoframe_unpack
#define	hdmi_spd_infoframe_check	linux_hdmi_spd_infoframe_check
#define	hdmi_spd_infoframe_init		linux_hdmi_spd_infoframe_init
#define	hdmi_spd_infoframe_pack		linux_hdmi_spd_infoframe_pack
#define	hdmi_vendor_infoframe_check	linux_hdmi_vendor_infoframe_check
#define	hdmi_vendor_infoframe_init	linux_hdmi_vendor_infoframe_init
#define	hdmi_vendor_infoframe_length	linux_hdmi_vendor_infoframe_length
#define	hdmi_vendor_infoframe_pack	linux_hdmi_vendor_infoframe_pack

enum hdmi_3d_structure {
	HDMI_3D_STRUCTURE_INVALID		= -1,
	HDMI_3D_STRUCTURE_FRAME_PACKING		= 0,
	HDMI_3D_STRUCTURE_FIELD_ALTERNATIVE	= 1,
	HDMI_3D_STRUCTURE_LINE_ALTERNATIVE	= 2,
	HDMI_3D_STRUCTURE_SIDE_BY_SIDE_FULL	= 3,
	HDMI_3D_STRUCTURE_L_DEPTH		= 4,
	HDMI_3D_STRUCTURE_L_DEPTH_GFX_GFX_DEPTH	= 5,
	HDMI_3D_STRUCTURE_TOP_AND_BOTTOM	= 6,
	HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF	= 8,
};

enum hdmi_active_aspect {
	HDMI_ACTIVE_ASPECT_NONE			= 0,
	HDMI_ACTIVE_ASPECT_16_9_TOP		= 2,
	HDMI_ACTIVE_ASPECT_14_9_TOP		= 3,
	HDMI_ACTIVE_ASPECT_16_9_CENTER		= 4,
	HDMI_ACTIVE_ASPECT_PICTURE		= 8,
	HDMI_ACTIVE_ASPECT_4_3			= 9,
	HDMI_ACTIVE_ASPECT_16_9			= 10,
	HDMI_ACTIVE_ASPECT_14_9			= 11,
	HDMI_ACTIVE_ASPECT_4_3_SP_14_9		= 13,
	HDMI_ACTIVE_ASPECT_16_9_SP_14_9		= 14,
	HDMI_ACTIVE_ASPECT_16_9_SP_4_3		= 15,
};

enum hdmi_audio_coding_type {
	HDMI_AUDIO_CODING_TYPE_STREAM		= 0,
	HDMI_AUDIO_CODING_TYPE_PCM		= 1,
	HDMI_AUDIO_CODING_TYPE_AC3		= 2,
	HDMI_AUDIO_CODING_TYPE_MPEG1		= 3,
	HDMI_AUDIO_CODING_TYPE_MP3		= 4,
	HDMI_AUDIO_CODING_TYPE_MPEG2		= 5,
	HDMI_AUDIO_CODING_TYPE_AAC_LC		= 6,
	HDMI_AUDIO_CODING_TYPE_DTS		= 7,
	HDMI_AUDIO_CODING_TYPE_ATRAC		= 8,
	HDMI_AUDIO_CODING_TYPE_DSD		= 9,
	HDMI_AUDIO_CODING_TYPE_EAC3		= 10,
	HDMI_AUDIO_CODING_TYPE_DTS_HD		= 11,
	HDMI_AUDIO_CODING_TYPE_MLP		= 12,
	HDMI_AUDIO_CODING_TYPE_DST		= 13,
	HDMI_AUDIO_CODING_TYPE_WMA_PRO		= 14,
};

enum hdmi_audio_coding_type_ext {
	HDMI_AUDIO_CODING_TYPE_EXT_STREAM	= 0,
	HDMI_AUDIO_CODING_TYPE_EXT_HE_AAC	= 1,
	HDMI_AUDIO_CODING_TYPE_EXT_HE_AAC_V2	= 2,
	HDMI_AUDIO_CODING_TYPE_EXT_MPEG_SURROUND = 3,
};

enum hdmi_audio_sample_frequency {
	HDMI_AUDIO_SAMPLE_FREQUENCY_STREAM	= 0,
	HDMI_AUDIO_SAMPLE_FREQUENCY_32000	= 1,
	HDMI_AUDIO_SAMPLE_FREQUENCY_44100	= 2,
	HDMI_AUDIO_SAMPLE_FREQUENCY_48000	= 3,
	HDMI_AUDIO_SAMPLE_FREQUENCY_88200	= 4,
	HDMI_AUDIO_SAMPLE_FREQUENCY_96000	= 5,
	HDMI_AUDIO_SAMPLE_FREQUENCY_176400	= 6,
	HDMI_AUDIO_SAMPLE_FREQUENCY_192000	= 7,
};

enum hdmi_audio_sample_size {
	HDMI_AUDIO_SAMPLE_SIZE_STREAM		= 0,
	HDMI_AUDIO_SAMPLE_SIZE_16		= 1,
	HDMI_AUDIO_SAMPLE_SIZE_20		= 2,
	HDMI_AUDIO_SAMPLE_SIZE_24		= 3,
};

enum hdmi_colorimetry {
	HDMI_COLORIMETRY_NONE			= 0,
	HDMI_COLORIMETRY_ITU_601		= 1,
	HDMI_COLORIMETRY_ITU_709		= 2,
	HDMI_COLORIMETRY_EXTENDED		= 3,
};

enum hdmi_colorspace {
	HDMI_COLORSPACE_RGB			= 0,
	HDMI_COLORSPACE_YUV422			= 1,
	HDMI_COLORSPACE_YUV444			= 2,
	HDMI_COLORSPACE_YUV420			= 3,
	HDMI_COLORSPACE_IDO_DEFINED		= 7,
};

enum hdmi_content_type {
	HDMI_CONTENT_TYPE_GRAPHICS		= 0,
	HDMI_CONTENT_TYPE_PHOTO			= 1,
	HDMI_CONTENT_TYPE_CINEMA		= 2,
	HDMI_CONTENT_TYPE_GAME			= 3,
};

enum hdmi_extended_colorimetry {
	HDMI_EXTENDED_COLORIMETRY_XV_YCC_601	= 0,
	HDMI_EXTENDED_COLORIMETRY_XV_YCC_709	= 1,
	HDMI_EXTENDED_COLORIMETRY_S_YCC_601	= 2,
	HDMI_EXTENDED_COLORIMETRY_OPYCC_601	= 3,
	HDMI_EXTENDED_COLORIMETRY_OPRGB		= 4,
};

enum hdmi_nups {
	HDMI_NUPS_UNKNOWN			= 0,
	HDMI_NUPS_HORIZONTAL			= 1,
	HDMI_NUPS_VERTICAL			= 2,
	HDMI_NUPS_BOTH				= 3,
};

enum hdmi_picture_aspect {
	HDMI_PICTURE_ASPECT_NONE		= 0,
	HDMI_PICTURE_ASPECT_4_3			= 1,
	HDMI_PICTURE_ASPECT_16_9		= 2,
	HDMI_PICTURE_ASPECT_64_27		= 3,
	HDMI_PICTURE_ASPECT_256_135		= 4,
	HDMI_PICTURE_ASPECT_RESERVED		= 5,
};

enum hdmi_quantization_range {
	HDMI_QUANTIZATION_RANGE_DEFAULT		= 0,
	HDMI_QUANTIZATION_RANGE_LIMITED		= 1,
	HDMI_QUANTIZATION_RANGE_FULL		= 2,
};

enum hdmi_scan_mode {
	HDMI_SCAN_MODE_NONE			= 0,
	HDMI_SCAN_MODE_OVERSCAN			= 1,
	HDMI_SCAN_MODE_UNDERSCAN		= 2,
};

enum hdmi_ycc_quantization_range {
	HDMI_YCC_QUANTIZATION_RANGE_LIMITED	= 0,
	HDMI_YCC_QUANTIZATION_RANGE_FULL	= 1,
};

enum hdmi_packet_type {
        HDMI_PACKET_TYPE_NULL = 0x00,
        HDMI_PACKET_TYPE_AUDIO_CLOCK_REGEN	= 0x01,
        HDMI_PACKET_TYPE_AUDIO_SAMPLE		= 0x02,
        HDMI_PACKET_TYPE_GENERAL_CONTROL	= 0x03,
        HDMI_PACKET_TYPE_ACP			= 0x04,
        HDMI_PACKET_TYPE_ISRC1			= 0x05,
        HDMI_PACKET_TYPE_ISRC2			= 0x06,
        HDMI_PACKET_TYPE_ONE_BIT_AUDIO_SAMPLE	= 0x07,
        HDMI_PACKET_TYPE_DST_AUDIO		= 0x08,
        HDMI_PACKET_TYPE_HBR_AUDIO_STREAM	= 0x09,
        HDMI_PACKET_TYPE_GAMUT_METADATA		= 0x0a,
};

enum hdmi_infoframe_type {
	HDMI_INFOFRAME_TYPE_VENDOR		= 0x81,
	HDMI_INFOFRAME_TYPE_AVI			= 0x82,
	HDMI_INFOFRAME_TYPE_SPD			= 0x83,
	HDMI_INFOFRAME_TYPE_AUDIO		= 0x84,
	HDMI_INFOFRAME_TYPE_DRM			= 0x87,
};

enum hdmi_eotf {
	HDMI_EOTF_TRADITIONAL_GAMMA_SDR		= 0,
	HDMI_EOTF_TRADITIONAL_GAMMA_HDR		= 1,
	HDMI_EOTF_SMPTE_ST2084			= 2,
	HDMI_EOTF_BT_2100_HLG			= 3,
};

enum hdmi_metadata_type {
	HDMI_STATIC_METADATA_TYPE1		= 1,
};

struct hdmi_type1 {
	enum hdmi_eotf			eotf;
	enum hdmi_metadata_type		metadata_type;
	uint16_t			min_cll;
	uint16_t			max_cll;
	uint16_t			max_fall;
};

struct hdr_sink_metadata {
	struct hdmi_type1		hdmi_type1;
};

#define	HDMI_INFOFRAME_SIZE(TYPE)					      \
	(HDMI_INFOFRAME_HEADER_SIZE + HDMI_##TYPE##_INFOFRAME_SIZE)

#define	HDMI_INFOFRAME_HEADER_SIZE	4
struct hdmi_infoframe_header {
	enum hdmi_infoframe_type	type;
	uint8_t				version;
	uint8_t				length;
	/* checksum */
};

#define	HDMI_AUDIO_INFOFRAME_SIZE	10
struct hdmi_audio_infoframe {
	struct hdmi_infoframe_header	header;
	uint8_t				channels;
	enum hdmi_audio_coding_type	coding_type;
	enum hdmi_audio_sample_size	sample_size;
	enum hdmi_audio_sample_frequency sample_frequency;
	enum hdmi_audio_coding_type_ext	coding_type_ext;
	uint8_t				channel_allocation;
	uint8_t				level_shift_value;
	bool				downmix_inhibit;
};

#define	HDMI_AVI_INFOFRAME_SIZE		13
struct hdmi_avi_infoframe {
	struct hdmi_infoframe_header	header;
	enum hdmi_colorspace		colorspace;
	enum hdmi_scan_mode		scan_mode;
	enum hdmi_colorimetry		colorimetry;
	enum hdmi_picture_aspect	picture_aspect;
	enum hdmi_active_aspect		active_aspect;
	bool				itc;
	enum hdmi_extended_colorimetry	extended_colorimetry;
	enum hdmi_quantization_range	quantization_range;
	enum hdmi_nups			nups;
	uint8_t				video_code;
	enum hdmi_ycc_quantization_range ycc_quantization_range;
	enum hdmi_content_type		content_type;
	uint8_t				pixel_repeat;
	uint16_t			top_bar;
	uint16_t			bottom_bar;
	uint16_t			left_bar;
	uint16_t			right_bar;
};

#define	HDMI_DRM_INFOFRAME_SIZE		26
struct hdmi_drm_infoframe {
	struct hdmi_infoframe_header	header;
	enum hdmi_eotf			eotf;
	enum hdmi_metadata_type		metadata_type;
	struct {
		uint16_t	x, y;
	}				display_primaries[3];
	struct {
		uint16_t	x, y;
	}				white_point;
	uint16_t			max_display_mastering_luminance;
	uint16_t			min_display_mastering_luminance;
	uint16_t			max_cll;
	uint16_t			max_fall;
};

#define	HDMI_SPD_INFOFRAME_SIZE		25
struct hdmi_spd_infoframe {
	struct hdmi_infoframe_header	header;
	char				vendor[8];
	char				product[16];
	enum hdmi_spd_sdi {
	        HDMI_SPD_SDI_UNKNOWN	= 0,
		HDMI_SPD_SDI_DSTB	= 1,
		HDMI_SPD_SDI_DVDP	= 2,
		HDMI_SPD_SDI_DVHS	= 3,
		HDMI_SPD_SDI_HDDVR	= 4,
		HDMI_SPD_SDI_DVC	= 5,
		HDMI_SPD_SDI_DSC	= 6,
		HDMI_SPD_SDI_VCD	= 7,
		HDMI_SPD_SDI_GAME	= 8,
		HDMI_SPD_SDI_PC		= 9,
		HDMI_SPD_SDI_BD		= 10,
		HDMI_SPD_SDI_SACD	= 11,
		HDMI_SPD_SDI_HDDVD	= 12,
		HDMI_SPD_SDI_PMP	= 13,
	}				sdi;
};

#define	HDMI_IEEE_OUI		0x000c03
#define	HDMI_FORUM_IEEE_OUI	0xc45dd8

struct hdmi_vendor_infoframe {
	struct hdmi_infoframe_header	header;
	uint32_t			oui;
	uint8_t				vic;
	enum hdmi_3d_structure		s3d_struct;
	unsigned			s3d_ext_data;
};

union hdmi_vendor_any_infoframe {
	struct {
		struct hdmi_infoframe_header	header;
		uint32_t			oui;
	}				any;
	struct hdmi_vendor_infoframe	hdmi;
};

union hdmi_infoframe {
	struct hdmi_infoframe_header	any;
	struct hdmi_avi_infoframe	avi;
	struct hdmi_spd_infoframe	spd;
	struct hdmi_audio_infoframe	audio;
	struct hdmi_drm_infoframe	drm;
	union hdmi_vendor_any_infoframe	vendor;
};

int hdmi_audio_infoframe_init(struct hdmi_audio_infoframe *);
ssize_t hdmi_audio_infoframe_pack(const struct hdmi_audio_infoframe *, void *,
    size_t);

int hdmi_avi_infoframe_init(struct hdmi_avi_infoframe *);
int hdmi_avi_infoframe_check(const struct hdmi_avi_infoframe *);
ssize_t hdmi_avi_infoframe_pack(const struct hdmi_avi_infoframe *, void *,
    size_t);

int hdmi_drm_infoframe_init(struct hdmi_drm_infoframe *);
int hdmi_drm_infoframe_check(const struct hdmi_drm_infoframe *);
ssize_t hdmi_drm_infoframe_pack(const struct hdmi_drm_infoframe *, void *,
    size_t);
int hdmi_drm_infoframe_pack_only(const struct hdmi_drm_infoframe *, void *,
    size_t);

int hdmi_spd_infoframe_init(struct hdmi_spd_infoframe *, const char *,
    const char *);
int hdmi_spd_infoframe_check(const struct hdmi_spd_infoframe *);
ssize_t hdmi_spd_infoframe_pack(const struct hdmi_spd_infoframe *, void *,
    size_t);

int hdmi_vendor_infoframe_init(struct hdmi_vendor_infoframe *);
int hdmi_vendor_infoframe_check(const struct hdmi_vendor_infoframe *);
ssize_t hdmi_vendor_infoframe_pack(const struct hdmi_vendor_infoframe *,
    void *, size_t);

ssize_t hdmi_infoframe_pack(const union hdmi_infoframe *, void *, size_t);
ssize_t hdmi_infoframe_pack_only(const union hdmi_infoframe *, void *, size_t);
int hdmi_infoframe_unpack(union hdmi_infoframe *, const void *, size_t);
void hdmi_infoframe_log(const char *, struct device *,
    const union hdmi_infoframe *);

#endif	/* _LINUX_HDMI_H_ */
