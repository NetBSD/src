/*	$NetBSD: ms.c,v 1.18.12.1 2002/02/28 04:06:53 nathanw Exp $ */

/*
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ms.c,v 1.18.12.1 2002/02/28 04:06:53 nathanw Exp $");

/*
 * Mouse driver.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/tty.h>
#include <sys/signalvar.h>

#include <amiga/dev/event_var.h>
#include <amiga/dev/vuid_event.h>

#include <amiga/amiga/custom.h>
#include <amiga/amiga/cia.h>
#include <amiga/amiga/device.h>

#include <sys/conf.h>
#include <machine/conf.h>

void msattach(struct device *, struct device *, void *);
int msmatch(struct device *, struct cfdata *, void *);

/* per-port state */
struct ms_port {
	int	ms_portno;	   /* which hardware port, for msintr() */

	struct callout ms_intr_ch;

	u_char	ms_horc;	   /* horizontal counter on last scan */
  	u_char	ms_verc;	   /* vertical counter on last scan */
	char	ms_mb;		   /* mouse button state */
	char	ms_ub;		   /* user button state */
	int	ms_dx;		   /* delta-x */
	int	ms_dy;		   /* delta-y */
	volatile int ms_ready;	   /* event queue is ready */
	struct	evvar ms_events;   /* event queue state */
};

#define	MS_NPORTS	2

struct ms_softc {
	struct device sc_dev;		/* base device */
	struct ms_port sc_ports[MS_NPORTS];
};

struct cfattach ms_ca = {
	sizeof(struct ms_softc), msmatch, msattach
};

void msintr(void *);
void ms_enable(struct ms_port *);
void ms_disable(struct ms_port *);

extern struct cfdriver ms_cd;

#define	MS_UNIT(d)	((minor(d) & ~0x1) >> 1)
#define	MS_PORT(d)	(minor(d) & 0x1)

/*
 * Given a dev_t, return a pointer to the port's hardware state.
 * Assumes the unit to be valid, so do *not* utilize this in msopen().
 */
#define	MS_DEV2MSPORT(d) \
    (&(((struct ms_softc *)getsoftc(ms_cd, MS_UNIT(d)))->sc_ports[MS_PORT(d)]))

int
msmatch(struct device *pdp, struct cfdata *cfp, void *auxp)
{
	static int ms_matched = 0;

	/* Allow only one instance. */
	if (!matchname((char *)auxp, "ms") || ms_matched)
		return 0;

	ms_matched = 1;
	return 1;
}

void
msattach(struct device *pdp, struct device *dp, void *auxp)
{
	struct ms_softc *sc = (void *) dp;
	int i;

	printf("\n");
	for (i = 0; i < MS_NPORTS; i++) {
		sc->sc_ports[i].ms_portno = i;
		callout_init(&sc->sc_ports[i].ms_intr_ch);
	}
}

/*
 * Amiga mice are hooked up to one of the two "game" ports, where
 * the main mouse is usually on the first port, and port 2 can
 * be used by a joystick. Nevertheless, we support two mouse
 * devices, /dev/mouse0 and /dev/mouse1 (with a link of /dev/mouse to
 * the device that represents the port of the mouse in use).
 */

/*
 * enable scanner, called when someone opens the port.
 */
void
ms_enable(struct ms_port *ms)
{

	/*
	 * use this as flag to the "interrupt" to tell it when to
	 * shut off (when it's reset to 0).
	 */
	ms->ms_ready = 1;

	callout_reset(&ms->ms_intr_ch, 2, msintr, ms);
}

/*
 * disable scanner. Just set ms_ready to 0, and after the next
 * timeout taken, no further timeouts will be initiated.
 */
void
ms_disable(struct ms_port *ms)
{
	int s;

	s = splhigh ();
	ms->ms_ready = 0;
	/*
	 * sync with the interrupt
	 */
	tsleep(ms, PZERO - 1, "mouse-disable", 0);
	splx(s);
}


/*
 * we're emulating a mousesystems serial mouse here..
 */
