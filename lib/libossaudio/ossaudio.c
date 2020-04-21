/*	$NetBSD: ossaudio.c,v 1.33.12.3 2020/04/21 18:41:59 martin Exp $	*/

/*-
 * Copyright (c) 1997 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: ossaudio.c,v 1.33.12.3 2020/04/21 18:41:59 martin Exp $");

/*
 * This is an OSS (Linux) sound API emulator.
 * It provides the essentials of the API.
 */

/* XXX This file is essentially the same as sys/compat/ossaudio.c.
 * With some preprocessor magic it could be the same file.
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

static int getvol(u_int, u_char);
static void setvol(int, int, bool);

static void setchannels(int, int, int);
static void setblocksize(int, struct audio_info *);

static int audio_ioctl(int, unsigned long, void *);
static int mixer_ioctl(int, unsigned long, void *);
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
		return mixer_ioctl(fd, com, argp);
	else
		return ioctl(fd, com, argp);
}

static int
audio_ioctl(int fd, unsigned long com, void *argp)
{

	struct audio_info tmpinfo, hwfmt;
	struct audio_offset tmpoffs;
	struct audio_buf_info bufinfo;
	struct count_info cntinfo;
	struct audio_encoding tmpenc;
	struct oss_sysinfo tmpsysinfo;
	struct oss_audioinfo *tmpaudioinfo;
	audio_device_t tmpaudiodev;
	struct stat tmpstat;
	dev_t devno;
	char version[32] = "4.01";
	char license[16] = "NetBSD";
	u_int u;
	u_int encoding;
	u_int precision;
	int idat, idata;
	int retval;
	int i;

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
	case SNDCTL_DSP_POST:
		/* This call is merely advisory, and may be a nop. */
		break;
	case SNDCTL_DSP_SPEED:
		AUDIO_INITINFO(&tmpinfo);
		tmpinfo.play.sample_rate =
		tmpinfo.record.sample_rate = INTARG;
		/*
		 * The default NetBSD behavior if an unsupported sample rate
		 * is set is to return an error code and keep the rate at the
		 * default of 8000 Hz.
		 * 
		 * However, OSS specifies that a sample rate supported by the
		 * hardware is returned if the exact rate could not be set.
		 * 
		 * So, if the chosen sample rate is invalid, set and return
		 * the current hardware rate.
		 */
		if (ioctl(fd, AUDIO_SETINFO, &tmpinfo) < 0) {
			/* Don't care that SETINFO failed the first time... */
			errno = 0;
			retval = ioctl(fd, AUDIO_GETFORMAT, &hwfmt);
			if (retval < 0)
				return retval;
			retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
			if (retval < 0)
				return retval;
			tmpinfo.play.sample_rate =
			tmpinfo.record.sample_rate =
			    (tmpinfo.mode == AUMODE_RECORD) ?
			    hwfmt.record.sample_rate :
			    hwfmt.play.sample_rate;
			retval = ioctl(fd, AUDIO_SETINFO, &tmpinfo);
			if (retval < 0)
				return retval;
		}
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
		if ((idat & 0xffff) < 4 || (idat & 0xffff) > 17)
			return EINVAL;
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
		retval = ioctl(fd, AUDIO_GETPROPS, &idata);
		if (retval < 0)
			return retval;
		idat = DSP_CAP_TRIGGER;
		if (idata & AUDIO_PROP_FULLDUPLEX)
			idat |= DSP_CAP_DUPLEX;
		if (idata & AUDIO_PROP_MMAP)
			idat |= DSP_CAP_MMAP;
		INTARG = idat;
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
	case SNDCTL_DSP_GETOPTR:
		retval = ioctl(fd, AUDIO_GETOOFFS, &tmpoffs);
		if (retval < 0)
			return retval;
		cntinfo.bytes = tmpoffs.samples;
		cntinfo.blocks = tmpoffs.deltablks;
		cntinfo.ptr = tmpoffs.offset;
		*(struct count_info *)argp = cntinfo;
		break;
	case SNDCTL_SYSINFO:
		strlcpy(tmpsysinfo.product, "OSS/NetBSD",
		    sizeof tmpsysinfo.product);
		strlcpy(tmpsysinfo.version, version, sizeof tmpsysinfo.version);
		strlcpy(tmpsysinfo.license, license, sizeof tmpsysinfo.license);
		tmpsysinfo.versionnum = SOUND_VERSION;
		memset(tmpsysinfo.options, 0, 8);
		tmpsysinfo.numaudios = OSS_MAX_AUDIO_DEVS;
		tmpsysinfo.numaudioengines = 1;
		memset(tmpsysinfo.openedaudio, 0, sizeof(tmpsysinfo.openedaudio));
		tmpsysinfo.numsynths = 1;
		tmpsysinfo.nummidis = -1;
		tmpsysinfo.numtimers = -1;
		tmpsysinfo.nummixers = 1;
		tmpsysinfo.numcards = 1;
		memset(tmpsysinfo.openedmidi, 0, sizeof(tmpsysinfo.openedmidi));
		*(struct oss_sysinfo *)argp = tmpsysinfo;
		break;
	case SNDCTL_ENGINEINFO:
	case SNDCTL_AUDIOINFO:
		devno = 0;
		tmpaudioinfo = (struct oss_audioinfo*)argp;
		if (tmpaudioinfo == NULL)
			return EINVAL;
		if (tmpaudioinfo->dev < 0) {
			fstat(fd, &tmpstat);
			if ((tmpstat.st_rdev & 0xff00) == 0x2a00)
				devno = tmpstat.st_rdev & 0xff;
			if (devno >= 0x80)
				tmpaudioinfo->dev = devno & 0x7f;
		}
		if (tmpaudioinfo->dev < 0)
			tmpaudioinfo->dev = 0;

		snprintf(tmpaudioinfo->devnode, OSS_DEVNODE_SIZE,
		    "/dev/audio%d", tmpaudioinfo->dev); 

		retval = ioctl(fd, AUDIO_GETDEV, &tmpaudiodev);
		if (retval < 0)
			return retval;
		retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		retval = ioctl(fd, AUDIO_GETPROPS, &idata);
		if (retval < 0)
			return retval;
		idat = DSP_CAP_TRIGGER;
		if (idata & AUDIO_PROP_FULLDUPLEX)
			idat |= DSP_CAP_DUPLEX;
		if (idata & AUDIO_PROP_MMAP)
			idat |= DSP_CAP_MMAP;
		idat = PCM_CAP_INPUT | PCM_CAP_OUTPUT;
		strlcpy(tmpaudioinfo->name, tmpaudiodev.name,
		    sizeof tmpaudioinfo->name);
		tmpaudioinfo->busy = tmpinfo.play.open;
		tmpaudioinfo->pid = -1;
		tmpaudioinfo->caps = idat;
		ioctl(fd, SNDCTL_DSP_GETFMTS, &tmpaudioinfo->iformats);
		tmpaudioinfo->oformats = tmpaudioinfo->iformats;
		tmpaudioinfo->magic = -1;
		memset(tmpaudioinfo->cmd, 0, 64);
		tmpaudioinfo->card_number = -1;
		memset(tmpaudioinfo->song_name, 0, 64);
		memset(tmpaudioinfo->label, 0, 16);
		tmpaudioinfo->port_number = tmpinfo.play.port;
		tmpaudioinfo->mixer_dev = tmpaudioinfo->dev;
		tmpaudioinfo->legacy_device = -1;
		tmpaudioinfo->enabled = 1;
		tmpaudioinfo->flags = -1;
		tmpaudioinfo->min_rate = tmpinfo.play.sample_rate;
		tmpaudioinfo->max_rate = tmpinfo.play.sample_rate;
		tmpaudioinfo->nrates = 2;
		for (i = 0; i < tmpaudioinfo->nrates; i++)
			tmpaudioinfo->rates[i] = tmpinfo.play.sample_rate;
		tmpaudioinfo->min_channels = tmpinfo.play.channels;
		tmpaudioinfo->max_channels = tmpinfo.play.channels;
		tmpaudioinfo->binding = -1;
		tmpaudioinfo->rate_source = -1;
		memset(tmpaudioinfo->handle, 0, 16);
		tmpaudioinfo->next_play_engine = 0;
		tmpaudioinfo->next_rec_engine = 0;
		argp = tmpaudioinfo;
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
		return EINVAL;
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
 * some will not be available to Linux */
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
 * emulation of the Linux mixer ioctls.  Cache the information
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

