/*	$NetBSD: bsd_audioio.h,v 1.2 1998/01/05 07:03:46 perry Exp $	*/

/*
 * Copyright (c) 1991-1993 Regents of the University of California.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef _BSD_AUDIOIO_H_
#define _BSD_AUDIOIO_H_

/*
 * Audio device
 */
struct audio_prinfo {
	u_int	sample_rate;
	u_int	channels;
	u_int	precision;
	u_int	encoding;
	u_int	gain;
	u_int	port;
	u_long	seek;		/* BSD extension */
	u_int	ispare[3];
	/* Current state of device: */
	u_int	samples;	/* number of samples */
	u_int	eof;		/* End Of File (zero-size writes) counter */
	u_char	pause;		/* non-zero if paused, zero to resume */
	u_char	error;		/* non-zero if underflow/overflow ocurred */
	u_char	waiting;	/* non-zero if another process hangs in open */
	u_char	cspare[3];
	u_char	open;		/* non-zero if currently open */
	u_char	active;		/* non-zero if I/O is currently active */
};
typedef struct audio_prinfo audio_prinfo_t;

struct audio_info {
	struct	audio_prinfo play;	/* Info for play (output) side */
	struct	audio_prinfo record;	/* Info for record (input) side */
	u_int	monitor_gain;
	/* BSD extensions */
	u_int	blocksize;	/* input blocking threshold */
	u_int	hiwat;		/* output high water mark */
	u_int	lowat;		/* output low water mark */
	u_int	backlog;	/* samples of output backlog to gen. */
	u_int	mode;
#define AUMODE_PLAY 0
#define AUMODE_RECORD 1
#define AUMODE_PLAY_ALL	0x04	/* play all samples--no real-time correction */
};
typedef struct audio_info audio_info_t;

#ifdef _KERNEL
#define AUDIO_INITINFO(p)\
	{ register int n = sizeof(struct audio_info); \
	  register u_char *q = (u_char *) p; \
	  while (n-- > 0) *q++ = 0xff; }

#else
#define AUDIO_INITINFO(p)\
	(void)memset((void *)(p), 0xff, sizeof(struct audio_info))
#endif

/*
 * /dev/audio ioctls.  needs comments!
 */
#define AUDIO_MIN_GAIN (0)
#define AUDIO_MAX_GAIN (255)

#define AUDIO_ENCODING_ULAW	(1)
#define AUDIO_ENCODING_ALAW	(2)
#define AUDIO_ENCODING_LINEAR	(3)
#define AUDIO_ENCODING_ADPCM	(4)

#define AUDIO_GETINFO	_IOR('A', 21, struct audio_info)
#define AUDIO_SETINFO	_IOWR('A', 22, struct audio_info)
#define AUDIO_DRAIN	_IO('A', 23)
#define AUDIO_FLUSH	_IO('A', 24)
#define AUDIO_WSEEK	_IOR('A', 25, u_long)
#define AUDIO_RERROR	_IOR('A', 26, int)
#define AUDIO_GETMAP	_IOR('A', 27, struct mapreg)
#define	AUDIO_SETMAP	_IOW('A', 28, struct mapreg)

#define AUDIO_SPEAKER   	1
#define AUDIO_HEADPHONE		2

#endif /* _BSD_AUDIOIO_H_ */
