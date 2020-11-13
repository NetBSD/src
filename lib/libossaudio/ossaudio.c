/*	$NetBSD: ossaudio.c,v 1.64 2020/11/13 09:02:39 nia Exp $	*/

/*-
 * Copyright (c) 1997, 2020 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: ossaudio.c,v 1.64 2020/11/13 09:02:39 nia Exp $");

/*
 * This is an Open Sound System compatibility layer, which provides
 * fairly complete ioctl emulation for OSSv3 and some of OSSv4.
 *
 * The canonical OSS specification is available at
 * http://manuals.opensound.com/developer/
 * 
 * This file is similar to sys/compat/ossaudio.c with additional OSSv4
 * compatibility.
 */

#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/audioio.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <limits.h>
#include <stdarg.h>
#include <stdbool.h>

#include "soundcard.h"
#undef ioctl

#define GET_DEV(com) ((com) & 0xff)

#define TO_OSSVOL(x)	(((x) * 100 + 127) / 255)
#define FROM_OSSVOL(x)	((((x) > 100 ? 100 : (x)) * 255 + 50) / 100)

#define GETPRINFO(info, name)	\
	(((info)->mode == AUMODE_RECORD) \
	    ? (info)->record.name : (info)->play.name)

static struct audiodevinfo *getdevinfo(int);

static int getaudiocount(void);
static int getmixercount(void);
static int getmixercontrolcount(int);

static int getcaps(int, int *);

static int getvol(u_int, u_char);
static void setvol(int, int, bool);

static void setchannels(int, int, int);
static void setblocksize(int, struct audio_info *);

static int audio_ioctl(int, unsigned long, void *);
static int mixer_oss3_ioctl(int, unsigned long, void *);
static int mixer_oss4_ioctl(int, unsigned long, void *);
static int global_oss4_ioctl(int, unsigned long, void *);
static int opaque_to_enum(struct audiodevinfo *, audio_mixer_name_t *, int);
static int enum_to_ord(struct audiodevinfo *, int);
static int enum_to_mask(struct audiodevinfo *, int);

#define INTARG (*(int*)argp)

int
_oss_ioctl(int fd, unsigned long com, ...)
{
	va_list ap;
	void *argp;

	va_start(ap, com);
	argp = va_arg(ap, void *);
	va_end(ap);

	if (IOCGROUP(com) == 'P')
		return audio_ioctl(fd, com, argp);
	else if (IOCGROUP(com) == 'M')
		return mixer_oss3_ioctl(fd, com, argp);
	else if (IOCGROUP(com) == 'X')
		return mixer_oss4_ioctl(fd, com, argp);
	else if (IOCGROUP(com) == 'Y')
		return global_oss4_ioctl(fd, com, argp);
	else
		return ioctl(fd, com, argp);
}

