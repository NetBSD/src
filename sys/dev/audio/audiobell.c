/*	$NetBSD: audiobell.c,v 1.2 2019/05/08 13:40:17 isaki Exp $	*/

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

#include <sys/types.h>
__KERNEL_RCSID(0, "$NetBSD: audiobell.c,v 1.2 2019/05/08 13:40:17 isaki Exp $");

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

/* 44.1 kHz should reduce hum at higher pitches. */
#define BELL_SAMPLE_RATE	44100

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
	struct audiobell_arg bellarg;
	audio_file_t *file;
	audio_track_t *ptrack;
	struct uio auio;
	struct iovec aiov;
	int i;
	int remaincount;
	int remainlen;
	int wave1count;
	int wave1len;
	int len;
	int16_t vol;

	KASSERT(volume <= 100);

	/* The audio system isn't built for polling. */
	if (poll)
		return;

	/* Limit the pitch from 20Hz to Nyquist frequency. */
	if (pitch > BELL_SAMPLE_RATE / 2)
		pitch = BELL_SAMPLE_RATE;
	if (pitch < 20)
		pitch = 20;

	buf = NULL;
	audio = AUDIO_DEVICE | device_unit((device_t)dev);

	memset(&bellarg, 0, sizeof(bellarg));
	bellarg.encoding = AUDIO_ENCODING_SLINEAR_NE;
	bellarg.precision = 16;
	bellarg.channels = 1;
	bellarg.sample_rate = BELL_SAMPLE_RATE;

	/* If not configured, we can't beep. */
	if (audiobellopen(audio, &bellarg) != 0)
		return;

	file = bellarg.file;
	ptrack = file->ptrack;

	/* msec to sample count. */
	remaincount = period * BELL_SAMPLE_RATE / 1000;
	remainlen = remaincount * sizeof(int16_t);

	wave1count = BELL_SAMPLE_RATE / pitch;
	wave1len = wave1count * sizeof(int16_t);

	buf = malloc(wave1len, M_TEMP, M_WAITOK);
	if (buf == NULL)
		goto out;

	/* Generate single square wave.  It's enough to beep. */
	vol = 32767 * volume / 100;
	for (i = 0; i < wave1count / 2; i++) {
		buf[i] = vol;
	}
	vol = -vol;
	for (; i < wave1count; i++) {
		buf[i] = vol;
	}

	/* Write while paused to avoid begin inserted silence. */
	ptrack->is_pause = true;
	for (; remainlen > 0; remainlen -= wave1len) {
		len = uimin(remainlen, wave1len);
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
