/*	$NetBSD: hdmi.h,v 1.14 2021/12/19 11:45:35 riastradh Exp $	*/

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
#include <sys/param.h>
#include <sys/errno.h>
#include <sys/systm.h>

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

static inline void
hdmi_infoframe_header_init(struct hdmi_infoframe_header *header,
    enum hdmi_infoframe_type type, uint8_t vers, uint8_t length)
{

	header->type = type;
	header->version = vers;
	header->length = length;
}

static inline int
hdmi_infoframe_header_check(const struct hdmi_infoframe_header *header,
    enum hdmi_infoframe_type type, uint8_t vers, uint8_t length)
{

	if (header->type != type ||
	    header->version != vers ||
	    header->length != length)
		return -EINVAL;
	return 0;
}

static inline int
hdmi_infoframe_header_pack(const struct hdmi_infoframe_header *header,
    uint8_t length, void *buf, size_t size)
{
	uint8_t *const p = buf;

	if (length < HDMI_INFOFRAME_HEADER_SIZE)
		return -ENOSPC;
	if (size < length)
		return -ENOSPC;

	p[0] = header->type;
	p[1] = header->version;
	p[2] = (length - HDMI_INFOFRAME_HEADER_SIZE);
	p[3] = 0;		/* checksum */

	return HDMI_INFOFRAME_HEADER_SIZE;
}

static inline uint8_t
hdmi_infoframe_checksum(const void *buf, size_t length)
{
	const uint8_t *p = buf;
	uint8_t checksum = 0;

	while (length--)
		checksum += *p++;

	return 256 - checksum;
}

static inline int
hdmi_infoframe_header_unpack(struct hdmi_infoframe_header *header,
    const void *buf, size_t size)
{
	const uint8_t *const p = buf;

	if (size < HDMI_INFOFRAME_HEADER_SIZE)
		return -EINVAL;
	if (p[2] > size - HDMI_INFOFRAME_HEADER_SIZE)
		return -EINVAL;
	if (hdmi_infoframe_checksum(buf, p[2] + HDMI_INFOFRAME_HEADER_SIZE))
		return -EINVAL;

	hdmi_infoframe_header_init(header, p[0], p[1], p[2]);
	return HDMI_INFOFRAME_HEADER_SIZE;
}

static inline void
hdmi_infoframe_set_checksum(void *buf, size_t length)
{
	uint8_t *p = buf;

	p[3] = hdmi_infoframe_checksum(buf, length);
}

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

static inline int
hdmi_audio_infoframe_init(struct hdmi_audio_infoframe *frame)
{
	static const struct hdmi_audio_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_AUDIO,
	    1, HDMI_AUDIO_INFOFRAME_SIZE);

	return 0;
}

static inline ssize_t
hdmi_audio_infoframe_pack(const struct hdmi_audio_infoframe *frame, void *buf,
    size_t size)
{
	const size_t length = HDMI_INFOFRAME_HEADER_SIZE +
	    HDMI_AUDIO_INFOFRAME_SIZE;
	uint8_t channels = 0;
	uint8_t *p = buf;
	int ret;

	KASSERT(frame->header.length == HDMI_AUDIO_INFOFRAME_SIZE);

	ret = hdmi_infoframe_header_pack(&frame->header, length, p, size);
	if (ret < 0)
		return ret;
	KASSERT(ret == HDMI_INFOFRAME_HEADER_SIZE);
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	if (frame->channels >= 2)
		channels = frame->channels - 1;

	p[0] = __SHIFTIN(frame->coding_type, __BITS(7,4));
	p[0] |= __SHIFTIN(channels, __BITS(2,0));

	p[1] = __SHIFTIN(frame->sample_frequency, __BITS(4,2));
	p[1] |= __SHIFTIN(frame->sample_size, __BITS(1,0));

	p[2] = __SHIFTIN(frame->coding_type_ext, __BITS(5,0));

	p[3] = __SHIFTIN(frame->level_shift_value, __BITS(6,3));

	p[4] = __SHIFTIN(frame->downmix_inhibit? 1 : 0, __BIT(7));

	/* PB6 to PB10 are reserved */
	p[5] = 0;
	p[6] = 0;
	p[7] = 0;
	p[8] = 0;
	p[9] = 0;

	CTASSERT(HDMI_AUDIO_INFOFRAME_SIZE == 10);

	hdmi_infoframe_set_checksum(buf, length);

	return length;
}

