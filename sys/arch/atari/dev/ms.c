/*	$NetBSD: ms.c,v 1.26 2014/07/25 08:10:32 dholland Exp $	*/

/*
 * Copyright (c) 1995 Leo Weppelman.
 * All rights reserved.
 *
 * based on:
 *
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * All advertising materials mentioning features or use of this software
 * must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Lawrence Berkeley Laboratory.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)ms.c	8.1 (Berkeley) 6/11/93
 *
 * Header: ms.c,v 1.5 92/11/26 01:28:47 torek Exp  (LBL)
 */

/*
 * Mouse driver.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ms.c,v 1.26 2014/07/25 08:10:32 dholland Exp $");

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/tty.h>
#include <sys/signalvar.h>

#include <machine/msioctl.h>
#include <atari/dev/event_var.h>
#include <atari/dev/vuid_event.h>
#include <atari/dev/kbdvar.h>
#include <atari/dev/msvar.h>

#include "mouse.h"
#if NMOUSE > 0

/* there's really no more physical ports on an atari. */
#if NMOUSE > 1
#undef NMOUSE
#define NMOUSE 1
#endif

typedef void	(*FPV)(void *);

static struct ms_softc	ms_softc[NMOUSE];

dev_type_open(msopen);
dev_type_close(msclose);
dev_type_read(msread);
dev_type_ioctl(msioctl);
dev_type_poll(mspoll);
dev_type_kqfilter(mskqfilter);

const struct cdevsw ms_cdevsw = {
	.d_open = msopen,
	.d_close = msclose,
	.d_read = msread,
	.d_write = nowrite,
	.d_ioctl = msioctl,
	.d_stop = nostop,
	.d_tty = notty,
	.d_poll = mspoll,
	.d_mmap = nommap,
	.d_kqfilter = mskqfilter,
	.d_discard = nodiscard,
	.d_flag = 0
};

static	void	ms_3b_delay(struct ms_softc *);

int
mouseattach(int cnt)
{
	printf("1 mouse configured\n");
	ms_softc[0].ms_emul3b = 1;
	callout_init(&ms_softc[0].ms_delay_ch, 0);
	return(NMOUSE);
}

static void
ms_3b_delay(struct ms_softc *ms)
{
	REL_MOUSE	rel_ms;

	rel_ms.id = TIMEOUT_ID;
	rel_ms.dx = rel_ms.dy = 0;
	mouse_soft(&rel_ms, sizeof(rel_ms), KBD_TIMEO_PKG);
}
/* 
 * Note that we are called from the keyboard software interrupt!
 */