static int
audio_ioctl(int fd, unsigned long com, void *argp)
{

	struct audio_info tmpinfo, hwfmt;
	struct audio_offset tmpoffs;
	struct audio_buf_info bufinfo;
	struct audio_errinfo *tmperrinfo;
	struct count_info cntinfo;
	struct audio_encoding tmpenc;
	u_int u;
	u_int encoding;
	u_int precision;
	int perrors, rerrors;
	static int totalperrors = 0;
	static int totalrerrors = 0;
	oss_count_t osscount;
	int idat;
	int retval;

	idat = 0;

	switch (com) {
	case SNDCTL_DSP_RESET:
		retval = ioctl(fd, AUDIO_FLUSH, 0);
		if (retval < 0)
			return retval;
		break;
	case SNDCTL_DSP_SYNC:
		retval = ioctl(fd, AUDIO_DRAIN, 0);
		if (retval < 0)
			return retval;
		break;
	case SNDCTL_DSP_GETERROR:
		tmperrinfo = (struct audio_errinfo *)argp;
		if (tmperrinfo == NULL) {
			errno = EINVAL;
			return -1;
		}
		memset(tmperrinfo, 0, sizeof(struct audio_errinfo));
		if ((retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo)) < 0)
			return retval;
		/*
		 * OSS requires that we return counters that are relative to
		 * the last call. We must maintain state here...
		 */
		if (ioctl(fd, AUDIO_PERROR, &perrors) != -1) {
			perrors /= ((tmpinfo.play.precision / NBBY) *
			    tmpinfo.play.channels);
			tmperrinfo->play_underruns =
			    (perrors / tmpinfo.blocksize) - totalperrors;
			totalperrors += tmperrinfo->play_underruns;
		}
		if (ioctl(fd, AUDIO_RERROR, &rerrors) != -1) {
			rerrors /= ((tmpinfo.record.precision / NBBY) *
			    tmpinfo.record.channels);
			tmperrinfo->rec_overruns =
			    (rerrors / tmpinfo.blocksize) - totalrerrors;
			totalrerrors += tmperrinfo->rec_overruns;
		}
		break;
	case SNDCTL_DSP_COOKEDMODE:
		/*
		 * NetBSD is always running in "cooked mode" - the kernel
		 * always performs format conversions.
		 */
		INTARG = 1;
		break;
	case SNDCTL_DSP_POST:
		/* This call is merely advisory, and may be a nop. */
		break;
	case SNDCTL_DSP_SPEED:
		AUDIO_INITINFO(&tmpinfo);
		/*
		 * In Solaris, 0 is used a special value to query the
		 * current rate. This seems useful to support.
		 */
		if (INTARG == 0) {
			retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
			if (retval < 0)
				return retval;
			retval = ioctl(fd, AUDIO_GETFORMAT, &hwfmt);
			if (retval < 0)
				return retval;
			INTARG = (tmpinfo.mode == AUMODE_RECORD) ?
			    hwfmt.record.sample_rate :
			    hwfmt.play.sample_rate;
		}
		/*
		 * Conform to kernel limits.
		 * NetBSD will reject unsupported sample rates, but OSS
		 * applications need to be able to negotiate a supported one.
		 */
		if (INTARG < 1000)
			INTARG = 1000;
		if (INTARG > 192000)
			INTARG = 192000;
		tmpinfo.play.sample_rate =
		tmpinfo.record.sample_rate = INTARG;
		retval = ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		/* FALLTHRU */
	case SOUND_PCM_READ_RATE:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = GETPRINFO(&tmpinfo, sample_rate);
		break;
	case SNDCTL_DSP_STEREO:
		AUDIO_INITINFO(&tmpinfo);
		tmpinfo.play.channels =
		tmpinfo.record.channels = INTARG ? 2 : 1;
		(void) ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = GETPRINFO(&tmpinfo, channels) - 1;
		break;
	case SNDCTL_DSP_GETBLKSIZE:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		setblocksize(fd, &tmpinfo);
		INTARG = tmpinfo.blocksize;
		break;
	case SNDCTL_DSP_SETFMT:
		AUDIO_INITINFO(&tmpinfo);
		switch (INTARG) {
		case AFMT_MU_LAW:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULAW;
			break;
		case AFMT_A_LAW:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ALAW;
			break;
		case AFMT_U8:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULINEAR;
			break;
		case AFMT_S8:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 8;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR;
			break;
		case AFMT_S16_LE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
			break;
		case AFMT_S16_BE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR_BE;
			break;
		case AFMT_U16_LE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULINEAR_LE;
			break;
		case AFMT_U16_BE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_ULINEAR_BE;
			break;
		/*
		 * XXX: When the kernel supports 24-bit LPCM by default,
		 * the 24-bit formats should be handled properly instead
		 * of falling back to 32 bits.
		 */
		case AFMT_S24_LE:
		case AFMT_S32_LE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 32;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR_LE;
			break;
		case AFMT_S24_BE:
		case AFMT_S32_BE:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 32;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_SLINEAR_BE;
			break;
		case AFMT_AC3:
			tmpinfo.play.precision =
			tmpinfo.record.precision = 16;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding = AUDIO_ENCODING_AC3;
			break;
		default:
			/*
			 * OSSv4 specifies that if an invalid format is chosen
			 * by an application then a sensible format supported
			 * by the hardware is returned.
			 *
			 * In this case, we pick the current hardware format.
			 */
			retval = ioctl(fd, AUDIO_GETFORMAT, &hwfmt);
			if (retval < 0)
				return retval;
			retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
			if (retval < 0)
				return retval;
			tmpinfo.play.encoding =
			tmpinfo.record.encoding =
			    (tmpinfo.mode == AUMODE_RECORD) ?
			    hwfmt.record.encoding : hwfmt.play.encoding;
			tmpinfo.play.precision =
			tmpinfo.record.precision =
			    (tmpinfo.mode == AUMODE_RECORD) ?
			    hwfmt.record.precision : hwfmt.play.precision ;
			break;
		}
		/*
		 * In the post-kernel-mixer world, assume that any error means
		 * it's fatal rather than an unsupported format being selected.
		 */
		retval = ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		/* FALLTHRU */
	case SOUND_PCM_READ_BITS:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		encoding = GETPRINFO(&tmpinfo, encoding);
		precision = GETPRINFO(&tmpinfo, precision);
		switch (encoding) {
		case AUDIO_ENCODING_ULAW:
			idat = AFMT_MU_LAW;
			break;
		case AUDIO_ENCODING_ALAW:
			idat = AFMT_A_LAW;
			break;
		case AUDIO_ENCODING_SLINEAR_LE:
			if (precision == 32)
				idat = AFMT_S32_LE;
			else if (precision == 24)
				idat = AFMT_S24_LE;
			else if (precision == 16)
				idat = AFMT_S16_LE;
			else
				idat = AFMT_S8;
			break;
		case AUDIO_ENCODING_SLINEAR_BE:
			if (precision == 32)
				idat = AFMT_S32_BE;
			else if (precision == 24)
				idat = AFMT_S24_BE;
			else if (precision == 16)
				idat = AFMT_S16_BE;
			else
				idat = AFMT_S8;
			break;
		case AUDIO_ENCODING_ULINEAR_LE:
			if (precision == 16)
				idat = AFMT_U16_LE;
			else
				idat = AFMT_U8;
			break;
		case AUDIO_ENCODING_ULINEAR_BE:
			if (precision == 16)
				idat = AFMT_U16_BE;
			else
				idat = AFMT_U8;
			break;
		case AUDIO_ENCODING_ADPCM:
			idat = AFMT_IMA_ADPCM;
			break;
		case AUDIO_ENCODING_AC3:
			idat = AFMT_AC3;
			break;
		}
		INTARG = idat;
		break;
	case SNDCTL_DSP_CHANNELS:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		setchannels(fd, tmpinfo.mode, INTARG);
		/* FALLTHRU */
	case SOUND_PCM_READ_CHANNELS:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = GETPRINFO(&tmpinfo, channels);
		break;
	case SOUND_PCM_WRITE_FILTER:
	case SOUND_PCM_READ_FILTER:
		errno = EINVAL;
		return -1; /* XXX unimplemented */
	case SNDCTL_DSP_SUBDIVIDE:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		setblocksize(fd, &tmpinfo);
		idat = INTARG;
		if (idat == 0)
			idat = tmpinfo.play.buffer_size / tmpinfo.blocksize;
		idat = (tmpinfo.play.buffer_size / idat) & -4;
		AUDIO_INITINFO(&tmpinfo);
		tmpinfo.blocksize = idat;
		retval = ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = tmpinfo.play.buffer_size / tmpinfo.blocksize;
		break;
	case SNDCTL_DSP_SETFRAGMENT:
		AUDIO_INITINFO(&tmpinfo);
		idat = INTARG;
		if ((idat & 0xffff) < 4 || (idat & 0xffff) > 17) {
			errno = EINVAL;
			return -1;
		}
		tmpinfo.blocksize = 1 << (idat & 0xffff);
		tmpinfo.hiwat = ((unsigned)idat >> 16) & 0x7fff;
		if (tmpinfo.hiwat == 0)	/* 0 means set to max */
			tmpinfo.hiwat = 65536;
		(void) ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		u = tmpinfo.blocksize;
		for(idat = 0; u > 1; idat++, u >>= 1)
			;
		idat |= (tmpinfo.hiwat & 0x7fff) << 16;
		INTARG = idat;
		break;
	case SNDCTL_DSP_GETFMTS:
		for(idat = 0, tmpenc.index = 0;
		    ioctl(fd, AUDIO_GETENC, &tmpenc) == 0;
		    tmpenc.index++) {
			switch(tmpenc.encoding) {
			case AUDIO_ENCODING_ULAW:
				idat |= AFMT_MU_LAW;
				break;
			case AUDIO_ENCODING_ALAW:
				idat |= AFMT_A_LAW;
				break;
			case AUDIO_ENCODING_SLINEAR:
				idat |= AFMT_S8;
				break;
			case AUDIO_ENCODING_SLINEAR_LE:
				if (tmpenc.precision == 32)
					idat |= AFMT_S32_LE;
				else if (tmpenc.precision == 24)
					idat |= AFMT_S24_LE;
				else if (tmpenc.precision == 16)
					idat |= AFMT_S16_LE;
				else
					idat |= AFMT_S8;
				break;
			case AUDIO_ENCODING_SLINEAR_BE:
				if (tmpenc.precision == 32)
					idat |= AFMT_S32_BE;
				else if (tmpenc.precision == 24)
					idat |= AFMT_S24_BE;
				else if (tmpenc.precision == 16)
					idat |= AFMT_S16_BE;
				else
					idat |= AFMT_S8;
				break;
			case AUDIO_ENCODING_ULINEAR:
				idat |= AFMT_U8;
				break;
			case AUDIO_ENCODING_ULINEAR_LE:
				if (tmpenc.precision == 16)
					idat |= AFMT_U16_LE;
				else
					idat |= AFMT_U8;
				break;
			case AUDIO_ENCODING_ULINEAR_BE:
				if (tmpenc.precision == 16)
					idat |= AFMT_U16_BE;
				else
					idat |= AFMT_U8;
				break;
			case AUDIO_ENCODING_ADPCM:
				idat |= AFMT_IMA_ADPCM;
				break;
			case AUDIO_ENCODING_AC3:
				idat |= AFMT_AC3;
				break;
			default:
				break;
			}
		}
		INTARG = idat;
		break;
	case SNDCTL_DSP_GETOSPACE:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		setblocksize(fd, &tmpinfo);
		bufinfo.fragsize = tmpinfo.blocksize;
		bufinfo.fragments = tmpinfo.hiwat - (tmpinfo.play.seek
		    + tmpinfo.blocksize - 1) / tmpinfo.blocksize;
		bufinfo.fragstotal = tmpinfo.hiwat;
		bufinfo.bytes = tmpinfo.hiwat * tmpinfo.blocksize
		    - tmpinfo.play.seek;
		*(struct audio_buf_info *)argp = bufinfo;
		break;
	case SNDCTL_DSP_GETISPACE:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		setblocksize(fd, &tmpinfo);
		bufinfo.fragsize = tmpinfo.blocksize;
		bufinfo.fragments = tmpinfo.record.seek / tmpinfo.blocksize;
		bufinfo.fragstotal =
		    tmpinfo.record.buffer_size / tmpinfo.blocksize;
		bufinfo.bytes = tmpinfo.record.seek;
		*(struct audio_buf_info *)argp = bufinfo;
		break;
	case SNDCTL_DSP_NONBLOCK:
		idat = 1;
		retval = ioctl(fd, FIONBIO, &idat);
		if (retval < 0)
			return retval;
		break;
	case SNDCTL_DSP_GETCAPS:
		retval = getcaps(fd, (int *)argp);
		if (retval < 0)
			return retval;
		break;
	case SNDCTL_DSP_SETTRIGGER:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		AUDIO_INITINFO(&tmpinfo);
		if (tmpinfo.mode & AUMODE_PLAY)
			tmpinfo.play.pause = (INTARG & PCM_ENABLE_OUTPUT) == 0;
		if (tmpinfo.mode & AUMODE_RECORD)
			tmpinfo.record.pause = (INTARG & PCM_ENABLE_INPUT) == 0;
		(void)ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		/* FALLTHRU */
	case SNDCTL_DSP_GETTRIGGER:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		idat = 0;
		if ((tmpinfo.mode & AUMODE_PLAY) && !tmpinfo.play.pause)
			idat |= PCM_ENABLE_OUTPUT;
		if ((tmpinfo.mode & AUMODE_RECORD) && !tmpinfo.record.pause)
			idat |= PCM_ENABLE_INPUT;
		INTARG = idat;
		break;
	case SNDCTL_DSP_GETIPTR:
		retval = ioctl(fd, AUDIO_GETIOFFS, &tmpoffs);
		if (retval < 0)
			return retval;
		cntinfo.bytes = tmpoffs.samples;
		cntinfo.blocks = tmpoffs.deltablks;
		cntinfo.ptr = tmpoffs.offset;
		*(struct count_info *)argp = cntinfo;
		break;
	case SNDCTL_DSP_CURRENT_IPTR:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		/* XXX: 'samples' may wrap */
		memset(osscount.filler, 0, sizeof(osscount.filler));
		osscount.samples = tmpinfo.record.samples /
		    ((tmpinfo.record.precision / NBBY) *
			tmpinfo.record.channels);
		osscount.fifo_samples = tmpinfo.record.seek /
		    ((tmpinfo.record.precision / NBBY) *
			tmpinfo.record.channels);
		*(oss_count_t *)argp = osscount;
		break;
	case SNDCTL_DSP_GETOPTR:
		retval = ioctl(fd, AUDIO_GETOOFFS, &tmpoffs);
		if (retval < 0)
			return retval;
		cntinfo.bytes = tmpoffs.samples;
		cntinfo.blocks = tmpoffs.deltablks;
		cntinfo.ptr = tmpoffs.offset;
		*(struct count_info *)argp = cntinfo;
		break;
	case SNDCTL_DSP_CURRENT_OPTR:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		/* XXX: 'samples' may wrap */
		memset(osscount.filler, 0, sizeof(osscount.filler));
		osscount.samples = tmpinfo.play.samples /
		    ((tmpinfo.play.precision / NBBY) *
			tmpinfo.play.channels);
		osscount.fifo_samples = tmpinfo.play.seek /
		    ((tmpinfo.play.precision / NBBY) *
			tmpinfo.play.channels);
		*(oss_count_t *)argp = osscount;
		break;
	case SNDCTL_DSP_SETPLAYVOL:
		setvol(fd, INTARG, false);
		/* FALLTHRU */
	case SNDCTL_DSP_GETPLAYVOL:
		retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = getvol(tmpinfo.play.gain, tmpinfo.play.balance);
		break;
	case SNDCTL_DSP_SETRECVOL:
		setvol(fd, INTARG, true);
		/* FALLTHRU */
	case SNDCTL_DSP_GETRECVOL:
		retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = getvol(tmpinfo.record.gain, tmpinfo.record.balance);
		break;
	case SNDCTL_DSP_SKIP:
	case SNDCTL_DSP_SILENCE: 
		errno = EINVAL;
		return -1;
	case SNDCTL_DSP_SETDUPLEX:
		idat = 1;
		retval = ioctl(fd, AUDIO_SETFD, &idat);
		if (retval < 0)
			return retval;
		break;
	case SNDCTL_DSP_GETODELAY:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		idat = tmpinfo.play.seek + tmpinfo.blocksize / 2;
		INTARG = idat;
		break;
	case SNDCTL_DSP_PROFILE:
		/* This gives just a hint to the driver,
		 * implementing it as a NOP is ok
		 */     
		break;
	case SNDCTL_DSP_MAPINBUF:
	case SNDCTL_DSP_MAPOUTBUF:
	case SNDCTL_DSP_SETSYNCRO:
		errno = EINVAL;
		return -1; /* XXX unimplemented */
	default:
		errno = EINVAL;
		return -1;
	}

	return 0;
}


