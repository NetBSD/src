/*	$NetBSD: auconv.h,v 1.11 2004/12/06 13:28:34 wiz Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _SYS_DEV_AUCONV_H_
#define _SYS_DEV_AUCONV_H_

/* Convert between signed and unsigned. */
extern void change_sign8(void *, u_char *, int);
extern void change_sign16_le(void *, u_char *, int);
extern void change_sign16_be(void *, u_char *, int);
/* Convert between little and big endian. */
extern void swap_bytes(void *, u_char *, int);
extern void swap_bytes_change_sign16_le(void *, u_char *, int);
extern void swap_bytes_change_sign16_be(void *, u_char *, int);
extern void change_sign16_swap_bytes_le(void *, u_char *, int);
extern void change_sign16_swap_bytes_be(void *, u_char *, int);
/* Byte expansion/contraction */
extern void linear8_to_linear16_le(void *, u_char *, int);
extern void linear8_to_linear16_be(void *, u_char *, int);
extern void linear16_to_linear8_le(void *, u_char *, int);
extern void linear16_to_linear8_be(void *, u_char *, int);
/* Byte expansion/contraction with sign change */
extern void ulinear8_to_slinear16_le(void *, u_char *, int);
extern void ulinear8_to_slinear16_be(void *, u_char *, int);
extern void slinear16_to_ulinear8_le(void *, u_char *, int);
extern void slinear16_to_ulinear8_be(void *, u_char *, int);

struct audio_format {
	/**
	 * Device-dependent audio drivers may use this field freely.
	 */
	void *driver_data;

	/**
	 * combination of AUMODE_PLAY and AUMODE_RECORD
	 */
	int32_t mode;

	/**
	 * Encoding type.  AUDIO_ENCODING_*.
	 * Don't use AUDIO_ENCODING_SLINEAR/ULINEAR/LINEAR/LINEAR8
	 */
	u_int encoding;

	/**
	 * The size of valid bits in one sample.
	 * It must be <= subframe_size.
	 */
	u_int precision;

	/**
	 * The bit size of one sample.
	 * It must be >= precision, and is usualy a multiple of 8.
	 */
	u_int subframe_size;

	/**
	 * The number of channels.  >= 1
	 */
	u_int channels;

	u_int channel_mask;
#define	AUFMT_UNKNOWN_POSITION		0U
#define	AUFMT_FRONT_LEFT		0x00001U /* USB audio compatible */
#define	AUFMT_FRONT_RIGHT		0x00002U /* USB audio compatible */
#define	AUFMT_FRONT_CENTER		0x00004U /* USB audio compatible */
#define	AUFMT_LOW_FREQUENCY		0x00008U /* USB audio compatible */
#define	AUFMT_BACK_LEFT			0x00010U /* USB audio compatible */
#define	AUFMT_BACK_RIGHT		0x00020U /* USB audio compatible */
#define	AUFMT_FRONT_LEFT_OF_CENTER	0x00040U /* USB audio compatible */
#define	AUFMT_FRONT_RIGHT_OF_CENTER	0x00080U /* USB audio compatible */
#define	AUFMT_FRONT_BACK_CENTER		0x00100U /* USB audio compatible */
#define	AUFMT_FRONT_SIDE_LEFT		0x00200U /* USB audio compatible */
#define	AUFMT_FRONT_SIDE_RIGHT		0x00400U /* USB audio compatible */
#define	AUFMT_FRONT_TOP_CENTER		0x00800U /* USB audio compatible */
#define	AUFMT_FRONT_TOP_FRONT_LEFT	0x01000U
#define	AUFMT_FRONT_TOP_FRONT_CENTER	0x02000U
#define	AUFMT_FRONT_TOP_FRONT_RIGHT	0x04000U
#define	AUFMT_FRONT_TOP_BACK_LEFT	0x08000U
#define	AUFMT_FRONT_TOP_BACK_CENTER	0x10000U
#define	AUFMT_FRONT_TOP_BACK_RIGHT	0x20000U

#define	AUFMT_MONAURAL		AUFMT_FRONT_CENTER
#define	AUFMT_STEREO		(AUFMT_FRONT_LEFT | AUFMT_FRONT_RIGHT)
#define	AUFMT_SURROUND4		(AUFMT_STEREO | AUFMT_BACK_LEFT \
				| AUFMT_BACK_RIGHT)
#define	AUFMT_DOLBY_5_1		(AUFMT_SURROUND4 | AUFMT_FRONT_CENTER \
				| AUFMT_LOW_FREQUENCY)

	/**
	 * 0: frequency[0] is lower limit, and frequency[1] is higher limit.
	 * 1-16: frequency[0] to frequency[frequency_type-1] are valid.
	 */
	u_int frequency_type;

#define	AUFMT_MAX_FREQUENCIES	16
	/**
	 * sampling rates
	 */
	u_long frequency[AUFMT_MAX_FREQUENCIES];
};

#define	AUFMT_INVALIDATE(fmt)	(fmt)->mode |= 0x80000000
#define	AUFMT_VALIDATE(fmt)	(fmt)->mode &= 0x7fffffff
#define	AUFMT_IS_VALID(fmt)	(((fmt)->mode & 0x80000000) == 0)

struct audio_encoding_set;
struct audio_params;
extern int auconv_set_converter(const struct audio_format *, int,
				int, struct audio_params *, int);
extern int auconv_create_encodings(const struct audio_format *, int,
				   struct audio_encoding_set **);
extern int auconv_delete_encodings(struct audio_encoding_set *);
extern int auconv_query_encoding(const struct audio_encoding_set *,
				 audio_encoding_t *);

#endif /* !_SYS_DEV_AUCONV_H_ */
