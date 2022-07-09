/*	$NetBSD: linux_hdmi.c,v 1.3 2022/07/09 18:11:36 riastradh Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_hdmi.c,v 1.3 2022/07/09 18:11:36 riastradh Exp $");

#include <sys/types.h>

#include <sys/device.h>
#include <sys/errno.h>
#include <sys/systm.h>

#include <lib/libkern/libkern.h>

#include <linux/hdmi.h>

/* Infoframe headers */

static void
hdmi_infoframe_header_init(struct hdmi_infoframe_header *header,
    enum hdmi_infoframe_type type, uint8_t vers, uint8_t length)
{

	header->type = type;
	header->version = vers;
	header->length = length;
}

static int
hdmi_infoframe_header_check(const struct hdmi_infoframe_header *header,
    enum hdmi_infoframe_type type, uint8_t vers, uint8_t length)
{

	if (header->type != type ||
	    header->version != vers ||
	    header->length != length)
		return -EINVAL;
	return 0;
}

static int
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

static uint8_t
hdmi_infoframe_checksum(const void *buf, size_t length)
{
	const uint8_t *p = buf;
	uint8_t checksum = 0;

	while (length--)
		checksum += *p++;

	return 256 - checksum;
}

static int
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

static void
hdmi_infoframe_set_checksum(void *buf, size_t length)
{
	uint8_t *p = buf;

	p[3] = hdmi_infoframe_checksum(buf, length);
}

/* Audio infoframes */

int
hdmi_audio_infoframe_init(struct hdmi_audio_infoframe *frame)
{
	static const struct hdmi_audio_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_AUDIO,
	    1, HDMI_AUDIO_INFOFRAME_SIZE);

	return 0;
}

ssize_t
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

static int
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

/* AVI infoframes */

int
hdmi_avi_infoframe_init(struct hdmi_avi_infoframe *frame)
{
	static const struct hdmi_avi_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_AVI, 2,
	    HDMI_AVI_INFOFRAME_SIZE);

	return 0;
}

int
hdmi_avi_infoframe_check(const struct hdmi_avi_infoframe *frame)
{
	int ret;

	ret = hdmi_infoframe_header_check(&frame->header,
	    HDMI_INFOFRAME_TYPE_AVI, 2, HDMI_AVI_INFOFRAME_SIZE);
	if (ret)
		return ret;

	return 0;
}

ssize_t
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

static int
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

/* DRM infoframes */

int
hdmi_drm_infoframe_init(struct hdmi_drm_infoframe *frame)
{
	static const struct hdmi_drm_infoframe zero_frame;

	*frame = zero_frame;

	hdmi_infoframe_header_init(&frame->header, HDMI_INFOFRAME_TYPE_DRM,
	    1, HDMI_DRM_INFOFRAME_SIZE);

	return 0;
}

int
hdmi_drm_infoframe_check(const struct hdmi_drm_infoframe *frame)
{
	int ret;

	ret = hdmi_infoframe_header_check(&frame->header,
	    HDMI_INFOFRAME_TYPE_DRM, 1, HDMI_DRM_INFOFRAME_SIZE);
	if (ret)
		return ret;

	return 0;
}

__strong_alias(linux_hdmi_drm_infoframe_pack_only,linux_hdmi_drm_infoframe_pack) /* XXX */

int
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

static int
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

/* SPD infoframes */

int
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

int
hdmi_spd_infoframe_check(const struct hdmi_spd_infoframe *frame)
{
	int ret;

	ret = hdmi_infoframe_header_check(&frame->header,
	    HDMI_INFOFRAME_TYPE_SPD, 1, HDMI_SPD_INFOFRAME_SIZE);
	if (ret)
		return ret;

	return 0;
}

ssize_t
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

static int
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

/* Vendor infoframes */

int
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

static size_t
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

int
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

int
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

static int
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

/* union infoframe */

__strong_alias(linux_hdmi_infoframe_pack_only,linux_hdmi_infoframe_pack) /* XXX */

ssize_t
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

int
hdmi_infoframe_unpack(union hdmi_infoframe *frame, const void *buf,
    size_t size)
{
	struct hdmi_infoframe_header header;
	int ret;

	ret = hdmi_infoframe_header_unpack(&header, buf, size);
	if (ret)
		return ret;
	switch (header.type) {
	case HDMI_INFOFRAME_TYPE_VENDOR:
		return hdmi_vendor_infoframe_unpack(&frame->vendor.hdmi, buf,
		    size);
	case HDMI_INFOFRAME_TYPE_AVI:
		return hdmi_avi_infoframe_unpack(&frame->avi, buf, size);
	case HDMI_INFOFRAME_TYPE_SPD:
		return hdmi_spd_infoframe_unpack(&frame->spd, buf, size);
	case HDMI_INFOFRAME_TYPE_AUDIO:
		return hdmi_audio_infoframe_unpack(&frame->audio, buf, size);
	case HDMI_INFOFRAME_TYPE_DRM:
		return hdmi_drm_infoframe_unpack(&frame->drm, buf, size);
	default:
		return -EINVAL;
	}
}

void
hdmi_infoframe_log(const char *level, struct device *device,
    const union hdmi_infoframe *frame)
{

	hexdump(printf, device_xname(device), frame, sizeof(*frame));
}
