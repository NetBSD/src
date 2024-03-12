/*	$NetBSD: libaudio.h,v 1.21.8.1 2024/03/12 10:04:22 martin Exp $	*/

/*
 * Copyright (c) 1999, 2009, 2013, 2015, 2019, 2024 Matthew R. Green
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * audio formats
 */
#define AUDIO_FORMAT_DEFAULT	-1
#define AUDIO_FORMAT_NONE	1
#define AUDIO_FORMAT_SUN	2
#define AUDIO_FORMAT_WAV	3

int	audio_format_from_str (char *);

/*
 * Audio encoding formats; this is a additional to those set
 * in sys/audioio.h, but with a large offset to avoid future
 * conflicts (additional ones are libaudio-software only.)
 *
 * This is to support floating-point WAV files.  These require
 * software conversion to a supported format.
 */
#define	AUDIO_ENCODING_LIBAUDIO_FLOAT32    1001	/* 32-bit IEEE FP. */
#define	AUDIO_ENCODING_LIBAUDIO_FLOAT64    1002	/* 64-bit IEEE FP. */

/*
 * We copy the Sun/NeXT on-disk audio header format and document what
 * we know of it here.
 *
 * The header size appears to be an offset to where the data really
 * begins, rather than defining the real length of the audio header.
 * The Sun/NeXT audio format seems to only use 24 bytes of data (with
 * an additional 8 bytes of nuls written, padding it to 32 bytes).
 *
 * If the size of the audio data is unknown (eg, reading from a pipe)
 * the Sun demo audio tools place AUDIO_UNKNOWN_SIZE in the
 * `data_size' member.
 *
 * For stereo data, the channels appear to be interleaved with the
 * left channel first.  For more channels, who knows?
 */

/*
 * This is the Sun/NeXT audio file magic value.  Note that it
 * is also `.snd' in ASCII.
 */
#define	AUDIO_FILE_MAGIC		((u_int32_t)0x2e736e64)
#define AUDIO_UNKNOWN_SIZE		((unsigned)(~0))

typedef struct {
	u_int32_t	magic;
	u_int32_t	hdr_size;	/* header size; in bytes */
	u_int32_t	data_size;	/* optional; in bytes */
	u_int32_t	encoding;	/* see below */
	u_int32_t	sample_rate;	/* per second */
	u_int32_t	channels;	/* number of interleaved channels */
} sun_audioheader;

#define Audio_filehdr sun_audioheader	/* SunOS compat(?) */

/*
 * these are the types of "encoding" for above.  taken from the
 * SunOS <multimedia/audio_filehdr.h>.
 */
#define	AUDIO_FILE_ENCODING_MULAW_8		1
#define	AUDIO_FILE_ENCODING_LINEAR_8		2
#define	AUDIO_FILE_ENCODING_LINEAR_16		3
#define	AUDIO_FILE_ENCODING_LINEAR_24		4
#define	AUDIO_FILE_ENCODING_LINEAR_32		5
#define	AUDIO_FILE_ENCODING_FLOAT		6
#define	AUDIO_FILE_ENCODING_DOUBLE		7
#define	AUDIO_FILE_ENCODING_ADPCM_G721		23
#define	AUDIO_FILE_ENCODING_ADPCM_G722		24
#define	AUDIO_FILE_ENCODING_ADPCM_G723_3	25
#define	AUDIO_FILE_ENCODING_ADPCM_G723_5	26
#define	AUDIO_FILE_ENCODING_ALAW_8		27

const char *audio_enc_from_val (int);
int	audio_enc_to_val (const char *);

int	audio_sun_to_encoding (int, u_int *, u_int *);
int	audio_encoding_to_sun (int, int, int *);

/*
 * RIFF WAVE files.  Sources: RFC 2361, and various Microsoft docs
 * https://learn.microsoft.com/en-us/windows/win32/xaudio2/resource-interchange-file-format--riff-
 * https://learn.microsoft.com/en-us/previous-versions/windows/hardware/design/dn653308(v=vs.85)
 * "Multimedia Programming Interface and Data Specifications 1.0" chapter 4
 */