int
mixer_ioctl(int fd, unsigned long com, void *argp)
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
		if (di->source == -1)
			return EINVAL;
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
		if (di->source == -1)
			return EINVAL;
		mc.dev = di->source;
		idat = INTARG;
		if (di->caps & SOUND_CAP_EXCL_INPUT) {
			mc.type = AUDIO_MIXER_ENUM;
			for(i = 0; i < SOUND_MIXER_NRDEVICES; i++)
				if (idat & (1 << i))
					break;
			if (i >= SOUND_MIXER_NRDEVICES ||
			    di->devmap[i] == -1)
				return EINVAL;
			mc.un.ord = enum_to_ord(di, di->devmap[i]);
		} else {
			mc.type = AUDIO_MIXER_SET;
			mc.un.mask = 0;
			for(i = 0; i < SOUND_MIXER_NRDEVICES; i++) {
				if (idat & (1 << i)) {
					if (di->devmap[i] == -1)
						return EINVAL;
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
			if (di->devmap[n] == -1)
				return EINVAL;
			mc.dev = di->devmap[n];
			mc.type = AUDIO_MIXER_VALUE;
		    doread:
			mc.un.value.num_channels =
			    di->stereomask & (1 << (u_int)n) ? 2 : 1;
			retval = ioctl(fd, AUDIO_MIXER_READ, &mc);
			if (retval < 0)
				return retval;
			if (mc.type != AUDIO_MIXER_VALUE)
				return EINVAL;
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
			if (di->devmap[n] == -1)
				return EINVAL;
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
