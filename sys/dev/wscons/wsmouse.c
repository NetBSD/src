/* $NetBSD: wsmouse.c,v 1.6 1999/01/10 18:22:14 augustss Exp $ */

/*
 * Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

static const char _copyright[] __attribute__ ((unused)) =
    "Copyright (c) 1996, 1997 Christopher G. Demetriou.  All rights reserved.";
static const char _rcsid[] __attribute__ ((unused)) =
    "$NetBSD: wsmouse.c,v 1.6 1999/01/10 18:22:14 augustss Exp $";

/*
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
 */

/*
 * Mouse driver.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/device.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>
#include <dev/wscons/wseventvar.h>

#include "wsmouse.h"

struct wsmouse_softc {
	struct device	sc_dv;

	const struct wsmouse_accessops *sc_accessops;
	void		*sc_accesscookie;

	int		sc_ready;	/* accepting events */
	struct wseventvar sc_events;	/* event queue state */

	u_int		sc_mb;		/* mouse button state */
	u_int		sc_ub;		/* user button state */
	int		sc_dx;		/* delta-x */
	int		sc_dy;		/* delta-y */
	int		sc_dz;		/* delta-z */
};

int	wsmouse_match __P((struct device *, struct cfdata *, void *));
void	wsmouse_attach __P((struct device *, struct device *, void *));

struct cfattach wsmouse_ca = {
	sizeof (struct wsmouse_softc), wsmouse_match, wsmouse_attach,
};

#if NWSMOUSE > 0
extern struct cfdriver wsmouse_cd;
#endif /* NWSMOUSE > 0 */

cdev_decl(wsmouse);

/*
 * Print function (for parent devices).
 */
int
wsmousedevprint(aux, pnp)
	void *aux;
	const char *pnp;
{

	if (pnp)
		printf("wsmouse at %s", pnp);
	return (UNCONF);
}

int
wsmouse_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{

	return (1);
}

void
wsmouse_attach(parent, self, aux)
        struct device *parent, *self;
	void *aux;
{
        struct wsmouse_softc *sc = (struct wsmouse_softc *)self;
	struct wsmousedev_attach_args *ap = aux;

	printf("\n");

	sc->sc_accessops = ap->accessops;
	sc->sc_accesscookie = ap->accesscookie;
	sc->sc_ready = 0;				/* sanity */
}

void
wsmouse_input(wsmousedev, btns, dx, dy, dz)
	struct device *wsmousedev;
	u_int btns;			/* 0 is up */
	int dx, dy, dz;
{
	struct wsmouse_softc *sc = (struct wsmouse_softc *)wsmousedev;
	struct wscons_event *ev;
	int mb, ub, d, get, put, any;

        /*
         * Discard input if not ready.
         */
	if (sc->sc_ready == 0)
		return;

	sc->sc_mb = btns;
	sc->sc_dx += dx;
	sc->sc_dy += dy;
	sc->sc_dz += dz;

	/*
	 * We have at least one event (mouse button, delta-X, or
	 * delta-Y; possibly all three, and possibly three separate
	 * button events).  Deliver these events until we are out
	 * of changes or out of room.  As events get delivered,
	 * mark them `unchanged'.
	 */
	any = 0;
	get = sc->sc_events.get;
	put = sc->sc_events.put;
	ev = &sc->sc_events.q[put];

	/* NEXT prepares to put the next event, backing off if necessary */
#define	NEXT								\
	if ((++put) % WSEVENT_QSIZE == get) {				\
		put--;							\
		goto out;						\
	}
	/* ADVANCE completes the `put' of the event */
#define	ADVANCE								\
	ev++;								\
	if (put >= WSEVENT_QSIZE) {					\
		put = 0;						\
		ev = &sc->sc_events.q[0];				\
	}								\
	any = 1
	/* TIMESTAMP sets `time' field of the event to the current time */
#define TIMESTAMP							\
	do {								\
		int s;							\
		s = splhigh();						\
		TIMEVAL_TO_TIMESPEC(&time, &ev->time);			\
		splx(s);						\
	} while (0)

	mb = sc->sc_mb;
	ub = sc->sc_ub;
	while ((d = mb ^ ub) != 0) {
		/*
		 * Mouse button change.  Find the first change and drop
		 * it into the event queue.
		 */
		NEXT;
		ev->value = ffs(d) - 1;

		KASSERT(ev->value >= 0);

		d = 1 << ev->value;
		ev->type =
		    (mb & d) ? WSCONS_EVENT_MOUSE_DOWN : WSCONS_EVENT_MOUSE_UP;
		TIMESTAMP;
		ADVANCE;
		ub ^= d;
	}
	if (sc->sc_dx) {
		NEXT;
		ev->type = WSCONS_EVENT_MOUSE_DELTA_X;
		ev->value = sc->sc_dx;
		TIMESTAMP;
		ADVANCE;
		sc->sc_dx = 0;
	}
	if (sc->sc_dy) {
		NEXT;
		ev->type = WSCONS_EVENT_MOUSE_DELTA_Y;
		ev->value = sc->sc_dy;
		TIMESTAMP;
		ADVANCE;
		sc->sc_dy = 0;
	}
	if (sc->sc_dz) {
		NEXT;
		ev->type = WSCONS_EVENT_MOUSE_DELTA_Z;
		ev->value = sc->sc_dz;
		TIMESTAMP;
		ADVANCE;
		sc->sc_dz = 0;
	}
out:
	if (any) {
		sc->sc_ub = ub;
		sc->sc_events.put = put;
		WSEVENT_WAKEUP(&sc->sc_events);
	}
}