/* If the NetBSD mixer device should have more than NETBSD_MAXDEVS devices
 * some will not be available to OSS applications */
#define NETBSD_MAXDEVS 64
struct audiodevinfo {
	int done;
	dev_t dev;
	int16_t devmap[SOUND_MIXER_NRDEVICES],
	        rdevmap[NETBSD_MAXDEVS];
	char names[NETBSD_MAXDEVS][MAX_AUDIO_DEV_LEN];
	int enum2opaque[NETBSD_MAXDEVS];
        u_long devmask, recmask, stereomask;
	u_long caps;
	int source;
};

static int
opaque_to_enum(struct audiodevinfo *di, audio_mixer_name_t *label, int opq)
{
	int i, o;

	for (i = 0; i < NETBSD_MAXDEVS; i++) {
		o = di->enum2opaque[i];
		if (o == opq)
			break;
		if (o == -1 && label != NULL &&
		    !strncmp(di->names[i], label->name, sizeof di->names[i])) {
			di->enum2opaque[i] = opq;
			break;
		}
	}
	if (i >= NETBSD_MAXDEVS)
		i = -1;
	/*printf("opq_to_enum %s %d -> %d\n", label->name, opq, i);*/
	return (i);
}

static int
enum_to_ord(struct audiodevinfo *di, int enm)
{
	if (enm >= NETBSD_MAXDEVS)
		return (-1);

	/*printf("enum_to_ord %d -> %d\n", enm, di->enum2opaque[enm]);*/
	return (di->enum2opaque[enm]);
}