/*
 * This is the WAV audio file magic value.  Note that it
 * is also `RIFF' and `WAVE' in ASCII.
 */
#define	WAVAUDIO_FILE_MAGIC_RIFF	((u_int32_t)0x52494646)
#define	WAVAUDIO_FILE_MAGIC_WAVE	((u_int32_t)0x57415645)
#define	WAVAUDIO_FILE_MAGIC_FMT		((u_int32_t)0x666d7420)
#define	WAVAUDIO_FILE_MAGIC_DATA	((u_int32_t)0x64617461)

/* From RFC 2361 */
#define WAVE_FORMAT_UNKNOWN		(0x0000)
#define WAVE_FORMAT_PCM			(0x0001)
#define WAVE_FORMAT_ADPCM		(0x0002)
#define WAVE_FORMAT_IEEE_FLOAT		(0x0003)
#define WAVE_FORMAT_VSELP		(0x0004)
#define WAVE_FORMAT_IBM_CVSD		(0x0005)
#define WAVE_FORMAT_ALAW		(0x0006)
#define WAVE_FORMAT_MULAW		(0x0007)
#define WAVE_FORMAT_OKI_ADPCM		(0x0010)
#define WAVE_FORMAT_IMA_ADPCM		(0x0011)
#define WAVE_FORMAT_MEDIASPACE_ADPCM	(0x0012)
#define WAVE_FORMAT_SIERRA_ADPCM	(0x0013)
#define WAVE_FORMAT_G723_ADPCM		(0x0014)
#define WAVE_FORMAT_DIGISTD		(0x0015)
#define WAVE_FORMAT_DIGIFIX		(0x0016)
#define WAVE_FORMAT_DIALOGIC_OKI_ADPCM	(0x0017)
#define WAVE_FORMAT_MEDIAVISION_ADPCM	(0x0018)
#define WAVE_FORMAT_CU_CODEC		(0x0019)
#define WAVE_FORMAT_YAMAHA_ADPCM	(0x0020)
#define WAVE_FORMAT_SONARC		(0x0021)
#define WAVE_FORMAT_DSPGROUP_TRUESPEECH	(0x0022)
#define WAVE_FORMAT_ECHOSC1		(0x0023)
#define WAVE_FORMAT_AUDIOFILE_AF36	(0x0024)
#define WAVE_FORMAT_APTX		(0x0025)
#define WAVE_FORMAT_AUDIOFILE_AF10	(0x0026)
#define WAVE_FORMAT_PROSODY_1612	(0x0027)
#define WAVE_FORMAT_LRC			(0x0028)
#define WAVE_FORMAT_DOLBY_AC2		(0x0030)
#define WAVE_FORMAT_GSM610		(0x0031)
#define WAVE_FORMAT_MSNAUDIO		(0x0032)
#define WAVE_FORMAT_ANTEX_ADPCME	(0x0033)
#define WAVE_FORMAT_CONTROL_RES_VQLPC	(0x0034)
#define WAVE_FORMAT_DIGIREAL		(0x0035)
#define WAVE_FORMAT_DIGIADPCM		(0x0036)
#define WAVE_FORMAT_CONTROL_RES_CR10	(0x0037)
#define WAVE_FORMAT_NMS_VBXADPCM	(0x0038)
#define WAVE_FORMAT_ROLAND_RDAC		(0x0039)
#define WAVE_FORMAT_ECHOSC3		(0x003a)
#define WAVE_FORMAT_ROCKWELL_ADPCM	(0x003b)
#define WAVE_FORMAT_ROCKWELL_DIGITALK	(0x003c)
#define WAVE_FORMAT_XEBEC		(0x003d)
#define WAVE_FORMAT_G721_ADPCM		(0x0040)
#define WAVE_FORMAT_G728_CELP		(0x0041)
#define WAVE_FORMAT_MSG723		(0x0042)
#define WAVE_FORMAT_MPEG		(0x0050)
#define WAVE_FORMAT_RT24		(0x0052)
#define WAVE_FORMAT_PAC			(0x0053)
#define WAVE_FORMAT_MPEGLAYER3		(0x0055)
#define WAVE_FORMAT_LUCENT_G723		(0x0059)
#define WAVE_FORMAT_CIRRUS		(0x0060)
#define WAVE_FORMAT_ESPCM		(0x0061)
#define WAVE_FORMAT_VOXWARE		(0x0062)
#define WAVE_FORMAT_CANOPUS_ATRAC	(0x0063)
#define WAVE_FORMAT_G726_ADPCM		(0x0064)
#define WAVE_FORMAT_G722_ADPCM		(0x0065)
#define WAVE_FORMAT_DSAT		(0x0066)
#define WAVE_FORMAT_DSAT_DISPLAY	(0x0067)
#define WAVE_FORMAT_VOXWARE_BYTE_ALIGNED (0x0069)
#define WAVE_FORMAT_VOXWARE_AC8		(0x0070)
#define WAVE_FORMAT_VOXWARE_AC10	(0x0071)
#define WAVE_FORMAT_VOXWARE_AC16	(0x0072)
#define WAVE_FORMAT_VOXWARE_AC20	(0x0073)
#define WAVE_FORMAT_VOXWARE_RT24	(0x0074)
#define WAVE_FORMAT_VOXWARE_RT29	(0x0075)
#define WAVE_FORMAT_VOXWARE_RT29HW	(0x0076)
#define WAVE_FORMAT_VOXWARE_VR12	(0x0077)
#define WAVE_FORMAT_VOXWARE_VR18	(0x0078)
#define WAVE_FORMAT_VOXWARE_TQ40	(0x0079)
#define WAVE_FORMAT_SOFTSOUND		(0x0080)
#define WAVE_FORMAT_VOXWARE_TQ60	(0x0081)
#define WAVE_FORMAT_MSRT24		(0x0082)
#define WAVE_FORMAT_G729A		(0x0083)
#define WAVE_FORMAT_MVI_MV12		(0x0084)
#define WAVE_FORMAT_DF_G726		(0x0085)
#define WAVE_FORMAT_DF_GSM610		(0x0086)
#define WAVE_FORMAT_ISIAUDIO		(0x0088)
#define WAVE_FORMAT_ONLIVE		(0x0089)
#define WAVE_FORMAT_SBC24		(0x0091)
#define WAVE_FORMAT_DOLBY_AC3_SPDIF	(0x0092)
#define WAVE_FORMAT_ZYXEL_ADPCM		(0x0097)
#define WAVE_FORMAT_PHILIPS_LPCBB	(0x0098)
#define WAVE_FORMAT_PACKED		(0x0099)
#define WAVE_FORMAT_RHETOREX_ADPCM	(0x0100)
#define WAVE_FORMAT_IRAT		(0x0101)
#define WAVE_FORMAT_VIVO_G723		(0x0111)
#define WAVE_FORMAT_VIVO_SIREN		(0x0112)
#define WAVE_FORMAT_DIGITAL_G723	(0x0123)
#define WAVE_FORMAT_CREATIVE_ADPCM	(0x0200)
#define WAVE_FORMAT_CREATIVE_FASTSPEECH8 (0x0202)
#define WAVE_FORMAT_CREATIVE_FASTSPEECH10 (0x0203)
#define WAVE_FORMAT_QUARTERDECK		(0x0220)
#define WAVE_FORMAT_FM_TOWNS_SND	(0x0300)
#define WAVE_FORMAT_BTV_DIGITAL		(0x0400)
#define WAVE_FORMAT_VME_VMPCM		(0x0680)
#define WAVE_FORMAT_OLIGSM		(0x1000)
#define WAVE_FORMAT_OLIADPCM		(0x1001)
#define WAVE_FORMAT_OLICELP		(0x1002)
#define WAVE_FORMAT_OLISBC		(0x1003)
#define WAVE_FORMAT_OLIOPR		(0x1004)
#define WAVE_FORMAT_LH_CODEC		(0x1100)
#define WAVE_FORMAT_NORRIS		(0x1400)
#define WAVE_FORMAT_ISIAUDIO2		(0x1401)
#define WAVE_FORMAT_SOUNDSPACE_MUSICOMPRESS (0x1500)
#define WAVE_FORMAT_DVM			(0x2000)

