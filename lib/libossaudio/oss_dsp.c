/*	$NetBSD: oss_dsp.c,v 1.2 2021/06/08 19:26:48 nia Exp $	*/

/*-
 * Copyright (c) 1997-2021 The NetBSD Foundation, Inc.
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
__RCSID("$NetBSD: oss_dsp.c,v 1.2 2021/06/08 19:26:48 nia Exp $");

#include <sys/audioio.h>
#include <stdbool.h>
#include <errno.h>
#include "internal.h"

#define GETPRINFO(info, name)	\
	(((info)->mode == AUMODE_RECORD) \
	    ? (info)->record.name : (info)->play.name)

static int encoding_to_format(u_int, u_int);
static int format_to_encoding(int, struct audio_info *);

static int get_vol(u_int, u_char);
static void set_vol(int, int, bool);

static void set_channels(int, int, int);

oss_private int
_oss_dsp_ioctl(int fd, unsigned long com, void *argp)
{

	struct audio_info tmpinfo, hwfmt;
	struct audio_offset tmpoffs;
	struct audio_buf_info bufinfo;
	struct audio_errinfo *tmperrinfo;
	struct count_info cntinfo;
	struct audio_encoding tmpenc;
	u_int u;
	int perrors, rerrors;
	static int totalperrors = 0;
	static int totalrerrors = 0;
	oss_mixer_enuminfo *ei;
	oss_count_t osscount;
	int idat;
	int retval;

	idat = 0;

	switch (com) {
	case SNDCTL_DSP_HALT_INPUT:
	case SNDCTL_DSP_HALT_OUTPUT:
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
		AUDIO_INITINFO(&tmpinfo);
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
		INTARG = tmpinfo.blocksize;
		break;
	case SNDCTL_DSP_SETFMT:
		AUDIO_INITINFO(&tmpinfo);
		retval = format_to_encoding(INTARG, &tmpinfo);
		if (retval < 0) {
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
		if (tmpinfo.mode == AUMODE_RECORD)
			retval = encoding_to_format(tmpinfo.record.encoding,
			    tmpinfo.record.precision);
		else
			retval = encoding_to_format(tmpinfo.play.encoding,
			    tmpinfo.play.precision);
		if (retval < 0) {
			errno = EINVAL;
			return retval;
		}
		INTARG = retval;
		break;
	case SNDCTL_DSP_CHANNELS:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		set_channels(fd, tmpinfo.mode, INTARG);
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
			retval = encoding_to_format(tmpenc.encoding,
			    tmpenc.precision);
			if (retval != -1)
				idat |= retval;
		}
		INTARG = idat;
		break;
	case SNDCTL_DSP_GETOSPACE:
		retval = ioctl(fd, AUDIO_GETBUFINFO, &tmpinfo);
		if (retval < 0)
			return retval;
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
		retval = _oss_get_caps(fd, (int *)argp);
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
		set_vol(fd, INTARG, false);
		/* FALLTHRU */
	case SNDCTL_DSP_GETPLAYVOL:
		retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = get_vol(tmpinfo.play.gain, tmpinfo.play.balance);
		break;
	case SNDCTL_DSP_SETRECVOL:
		set_vol(fd, INTARG, true);
		/* FALLTHRU */
	case SNDCTL_DSP_GETRECVOL:
		retval = ioctl(fd, AUDIO_GETINFO, &tmpinfo);
		if (retval < 0)
			return retval;
		INTARG = get_vol(tmpinfo.record.gain, tmpinfo.record.balance);
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
	case SNDCTL_DSP_GET_PLAYTGT_NAMES:
	case SNDCTL_DSP_GET_RECSRC_NAMES:
		ei = (oss_mixer_enuminfo *)argp;
		ei->nvalues = 1;
		ei->version = 0;
		ei->strindex[0] = 0;
		strlcpy(ei->strings, "primary", OSS_ENUM_STRINGSIZE);
		break;
	case SNDCTL_DSP_SET_PLAYTGT:
	case SNDCTL_DSP_SET_RECSRC:
	case SNDCTL_DSP_GET_PLAYTGT:
	case SNDCTL_DSP_GET_RECSRC:
		/* We have one recording source and play target. */
		INTARG = 0;
		break;
	default:
		errno = EINVAL;
		return -1;
	}

	return 0;
}

