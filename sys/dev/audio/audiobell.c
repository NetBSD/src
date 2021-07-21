/*	$NetBSD: audiobell.c,v 1.5 2021/07/21 06:35:44 skrll Exp $	*/

/*
 * Copyright (c) 1999 Richard Earnshaw
 * Copyright (c) 2004 Ben Harris
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
 *	This product includes software developed by the RiscBSD team.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: audiobell.c,v 1.5 2021/07/21 06:35:44 skrll Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/audioio.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/uio.h>

#include <dev/audio/audio_if.h>
#include <dev/audio/audiovar.h>
#include <dev/audio/audiodef.h>
#include <dev/audio/audiobellvar.h>

/*
 * The hexadecagon is sufficiently close to a sine wave.
 * Audiobell always outputs this 16 points data but changes its playback
 * frequency.  In addition, audio layer does linear interpolation in the
 * frequency conversion stage, so the waveform becomes smooth.
 * When the playback frequency rises (or the device frequency is not enough
 * high) and one wave cannot be expressed with 16 points, the data is thinned
 * out by power of two, like 8 points -> 4 points (triangular wave)
 * -> 2 points (rectangular wave).
 */

/* Amplitude.  Full scale amplitude is too loud. */
#define A(x) ((x) * 0.6)

/* (sin(2*pi * (x/16)) * 32767 / 100) << 16 */
static const int32_t sinewave[] = {
	A(        0),
	A(  8217813),
	A( 15184539),
	A( 19839556),
	A( 21474181),
	A( 19839556),
	A( 15184539),
	A(  8217813),
	A(        0),
	A( -8217814),
	A(-15184540),
	A(-19839557),
	A(-21474182),
	A(-19839557),
	A(-15184540),
	A( -8217814),
};
#undef A

/*
 * The minimum and the maximum buffer sizes must be a multiple of 32
 * (32 = countof(sinewave) * sizeof(uint16_t)).
 */
#define MINBUFSIZE	(1024)
#define MAXBUFSIZE	(4096)

/*
 * dev is a device_t for the audio device to use.
 * pitch is the pitch of the bell in Hz,
 * period is the length in ms,
 * volume is the amplitude in % of max,
 * poll is no longer used.
 */
void
audiobell(void *dev, u_int pitch, u_int period, u_int volume, int poll)
{
	dev_t audio;
	int16_t *buf;
	audio_file_t *file;
	audio_track_t *ptrack;
	struct uio auio;
	struct iovec aiov;
	u_int i;
	u_int j;
	u_int remaincount;
	u_int remainbytes;
	u_int wave1count;
	u_int wave1bytes;
	u_int bufbytes;
	u_int len;
	u_int step;
	u_int offset;
	u_int play_sample_rate;
	u_int mixer_sample_rate;

	KASSERT(volume <= 100);

	/* Playing for 0msec does nothing. */
	if (period == 0)
		return;

	/* The audio system isn't built for polling. */
	if (poll)
		return;

	buf = NULL;
	audio = AUDIO_DEVICE | device_unit((device_t)dev);

	/* If not configured, we can't beep. */
	if (audiobellopen(audio, &file) != 0)
		return;

	ptrack = file->ptrack;
	mixer_sample_rate = ptrack->mixer->track_fmt.sample_rate;

	/* Limit pitch */
	if (pitch < 20)
		pitch = 20;

	offset = 0;
	if (pitch <= mixer_sample_rate / 16) {
		/* 16-point sine wave */
		step = 1;
	} else if (pitch <= mixer_sample_rate / 8) {
		/* 8-point sine wave */
		step = 2;
	} else if (pitch <= mixer_sample_rate / 4) {
		/* 4-point sine wave, aka, triangular wave */
		step = 4;
	} else {
		/* Rectangular wave */
		if (pitch > mixer_sample_rate / 2)
			pitch = mixer_sample_rate / 2;
		step = 8;
		offset = 4;
	}

	wave1count = __arraycount(sinewave) / step;
	play_sample_rate = pitch * wave1count;
	audiobellsetrate(file, play_sample_rate);

	/* msec to sample count */
	remaincount = play_sample_rate * period / 1000;
	/* Roundup to full wave */
	remaincount = roundup(remaincount, wave1count);
	remainbytes = remaincount * sizeof(int16_t);
	wave1bytes = wave1count * sizeof(int16_t);

	/* Based on 3*usrbuf_blksize, but not too small or too large */
	bufbytes = ptrack->usrbuf_blksize * NBLKHW;
	if (bufbytes < MINBUFSIZE)
		bufbytes = MINBUFSIZE;
	else if (bufbytes > MAXBUFSIZE)
		bufbytes = MAXBUFSIZE;
	else
		bufbytes = roundup(bufbytes, wave1bytes);
	bufbytes = uimin(bufbytes, remainbytes);
	KASSERT(bufbytes != 0);
	buf = malloc(bufbytes, M_TEMP, M_WAITOK);
	if (buf == NULL)
		goto out;

	/* Generate sinewave with specified volume */
	j = offset;
	for (i = 0; i < bufbytes / sizeof(int16_t); i++) {
		/* XXX audio already has track volume feature though #if 0 */
		buf[i] = AUDIO_SCALEDOWN(sinewave[j] * (int)volume, 16);
		j += step;
		j %= __arraycount(sinewave);
	}

	/* Write while paused to avoid inserting silence. */
	ptrack->is_pause = true;
	for (; remainbytes > 0; remainbytes -= len) {
		len = uimin(remainbytes, bufbytes);
		aiov.iov_base = (void *)buf;
		aiov.iov_len = len;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_resid = len;
		auio.uio_rw = UIO_WRITE;
		UIO_SETUP_SYSSPACE(&auio);
		if (audiobellwrite(file, &auio) != 0)
			goto out;

		if (ptrack->usrbuf.used >= ptrack->usrbuf_blksize * NBLKHW)
			ptrack->is_pause = false;
	}
	/* Here we go! */
	ptrack->is_pause = false;
out:
	if (buf != NULL)
		free(buf, M_TEMP);
	audiobellclose(file);
}