static int
enum_to_mask(struct audiodevinfo *di, int enm)
{
	int m;
	if (enm >= NETBSD_MAXDEVS)
		return (0);

	m = di->enum2opaque[enm];
	if (m == -1)
		m = 0;
	/*printf("enum_to_mask %d -> %d\n", enm, di->enum2opaque[enm]);*/
	return (m);
}

/*
 * Collect the audio device information to allow faster
 * emulation of the OSSv3 mixer ioctls.  Cache the information
 * to eliminate the overhead of repeating all the ioctls needed
 * to collect the information.
 */
static struct audiodevinfo *
getdevinfo(int fd)
{
	mixer_devinfo_t mi;
	int i, j, e;
	static struct {
		const char *name;
		int code;
	} *dp, devs[] = {
		{ AudioNmicrophone,	SOUND_MIXER_MIC },
		{ AudioNline,		SOUND_MIXER_LINE },
		{ AudioNcd,		SOUND_MIXER_CD },
		{ AudioNdac,		SOUND_MIXER_PCM },
		{ AudioNaux,		SOUND_MIXER_LINE1 },
		{ AudioNrecord,		SOUND_MIXER_IMIX },
		{ AudioNmaster,		SOUND_MIXER_VOLUME },
		{ AudioNtreble,		SOUND_MIXER_TREBLE },
		{ AudioNbass,		SOUND_MIXER_BASS },
		{ AudioNspeaker,	SOUND_MIXER_SPEAKER },
/*		{ AudioNheadphone,	?? },*/
		{ AudioNoutput,		SOUND_MIXER_OGAIN },
		{ AudioNinput,		SOUND_MIXER_IGAIN },
/*		{ AudioNmaster,		SOUND_MIXER_SPEAKER },*/
/*		{ AudioNstereo,		?? },*/
/*		{ AudioNmono,		?? },*/
		{ AudioNfmsynth,	SOUND_MIXER_SYNTH },
/*		{ AudioNwave,		SOUND_MIXER_PCM },*/
		{ AudioNmidi,		SOUND_MIXER_SYNTH },
/*		{ AudioNmixerout,	?? },*/
		{ 0, -1 }
	};
	static struct audiodevinfo devcache = { .done = 0 };
	struct audiodevinfo *di = &devcache;
	struct stat sb;
	size_t mlen, dlen;

	/* Figure out what device it is so we can check if the
	 * cached data is valid.
	 */
	if (fstat(fd, &sb) < 0)
		return 0;
	if (di->done && di->dev == sb.st_dev)
		return di;

	di->done = 1;
	di->dev = sb.st_dev;
	di->devmask = 0;
	di->recmask = 0;
	di->stereomask = 0;
	di->source = ~0;
	di->caps = 0;
	for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
		di->devmap[i] = -1;
	for(i = 0; i < NETBSD_MAXDEVS; i++) {
		di->rdevmap[i] = -1;
		di->names[i][0] = '\0';
		di->enum2opaque[i] = -1;
	}
	for(i = 0; i < NETBSD_MAXDEVS; i++) {
		mi.index = i;
		if (ioctl(fd, AUDIO_MIXER_DEVINFO, &mi) < 0)
			break;
		switch(mi.type) {
		case AUDIO_MIXER_VALUE:
			for(dp = devs; dp->name; dp++) {
				if (strcmp(dp->name, mi.label.name) == 0)
					break;
				dlen = strlen(dp->name);
				mlen = strlen(mi.label.name);
				if (dlen < mlen
				    && mi.label.name[mlen-dlen-1] == '.'
				    && strcmp(dp->name,
				    mi.label.name + mlen - dlen) == 0)
					break;
			}
			if (dp->code >= 0) {
				di->devmap[dp->code] = i;
				di->rdevmap[i] = dp->code;
				di->devmask |= 1 << dp->code;
				if (mi.un.v.num_channels == 2)
					di->stereomask |= 1 << dp->code;
				strlcpy(di->names[i], mi.label.name,
					sizeof di->names[i]);
			}
			break;
		}
	}
	for(i = 0; i < NETBSD_MAXDEVS; i++) {
		mi.index = i;
		if (ioctl(fd, AUDIO_MIXER_DEVINFO, &mi) < 0)
			break;
		if (strcmp(mi.label.name, AudioNsource) != 0)
			continue;
		di->source = i;
		switch(mi.type) {
		case AUDIO_MIXER_ENUM:
			for(j = 0; j < mi.un.e.num_mem; j++) {
				e = opaque_to_enum(di,
						   &mi.un.e.member[j].label,
						   mi.un.e.member[j].ord);
				if (e >= 0)
					di->recmask |= 1 << di->rdevmap[e];
			}
			di->caps = SOUND_CAP_EXCL_INPUT;
			break;
		case AUDIO_MIXER_SET:
			for(j = 0; j < mi.un.s.num_mem; j++) {
				e = opaque_to_enum(di,
						   &mi.un.s.member[j].label,
						   mi.un.s.member[j].mask);
				if (e >= 0)
					di->recmask |= 1 << di->rdevmap[e];
			}
			break;
		}
	}
	return di;
}