int
wsmouseopen(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
#if NWSMOUSE > 0
	struct wsmouse_softc *sc;
	int error, unit;

	unit = minor(dev);
	if (unit >= wsmouse_cd.cd_ndevs ||	/* make sure it was attached */
	    (sc = wsmouse_cd.cd_devs[unit]) == NULL)
		return (ENXIO);

	if ((flags & (FREAD | FWRITE)) == FWRITE)
		return (0);			/* always allow open for write
						   so ioctl() is possible. */

	if (sc->sc_events.io)			/* and that it's not in use */
		return (EBUSY);

	sc->sc_events.io = p;
	wsevent_init(&sc->sc_events);		/* may cause sleep */

	sc->sc_ready = 1;			/* start accepting events */

	/* enable the device, and punt if that's not possible */
	error = (*sc->sc_accessops->enable)(sc->sc_accesscookie);
	if (error) {
		sc->sc_ready = 0;		/* stop accepting events */
		wsevent_fini(&sc->sc_events);
		sc->sc_events.io = NULL;
		return (error);
	}

	return (0);
#else
	return (ENXIO);
#endif /* NWSMOUSE > 0 */
}

int
wsmouseclose(dev, flags, mode, p)
	dev_t dev;
	int flags, mode;
	struct proc *p;
{
#if NWSMOUSE > 0
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];

	if ((flags & (FREAD | FWRITE)) == FWRITE)
		return (0);			/* see wsmouseopen() */

	(*sc->sc_accessops->disable)(sc->sc_accesscookie);

	sc->sc_ready = 0;			/* stop accepting events */
	wsevent_fini(&sc->sc_events);
	sc->sc_events.io = NULL;
	return (0);
#else
	return (ENXIO);
#endif /* NWSMOUSE > 0 */
}

int
wsmouseread(dev, uio, flags)
	dev_t dev;
	struct uio *uio;
	int flags;
{
#if NWSMOUSE > 0
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];

	return (wsevent_read(&sc->sc_events, uio, flags));
#else
	return (ENXIO);
#endif /* NWSMOUSE > 0 */
}

int
wsmouseioctl(dev, cmd, data, flag, p)
	dev_t dev;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
#if NWSMOUSE > 0
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];
	int error;

	/*
	 * Try the generic ioctls that the wsmouse interface supports.
	 */
	switch (cmd) {
	case FIONBIO:		/* we will remove this someday (soon???) */
		return (0);

	case FIOASYNC:
		sc->sc_events.async = *(int *)data != 0;
		return (0);

	case TIOCSPGRP:
		if (*(int *)data != sc->sc_events.io->p_pgid)
			return (EPERM);
		return (0);
	}

	/*
	 * Try the mouse driver for WSMOUSEIO ioctls.  It returns -1
	 * if it didn't recognize the request.
	 */
	error = (*sc->sc_accessops->ioctl)(sc->sc_accesscookie, cmd,
	    data, flag, p);
	return (error != -1 ? error : ENOTTY);
#else
	return (ENXIO);
#endif /* NWSMOUSE > 0 */
}

int
wsmousepoll(dev, events, p)
	dev_t dev;
	int events;
	struct proc *p;
{
#if NWSMOUSE > 0
	struct wsmouse_softc *sc = wsmouse_cd.cd_devs[minor(dev)];

	return (wsevent_poll(&sc->sc_events, events, p));
#else
	return (0);
#endif /* NWSMOUSE > 0 */
}