void
mouse_soft(REL_MOUSE *rel_ms, int size, int type)
{
	struct ms_softc		*ms = &ms_softc[0];
	struct firm_event	*fe, *fe2;
	REL_MOUSE		fake_mouse;
	int			get, put;
	int			sps;
	u_char			mbut, bmask;
	int			flush_buttons;

	if (ms->ms_events.ev_io == NULL)
		return;

	switch (type) {
	    case KBD_JOY1_PKG:
		/*
		 * Ignore if in emulation mode
		 */
		if (ms->ms_emul3b)
			return;

		/*
		 * There are some mice that have their middle button
		 * wired to the 'up' bit of joystick 1....
		 * Simulate a mouse packet with dx = dy = 0, the middle
		 * button state set by UP and the other buttons unchanged.
		 * Flush all button changes.
		 */
		flush_buttons = 1;
		fake_mouse.id = (rel_ms->dx & 1 ? 4 : 0) | (ms->ms_buttons & 3);
		fake_mouse.dx = fake_mouse.dy = 0;
		rel_ms = &fake_mouse;
		break;
	    case KBD_TIMEO_PKG:
		/*
		 * Timeout package. No button changes and no movement.
		 * Flush all button changes.
		 */
		flush_buttons = 1;
		fake_mouse.id = ms->ms_buttons;
		fake_mouse.dx = fake_mouse.dy = 0;
		rel_ms = &fake_mouse;
		break;
	    case KBD_RMS_PKG:
		/*
		 * Normal mouse package. Always copy the middle button
		 * status. The emulation code decides if button changes
		 * must be flushed.
		 */
		rel_ms->id = (ms->ms_buttons & 4) | (rel_ms->id & 3);
		flush_buttons = (ms->ms_emul3b) ? 0 : 1;
		break;
	    default:
		return;
	}

	sps = splev();
	get = ms->ms_events.ev_get;
	put = ms->ms_events.ev_put;
	fe  = &ms->ms_events.ev_q[put];

	if ((type != KBD_TIMEO_PKG) && ms->ms_emul3b && ms->ms_bq_idx)
		callout_stop(&ms->ms_delay_ch);

	/*
	 * Button states are encoded in the lower 3 bits of 'id'
	 */
	if (!(mbut = (rel_ms->id ^ ms->ms_buttons)) && (put != get)) {
		/*
		 * Compact dx/dy messages. Always generate an event when
		 * a button is pressed or the event queue is empty.
		 */
		ms->ms_dx += rel_ms->dx;
		ms->ms_dy += rel_ms->dy;
		goto out;
	}
	rel_ms->dx += ms->ms_dx;
	rel_ms->dy += ms->ms_dy;
	ms->ms_dx = ms->ms_dy = 0;

	/*
	 * Output location events _before_ button events ie. make sure
	 * the button is pressed at the correct location.
	 */
	if (rel_ms->dx) {
		if ((++put) % EV_QSIZE == get) {
			put--;
			goto out;
		}
		fe->id    = LOC_X_DELTA;
		fe->value = rel_ms->dx;
		firm_gettime(fe);
		if (put >= EV_QSIZE) {
			put = 0;
			fe  = &ms->ms_events.ev_q[0];
		}
		else fe++;
	}
	if (rel_ms->dy) {
		if ((++put) % EV_QSIZE == get) {
			put--;
			goto out;
		}
		fe->id    = LOC_Y_DELTA;
		fe->value = rel_ms->dy;
		firm_gettime(fe);
		if (put >= EV_QSIZE) {
			put = 0;
			fe  = &ms->ms_events.ev_q[0];
		}
		else fe++;
	}
	if (mbut && (type != KBD_TIMEO_PKG)) {
		for (bmask = 1; bmask < 0x08; bmask <<= 1) {
			if (!(mbut & bmask))
				continue;
			fe2 = &ms->ms_bq[ms->ms_bq_idx++];
			if (bmask == 1)
				fe2->id = MS_RIGHT;
			else if (bmask == 2)
				fe2->id = MS_LEFT;
			else fe2->id = MS_MIDDLE;
			fe2->value = rel_ms->id & bmask ? VKEY_DOWN : VKEY_UP;
			firm_gettime(fe2);
		}
	}

	/*
	 * Handle 3rd button emulation.
	 */
	if (ms->ms_emul3b && ms->ms_bq_idx && (type != KBD_TIMEO_PKG)) {
		/*
		 * If the middle button is pressed, any change to
		 * one of the other buttons releases all.
		 */
		if ((ms->ms_buttons & 4) && (mbut & 3)) {
			ms->ms_bq[0].id = MS_MIDDLE;
			ms->ms_bq_idx   = 1;
			rel_ms->id      = 0;
			flush_buttons   = 1;
			goto out;
		}
	    	if (ms->ms_bq_idx == 2) {
			if (ms->ms_bq[0].value == ms->ms_bq[1].value) {
				/* Must be 2 button presses! */
				ms->ms_bq[0].id = MS_MIDDLE;
				ms->ms_bq_idx   = 1;
				rel_ms->id      = 7;
			}
		}
		else if (ms->ms_bq[0].value == VKEY_DOWN) {
			callout_reset(&ms->ms_delay_ch, 10,
			    (FPV)ms_3b_delay, (void *)ms);
			goto out;
		}
		flush_buttons   = 1;
	}
out:
	if (flush_buttons) {
		int	i;

		for (i = 0; i < ms->ms_bq_idx; i++) {
			if ((++put) % EV_QSIZE == get) {
				ms->ms_bq_idx = 0;
				put--;
				goto out;
			}
			*fe = ms->ms_bq[i];
			if (put >= EV_QSIZE) {
				put = 0;
				fe  = &ms->ms_events.ev_q[0];
			}
			else fe++;
		}
		ms->ms_bq_idx = 0;
	}
	ms->ms_events.ev_put = put;
	ms->ms_buttons       = rel_ms->id;
	splx(sps);
	EV_WAKEUP(&ms->ms_events);
}

