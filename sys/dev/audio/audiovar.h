/*	$NetBSD: audiovar.h,v 1.2 2019/05/08 13:40:17 isaki Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by TAMURA Kent
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
 *	From: Header: audiovar.h,v 1.3 93/07/18 14:07:25 mccanne Exp  (LBL)
 */

#ifndef _SYS_DEV_AUDIO_AUDIOVAR_H_
#define _SYS_DEV_AUDIO_AUDIOVAR_H_

#if defined(_KERNEL)
#include <sys/condvar.h>
#include <sys/proc.h>
#include <sys/queue.h>

#include <dev/audio/audio_if.h>
#include <dev/audio/audiofil.h>
#else
#include <stdint.h>
#include <stdbool.h>
#include "compat.h"
#include "userland.h"
#include "audiofil.h"
#endif /* _KERNEL */

/*
 * Whether supports [US]LINEAR24/24 as userland format.
 */
/* #define AUDIO_SUPPORT_LINEAR24 */

/*
 * Frequency range.
 * For lower limit, there are some antique machines who supports under
 * 4000Hz, so that we accept 1000Hz as lower limit, regardless of
 * practicality(?).
 * For upper limit, there are some devices who supports 384000Hz, but
 * I don't have them. :-)
 */
#define AUDIO_MIN_FREQUENCY (1000)
#define AUDIO_MAX_FREQUENCY (192000)

typedef struct audio_file audio_file_t;
typedef struct audio_trackmixer audio_trackmixer_t;

/* ring buffer */
typedef struct {
	audio_format2_t fmt;	/* format */
	int  capacity;		/* capacity by frame */
	int  head;		/* head position in frame */
	int  used;		/* used frame count */
	void *mem;		/* sample ptr */
} audio_ring_t;

#define AUDIO_N_PORTS 4

struct au_mixer_ports {
	int	index;		/* index of port-selector mixerctl */
	int	master;		/* index of master mixerctl */
	int	nports;		/* number of selectable ports */
	bool	isenum;		/* selector is enum type */
	u_int	allports;	/* all aumasks or'd */
	u_int	aumask[AUDIO_N_PORTS];	/* exposed value of "ports" */
	int	misel [AUDIO_N_PORTS];	/* ord of port, for selector */
	int	miport[AUDIO_N_PORTS];	/* index of port's mixerctl */
	bool	isdual;		/* has working mixerout */
	int	mixerout;	/* ord of mixerout, for dual case */
	int	cur_port;	/* the port that gain actually controls when
				   mixerout is selected, for dual case */
};

struct audio_softc {
	/* Myself (e.g.; audio0, audio1, ...) */
	device_t	sc_dev;

	/* Hardware device struct (e.g.; sb0, hdafg0, ...) */
	device_t	hw_dev;

	/*
	 * Hardware interface and driver handle.
	 * hw_if == NULL means that the device is (attached but) disabled.
	 */
	const struct audio_hw_if *hw_if;
	void		*hw_hdl;

	/*
	 * List of opened descriptors.
	 * Must be protected by sc_intr_lock.
	 */
	SLIST_HEAD(, audio_file) sc_files;

	/*
	 * Blocksize in msec.
	 * Must be protected by sc_lock.
	 */
	int sc_blk_ms;

	/*
	 * Track mixer for playback and recording.
	 * If null, the mixer is disabled.
	 */
	audio_trackmixer_t *sc_pmixer;
	audio_trackmixer_t *sc_rmixer;

	/*
	 * Opening track counter.
	 * Must be protected by sc_lock.
	 */
	int sc_popens;
	int sc_ropens;

	/*
	 * true if the track mixer is running.
	 * Must be protected by sc_lock.
	 */
	bool sc_pbusy;
	bool sc_rbusy;

	/*
	 * These four are the parameters sustained with /dev/sound.
	 * Must be protected by sc_lock.
	 */
	audio_format2_t sc_sound_pparams;
	audio_format2_t sc_sound_rparams;
	bool 		sc_sound_ppause;
	bool		sc_sound_rpause;

	/* recent info for /dev/sound */
	/* XXX TODO */
	struct audio_info sc_ai;

	/*
	 * Playback(write)/Recording(read) selector.
	 * Must be protected by sc_lock.
	 */
	struct selinfo sc_wsel;
	struct selinfo sc_rsel;