static inline int
hdmi_audio_infoframe_unpack(struct hdmi_audio_infoframe *frame,
    const void *buf, size_t size)
{
	const uint8_t *p = buf;
	int ret;

	memset(frame, 0, sizeof(*frame));

	ret = hdmi_infoframe_header_unpack(&frame->header, p, size);
	if (ret)
		return ret;
	if (frame->header.length != HDMI_AUDIO_INFOFRAME_SIZE)
		return -EINVAL;
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	frame->coding_type = __SHIFTOUT(p[0], __BITS(7,4));
	frame->channels = __SHIFTOUT(p[0], __BITS(2,0));

	frame->sample_frequency = __SHIFTOUT(p[1], __BITS(4,2));
	frame->sample_size = __SHIFTOUT(p[1], __BITS(1,0));

	frame->coding_type_ext = __SHIFTOUT(p[2], __BITS(5,0));

	frame->level_shift_value = __SHIFTOUT(p[3], __BITS(6,3));

	frame->downmix_inhibit = __SHIFTOUT(p[4], __BIT(7));

	return 0;
}

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

static inline int
hdmi_avi_infoframe_init(struct hdmi_avi_infoframe *frame)
{
	static const struct hdmi_avi_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_AVI, 2,
	    HDMI_AVI_INFOFRAME_SIZE);

	return 0;
}

static inline int
hdmi_avi_infoframe_check(const struct hdmi_avi_infoframe *frame)
{
	int ret;

	ret = hdmi_infoframe_header_check(&frame->header,
	    HDMI_INFOFRAME_TYPE_AVI, 2, HDMI_AVI_INFOFRAME_SIZE);
	if (ret)
		return ret;

	return 0;
}

static inline ssize_t
hdmi_avi_infoframe_pack(const struct hdmi_avi_infoframe *frame, void *buf,
    size_t size)
{
	const size_t length = HDMI_INFOFRAME_HEADER_SIZE +
	    HDMI_AVI_INFOFRAME_SIZE;
	uint8_t *p = buf;
	int ret;

	KASSERT(frame->header.length == HDMI_AVI_INFOFRAME_SIZE);

	ret = hdmi_infoframe_header_pack(&frame->header, length, p, size);
	if (ret < 0)
		return ret;
	KASSERT(ret == HDMI_INFOFRAME_HEADER_SIZE);
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	p[0] = __SHIFTIN(frame->colorspace, __BITS(6,5));
	p[0] |= __SHIFTIN(frame->active_aspect & 0xf? 1 : 0, __BIT(4));
	p[0] |= __SHIFTIN(frame->top_bar || frame->bottom_bar, __BIT(3));
	p[0] |= __SHIFTIN(frame->left_bar || frame->right_bar, __BIT(2));
	p[0] |= __SHIFTIN(frame->scan_mode, __BITS(1,0));

	p[1] = __SHIFTIN(frame->colorimetry, __BITS(7,6));
	p[1] |= __SHIFTIN(frame->picture_aspect, __BITS(5,4));
	p[1] |= __SHIFTIN(frame->active_aspect, __BITS(3,0));

	p[2] = __SHIFTIN(frame->itc? 1 : 0, __BIT(7));
	p[2] |= __SHIFTIN(frame->extended_colorimetry, __BITS(6,4));
	p[2] |= __SHIFTIN(frame->quantization_range, __BITS(3,2));
	p[2] |= __SHIFTIN(frame->nups, __BITS(1,0));

	p[3] = frame->video_code;

	p[4] = __SHIFTIN(frame->ycc_quantization_range, __BITS(7,6));
	p[4] |= __SHIFTIN(frame->content_type, __BITS(5,4));
	p[4] |= __SHIFTIN(frame->pixel_repeat, __BITS(3,0));

	le16enc(&p[5], frame->top_bar);
	le16enc(&p[7], frame->bottom_bar);
	le16enc(&p[9], frame->left_bar);
	le16enc(&p[11], frame->right_bar);
	CTASSERT(HDMI_AVI_INFOFRAME_SIZE == 13);

	hdmi_infoframe_set_checksum(buf, length);

	return length;
}

