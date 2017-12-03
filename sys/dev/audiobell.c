/*	$NetBSD: audiobell.c,v 1.8.22.1 2017/12/03 11:36:58 jdolecek Exp $	*/


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
__KERNEL_RCSID(0, "$NetBSD: audiobell.c,v 1.8.22.1 2017/12/03 11:36:58 jdolecek Exp $");

#include <sys/audioio.h>
#include <sys/conf.h>
#include <sys/device.h>
#include <sys/fcntl.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/ioctl.h>
#include <sys/malloc.h>
#include <sys/null.h>
#include <sys/systm.h>
#include <sys/uio.h>
#include <sys/unistd.h>

#include <dev/audio_if.h>
#include <dev/audiovar.h>
#include <dev/audiobellvar.h>
#include <dev/audiobelldata.h>

/* 44.1 kHz should reduce hum at higher pitches. */
#define BELL_SAMPLE_RATE	44100
#define BELL_SHIFT		3

static inline void
audiobell_expandwave(int16_t *buf)
{
	u_int i;

	for (i = 0; i < __arraycount(sinewave); i++)
		buf[i] = sinewave[i];
	for (i = __arraycount(sinewave); i < __arraycount(sinewave) * 2; i++)
		 buf[i] = buf[__arraycount(sinewave) * 2 - i - 1];
	for (i = __arraycount(sinewave) * 2; i < __arraycount(sinewave) * 4; i++)
		buf[i] = -buf[__arraycount(sinewave) * 4 - i - 1];
}

/*
 * The algorithm here is based on that described in the RISC OS Programmer's
 * Reference Manual (pp1624--1628).
 */
static inline int
audiobell_synthesize(int16_t *buf, u_int pitch, u_int period, u_int volume,
    uint16_t *phase)
{
	int16_t *wave;

	wave = malloc(sizeof(sinewave) * 4, M_TEMP, M_WAITOK);
	if (wave == NULL)
		return -1;
	audiobell_expandwave(wave);
	pitch = pitch * ((sizeof(sinewave) * 4) << BELL_SHIFT) /
	    BELL_SAMPLE_RATE / 2;
	period = period * BELL_SAMPLE_RATE / 1000 / 2;

	for (; period != 0; period--) {
		*buf++ = wave[*phase >> BELL_SHIFT];
		*phase += pitch;
	}

	free(wave, M_TEMP);
	return 0;
}

void
audiobell(void *v, u_int pitch, u_int period, u_int volume, int poll)
{
	dev_t audio;
	int16_t *buf;
	uint16_t phase;
	struct audio_info ai;
	struct uio auio;
	struct iovec aiov;
	struct file *fp;
	int size, len, fd;

	KASSERT(volume <= 100);

	fd = -1;
	fp = NULL;
	buf = NULL;
	audio = AUDIO_DEVICE | device_unit((device_t)v);

	/* The audio system isn't built for polling. */
	if (poll)
		return;

	/* If not configured, we can't beep. */
	if (audiobellopen(audio, FWRITE, 0, NULL, &fp) != EMOVEFD || fp == NULL)
		return;
	fd = curlwp->l_dupfd;	/* save the fd for closing when done */

	if (audiobellioctl(fp, AUDIO_GETINFO, &ai) != 0)
		goto out;

	AUDIO_INITINFO(&ai);
	ai.mode = AUMODE_PLAY;
	ai.play.sample_rate = BELL_SAMPLE_RATE;
	ai.play.precision = 16;
	ai.play.channels = 1;
	ai.play.gain = 255 * volume / 100;

#if BYTE_ORDER == LITTLE_ENDIAN
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
#else
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_BE;
#endif

	if (audiobellioctl(fp, AUDIO_SETINFO, &ai) != 0)
		goto out;

	if (ai.blocksize < BELL_SAMPLE_RATE)
		ai.blocksize = BELL_SAMPLE_RATE;

	len = period * BELL_SAMPLE_RATE / 1000 * 2;
	size = min(len, ai.blocksize);
	if (size == 0)
		goto out;

	buf = malloc(size, M_TEMP, M_WAITOK);
	if (buf == NULL)
		goto out;
 
	phase = 0;
	while (len > 0) {
		size = min(len, ai.blocksize);
		if (audiobell_synthesize(buf, pitch, size *
				1000 / BELL_SAMPLE_RATE, volume, &phase) != 0)
			goto out;
		aiov.iov_base = (void *)buf;
		aiov.iov_len = size;
		auio.uio_iov = &aiov;
		auio.uio_iovcnt = 1;
		auio.uio_offset = 0;
		auio.uio_resid = size;
		auio.uio_rw = UIO_WRITE;
		UIO_SETUP_SYSSPACE(&auio);

		if (audiobellwrite(fp, NULL, &auio, NULL, 0) != 0)
			break;
		len -= size;
	}
out:
	if (buf != NULL)
		free(buf, M_TEMP);
	if (fd >= 0) {
		fd_getfile(fd);
		fd_close(fd);
	}
	audiobellclose(fp);
}