#define WAVE_FORMAT_EXTENSIBLE		(0xfffe)

const char *wav_enc_from_val (int);

typedef struct {
	char		name[4];
	u_int32_t	len;
} wav_audioheaderpart;

typedef struct {
	u_int16_t	tag;
	u_int16_t	channels;
	u_int32_t	sample_rate;
	u_int32_t	avg_bytes_per_sec;
	u_int16_t	alignment;
	u_int16_t	bits_per_sample;
} __packed wav_audioheaderfmt;

typedef struct {
	u_int16_t	len;
	u_int16_t	valid_bits;
	u_int32_t	speaker_pos_mask;
	u_int16_t	sub_tag;
	u_int8_t	guid[14];
} __packed wav_audiohdrextensible;

/* returns size of header, or -ve for failure */
ssize_t audio_wav_parse_hdr (void *, size_t, u_int *, u_int *, u_int *, u_int *, off_t *);

extern int verbose;

/*
 * audio routine error codes
 */
#define AUDIO_ENOENT		-1		/* no such audio format */
#define AUDIO_ESHORTHDR		-2		/* short header */
#define AUDIO_EWAVUNSUPP	-3		/* WAV: unsupported file */
#define AUDIO_EWAVBADPCM	-4		/* WAV: bad PCM bps */
#define AUDIO_EWAVNODATA	-5		/* WAV: missing data */
#define AUDIO_EINTERNAL		-6		/* internal error */