static inline int
hdmi_avi_infoframe_unpack(struct hdmi_avi_infoframe *frame, const void *buf,
    size_t size)
{
	const uint8_t *p = buf;
	int ret;

	memset(frame, 0, sizeof(*frame));

	ret = hdmi_infoframe_header_unpack(&frame->header, p, size);
	if (ret)
		return ret;
	if (frame->header.length != HDMI_AVI_INFOFRAME_SIZE)
		return -EINVAL;
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	frame->colorspace = __SHIFTOUT(p[0], __BITS(6,5));
	frame->scan_mode = __SHIFTOUT(p[0], __BITS(1,0));

	frame->colorimetry = __SHIFTOUT(p[1], __BITS(7,6));
	frame->picture_aspect = __SHIFTOUT(p[1], __BITS(5,4));
	if (p[0] & __BIT(4))
		frame->active_aspect = __SHIFTOUT(p[1], __BITS(3,0));

	frame->itc = __SHIFTOUT(p[2], __BIT(7));
	frame->extended_colorimetry = __SHIFTOUT(p[2], __BITS(6,4));
	frame->quantization_range = __SHIFTOUT(p[2], __BITS(3,2));
	frame->nups = __SHIFTOUT(p[2], __BITS(1,0));

	frame->video_code = p[3];

	frame->ycc_quantization_range = __SHIFTOUT(p[4], __BITS(7,6));
	frame->content_type = __SHIFTOUT(p[4], __BITS(5,4));
	frame->pixel_repeat = __SHIFTOUT(p[4], __BITS(3,0));

	if (p[0] & __BIT(3)) {
		frame->top_bar = le16dec(&p[5]);
		frame->bottom_bar = le16dec(&p[7]);
	}
	if (p[0] & __BIT(2)) {
		frame->left_bar = le16dec(&p[9]);
		frame->right_bar = le16dec(&p[11]);
	}

	return 0;
}

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

static inline int
hdmi_drm_infoframe_init(struct hdmi_drm_infoframe *frame)
{
	static const struct hdmi_drm_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_DRM,
	    1, HDMI_DRM_INFOFRAME_SIZE);

	return 0;
}

static inline int
hdmi_drm_infoframe_check(const struct hdmi_drm_infoframe *frame)
{
	int ret;

	ret = hdmi_infoframe_header_check(&frame->header,
	    HDMI_INFOFRAME_TYPE_DRM, 1, HDMI_DRM_INFOFRAME_SIZE);
	if (ret)
		return ret;

	return 0;
}

#define	hdmi_drm_infoframe_pack_only	hdmi_drm_infoframe_pack /* XXX */

static inline int
hdmi_drm_infoframe_pack(const struct hdmi_drm_infoframe *frame,
    void *buf, size_t size)
{
	const size_t length = HDMI_INFOFRAME_HEADER_SIZE +
	    HDMI_DRM_INFOFRAME_SIZE;
	uint8_t *p = buf;
	unsigned i;
	int ret;

	KASSERT(frame->header.length == HDMI_DRM_INFOFRAME_SIZE);

	ret = hdmi_infoframe_header_pack(&frame->header, length, p, size);
	if (ret < 0)
		return ret;
	KASSERT(ret == HDMI_INFOFRAME_HEADER_SIZE);
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	p[0] = frame->eotf;
	p[1] = frame->metadata_type;
	for (i = 0; i < __arraycount(frame->display_primaries); i++) {
		le16enc(&p[2 + 4*i], frame->display_primaries[i].x);
		le16enc(&p[2 + 4*i + 2], frame->display_primaries[i].y);
	}
	le16enc(&p[14], frame->white_point.x);
	le16enc(&p[16], frame->white_point.y);
	le16enc(&p[18], frame->min_display_mastering_luminance);
	le16enc(&p[20], frame->max_display_mastering_luminance);
	le16enc(&p[22], frame->max_cll);
	le16enc(&p[24], frame->max_fall);
	CTASSERT(HDMI_DRM_INFOFRAME_SIZE == 26);

	hdmi_infoframe_set_checksum(buf, length);

	return length;
}

static inline int
hdmi_drm_infoframe_unpack(struct hdmi_drm_infoframe *frame, const void *buf,
    size_t size)
{
	const uint8_t *p = buf;
	unsigned i;
	int ret;

	memset(frame, 0, sizeof(*frame));

	ret = hdmi_infoframe_header_unpack(&frame->header, p, size);
	if (ret)
		return ret;
	if (frame->header.length != HDMI_DRM_INFOFRAME_SIZE)
		return -EINVAL;
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	frame->eotf = p[0];
	frame->metadata_type = p[1];
	for (i = 0; i < __arraycount(frame->display_primaries); i++) {
		frame->display_primaries[i].x = le16dec(&p[2 + 4*i]);
		frame->display_primaries[i].y = le16dec(&p[2 + 4*i + 2]);
	}
	frame->white_point.x = le16dec(&p[14]);
	frame->white_point.y = le16dec(&p[16]);
	frame->min_display_mastering_luminance = le16dec(&p[18]);
	frame->max_display_mastering_luminance = le16dec(&p[20]);
	frame->max_cll = le16dec(&p[22]);
	frame->max_fall = le16dec(&p[24]);

	return 0;
}

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

