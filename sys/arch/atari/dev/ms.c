/*	$NetBSD: ms.c,v 1.2 1995/06/26 14:31:29 leo Exp $

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/systm.h>
#include <sys/tty.h>

#include <atari/dev/event_var.h>
#include <atari/dev/vuid_event.h>

#include "mouse.h"
#if NMOUSE > 0

/* there's really no more physical ports on an atari. */
#if NMOUSE > 1
#undef NMOUSE
#define NMOUSE 1
#endif

int
mouseattach(cnt)
	int cnt;
{
	printf("1 mouse configured\n");
	return(NMOUSE);
}

/*
 * Mouse specific packages produced by the keyboard. Currently, we only
 * define the REL_MOUSE package, as this is the only one used.
 */
typedef struct {
	u_char	id;
	char	dx;
	char	dy;
} REL_MOUSE;

#define IS_REL_MOUSE(id)	(((u_int)(id) & 0xF8) == 0xF8)

static struct ms_softc {
	u_char		ms_buttons; /* button states		*/
	struct	evvar	ms_events;  /* event queue state	*/
	int		ms_dx;	    /* accumulated dx		*/
	int		ms_dy;	    /* accumulated dy		*/
} ms_softc[NMOUSE];

/* 
 * Note that we are called from the keyboard software interrupt!
 */
void
mouse_soft(rel_ms, size)
REL_MOUSE	*rel_ms;
int		size;
{
	struct ms_softc		*ms = &ms_softc[0];
	struct firm_event	*fe;
	int			get, put;
	int			sps;
	u_char			mbut, bmask;

	if (!IS_REL_MOUSE(rel_ms->id))
		return;	/* Probably some other message */
	if (ms->ms_events.ev_io == NULL)
		return;

	sps = splev();
	get = ms->ms_events.ev_get;
	put = ms->ms_events.ev_put;
	fe  = &ms->ms_events.ev_q[put];

	/*
	 * Button states are encoded in the lower 2 bits of 'id'
	 */
	if (!(mbut = ((rel_ms->id & 3) ^ ms->ms_buttons)) && (put != get)) {
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
		fe->time  = time;
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
		fe->time  = time;
		if (put >= EV_QSIZE) {
			put = 0;
			fe  = &ms->ms_events.ev_q[0];
		}
		else fe++;
	}
	if (mbut) {
		for (bmask = 1; bmask < 0x04; bmask <<= 1) {
			if (!(mbut & bmask))
				continue;
			if ((++put) % EV_QSIZE == get) {
				put--;
				goto out;
			}
			fe->id    = bmask & 1 ? MS_RIGHT : MS_LEFT;
			fe->value = rel_ms->id & bmask ? VKEY_DOWN : VKEY_UP;
			fe->time  = time;
			if (put >= EV_QSIZE) {
				put = 0;
				fe  = &ms->ms_events.ev_q[0];
			}
			else fe++;
		}
	}
out:
	ms->ms_events.ev_put = put;
	ms->ms_buttons       = rel_ms->id & 3;
	splx(sps);
	EV_WAKEUP(&ms->ms_events);
}

int
msopen(dev, flags, mode, p)
dev_t		dev;
int		flags, mode;
struct proc	*p;
{
	u_char		report_ms[] = { 0x08 };
	struct ms_softc	*ms;
	int		unit;

	unit = minor(dev);
	ms   = &ms_softc[unit];

	if (unit >= NMOUSE)
		return(EXDEV);

	if (ms->ms_events.ev_io)
		return(EBUSY);

	ms->ms_events.ev_io = p;
	ms->ms_dx = ms->ms_dy = 0;
	ev_init(&ms->ms_events);	/* may cause sleep */

	/*
	 * Enable mouse reporting.
	 */
	kbd_write(report_ms, sizeof(report_ms));
	return(0);
}

int
msclose(dev, flags, mode, p)
dev_t		dev;
int		flags, mode;
struct proc	*p;
{
	u_char		disable_ms[] = { 0x12 };
	int		unit;
	struct ms_softc	*ms;

	unit = minor (dev);
	ms   = &ms_softc[unit];

	/*
	 * Turn off mouse interrogation.
	 */
	kbd_write(disable_ms, sizeof(disable_ms));
	ev_fini(&ms->ms_events);
	ms->ms_events.ev_io = NULL;
	return(0);
}

int
msread(dev, uio, flags)
dev_t		dev;
struct uio	*uio;
int		flags;
{
	struct ms_softc *ms;

	ms = &ms_softc[minor(dev)];
	return(ev_read(&ms->ms_events, uio, flags));
}

int
msioctl(dev, cmd, data, flag, p)
dev_t			dev;
u_long			cmd;
register caddr_t 	data;
int			flag;
struct proc		*p;
{
	struct ms_softc *ms;
	int		unit;

	unit = minor(dev);
	ms = &ms_softc[unit];

	switch (cmd) {
	case FIONBIO:		/* we will remove this someday (soon???) */
		return(0);
	case FIOASYNC:
		ms->ms_events.ev_async = *(int *)data != 0;
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
msselect(dev, rw, p)
dev_t		dev;
int		rw;
struct proc	*p;
{
	struct ms_softc *ms;

	ms = &ms_softc[minor(dev)];
	return(ev_select(&ms->ms_events, rw, p));
}
#endif /* NMOUSE > 0 */
