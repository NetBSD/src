/*	$NetBSD: h_pad.c,v 1.3 2019/06/20 12:14:46 isaki Exp $	*/

/*
 * Copyright (c) 2010 Antti Kantee.  All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/types.h>

#include <rump/rump.h>
#include <rump/rump_syscalls.h>

#include <err.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/audioio.h>

#include "h_pad_musa.c"

/*
 * Stuff some audio into /dev/audio, read it from /dev/pad.
 */

#define BUFSIZE 1024

static const int16_t mulaw_to_slinear16[256] = {
	0x8284, 0x8684, 0x8a84, 0x8e84, 0x9284, 0x9684, 0x9a84, 0x9e84,
	0xa284, 0xa684, 0xaa84, 0xae84, 0xb284, 0xb684, 0xba84, 0xbe84,
	0xc184, 0xc384, 0xc584, 0xc784, 0xc984, 0xcb84, 0xcd84, 0xcf84,
	0xd184, 0xd384, 0xd584, 0xd784, 0xd984, 0xdb84, 0xdd84, 0xdf84,
	0xe104, 0xe204, 0xe304, 0xe404, 0xe504, 0xe604, 0xe704, 0xe804,
	0xe904, 0xea04, 0xeb04, 0xec04, 0xed04, 0xee04, 0xef04, 0xf004,
	0xf0c4, 0xf144, 0xf1c4, 0xf244, 0xf2c4, 0xf344, 0xf3c4, 0xf444,
	0xf4c4, 0xf544, 0xf5c4, 0xf644, 0xf6c4, 0xf744, 0xf7c4, 0xf844,
	0xf8a4, 0xf8e4, 0xf924, 0xf964, 0xf9a4, 0xf9e4, 0xfa24, 0xfa64,
	0xfaa4, 0xfae4, 0xfb24, 0xfb64, 0xfba4, 0xfbe4, 0xfc24, 0xfc64,
	0xfc94, 0xfcb4, 0xfcd4, 0xfcf4, 0xfd14, 0xfd34, 0xfd54, 0xfd74,
	0xfd94, 0xfdb4, 0xfdd4, 0xfdf4, 0xfe14, 0xfe34, 0xfe54, 0xfe74,
	0xfe8c, 0xfe9c, 0xfeac, 0xfebc, 0xfecc, 0xfedc, 0xfeec, 0xfefc,
	0xff0c, 0xff1c, 0xff2c, 0xff3c, 0xff4c, 0xff5c, 0xff6c, 0xff7c,
	0xff88, 0xff90, 0xff98, 0xffa0, 0xffa8, 0xffb0, 0xffb8, 0xffc0,
	0xffc8, 0xffd0, 0xffd8, 0xffe0, 0xffe8, 0xfff0, 0xfff8, 0xfffc,
	0x7d7c, 0x797c, 0x757c, 0x717c, 0x6d7c, 0x697c, 0x657c, 0x617c,
	0x5d7c, 0x597c, 0x557c, 0x517c, 0x4d7c, 0x497c, 0x457c, 0x417c,
	0x3e7c, 0x3c7c, 0x3a7c, 0x387c, 0x367c, 0x347c, 0x327c, 0x307c,
	0x2e7c, 0x2c7c, 0x2a7c, 0x287c, 0x267c, 0x247c, 0x227c, 0x207c,
	0x1efc, 0x1dfc, 0x1cfc, 0x1bfc, 0x1afc, 0x19fc, 0x18fc, 0x17fc,
	0x16fc, 0x15fc, 0x14fc, 0x13fc, 0x12fc, 0x11fc, 0x10fc, 0x0ffc,
	0x0f3c, 0x0ebc, 0x0e3c, 0x0dbc, 0x0d3c, 0x0cbc, 0x0c3c, 0x0bbc,
	0x0b3c, 0x0abc, 0x0a3c, 0x09bc, 0x093c, 0x08bc, 0x083c, 0x07bc,
	0x075c, 0x071c, 0x06dc, 0x069c, 0x065c, 0x061c, 0x05dc, 0x059c,
	0x055c, 0x051c, 0x04dc, 0x049c, 0x045c, 0x041c, 0x03dc, 0x039c,
	0x036c, 0x034c, 0x032c, 0x030c, 0x02ec, 0x02cc, 0x02ac, 0x028c,
	0x026c, 0x024c, 0x022c, 0x020c, 0x01ec, 0x01cc, 0x01ac, 0x018c,
	0x0174, 0x0164, 0x0154, 0x0144, 0x0134, 0x0124, 0x0114, 0x0104,
	0x00f4, 0x00e4, 0x00d4, 0x00c4, 0x00b4, 0x00a4, 0x0094, 0x0084,
	0x0078, 0x0070, 0x0068, 0x0060, 0x0058, 0x0050, 0x0048, 0x0040,
	0x0038, 0x0030, 0x0028, 0x0020, 0x0018, 0x0010, 0x0008, 0x0000,
};