static inline int
hdmi_spd_infoframe_init(struct hdmi_spd_infoframe *frame, const char *vendor,
    const char *product)
{
	static const struct hdmi_spd_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_SPD,
	    1, HDMI_SPD_INFOFRAME_SIZE);

	strncpy(frame->vendor, vendor, sizeof(frame->vendor));
	strncpy(frame->product, product, sizeof(frame->product));

	return 0;
}

static inline int
hdmi_spd_infoframe_check(const struct hdmi_spd_infoframe *frame)
{
	int ret;

	ret = hdmi_infoframe_header_check(&frame->header,
	    HDMI_INFOFRAME_TYPE_SPD, 2, HDMI_SPD_INFOFRAME_SIZE);
	if (ret)
		return ret;

	return 0;
}

static inline ssize_t
hdmi_spd_infoframe_pack(const struct hdmi_spd_infoframe *frame, void *buf,
    size_t size)
{
	const size_t length = HDMI_INFOFRAME_HEADER_SIZE +
	    HDMI_SPD_INFOFRAME_SIZE;
	uint8_t *p = buf;
	int ret;

	KASSERT(frame->header.length == HDMI_SPD_INFOFRAME_SIZE);

	ret = hdmi_infoframe_header_pack(&frame->header, length, p, size);
	if (ret < 0)
		return ret;
	KASSERT(ret == HDMI_INFOFRAME_HEADER_SIZE);
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	memcpy(&p[0], frame->vendor, 8);
	memcpy(&p[8], frame->product, 16);
	p[24] = frame->sdi;
	CTASSERT(HDMI_SPD_INFOFRAME_SIZE == 25);

	hdmi_infoframe_set_checksum(buf, length);

	return length;
}

static inline int
hdmi_spd_infoframe_unpack(struct hdmi_spd_infoframe *frame, const void *buf,
    size_t size)
{
	const uint8_t *p = buf;
	int ret;

	memset(frame, 0, sizeof(*frame));

	ret = hdmi_infoframe_header_unpack(&frame->header, p, size);
	if (ret)
		return ret;
	if (frame->header.length != HDMI_SPD_INFOFRAME_SIZE)
		return -EINVAL;
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	memcpy(frame->vendor, &p[0], 8);
	memcpy(frame->product, &p[8], 8);
	frame->sdi = p[24];

	return 0;
}

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

static inline int
hdmi_vendor_infoframe_init(struct hdmi_vendor_infoframe *frame)
{
	static const struct hdmi_vendor_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_VENDOR,
	    1, 0 /* depends on s3d_struct */);

	frame->oui = HDMI_IEEE_OUI;
	frame->s3d_struct = HDMI_3D_STRUCTURE_INVALID;

	return 0;
}

static inline size_t
hdmi_vendor_infoframe_length(const struct hdmi_vendor_infoframe *frame)
{

	if (frame->vic) {
		return 5;
	} else if (frame->s3d_struct != HDMI_3D_STRUCTURE_INVALID) {
		if (frame->s3d_struct < HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF)
			return 5;
		else
			return 6;
	} else {
		return 4;
	}
}

static inline int
hdmi_vendor_infoframe_check(const struct hdmi_vendor_infoframe *frame)
{

	if (frame->header.type != HDMI_INFOFRAME_TYPE_VENDOR ||
	    frame->header.version != 1)
		return -EINVAL;
	/* frame->header.length not used when packing */

	/* At most one may be supplied.  */
	if (frame->vic != 0 && frame->s3d_struct != HDMI_3D_STRUCTURE_INVALID)
		return -EINVAL;

	return 0;
}

static inline int
hdmi_vendor_infoframe_pack(const struct hdmi_vendor_infoframe *frame,
    void *buf, size_t size)
{
	uint8_t *p = buf;
	size_t length;
	int ret;

	/* At most one may be supplied.  */
	if (frame->vic != 0 && frame->s3d_struct != HDMI_3D_STRUCTURE_INVALID)
		return -EINVAL;

	length = HDMI_INFOFRAME_HEADER_SIZE;
	length += hdmi_vendor_infoframe_length(frame);

	ret = hdmi_infoframe_header_pack(&frame->header, length, p, size);
	if (ret < 0)
		return ret;
	KASSERT(ret == HDMI_INFOFRAME_HEADER_SIZE);
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	p[0] = 0x03;
	p[1] = 0x0c;
	p[2] = 0x00;

	if (frame->vic) {
		p[3] = __SHIFTIN(0x1, __BITS(6,5));
		p[4] = frame->vic;
	} else if (frame->s3d_struct != HDMI_3D_STRUCTURE_INVALID) {
		p[3] = __SHIFTIN(0x2, __BITS(6,5));
		p[4] = __SHIFTIN(frame->s3d_struct, __BITS(7,4));
		if (frame->s3d_struct >= HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF)
			p[5] = __SHIFTIN(frame->s3d_ext_data, __BITS(7,4));
	} else {
		p[3] = __SHIFTIN(0x0, __BITS(6,5));
	}

	hdmi_infoframe_set_checksum(buf, length);

	return length;
}