#define AUDIO_MAXERRNO		5

/* and something to get a string associated with this error */
const char *audio_errstring (int);

/*
 * generic routines?
 */
void	decode_int (const char *, int *);
void	decode_uint (const char *, unsigned *);
void	decode_time (const char *, struct timeval *);
void	decode_encoding (const char *, int *);

/*
 * Track info, for reading/writing sun/wav header.
 *
 * Note that write_header() may change the values of format,
 * encoding.
 */

struct track_info {
	int	outfd;
	char	*header_info;
	int	format;
	int	encoding;
	int	precision;
	int	qflag;
	off_t	total_size;
	int	sample_rate;
	int	channels;
};

typedef void (*write_conv_func) (u_char *, int);

void	write_header (struct track_info *);
write_conv_func write_get_conv_func(struct track_info *);

/* backends for the above */
int sun_prepare_header(struct track_info *ti, void **hdrp, size_t *lenp, int *leftp);
int wav_prepare_header(struct track_info *ti, void **hdrp, size_t *lenp, int *leftp);
write_conv_func sun_write_get_conv_func(struct track_info *ti);
write_conv_func wav_write_get_conv_func(struct track_info *ti);

extern char	audio_default_info[8];

/*
 * get/put 16/32 bits of big/little endian data
 */
#include <sys/types.h>
#include <machine/endian.h>
#include <machine/bswap.h>

#if BYTE_ORDER == BIG_ENDIAN

#define getle16(v)	bswap16(v)
#define getle32(v)	bswap32(v)
#define getbe16(v)	(v)
#define getbe32(v)	(v)

#define putle16(x,v)	(x) = bswap16(v)
#define putle32(x,v)	(x) = bswap32(v)
#define putbe16(x,v)	(x) = (v)
#define putbe32(x,v)	(x) = (v)

#else

#define getle16(v)	(v)
#define getle32(v)	(v)
#define getbe16(v)	bswap16(v)
#define getbe32(v)	bswap32(v)

#define putle16(x,v)	(x) = (v)
#define putle32(x,v)	(x) = (v)
#define putbe16(x,v)	(x) = bswap16(v)
#define putbe32(x,v)	(x) = bswap32(v)

#endif
