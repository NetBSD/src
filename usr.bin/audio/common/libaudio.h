/*	$NetBSD: libaudio.h,v 1.3.8.1 2000/06/23 16:30:12 minoura Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
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
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

char	*audio_enc_from_val __P((int));
int	audio_enc_to_val __P((const char *));

int	audio_get_sun_encoding __P((int, int *, int *));

/*
 * M$ WAV files, info gleamed from sox sources
 */

/*
 * This is the WAV audio file magic value.  Note that it
 * is also `RIFF' and `WAVE' in ASCII.
 */
#define	WAVAUDIO_FILE_MAGIC_RIFF	((u_int32_t)0x52494646)
#define	WAVAUDIO_FILE_MAGIC_WAVE	((u_int32_t)0x57415645)
#define	WAVAUDIO_FILE_MAGIC_FMT		((u_int32_t)0x666d7420)
#define	WAVAUDIO_FILE_MAGIC_DATA	((u_int32_t)0x64617461)

/* purloined from public Microsoft RIFF docs via sox */
#define WAVE_FORMAT_UNKNOWN		(0x0000)
#define WAVE_FORMAT_PCM			(0x0001)
#define WAVE_FORMAT_ADPCM		(0x0002)
#define WAVE_FORMAT_ALAW		(0x0006)
#define WAVE_FORMAT_MULAW		(0x0007)
#define WAVE_FORMAT_OKI_ADPCM		(0x0010)
#define WAVE_FORMAT_DIGISTD		(0x0015)
#define WAVE_FORMAT_DIGIFIX		(0x0016)
#define IBM_FORMAT_MULAW		(0x0101)
#define IBM_FORMAT_ALAW			(0x0102)
#define IBM_FORMAT_ADPCM		(0x0103)

typedef struct {
	char		name[4];
	u_int32_t	len;
} wav_audioheaderpart;

typedef struct {
	u_int16_t	tag;
	u_int16_t	channels;
	u_int32_t	sample_rate;
	u_int32_t	avg_bps;
	u_int16_t	alignment;
	u_int16_t	bits_per_sample;
} wav_audioheaderfmt __attribute__((__packed__));

/* returns size of header, or -1 */
size_t audio_parse_wav_hdr __P((void *, size_t, int *, int *, int *, int *));

/*
 * audio routine error codes
 */
#define AUDIO_ENOENT		-1		/* no such audio format */
#define AUDIO_ESHORTHDR		-2		/* short header */
#define AUDIO_EWAVUNSUPP	-3		/* WAV: unsupported file */
#define AUDIO_EWAVBADPCM	-4		/* WAV: bad PCM bps */
#define AUDIO_EWAVNODATA	-5		/* WAV: missing data */

#define AUDIO_MAXERRNO		5

/* and something to get a string associated with this error */
const char *audio_errstring __P((int));

/*
 * generic routines?
 */
void	decode_int __P((const char *, int *));
void	decode_time __P((const char *, struct timeval *));
void	decode_encoding __P((const char *, int *));

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

#else

#define getle16(v)	(v)
#define getle32(v)	(v)
#define getbe16(v)	bswap16(v)
#define getbe32(v)	bswap32(v)

#endif