static inline int
hdmi_vendor_infoframe_unpack(struct hdmi_vendor_infoframe *frame,
    const void *buf, size_t size)
{
	const uint8_t *p = buf;
	int ret;

	memset(frame, 0, sizeof(*frame));

	ret = hdmi_infoframe_header_unpack(&frame->header, p, size);
	if (ret)
		return ret;
	if (frame->header.length < 4)
		return -EINVAL;
	p += HDMI_INFOFRAME_HEADER_SIZE;
	size -= HDMI_INFOFRAME_HEADER_SIZE;

	if (p[0] != 0x03 || p[1] != 0x0c || p[2] != 0x00)
		return -EINVAL;

	switch (__SHIFTOUT(p[3], __BITS(6,5))) {
	case 0x0:
		if (frame->header.length != 4)
			return -EINVAL;
		break;
	case 0x1:
		if (frame->header.length != 5)
			return -EINVAL;
		frame->vic = p[4];
		break;
	case 0x2:
		if (frame->header.length < 5)
			return -EINVAL;
		frame->s3d_struct = __SHIFTOUT(p[4], __BITS(7,4));
		if (frame->s3d_struct < HDMI_3D_STRUCTURE_SIDE_BY_SIDE_HALF) {
			if (frame->header.length != 5)
				return -EINVAL;
		} else {
			if (frame->header.length != 6)
				return -EINVAL;
			frame->s3d_ext_data = __SHIFTOUT(p[5], __BITS(7,4));
		}
		break;
	default:
		return -EINVAL;
	}

	return 0;
}

union hdmi_infoframe {
	struct hdmi_infoframe_header	any;
	struct hdmi_avi_infoframe	avi;
	struct hdmi_drm_infoframe	drm;
	struct hdmi_spd_infoframe	spd;
	union hdmi_vendor_any_infoframe	vendor;
};

#define	hdmi_infoframe_pack_only	hdmi_infoframe_pack /* XXX */

static inline ssize_t
hdmi_infoframe_pack(const union hdmi_infoframe *frame, void *buf, size_t size)
{

	switch (frame->any.type) {
	case HDMI_INFOFRAME_TYPE_AVI:
		return hdmi_avi_infoframe_pack(&frame->avi, buf, size);
	case HDMI_INFOFRAME_TYPE_DRM:
		return hdmi_drm_infoframe_pack(&frame->drm, buf, size);
	case HDMI_INFOFRAME_TYPE_SPD:
		return hdmi_spd_infoframe_pack(&frame->spd, buf, size);
	case HDMI_INFOFRAME_TYPE_VENDOR:
		return hdmi_vendor_infoframe_pack(&frame->vendor.hdmi, buf,
		    size);
	default:
		return -EINVAL;
	}
}

static inline int
hdmi_infoframe_unpack(union hdmi_infoframe *frame, const void *buf,
    size_t size)
{
	struct hdmi_infoframe_header header;
	int ret;

	ret = hdmi_infoframe_header_unpack(&header, buf, size);
	if (ret)
		return ret;
	switch (header.type) {
	case HDMI_INFOFRAME_TYPE_AVI:
		return hdmi_avi_infoframe_unpack(&frame->avi, buf, size);
	case HDMI_INFOFRAME_TYPE_DRM:
		return hdmi_drm_infoframe_unpack(&frame->drm, buf, size);
	case HDMI_INFOFRAME_TYPE_SPD:
		return hdmi_spd_infoframe_unpack(&frame->spd, buf, size);
	case HDMI_INFOFRAME_TYPE_VENDOR:
		return hdmi_vendor_infoframe_unpack(&frame->vendor.hdmi, buf,
		    size);
	default:
		return -EINVAL;
	}
}

static inline void
hdmi_infoframe_log(const char *level, struct device *device,
    const union hdmi_infoframe *frame)
{
	/* XXX */
}

#endif	/* _LINUX_HDMI_H_ */