static int
mixer_oss3_ioctl(int fd, unsigned long com, void *argp)
{
	struct audiodevinfo *di;
	struct mixer_info *omi;
	struct audio_device adev;
	mixer_ctrl_t mc;
	u_long idat, n;
	int i;
	int retval;
	int l, r, error, e;

	idat = 0;
	di = getdevinfo(fd);
	if (di == 0)
		return -1;

	switch (com) {
        case OSS_GETVERSION:
                idat = SOUND_VERSION;
                break;
	case SOUND_MIXER_INFO:
	case SOUND_OLD_MIXER_INFO:
		error = ioctl(fd, AUDIO_GETDEV, &adev);
		if (error)
			return (error);
		omi = argp;
		if (com == SOUND_MIXER_INFO)
			omi->modify_counter = 1;
		strlcpy(omi->id, adev.name, sizeof omi->id);
		strlcpy(omi->name, adev.name, sizeof omi->name);
		return 0;
	case SOUND_MIXER_READ_RECSRC:
		if (di->source == -1) {
			errno = EINVAL;
			return -1;
		}
		mc.dev = di->source;
		if (di->caps & SOUND_CAP_EXCL_INPUT) {
			mc.type = AUDIO_MIXER_ENUM;
			retval = ioctl(fd, AUDIO_MIXER_READ, &mc);
			if (retval < 0)
				return retval;
			e = opaque_to_enum(di, NULL, mc.un.ord);
			if (e >= 0)
				idat = 1 << di->rdevmap[e];
		} else {
			mc.type = AUDIO_MIXER_SET;
			retval = ioctl(fd, AUDIO_MIXER_READ, &mc);
			if (retval < 0)
				return retval;
			e = opaque_to_enum(di, NULL, mc.un.mask);
			if (e >= 0)
				idat = 1 << di->rdevmap[e];
		}
		break;
	case SOUND_MIXER_READ_DEVMASK:
		idat = di->devmask;
		break;
	case SOUND_MIXER_READ_RECMASK:
		idat = di->recmask;
		break;
	case SOUND_MIXER_READ_STEREODEVS:
		idat = di->stereomask;
		break;
	case SOUND_MIXER_READ_CAPS:
		idat = di->caps;
		break;
	case SOUND_MIXER_WRITE_RECSRC:
	case SOUND_MIXER_WRITE_R_RECSRC:
		if (di->source == -1) {
			errno = EINVAL;
			return -1;
		}
		mc.dev = di->source;
		idat = INTARG;
		if (di->caps & SOUND_CAP_EXCL_INPUT) {
			mc.type = AUDIO_MIXER_ENUM;
			for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
				if (idat & (1 << i))
					break;
			if (i >= SOUND_MIXER_NRDEVICES ||
			    di->devmap[i] == -1) {
				errno = EINVAL;
				return -1;
			}
			mc.un.ord = enum_to_ord(di, di->devmap[i]);
		} else {
			mc.type = AUDIO_MIXER_SET;
			mc.un.mask = 0;
			for(i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
				if (idat & (1 << i)) {
					if (di->devmap[i] == -1) {
						errno = EINVAL;
						return -1;
					}
					mc.un.mask |=
					    enum_to_mask(di, di->devmap[i]);
				}
			}
		}
		return ioctl(fd, AUDIO_MIXER_WRITE, &mc);
	default:
		if (MIXER_READ(SOUND_MIXER_FIRST) <= com &&
		    com < MIXER_READ(SOUND_MIXER_NRDEVICES)) {
			n = GET_DEV(com);
			if (di->devmap[n] == -1) {
				errno = EINVAL;
				return -1;
			}
			mc.dev = di->devmap[n];
			mc.type = AUDIO_MIXER_VALUE;
		    doread:
			mc.un.value.num_channels =
			    di->stereomask & (1 << (u_int)n) ? 2 : 1;
			retval = ioctl(fd, AUDIO_MIXER_READ, &mc);
			if (retval < 0)
				return retval;
			if (mc.type != AUDIO_MIXER_VALUE) {
				errno = EINVAL;
				return -1;
			}
			if (mc.un.value.num_channels != 2) {
				l = r =
				    mc.un.value.level[AUDIO_MIXER_LEVEL_MONO];
			} else {
				l = mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT];
				r = mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT];
			}
			idat = TO_OSSVOL(l) | (TO_OSSVOL(r) << 8);
			break;
		} else if ((MIXER_WRITE_R(SOUND_MIXER_FIRST) <= com &&
			   com < MIXER_WRITE_R(SOUND_MIXER_NRDEVICES)) ||
			   (MIXER_WRITE(SOUND_MIXER_FIRST) <= com &&
			   com < MIXER_WRITE(SOUND_MIXER_NRDEVICES))) {
			n = GET_DEV(com);
			if (di->devmap[n] == -1) {
				errno = EINVAL;
				return -1;
			}
			idat = INTARG;
			l = FROM_OSSVOL((u_int)idat & 0xff);
			r = FROM_OSSVOL(((u_int)idat >> 8) & 0xff);
			mc.dev = di->devmap[n];
			mc.type = AUDIO_MIXER_VALUE;
			if (di->stereomask & (1 << (u_int)n)) {
				mc.un.value.num_channels = 2;
				mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = l;
				mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] = r;
			} else {
				mc.un.value.num_channels = 1;
				mc.un.value.level[AUDIO_MIXER_LEVEL_MONO] =
				    (l + r) / 2;
			}
			retval = ioctl(fd, AUDIO_MIXER_WRITE, &mc);
			if (retval < 0)
				return retval;
			if (MIXER_WRITE(SOUND_MIXER_FIRST) <= com &&
			   com < MIXER_WRITE(SOUND_MIXER_NRDEVICES))
				return 0;
			goto doread;
		} else {
			errno = EINVAL;
			return -1;
		}
	}
	INTARG = (int)idat;
	return 0;
}