static int
get_vol(u_int gain, u_char balance)
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
set_vol(int fd, int volume, bool record)
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
set_channels(int fd, int mode, int nchannels)
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

/* Convert a NetBSD "encoding" to a OSS "format". */
static int
encoding_to_format(u_int encoding, u_int precision)
{
	switch(encoding) {
	case AUDIO_ENCODING_ULAW:
		return AFMT_MU_LAW;
	case AUDIO_ENCODING_ALAW:
		return AFMT_A_LAW;
	case AUDIO_ENCODING_SLINEAR:
		if (precision == 32)
			return AFMT_S32_NE;
		else if (precision == 24)
			return AFMT_S24_NE;
		else if (precision == 16)
			return AFMT_S16_NE;
		return AFMT_S8;
	case AUDIO_ENCODING_SLINEAR_LE:
		if (precision == 32)
			return AFMT_S32_LE;
		else if (precision == 24)
			return AFMT_S24_LE;
		else if (precision == 16)
			return AFMT_S16_LE;
		return AFMT_S8;
	case AUDIO_ENCODING_SLINEAR_BE:
		if (precision == 32)
			return AFMT_S32_BE;
		else if (precision == 24)
			return AFMT_S24_BE;
		else if (precision == 16)
			return AFMT_S16_BE;
		return AFMT_S8;
	case AUDIO_ENCODING_ULINEAR:
		if (precision == 16)
			return AFMT_U16_NE;
		return AFMT_U8;
	case AUDIO_ENCODING_ULINEAR_LE:
		if (precision == 16)
			return AFMT_U16_LE;
		return AFMT_U8;
	case AUDIO_ENCODING_ULINEAR_BE:
		if (precision == 16)
			return AFMT_U16_BE;
		return AFMT_U8;
	case AUDIO_ENCODING_ADPCM:
		return AFMT_IMA_ADPCM;
	case AUDIO_ENCODING_AC3:
		return AFMT_AC3;
	}
	return -1;
}

/* Convert an OSS "format" to a NetBSD "encoding". */
static int
format_to_encoding(int fmt, struct audio_info *tmpinfo)
{
	switch (fmt) {
	case AFMT_MU_LAW:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 8;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_ULAW;
		return 0;
	case AFMT_A_LAW:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 8;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_ALAW;
		return 0;
	case AFMT_U8:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 8;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_ULINEAR;
		return 0;
	case AFMT_S8:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 8;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_SLINEAR;
		return 0;
	case AFMT_S16_LE:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 16;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_SLINEAR_LE;
		return 0;
	case AFMT_S16_BE:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 16;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_SLINEAR_BE;
		return 0;
	case AFMT_U16_LE:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 16;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_ULINEAR_LE;
		return 0;
	case AFMT_U16_BE:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 16;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_ULINEAR_BE;
		return 0;
	/*
	 * XXX: When the kernel supports 24-bit LPCM by default,
	 * the 24-bit formats should be handled properly instead
	 * of falling back to 32 bits.
	 */
	case AFMT_S24_PACKED:
	case AFMT_S24_LE:
	case AFMT_S32_LE:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 32;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_SLINEAR_LE;
		return 0;
	case AFMT_S24_BE:
	case AFMT_S32_BE:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 32;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_SLINEAR_BE;
		return 0;
	case AFMT_AC3:
		tmpinfo->record.precision =
		tmpinfo->play.precision = 16;
		tmpinfo->record.encoding =
		tmpinfo->play.encoding = AUDIO_ENCODING_AC3;
		return 0;
	}
	return -1;
}