void
msintr(void *arg)
{
	static const char to_one[] = { 1, 2, 2, 4, 4, 4, 4 };
	static const int to_id[] = { MS_RIGHT, MS_MIDDLE, 0, MS_LEFT };
	struct ms_port *ms = arg;
	struct firm_event *fe;
	int mb, ub, d, get, put, any, port;
	u_char pra, *horc, *verc;
	u_short pot, count;
	short dx, dy;

	port = ms->ms_portno;

	horc = ((u_char *) &count) + 1;
	verc = (u_char *) &count;

	/*
	 * first read the three buttons.
	 */
	pot  = custom.potgor;
	pra  = ciaa.pra;
	pot >>= port == 0 ? 8 : 12;	/* contains right and middle button */
	pra >>= port == 0 ? 6 : 7;	/* contains left button */
	mb = (pot & 4) / 4 + (pot & 1) * 2 + (pra & 1) * 4;
	mb ^= 0x07;

	/*
	 * read current values of counter registers
	 */
	if (port == 0)
		count = custom.joy0dat;
	else
		count = custom.joy1dat;

	/*
	 * take care of wraparound
	 */
	dx = *horc - ms->ms_horc;
	if (dx < -127)
		dx += 255;
	else if (dx > 127)
		dx -= 255;
	dy = *verc - ms->ms_verc;
	if (dy < -127)
		dy += 255;
	else if (dy > 127)
		dy -= 255;

	/*
	 * remember current values for next scan
	 */
	ms->ms_horc = *horc;
	ms->ms_verc = *verc;

	ms->ms_dx = dx;
	ms->ms_dy = dy;
	ms->ms_mb = mb;

	if (dx || dy || ms->ms_ub != ms->ms_mb) {
		/*
		 * We have at least one event (mouse button, delta-X, or
		 * delta-Y; possibly all three, and possibly three separate
		 * button events).  Deliver these events until we are out of
		 * changes or out of room.  As events get delivered, mark them
		 * `unchanged'.
		 */
		any = 0;
		get = ms->ms_events.ev_get;
		put = ms->ms_events.ev_put;
		fe = &ms->ms_events.ev_q[put];

		mb = ms->ms_mb;
		ub = ms->ms_ub;
		while ((d = mb ^ ub) != 0) {
			/*
			 * Mouse button change.  Convert up to three changes
			 * to the `first' change, and drop it into the event
			 * queue.
			 */
			if ((++put) % EV_QSIZE == get) {
				put--;
				goto out;
			}

			d = to_one[d - 1];	/* from 1..7 to {1,2,4} */
			fe->id = to_id[d - 1];	/* from {1,2,4} to ID */
			fe->value = mb & d ? VKEY_DOWN : VKEY_UP;
			fe->time = time;
			fe++;

			if (put >= EV_QSIZE) {
				put = 0;
				fe = &ms->ms_events.ev_q[0];
			}
			any = 1;

			ub ^= d;
		}
		if (ms->ms_dx) {
			if ((++put) % EV_QSIZE == get) {
				put--;
				goto out;
			}

			fe->id = LOC_X_DELTA;
			fe->value = ms->ms_dx;
			fe->time = time;
			fe++;

			if (put >= EV_QSIZE) {
				put = 0;
				fe = &ms->ms_events.ev_q[0];
			}
			any = 1;

			ms->ms_dx = 0;
		}
		if (ms->ms_dy) {
			if ((++put) % EV_QSIZE == get) {
				put--;
				goto out;
			}

			fe->id = LOC_Y_DELTA;
			fe->value = ms->ms_dy;
			fe->time = time;
			fe++;

			if (put >= EV_QSIZE) {
				put = 0;
				fe = &ms->ms_events.ev_q[0];
			}
			any = 1;

			ms->ms_dy = 0;
		}
out:
		if (any) {
			ms->ms_ub = ub;
			ms->ms_events.ev_put = put;
			EV_WAKEUP(&ms->ms_events);
		}
	}

	/*
	 * reschedule handler, or if terminating,
	 * handshake with ms_disable
	 */
	if (ms->ms_ready)
		callout_reset(&ms->ms_intr_ch, 2, msintr, ms);
	else
		wakeup(ms);
}

int
msopen(dev_t dev, int flags, int mode, struct proc *p)
{
	struct ms_softc *sc;
	struct ms_port *ms;
	int unit, port;

	unit = MS_UNIT(dev);
	sc = (struct ms_softc *)getsoftc(ms_cd, unit);

	if (sc == NULL)
		return(EXDEV);

	port = MS_PORT(dev);
	ms = &sc->sc_ports[port];

	if (ms->ms_events.ev_io)
		return(EBUSY);

	/* initialize potgo bits for mouse mode */
	custom.potgo = custom.potgor | (0xf00 << (port * 4));

	ms->ms_events.ev_io = p;
	ev_init(&ms->ms_events);	/* may cause sleep */
	ms_enable(ms);
	return(0);
}

int
msclose(dev_t dev, int flags, int mode, struct proc *p)
{
	struct ms_port *ms;

	ms = MS_DEV2MSPORT(dev);

	ms_disable(ms);
	ev_fini(&ms->ms_events);
	ms->ms_events.ev_io = NULL;
	return(0);
}

int
msread(dev_t dev, struct uio *uio, int flags)
{
	struct ms_port *ms;

	ms = MS_DEV2MSPORT(dev);

	return(ev_read(&ms->ms_events, uio, flags));
}

int
msioctl(dev_t dev, u_long cmd, register caddr_t data, int flag,
        struct proc *p)
{
	struct ms_port *ms;

	ms = MS_DEV2MSPORT(dev);

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
mspoll(dev_t dev, int events, struct proc *p)
{
	struct ms_port *ms;

	ms = MS_DEV2MSPORT(dev);

	return(ev_poll(&ms->ms_events, events, p));
}