static int
mixer_oss4_ioctl(int fd, unsigned long com, void *argp)
{
	oss_audioinfo *tmpai;
	oss_card_info *cardinfo;
	oss_mixext *ext;
	oss_mixext_root root;
	oss_mixer_enuminfo *ei;
	oss_mixer_value *mv;
	oss_mixerinfo *mi;
	oss_sysinfo sysinfo;
	dev_t devno;
	struct stat tmpstat;
	struct audio_device dev;
	struct audio_format_query fmtq;
	struct mixer_devinfo mdi;
	struct mixer_ctrl mc;
	char devname[32];
	size_t len;
	int newfd = -1, tmperrno;
	int i, noffs;
	int retval;

	/*
	 * Note: it is difficult to translate the NetBSD concept of a "set"
	 * mixer control type to the OSSv4 API, as far as I can tell.
	 *
	 * This means they are treated like enums, i.e. only one entry in the
	 * set can be selected at a time.
	 */

	switch (com) {
	case SNDCTL_AUDIOINFO:
	/*
	 * SNDCTL_AUDIOINFO_EX is intended for underlying hardware devices
	 * that are to be opened in "exclusive mode" (bypassing the normal
	 * kernel mixer for exclusive control). NetBSD does not support
	 * bypassing the kernel mixer, so it's an alias of SNDCTL_AUDIOINFO.
	 */
	case SNDCTL_AUDIOINFO_EX:
	case SNDCTL_ENGINEINFO:
		devno = 0;
		tmpai = (struct oss_audioinfo*)argp;
		if (tmpai == NULL) {
			errno = EINVAL;
			return -1;
		}

		/*
		 * If the input device is -1, guess the device related to
		 * the open mixer device.
		 */
		if (tmpai->dev < 0) {
			fstat(fd, &tmpstat);
			if ((tmpstat.st_rdev & 0xff00) == 0x2a00)
				devno = tmpstat.st_rdev & 0xff;
			if (devno >= 0x80)
				tmpai->dev = devno & 0x7f;
		}
		if (tmpai->dev < 0)
			tmpai->dev = 0;

		snprintf(tmpai->devnode, sizeof(tmpai->devnode),
		    "/dev/audio%d", tmpai->dev);

		if ((newfd = open(tmpai->devnode, O_WRONLY)) < 0) {
			if ((newfd = open(tmpai->devnode, O_RDONLY)) < 0) {
				return newfd;
			}
		}

		retval = ioctl(newfd, AUDIO_GETDEV, &dev);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		if (getcaps(newfd, &tmpai->caps) < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		snprintf(tmpai->name, sizeof(tmpai->name),
		    "%s %s", dev.name, dev.version);
		tmpai->busy = 0;
		tmpai->pid = -1;
		ioctl(newfd, SNDCTL_DSP_GETFMTS, &tmpai->iformats);
		tmpai->oformats = tmpai->iformats;
		tmpai->magic = -1; /* reserved for "internal use" */
		memset(tmpai->cmd, 0, sizeof(tmpai->cmd));
		tmpai->card_number = -1;
		memset(tmpai->song_name, 0,
		    sizeof(tmpai->song_name));
		memset(tmpai->label, 0, sizeof(tmpai->label));
		tmpai->port_number = 0;
		tmpai->mixer_dev = tmpai->dev;
		tmpai->legacy_device = tmpai->dev;
		tmpai->enabled = 1;
		tmpai->flags = -1; /* reserved for "future versions" */
		tmpai->min_rate = 1000;
		tmpai->max_rate = 192000;
		tmpai->nrates = 0;
		tmpai->min_channels = 1;
		tmpai->max_channels = 2;
		for (fmtq.index = 0;
		    ioctl(newfd, AUDIO_QUERYFORMAT, &fmtq) != -1; ++fmtq.index) {
			if (fmtq.fmt.channels > (unsigned)tmpai->max_channels)
				tmpai->max_channels = fmtq.fmt.channels;
		}
		tmpai->binding = -1; /* reserved for "future versions" */
		tmpai->rate_source = -1;
		/*
		 * 'handle' is supposed to be globally unique. The closest
		 * we have to that is probably device nodes.
		 */
		strlcpy(tmpai->handle, tmpai->devnode,
		    sizeof(tmpai->handle));
		tmpai->next_play_engine = 0;
		tmpai->next_rec_engine = 0;
		argp = tmpai;
		close(newfd);
		break;
	case SNDCTL_CARDINFO:
		cardinfo = (oss_card_info *)argp;
		if (cardinfo == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (cardinfo->card != -1) {
			snprintf(devname, sizeof(devname),
			    "/dev/audio%d", cardinfo->card);
			newfd = open(devname, O_RDONLY);
			if (newfd < 0)
				return newfd;
		} else {
			newfd = fd;
		}
		retval = ioctl(newfd, AUDIO_GETDEV, &dev);
		tmperrno = errno;
		if (newfd != fd)
			close(newfd);
		if (retval < 0) {
			errno = tmperrno;
			return retval;
		}
		strlcpy(cardinfo->shortname, dev.name,
		    sizeof(cardinfo->shortname));
		snprintf(cardinfo->longname, sizeof(cardinfo->longname),
		    "%s %s %s", dev.name, dev.version, dev.config);
		memset(cardinfo->hw_info, 0, sizeof(cardinfo->hw_info));
		/*
		 * OSSv4 does not document this ioctl, and claims it should
		 * not be used by applications and is provided for "utiltiy
		 * programs included in OSS". We follow the Solaris
		 * implementation (which is documented) and leave these fields
		 * unset.
		 */
		cardinfo->flags = 0;
		cardinfo->intr_count = 0;
		cardinfo->ack_count = 0;
		break;
	case SNDCTL_SYSINFO:
		memset(&sysinfo, 0, sizeof(sysinfo));
		strlcpy(sysinfo.product,
		    "OSS/NetBSD", sizeof(sysinfo.product));
		strlcpy(sysinfo.version,
		    "4.01", sizeof(sysinfo.version));
		strlcpy(sysinfo.license,
		    "BSD", sizeof(sysinfo.license));
		sysinfo.versionnum = SOUND_VERSION;
		sysinfo.numaudios = 
		    sysinfo.numcards =
			getaudiocount();
		sysinfo.numaudioengines = 1;
		sysinfo.numsynths = 1;
		sysinfo.nummidis = -1;
		sysinfo.numtimers = -1;
		sysinfo.nummixers = getmixercount();
		*(struct oss_sysinfo *)argp = sysinfo;
		break;
	case SNDCTL_MIXERINFO:
		mi = (oss_mixerinfo *)argp;
		if (mi == NULL) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", mi->dev);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		retval = ioctl(newfd, AUDIO_GETDEV, &dev);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		strlcpy(mi->id, devname, sizeof(mi->id));
		snprintf(mi->name, sizeof(mi->name),
		    "%s %s", dev.name, dev.version);
		mi->card_number = mi->dev;
		mi->port_number = 0;
		mi->magic = 0;
		mi->enabled = 1;
		mi->caps = 0;
		mi->flags = 0;
		mi->nrext = getmixercontrolcount(newfd) + 1;
		mi->priority = UCHAR_MAX - mi->dev;
		strlcpy(mi->devnode, devname, sizeof(mi->devnode));
		mi->legacy_device = mi->dev;
		break;
	case SNDCTL_MIX_DESCRIPTION:
		/* No description available. */
		errno = ENOSYS;
		return -1;
	case SNDCTL_MIX_NRMIX:
		INTARG = getmixercount();
		break;
	case SNDCTL_MIX_NREXT:
		snprintf(devname, sizeof(devname), "/dev/mixer%d", INTARG);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		INTARG = getmixercontrolcount(newfd) + 1;
		close(newfd);
		break;
	case SNDCTL_MIX_EXTINFO:
		ext = (oss_mixext *)argp;
		snprintf(devname, sizeof(devname), "/dev/mixer%d", ext->dev);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		if (ext->ctrl == 0) {
			/*
			 * NetBSD has no concept of a "root mixer control", but
 			 * OSSv4 requires one to work. We fake one at 0 and
			 * simply add 1 to all real control indexes.
			 */
			retval = ioctl(newfd, AUDIO_GETDEV, &dev);
			tmperrno = errno;
			close(newfd);
			if (retval < 0) {
				errno = tmperrno;
				return -1;
			}
			memset(&root, 0, sizeof(root));
			strlcpy(root.id, devname, sizeof(root.id));
			snprintf(root.name, sizeof(root.name),
			    "%s %s", dev.name, dev.version);
			strlcpy(ext->id, devname, sizeof(ext->id));
			snprintf(ext->extname, sizeof(ext->extname),
			    "%s %s", dev.name, dev.version);
			strlcpy(ext->extname, "root", sizeof(ext->extname));
			ext->type = MIXT_DEVROOT;
			ext->minvalue = 0;
			ext->maxvalue = 0;
			ext->flags = 0;
			ext->parent = -1;
			ext->control_no = -1;
			ext->update_counter = 0;
			ext->rgbcolor = 0;
			memcpy(&ext->data, &root,
			    sizeof(root) > sizeof(ext->data) ?
			    sizeof(ext->data) : sizeof(root));
			return 0;
		}
		mdi.index = ext->ctrl - 1;
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		ext->flags = MIXF_READABLE | MIXF_WRITEABLE | MIXF_POLL;
		ext->parent = mdi.mixer_class + 1;
		strlcpy(ext->id, mdi.label.name, sizeof(ext->id));
		strlcpy(ext->extname, mdi.label.name, sizeof(ext->extname));
		len = strlen(ext->extname);
		memset(ext->data, 0, sizeof(ext->data));
		ext->control_no = -1;
		ext->update_counter = 0;
		ext->rgbcolor = 0;
		switch (mdi.type) {
		case AUDIO_MIXER_CLASS:
			ext->type = MIXT_GROUP;
			ext->parent = 0;
			ext->minvalue = 0;
			ext->maxvalue = 0;
			break;
		case AUDIO_MIXER_ENUM:
			ext->maxvalue = mdi.un.e.num_mem;
			ext->minvalue = 0;
			for (i = 0; i < mdi.un.e.num_mem; ++i) {
				ext->enum_present[i / 8] |= (1 << (i % 8));
			}
			if (mdi.un.e.num_mem == 2) {
				if (!strcmp(mdi.un.e.member[0].label.name, AudioNoff) &&
				    !strcmp(mdi.un.e.member[1].label.name, AudioNon)) {
					ext->type = MIXT_MUTE;
				} else {
					ext->type = MIXT_ENUM;
				}
			} else {
				ext->type = MIXT_ENUM;
			}
			break;
		case AUDIO_MIXER_SET:
			ext->maxvalue = mdi.un.s.num_mem;
			ext->minvalue = 0;
#ifdef notyet
			/*
			 * XXX: This is actually the correct type for "set"
			 * controls, but it seems no real world software
			 * supports it. The only documentation exists in
			 * the OSSv4 headers and describes it as "reserved
			 * for Sun's implementation".
			 */
			ext->type = MIXT_ENUM_MULTI;
#else
			ext->type = MIXT_ENUM;
#endif
			for (i = 0; i < mdi.un.s.num_mem; ++i) {
				ext->enum_present[i / 8] |= (1 << (i % 8));
			}
			break;
		case AUDIO_MIXER_VALUE:
			ext->maxvalue = UCHAR_MAX + 1;
			ext->minvalue = 0;
			if (mdi.un.v.num_channels == 2) {
				ext->type = MIXT_STEREOSLIDER;
			} else {
				ext->type = MIXT_MONOSLIDER;
			}
			break;
		}
		close(newfd);
		break;
	case SNDCTL_MIX_ENUMINFO:
		ei = (oss_mixer_enuminfo *)argp;
		if (ei == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (ei->ctrl == 0) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", ei->dev);
		if ((newfd = open(devname, O_RDONLY)) < 0)
			return newfd;
		mdi.index = ei->ctrl - 1;
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		tmperrno = errno;
		close(newfd);
		if (retval < 0) {
			errno = tmperrno;
			return retval;
		}
		ei->version = 0;
		switch (mdi.type) {
		case AUDIO_MIXER_ENUM:
			ei->nvalues = mdi.un.e.num_mem;
			noffs = 0;
			for (i = 0; i < ei->nvalues; ++i) {
				ei->strindex[i] = noffs;
				len = strlen(mdi.un.e.member[i].label.name) + 1;
				if ((noffs + len) >= sizeof(ei->strings)) {
				    errno = ENOMEM;
				    return -1;
				}
				memcpy(ei->strings + noffs,
				    mdi.un.e.member[i].label.name, len);
				noffs += len;
			}
			break;
		case AUDIO_MIXER_SET:
			ei->nvalues = mdi.un.s.num_mem;
			noffs = 0;
			for (i = 0; i < ei->nvalues; ++i) {
				ei->strindex[i] = noffs;
				len = strlen(mdi.un.s.member[i].label.name) + 1;
				if ((noffs + len) >= sizeof(ei->strings)) {
				    errno = ENOMEM;
				    return -1;
				}
				memcpy(ei->strings + noffs,
				    mdi.un.s.member[i].label.name, len);
				noffs += len;
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	case SNDCTL_MIX_WRITE:
		mv = (oss_mixer_value *)argp;
		if (mv == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (mv->ctrl == 0) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", mv->dev);
		if ((newfd = open(devname, O_RDWR)) < 0)
			return newfd;
		mdi.index = mc.dev = mv->ctrl - 1;
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		switch (mdi.type) {
		case AUDIO_MIXER_ENUM:
			if (mv->value >= mdi.un.e.num_mem) {
				close(newfd);
				errno = EINVAL;
				return -1;
			}
			mc.un.ord = mdi.un.e.member[mv->value].ord;
			break;
		case AUDIO_MIXER_SET:
			if (mv->value >= mdi.un.s.num_mem) {
				close(newfd);
				errno = EINVAL;
				return -1;
			}
#ifdef notyet
			mc.un.mask = 0;
			for (i = 0; i < mdi.un.s.num_mem; ++i) {
				if (mv->value & (1 << i)) {
					mc.un.mask |= mdi.un.s.member[mv->value].mask;
				}
			}
#else
			mc.un.mask = mdi.un.s.member[mv->value].mask;
#endif
			break;
		case AUDIO_MIXER_VALUE:
			if (mdi.un.v.num_channels != 2) {
				for (i = 0; i < mdi.un.v.num_channels; ++i) {
					mc.un.value.level[i] = mv->value;
				}
			} else {
			    mc.un.value.level[AUDIO_MIXER_LEVEL_LEFT] = 
				(mv->value >> 0) & 0xFF;
			    mc.un.value.level[AUDIO_MIXER_LEVEL_RIGHT] =
				(mv->value >> 8) & 0xFF;
			}
			break;
		}
		retval = ioctl(newfd, AUDIO_MIXER_WRITE, &mc);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		close(newfd);
		break;
	case SNDCTL_MIX_READ:
		mv = (oss_mixer_value *)argp;
		if (mv == NULL) {
			errno = EINVAL;
			return -1;
		}
		if (mv->ctrl == 0) {
			errno = EINVAL;
			return -1;
		}
		snprintf(devname, sizeof(devname), "/dev/mixer%d", mv->dev);
		if ((newfd = open(devname, O_RDWR)) < 0)
			return newfd;
		mdi.index = mc.dev = (mv->ctrl - 1);
		retval = ioctl(newfd, AUDIO_MIXER_DEVINFO, &mdi);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		mc.dev = mdi.index;
		retval = ioctl(newfd, AUDIO_MIXER_READ, &mc);
		if (retval < 0) {
			tmperrno = errno;
			close(newfd);
			errno = tmperrno;
			return retval;
		}
		close(newfd);
		mv->value = 0;
		switch (mdi.type) {
		case AUDIO_MIXER_ENUM:
			for (i = 0; i < mdi.un.e.num_mem; ++i) {
				if (mc.un.ord == mdi.un.e.member[i].ord) {
					mv->value = i;
					break;
				}
			}
			break;
		case AUDIO_MIXER_SET:
			for (i = 0; i < mdi.un.s.num_mem; ++i) {
#ifdef notyet
				if (mc.un.mask & mdi.un.s.member[i].mask)
					mv->value |= (1 << i);
#else
				if (mc.un.mask == mdi.un.s.member[i].mask) {
					mv->value = i;
					break;
				}
#endif
			}
			break;
		case AUDIO_MIXER_VALUE:
			if (mdi.un.v.num_channels != 2) {
				mv->value = mc.un.value.level[0];
			} else {
				mv->value = \
				    ((mc.un.value.level[1] & 0xFF) << 8) |
				    ((mc.un.value.level[0] & 0xFF) << 0);
			}
			break;
		default:
			errno = EINVAL;
			return -1;
		}
		break;
	default:
		errno = EINVAL;
		return -1;
	}
	return 0;
}

static int
global_oss4_ioctl(int fd, unsigned long com, void *argp)
{
	int retval = 0;

	switch (com) {
	/*
	 * These ioctls were added in OSSv4 with the idea that
	 * applications could apply strings to audio devices to
	 * display what they are using them for (e.g. with song
	 * names) in mixer applications. In practice, the popular
	 * implementations of the API in FreeBSD and Solaris treat
	 * these as a no-op and return EINVAL, and no software in the
	 * wild seems to use them.
	 */
	case SNDCTL_SETSONG:
	case SNDCTL_GETSONG:
	case SNDCTL_SETNAME:
	case SNDCTL_SETLABEL:
	case SNDCTL_GETLABEL:
		errno = EINVAL;
		retval = -1;
		break;
	default:
		errno = EINVAL;
		retval = -1;
		break;
	}
	return retval;
}

static int
getcaps(int fd, int *out)
{
	int props, caps;

	if (ioctl(fd, AUDIO_GETPROPS, &props) < 0)
		return -1;

	caps = DSP_CAP_TRIGGER;

	if (props & AUDIO_PROP_FULLDUPLEX)
		caps |= DSP_CAP_DUPLEX;
	if (props & AUDIO_PROP_MMAP)
		caps |= DSP_CAP_MMAP;
	if (props & AUDIO_PROP_CAPTURE)
		caps |= PCM_CAP_INPUT;
	if (props & AUDIO_PROP_PLAYBACK)
		caps |= PCM_CAP_OUTPUT;

	*out = caps;
	return 0;
}

static int
getaudiocount(void)
{
	char devname[32];
	int ndevs = 0;
	int tmpfd;
	int tmperrno = errno;

	do {
		snprintf(devname, sizeof(devname),
		    "/dev/audio%d", ndevs);
		if ((tmpfd = open(devname, O_RDONLY)) != -1 ||
		    (tmpfd = open(devname, O_WRONLY)) != -1) {
			ndevs++;
			close(tmpfd);
		}
	} while (tmpfd != -1);
	errno = tmperrno;
	return ndevs;
}

static int
getmixercount(void)
{
	char devname[32];
	int ndevs = 0;
	int tmpfd;
	int tmperrno = errno;

	do {
		snprintf(devname, sizeof(devname),
		    "/dev/mixer%d", ndevs);
		if ((tmpfd = open(devname, O_RDONLY)) != -1) {
			ndevs++;
			close(tmpfd);
		}
	} while (tmpfd != -1);
	errno = tmperrno;
	return ndevs;
}

static int
getmixercontrolcount(int fd)
{
	struct mixer_devinfo mdi;
	int ndevs = 0;

	do {
		mdi.index = ndevs++;
	} while (ioctl(fd, AUDIO_MIXER_DEVINFO, &mdi) != -1);

	return ndevs > 0 ? ndevs - 1 : 0;
}

static int
getvol(u_int gain, u_char balance)
{
	u_int l, r;

	if (balance == AUDIO_MID_BALANCE) {
		l = r = gain;
	} else if (balance < AUDIO_MID_BALANCE) {
		l = gain;
		r = (balance * gain) / AUDIO_MID_BALANCE;
	} else {
		r = gain;
		l = ((AUDIO_RIGHT_BALANCE - balance) * gain)
		    / AUDIO_MID_BALANCE;
	}

	return TO_OSSVOL(l) | (TO_OSSVOL(r) << 8);
}

static void
setvol(int fd, int volume, bool record)
{
	u_int lgain, rgain;
	struct audio_info tmpinfo;
	struct audio_prinfo *prinfo;

	AUDIO_INITINFO(&tmpinfo);
	prinfo = record ? &tmpinfo.record : &tmpinfo.play;

	lgain = FROM_OSSVOL((volume >> 0) & 0xff);
	rgain = FROM_OSSVOL((volume >> 8) & 0xff);

	if (lgain == rgain) {
		prinfo->gain = lgain;
		prinfo->balance = AUDIO_MID_BALANCE;
	} else if (lgain < rgain) {
		prinfo->gain = rgain;
		prinfo->balance = AUDIO_RIGHT_BALANCE -
		    (AUDIO_MID_BALANCE * lgain) / rgain;
	} else {
		prinfo->gain = lgain;
		prinfo->balance = (AUDIO_MID_BALANCE * rgain) / lgain;
	}

	(void)ioctl(fd, AUDIO_SETINFO, &tmpinfo);
}

/*
 * When AUDIO_SETINFO fails to set a channel count, the application's chosen
 * number is out of range of what the kernel allows.
 *
 * When this happens, we use the current hardware settings. This is just in
 * case an application is abusing SNDCTL_DSP_CHANNELS - OSSv4 always sets and
 * returns a reasonable value, even if it wasn't what the user requested.
 *
 * Solaris guarantees this behaviour if nchannels = 0.
 *
 * XXX: If a device is opened for both playback and recording, and supports
 * fewer channels for recording than playback, applications that do both will
 * behave very strangely. OSS doesn't allow for reporting separate channel
 * counts for recording and playback. This could be worked around by always
 * mixing recorded data up to the same number of channels as is being used
 * for playback.
 */
static void
setchannels(int fd, int mode, int nchannels)
{
	struct audio_info tmpinfo, hwfmt;

	if (ioctl(fd, AUDIO_GETFORMAT, &hwfmt) < 0) {
		errno = 0;
		hwfmt.record.channels = hwfmt.play.channels = 2;
	}

	if (mode & AUMODE_PLAY) {
		AUDIO_INITINFO(&tmpinfo);
		tmpinfo.play.channels = nchannels;
		if (ioctl(fd, AUDIO_SETINFO, &tmpinfo) < 0) {
			errno = 0;
			AUDIO_INITINFO(&tmpinfo);
			tmpinfo.play.channels = hwfmt.play.channels;
			(void)ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		}
	}

	if (mode & AUMODE_RECORD) {
		AUDIO_INITINFO(&tmpinfo);
		tmpinfo.record.channels = nchannels;
		if (ioctl(fd, AUDIO_SETINFO, &tmpinfo) < 0) {
			errno = 0;
			AUDIO_INITINFO(&tmpinfo);
			tmpinfo.record.channels = hwfmt.record.channels;
			(void)ioctl(fd, AUDIO_SETINFO, &tmpinfo);
		}
	}
}

/*
 * Check that the blocksize is a power of 2 as OSS wants.
 * If not, set it to be.
 */
static void
setblocksize(int fd, struct audio_info *info)
{
	struct audio_info set;
	size_t s;

	if (info->blocksize & (info->blocksize-1)) {
		for(s = 32; s < info->blocksize; s <<= 1)
			;
		AUDIO_INITINFO(&set);
		set.blocksize = s;
		ioctl(fd, AUDIO_SETINFO, &set);
		ioctl(fd, AUDIO_GETBUFINFO, info);
	}
}