	/*
	 * processes who want mixer SIGIO.
	 * Must be protected by sc_lock.
	 */
	struct	mixer_asyncs {
		struct mixer_asyncs *next;
		pid_t	pid;
	} *sc_async_mixer;

	/*
	 * Thread lock and interrupt lock obtained by get_locks().
	 */
	kmutex_t *sc_lock;
	kmutex_t *sc_intr_lock;

	/*
	 * Critical section.
	 * Must be protected by sc_lock.
	 */
	int sc_exlock;
	kcondvar_t sc_exlockcv;

	/*
	 * Must be protected by sc_lock (?)
	 */
	bool		sc_dying;

	/*
	 * If multiuser is false, other users who have different euid
	 * than the first user cannot open this device.
	 * Must be protected by sc_lock.
	 */
	bool sc_multiuser;
	kauth_cred_t sc_cred;

	struct sysctllog *sc_log;

	mixer_ctrl_t	*sc_mixer_state;
	int		sc_nmixer_states;
	struct au_mixer_ports sc_inports;
	struct au_mixer_ports sc_outports;
	int		sc_monitor_port;
	u_int	sc_lastgain;
};

#ifdef DIAGNOSTIC
#define DIAGNOSTIC_filter_arg(arg) audio_diagnostic_filter_arg(__func__, (arg))
#define DIAGNOSTIC_format2(fmt)	audio_diagnostic_format2(__func__, (fmt))
extern void audio_diagnostic_filter_arg(const char *,
	const audio_filter_arg_t *);
extern void audio_diagnostic_format2(const char *, const audio_format2_t *);
#else
#define DIAGNOSTIC_filter_arg(arg)
#define DIAGNOSTIC_format2(fmt)
#endif

/*
 * Return true if 'fmt' is the internal format.
 * It does not check for frequency and number of channels.
 */
static __inline bool
audio_format2_is_internal(const audio_format2_t *fmt)
{

	if (fmt->encoding != AUDIO_ENCODING_SLINEAR_NE)
		return false;
	if (fmt->precision != AUDIO_INTERNAL_BITS)
		return false;
	if (fmt->stride != AUDIO_INTERNAL_BITS)
		return false;
	return true;
}

/*
 * Return true if fmt's encoding is one of LINEAR.
 */
static __inline bool
audio_format2_is_linear(const audio_format2_t *fmt)
{
	return (fmt->encoding == AUDIO_ENCODING_SLINEAR_LE)
	    || (fmt->encoding == AUDIO_ENCODING_SLINEAR_BE)
	    || (fmt->encoding == AUDIO_ENCODING_ULINEAR_LE)
	    || (fmt->encoding == AUDIO_ENCODING_ULINEAR_BE);
}

/*
 * Return true if fmt's encoding is one of SLINEAR.
 */
static __inline bool
audio_format2_is_signed(const audio_format2_t *fmt)
{
	return (fmt->encoding == AUDIO_ENCODING_SLINEAR_LE)
	    || (fmt->encoding == AUDIO_ENCODING_SLINEAR_BE);
}

/*
 * Return fmt's endian as LITTLE_ENDIAN or BIG_ENDIAN.
 */
static __inline int
audio_format2_endian(const audio_format2_t *fmt)
{
	if (fmt->stride == 8) {
		/* HOST ENDIAN */
		return BYTE_ORDER;
	}

	if (fmt->encoding == AUDIO_ENCODING_SLINEAR_LE ||
	    fmt->encoding == AUDIO_ENCODING_ULINEAR_LE) {
		return LITTLE_ENDIAN;
	}
	if (fmt->encoding == AUDIO_ENCODING_SLINEAR_BE ||
	    fmt->encoding == AUDIO_ENCODING_ULINEAR_BE) {
		return BIG_ENDIAN;
	}
	return BYTE_ORDER;
}

/* Interfaces for audiobell. */
struct audiobell_arg {
	u_int sample_rate;	/* IN */
	u_int encoding;		/* IN */
	u_int channels;		/* IN */
	u_int precision;	/* IN */
	u_int blocksize;	/* OUT */
	audio_file_t *file;	/* OUT */
};
int audiobellopen(dev_t, struct audiobell_arg *);
int audiobellclose(audio_file_t *);
int audiobellwrite(audio_file_t *, struct uio *);

#endif /* !_SYS_DEV_AUDIO_AUDIOVAR_H_ */