int
msopen(dev_t dev, int flags, int mode, struct lwp *l)
{
	u_char		report_ms_joy[] = { 0x14, 0x08 };
	struct ms_softc	*ms;
	int		unit;

	unit = minor(dev);
	ms   = &ms_softc[unit];

	if (unit >= NMOUSE)
		return(EXDEV);

	if (ms->ms_events.ev_io)
		return(EBUSY);

	ms->ms_events.ev_io = l->l_proc;
	ms->ms_dx = ms->ms_dy = 0;
	ms->ms_buttons = 0;
	ms->ms_bq[0].id = ms->ms_bq[1].id = 0;
	ms->ms_bq_idx = 0;
	ev_init(&ms->ms_events);	/* may cause sleep */

	/*
	 * Enable mouse reporting.
	 */
	kbd_write(report_ms_joy, sizeof(report_ms_joy));
	return(0);
}

int
msclose(dev_t dev, int flags, int mode, struct lwp *l)
{
	u_char		disable_ms_joy[] = { 0x12, 0x1a };
	int		unit;
	struct ms_softc	*ms;

	unit = minor (dev);
	ms   = &ms_softc[unit];

	/*
	 * Turn off mouse interrogation.
	 */
	kbd_write(disable_ms_joy, sizeof(disable_ms_joy));
	ev_fini(&ms->ms_events);
	ms->ms_events.ev_io = NULL;
	return(0);
}

int
msread(dev_t dev, struct uio *uio, int flags)
{
	struct ms_softc *ms;

	ms = &ms_softc[minor(dev)];
	return(ev_read(&ms->ms_events, uio, flags));
}

int
msioctl(dev_t dev, u_long cmd, register void * data, int flag, struct lwp *l)
{
	struct ms_softc *ms;
	int		unit;

	unit = minor(dev);
	ms = &ms_softc[unit];

	switch (cmd) {
	case  MIOCS3B_EMUL:
		ms->ms_emul3b = (*(int *)data != 0) ? 1 : 0;
		return (0);
	case  MIOCG3B_EMUL:
		*(int *)data = ms->ms_emul3b;
		return (0);
	case FIONBIO:		/* we will remove this someday (soon???) */
		return(0);
	case FIOASYNC:
		ms->ms_events.ev_async = *(int *)data != 0;
		return(0);
	case FIOSETOWN:
		if (-*(int *)data != ms->ms_events.ev_io->p_pgid
		    && *(int *)data != ms->ms_events.ev_io->p_pid)
			return(EPERM);
		return(0);
	case TIOCSPGRP:
		if (*(int *)data != ms->ms_events.ev_io->p_pgid)
			return(EPERM);
		return(0);
	case VUIDGFORMAT:	/* we only do firm_events */
		*(int *)data = VUID_FIRM_EVENT;
		return(0);
	case VUIDSFORMAT:
		if (*(int *)data != VUID_FIRM_EVENT)
			return(EINVAL);
		return(0);
	}
	return(ENOTTY);
}

int
mspoll(dev_t dev, int events, struct lwp *l)
{
	struct ms_softc *ms;

	ms = &ms_softc[minor(dev)];
	return(ev_poll(&ms->ms_events, events, l));
}

int
mskqfilter(dev_t dev, struct knote *kn)
{
	struct ms_softc *ms;

	ms = &ms_softc[minor(dev)];
	return (ev_kqfilter(&ms->ms_events, kn));
}
#endif /* NMOUSE > 0 */