#define DPRINTF(n, fmt...)	do { \
	if (debug >= (n)) \
		printf(fmt);	\
} while (0)

int debug;

int
main(int argc, char *argv[])
{
	struct audio_info ai;
	int padfd, audiofd;
	ssize_t n;
	int i;
	int nframe;
	int outlen;
	uint32_t *outbuf;
	uint32_t *outp;
	int inplen;
	uint32_t *inpbuf;
	uint32_t actual;
	uint32_t expected;
	int c;
	enum {
		PRE,
		BODY,
		POST,
	} phase;

	while ((c = getopt(argc, argv, "d")) != -1) {
		switch (c) {
		case 'd':
			debug++;
			break;
		default:
			errx(1, "unknown option");
		}
	}

	/* Make input buffer (and it is also expected data). */
	inplen = sizeof(musa) * 4;
	inpbuf = (uint32_t *)malloc(inplen);
	if (inpbuf == NULL)
		err(1, "malloc: inpbuf");

	/* mulaw:mono to slinear_le16:stereo */
	for (i = 0; i < (int)sizeof(musa); i++) {
		int16_t s = mulaw_to_slinear16[musa[i]];
		uint32_t v = htole16((uint16_t)s);
		inpbuf[i] = (v << 16) | v;
	}

	outlen = BUFSIZE;
	outbuf = (uint32_t *)malloc(outlen);
	if (outbuf == NULL)
		err(1, "malloc: outbuf");

	DPRINTF(1, "init\n");
	rump_init();
	padfd = rump_sys_open("/dev/pad0", O_RDONLY);
	if (padfd == -1)
		err(1, "open pad");

	audiofd = rump_sys_open("/dev/audio0", O_RDWR);
	if (audiofd == -1)
		err(1, "open audio");

	DPRINTF(1, "ioctl\n");
	/* pad is SLINEAR_LE, 16bit, 2ch, 44100Hz. */
	AUDIO_INITINFO(&ai);
	ai.play.encoding = AUDIO_ENCODING_SLINEAR_LE;
	ai.play.precision = 16;
	ai.play.channels = 2;
	ai.play.sample_rate = 44100;
	n = rump_sys_ioctl(audiofd, AUDIO_SETINFO, &ai);
	if (n == -1)
		err(1, "ioctl");

	DPRINTF(1, "write %d\n", inplen);
	n = rump_sys_write(audiofd, inpbuf, inplen);
	if (n == -1)
		err(1, "write");
	if (n != inplen)
		errx(1, "write: n=%zd < %d", n, inplen);

	phase = PRE;
	i = 0;
	nframe = 0;
	outp = NULL;
	for (;;) {
		/* Read to outbuf when it is empty. */
		if (nframe == 0) {
			n = rump_sys_read(padfd, outbuf, outlen);
			if (n == -1)
				err(1, "read");
			if (n == 0)
				errx(1, "read: EOF");
			/* XXX Should I recover from this? */
			if (n % 4 != 0)
				errx(1, "read: n=%zd", n);

			nframe = n / 4;
			outp = outbuf;
		}

		if (phase == PRE) {
			/* Skip preceding silence part. */
			if (*outp == 0) {
				outp++;
				nframe--;
			} else {
				/* This is the first frame. */
				phase = BODY;
			}
		} else if (phase == BODY) {
			/* Compare wavedata. */
			expected = le32dec(outp);
			actual = le32dec(inpbuf + i);
			DPRINTF(2, "[%d] %08x %08x\n", i, actual, expected);
			if (actual != expected) {
				errx(1, "bad output [%d] %08x %08x",
				    i, actual, expected);
			}
			outp++;
			nframe--;
			i++;
			if (i >= (int)sizeof(musa)) {
				phase = POST;
				i = 0;
			}
		} else if (phase == POST) {
			/*
			 * There is no way to determine the end of playback.
			 * Therefore it detects and terminates with some
			 * continuous silence.
			 */
			actual = le32dec(outp);
			if (actual != 0)
				errx(1, "bad post output: %08x", actual);
			outp++;
			nframe--;
			i++;
			if (i >= (int)ai.play.sample_rate / 100)
				break;
		}
	}
	DPRINTF(1, "success\n");
	return 0;
}
