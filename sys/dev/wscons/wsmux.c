/*	$NetBSD: wsmux.c,v 1.12 2001/10/13 20:03:38 augustss Exp $	*/

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Author: Lennart Augustsson <augustss@carlstedt.se>
 *         Carlstedt Research & Technology
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include "wsmux.h"
#include "wsdisplay.h"
#include "wskbd.h"
#include "wsmouse.h"

#if NWSMUX > 0 || (NWSDISPLAY > 0 && NWSKBD > 0)

/*
 * wscons mux device.
 *
 * The mux device is a collection of real mice and keyboards and acts as 
 * a merge point for all the events from the different real devices.
 */

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/queue.h>
#include <sys/syslog.h>
#include <sys/systm.h>
#include <sys/tty.h>
#include <sys/signalvar.h>
#include <sys/device.h>

#include "opt_wsdisplay_compat.h"

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wseventvar.h>
#include <dev/wscons/wscons_callbacks.h>
#include <dev/wscons/wsmuxvar.h>

#ifdef WSMUX_DEBUG
#define DPRINTF(x)	if (wsmuxdebug) printf x
int	wsmuxdebug = 0;
#else
#define DPRINTF(x)
#endif

struct wsplink {
	LIST_ENTRY(wsplink) next;
	int type;
	struct wsmux_softc *mux; /* our mux device */
	/* The rest of the fields reflect a value in the multiplexee. */
	struct device *sc;	/* softc */
	struct wseventvar *sc_mevents; /* event var */
	struct wsmux_softc **sc_muxp; /* pointer to us */
	struct wsmuxops *sc_ops;
};

int wsmuxdoclose(struct device *, int, int, struct proc *);
int wsmux_set_display(struct device *, struct wsmux_softc *);

#if NWSMUX > 0

#define WSMUXDEV(n) ((n) & 0x7f)
#define WSMUXCTL(n) ((n) & 0x80)

cdev_decl(wsmux);

void wsmuxattach(int);

struct wsmuxops wsmux_muxops = {
	wsmuxopen, wsmuxdoclose, wsmuxdoioctl, wsmux_displayioctl,
	wsmux_set_display
};

void wsmux_setmax(int n);

int nwsmux = 0;
struct wsmux_softc **wsmuxdevs;

void
wsmux_setmax(int n)
{
	int i;

	if (n >= nwsmux) {
		i = nwsmux;
		nwsmux = n + 1;
		if (i != 0)
			wsmuxdevs = realloc(wsmuxdevs, 
					    nwsmux * sizeof (*wsmuxdevs), 
					    M_DEVBUF, M_NOWAIT);
		else
			wsmuxdevs = malloc(nwsmux * sizeof (*wsmuxdevs), 
					   M_DEVBUF, M_NOWAIT);
		if (wsmuxdevs == NULL)
			panic("wsmux_setmax: no memory\n");
		for (; i < nwsmux; i++)
			wsmuxdevs[i] = 0;
	}
}

/* From upper level */
void
wsmuxattach(int n)
{
}

/* Return mux n, create if necessary */
struct wsmux_softc *
wsmux_getmux(int n)
{
	struct wsmux_softc *sc;

	wsmux_setmax(n);
	sc = wsmuxdevs[n];
	if (sc == 0) {
		sc = wsmux_create("wsmux", n);
		if (sc == 0) {
			printf("wsmux: attach out of memory\n");
			return (NULL);
		}
		wsmuxdevs[n] = sc;
	}
	return (sc);
}

/* From mouse or keyboard. */
void
wsmux_attach(int n, int type, struct device *dsc, struct wseventvar *ev,
	struct wsmux_softc **psp, struct wsmuxops *ops)
{
	struct wsmux_softc *sc;
	int error;

	DPRINTF(("wsmux_attach: n=%d\n", n));
	sc = wsmux_getmux(n);
	error = wsmux_attach_sc(sc, type, dsc, ev, psp, ops);
	if (error)
		printf("wsmux_attach: error=%d\n", error);
}

/* From mouse or keyboard. */
void
wsmux_detach(int n, struct device *dsc)
{
#ifdef DIAGNOSTIC
	int error;

	if (n >= nwsmux || n < 0) {
		printf("wsmux_detach: detach is out of range\n");
		return;
	}
	if ((error = wsmux_detach_sc(wsmuxdevs[n], dsc)))
		printf("wsmux_detach: error=%d\n", error);
#else
	(void)wsmux_detach_sc(wsmuxdevs[n], dsc);
#endif
}

int
wsmuxopen(dev_t dev, int flags, int mode, struct proc *p)
{
	struct wsmux_softc *sc;
	struct wsplink *m;
	int munit, unit, error, nopen, lasterror;

	munit = minor(dev);
	unit = WSMUXDEV(munit);
	sc = wsmux_getmux(unit);
	if (sc == NULL)
		return (ENXIO);

	DPRINTF(("wsmuxopen: %s: sc=%p\n", sc->sc_dv.dv_xname, sc));
	if (WSMUXCTL(munit)) {
		/* This is the control device which does not allow reads. */
		if (flags & FREAD)
			return (EINVAL);
		return (0);
	}

	if (sc->sc_mux) {
		/* Grab the mux out of the greedy hands of the parent mux. */
		int error = wsmux_rem_mux(unit, sc->sc_mux);
		if (error)
			return (error);
	}

	if (sc->sc_events.io)
		return (EBUSY);

	sc->sc_events.io = p;
	sc->sc_flags = flags;
	sc->sc_mode = mode;
	sc->sc_p = p;
	wsevent_init(&sc->sc_events);		/* may cause sleep */

	nopen = 0;
	lasterror = 0;
	for (m = LIST_FIRST(&sc->sc_reals); m; m = LIST_NEXT(m, next)) {
		if (!m->sc_mevents->io && !*m->sc_muxp) {
			DPRINTF(("wsmuxopen: %s: m=%p dev=%s\n", 
				 sc->sc_dv.dv_xname, m, m->sc->dv_xname));
			error = m->sc_ops->dopen(makedev(0, m->sc->dv_unit),
						 flags, mode, p);
			if (error) {
				/* Ignore opens that fail */
				lasterror = error;
				DPRINTF(("wsmuxopen: open failed %d\n", 
					 error));
			} else {
				nopen++;
				*m->sc_muxp = sc;
			}
		}
	}

	if (nopen == 0 && lasterror != 0) {
		wsevent_fini(&sc->sc_events);
		sc->sc_events.io = NULL;
		return (lasterror);
	}

	return (0);
}

int
wsmuxclose(dev_t dev, int flags, int mode, struct proc *p)
{
	int munit = minor(dev);

	if (WSMUXCTL(munit)) {
		/* control device */
		return (0);
	}
	return wsmuxdoclose(&wsmuxdevs[WSMUXDEV(munit)]->sc_dv, flags, mode, p);
}

int
wsmuxread(dev_t dev, struct uio *uio, int flags)
{
	int munit = minor(dev);
	struct wsmux_softc *sc = wsmuxdevs[WSMUXDEV(munit)];

	if (WSMUXCTL(munit)) {
		/* control device */
		return (EINVAL);
	}

	if (!sc->sc_events.io)
		return (EACCES);

	return (wsevent_read(&sc->sc_events, uio, flags));
}

int
wsmuxioctl(dev_t dev, u_long cmd, caddr_t data, int flag, struct proc *p)
{
	int unit = WSMUXDEV(minor(dev));

	return wsmuxdoioctl(&wsmuxdevs[unit]->sc_dv, cmd, data, flag, p);
}

int
wsmuxpoll(dev_t dev, int events, struct proc *p)
{
	int munit = minor(dev);
	struct wsmux_softc *sc = wsmuxdevs[WSMUXDEV(munit)];

	if (WSMUXCTL(munit)) {
		/* control device */
		return (EINVAL);
	}

	if (!sc->sc_events.io)
		return (EACCES);

	return (wsevent_poll(&sc->sc_events, events, p));
}

int
wsmux_add_mux(int unit, struct wsmux_softc *muxsc)
{
	struct wsmux_softc *sc, *m;

	sc = wsmux_getmux(unit);
	if (sc == NULL)
		return (ENXIO);

	DPRINTF(("wsmux_add_mux: %s(%p) to %s(%p)\n", sc->sc_dv.dv_xname, sc,
		 muxsc->sc_dv.dv_xname, muxsc));

	if (sc->sc_mux || sc->sc_events.io)
		return (EBUSY);

	/* The mux we are adding must not be an ancestor of itself. */
	for (m = muxsc; m; m = m->sc_mux)
		if (m == sc)
			return (EINVAL);

	return (wsmux_attach_sc(muxsc, WSMUX_MUX, &sc->sc_dv, &sc->sc_events, 
				&sc->sc_mux, &wsmux_muxops));
}

int
wsmux_rem_mux(int unit, struct wsmux_softc *muxsc)
{
	struct wsmux_softc *sc;

	if (unit < 0 || unit >= nwsmux || (sc = wsmuxdevs[unit]) == NULL)
		return (ENXIO);
	
	DPRINTF(("wsmux_rem_mux: %s from %s\n", sc->sc_dv.dv_xname,
		 muxsc->sc_dv.dv_xname));

	return (wsmux_detach_sc(muxsc, &sc->sc_dv));
}

#endif /* NWSMUX > 0 */

struct wsmux_softc *
wsmux_create(const char *name, int unit)
{
	struct wsmux_softc *sc;

	DPRINTF(("wsmux_create: allocating\n"));
	sc = malloc(sizeof *sc, M_DEVBUF, M_NOWAIT);
	if (sc == NULL)
		return (NULL);
	memset(sc, 0, sizeof *sc);
	LIST_INIT(&sc->sc_reals);
	snprintf(sc->sc_dv.dv_xname, sizeof sc->sc_dv.dv_xname,
		 "%s%d", name, unit);
	sc->sc_dv.dv_unit = unit;
	return (sc);
}

int
wsmux_attach_sc(struct wsmux_softc *sc, int type, struct device *dsc,
	struct wseventvar *ev, struct wsmux_softc **psp, struct wsmuxops *ops)
{
	struct wsplink *m;
	int error;

	DPRINTF(("wsmux_attach_sc: %s: type=%d dsc=%p, *psp=%p\n",
		 sc->sc_dv.dv_xname, type, dsc, *psp));
	m = malloc(sizeof *m, M_DEVBUF, M_NOWAIT);
	if (m == NULL)
		return (ENOMEM);
	m->type = type;
	m->mux = sc;
	m->sc = dsc;
	m->sc_mevents = ev;
	m->sc_muxp = psp;
	m->sc_ops = ops;
	LIST_INSERT_HEAD(&sc->sc_reals, m, next);

	if (sc->sc_displaydv) {
		/* This is a display mux, so attach the new device to it. */
		DPRINTF(("wsmux_attach_sc: %s: set display %p\n", 
			 sc->sc_dv.dv_xname, sc->sc_displaydv));
		error = 0;
		if (m->sc_ops->dsetdisplay) {
			error = m->sc_ops->dsetdisplay(m->sc, sc);
			/* Ignore that the console already has a display. */
			if (error == EBUSY)
				error = 0;
			if (!error) {
				*m->sc_muxp = sc;
#ifdef WSDISPLAY_COMPAT_RAWKBD
				DPRINTF(("wsmux_attach_sc: on %s set rawkbd=%d\n",
					 m->sc->dv_xname, sc->sc_rawkbd));
				(void)m->sc_ops->dioctl(m->sc, 
					     WSKBDIO_SETMODE, 
					     (caddr_t)&sc->sc_rawkbd,
					     0, 0);
#endif
			}
		}
	} else if (sc->sc_events.io) {
		/* Mux is open, so open the new subdevice */
		DPRINTF(("wsmux_attach_sc: %s: calling open of %s\n",
			 sc->sc_dv.dv_xname, m->sc->dv_xname));
		/* mux already open, join in */
		error = m->sc_ops->dopen(makedev(0, m->sc->dv_unit),
					 sc->sc_flags, sc->sc_mode, sc->sc_p);
		if (!error)
			*m->sc_muxp = sc;
	} else {
		DPRINTF(("wsmux_attach_sc: %s not open\n",
			 sc->sc_dv.dv_xname));
		error = 0;
	}
	DPRINTF(("wsmux_attach_sc: done sc=%p psp=%p *psp=%p\n", 
		 sc, psp, *psp));

	return (error);
}

int
wsmux_detach_sc(struct wsmux_softc *sc, struct device *dsc)
{
	struct wsplink *m;
	int error = 0;

	DPRINTF(("wsmux_detach_sc: %s: dsc=%p\n", sc->sc_dv.dv_xname, dsc));
#ifdef DIAGNOSTIC
	if (sc == 0) {
		printf("wsmux_detach_sc: not allocated\n");
		return (ENXIO);
	}
#endif

	for (m = LIST_FIRST(&sc->sc_reals); m; m = LIST_NEXT(m, next)) {
		if (m->sc == dsc)
			break;
	}
#ifdef DIAGNOSTIC
	if (!m) {
		printf("wsmux_detach_sc: not found\n");
		return (ENXIO);
	}
#endif
	if (sc->sc_displaydv) {
		if (m->sc_ops->dsetdisplay)
			error = m->sc_ops->dsetdisplay(m->sc, 0);
		if (error)
			return (error);
		*m->sc_muxp = 0;
	} else if (*m->sc_muxp) {
		DPRINTF(("wsmux_detach_sc: close\n"));
		/* mux device is open, so close multiplexee */
		m->sc_ops->dclose(m->sc, FREAD, 0, 0);
		*m->sc_muxp = 0;
	}

	LIST_REMOVE(m, next);

	free(m, M_DEVBUF);
	DPRINTF(("wsmux_detach_sc: done sc=%p\n", sc));
	return (0);
}

int
wsmuxdoclose(struct device *dv, int flags, int mode, struct proc *p)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct wsplink *m;

	DPRINTF(("wsmuxclose: %s: sc=%p\n", sc->sc_dv.dv_xname, sc));

	for (m = LIST_FIRST(&sc->sc_reals); m; m = LIST_NEXT(m, next)) {
		if (*m->sc_muxp == sc) {
			DPRINTF(("wsmuxclose %s: m=%p dev=%s\n", 
				 sc->sc_dv.dv_xname, m, m->sc->dv_xname));
			m->sc_ops->dclose(m->sc, flags, mode, p);
			*m->sc_muxp = 0;
		}
	}

	wsevent_fini(&sc->sc_events);
	sc->sc_events.io = NULL;

	return (0);
}

int
wsmuxdoioctl(struct device *dv, u_long cmd, caddr_t data, int flag,
	     struct proc *p)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct wsplink *m;
	int error, ok;
	int s, put, get, n;
	struct wseventvar *evar;
	struct wscons_event *ev;
	struct timeval xxxtime;
	struct wsmux_device_list *l;

	DPRINTF(("wsmuxdoioctl: %s: sc=%p, cmd=%08lx\n", 
		 sc->sc_dv.dv_xname, sc, cmd));

	switch (cmd) {
	case WSMUX_INJECTEVENT:
		/* Inject an event, e.g., from moused. */
		if (!sc->sc_events.io)
			return (EACCES);

		evar = &sc->sc_events;
		s = spltty();
		get = evar->get;
		put = evar->put;
		if (++put % WSEVENT_QSIZE == get) {
			put--;
			splx(s);
			return (ENOSPC);
		}
		if (put >= WSEVENT_QSIZE)
			put = 0;
		ev = &evar->q[put];
		*ev = *(struct wscons_event *)data;
		microtime(&xxxtime);
		TIMEVAL_TO_TIMESPEC(&xxxtime, &ev->time);
		evar->put = put;
		WSEVENT_WAKEUP(evar);
		splx(s);
		return (0);
	case WSMUX_ADD_DEVICE:
#define d ((struct wsmux_device *)data)
		switch (d->type) {
#if NWSMOUSE > 0
		case WSMUX_MOUSE:
			return (wsmouse_add_mux(d->idx, sc));
#endif
#if NWSKBD > 0
		case WSMUX_KBD:
			return (wskbd_add_mux(d->idx, sc));
#endif
#if NWSMUX > 0
		case WSMUX_MUX:
			return (wsmux_add_mux(d->idx, sc));
#endif
		default:
			return (EINVAL);
		}
	case WSMUX_REMOVE_DEVICE:
		switch (d->type) {
#if NWSMOUSE > 0
		case WSMUX_MOUSE:
			return (wsmouse_rem_mux(d->idx, sc));
#endif
#if NWSKBD > 0
		case WSMUX_KBD:
			return (wskbd_rem_mux(d->idx, sc));
#endif
#if NWSMUX > 0
		case WSMUX_MUX:
			return (wsmux_rem_mux(d->idx, sc));
#endif
		default:
			return (EINVAL);
		}
#undef d
	case WSMUX_LIST_DEVICES:
		l = (struct wsmux_device_list *)data;
		for (n = 0, m = LIST_FIRST(&sc->sc_reals);
		     n < WSMUX_MAXDEV && m != NULL;
		     m = LIST_NEXT(m, next)) {
			l->devices[n].type = m->type;
			l->devices[n].idx = m->sc->dv_unit;
			n++;
		}
		l->ndevices = n;
		return (0);
#ifdef WSDISPLAY_COMPAT_RAWKBD
	case WSKBDIO_SETMODE:
		sc->sc_rawkbd = *(int *)data;
		DPRINTF(("wsmuxdoioctl: save rawkbd = %d\n", sc->sc_rawkbd));
		break;
#endif
	case FIOASYNC:
		sc->sc_events.async = *(int *)data != 0;
		return (0);
	case TIOCSPGRP:
		if (*(int *)data != sc->sc_events.io->p_pgid)
			return (EPERM);
		return (0);
	default:
		break;
	}

	if (sc->sc_events.io == NULL && sc->sc_displaydv == NULL)
		return (EACCES);

	/* Return 0 if any of the ioctl() succeeds, otherwise the last error */
	error = 0;
	ok = 0;
	for (m = LIST_FIRST(&sc->sc_reals); m; m = LIST_NEXT(m, next)) {
		DPRINTF(("wsmuxdoioctl: m=%p *m->sc_muxp=%p sc=%p\n",
			 m, *m->sc_muxp, sc));
		if (*m->sc_muxp == sc) {
			DPRINTF(("wsmuxdoioctl: %s: m=%p dev=%s\n", 
				 sc->sc_dv.dv_xname, m, m->sc->dv_xname));
			error = m->sc_ops->dioctl(m->sc, cmd, data, flag, p);
			if (!error)
				ok = 1;
		}
	}
	if (ok)
		error = 0;

	return (error);
}

int
wsmux_displayioctl(struct device *dv, u_long cmd, caddr_t data, int flag,
		   struct proc *p)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct wsplink *m;
	int error, ok;

	DPRINTF(("wsmux_displayioctl: %s: sc=%p, cmd=%08lx\n", 
		 sc->sc_dv.dv_xname, sc, cmd));

#ifdef WSDISPLAY_COMPAT_RAWKBD
	if (cmd == WSKBDIO_SETMODE) {
		sc->sc_rawkbd = *(int *)data;
		DPRINTF(("wsmux_displayioctl: rawkbd = %d\n", sc->sc_rawkbd));
	}		
#endif

	/* 
	 * Return 0 if any of the ioctl() succeeds, otherwise the last error.
	 * Return -1 if no mux component accepts the ioctl.
	 */
	error = -1;
	ok = 0;
	for (m = LIST_FIRST(&sc->sc_reals); m; m = LIST_NEXT(m, next)) {
		DPRINTF(("wsmux_displayioctl: m=%p sc=%p sc_muxp=%p\n", 
			 m, sc, *m->sc_muxp));
		if (m->sc_ops->ddispioctl && *m->sc_muxp == sc) {
			error = m->sc_ops->ddispioctl(m->sc, cmd, data,
						      flag, p);
			DPRINTF(("wsmux_displayioctl: m=%p dev=%s ==> %d\n", 
				 m, m->sc->dv_xname, error));
			if (!error)
				ok = 1;
		}
	}
	if (ok)
		error = 0;

	return (error);
}

int
wsmux_set_display(struct device *dv, struct wsmux_softc *muxsc)
{
	struct wsmux_softc *sc = (struct wsmux_softc *)dv;
	struct wsmux_softc *nsc = muxsc ? sc : 0;
	struct device *displaydv = muxsc ? muxsc->sc_displaydv : 0;
	struct device *odisplaydv;
	struct wsplink *m;
	int error, ok;

	DPRINTF(("wsmux_set_display: %s: displaydv=%p\n",
		 sc->sc_dv.dv_xname, displaydv));

	if (displaydv) {
		if (sc->sc_displaydv)
			return (EBUSY);
	} else {
		if (sc->sc_displaydv == NULL)
			return (ENXIO);
	}

	odisplaydv = sc->sc_displaydv;
	sc->sc_displaydv = displaydv;

	if (displaydv)
		printf("%s: connecting to %s\n",
		       sc->sc_dv.dv_xname, displaydv->dv_xname);
	ok = 0;
	error = 0;
	for (m = LIST_FIRST(&sc->sc_reals); m; m = LIST_NEXT(m, next)) {
		if (m->sc_ops->dsetdisplay &&
		    (nsc ? m->sc_mevents->io == 0 && *m->sc_muxp == 0 : 
		           *m->sc_muxp == sc)) {
			error = m->sc_ops->dsetdisplay(m->sc, nsc);
			DPRINTF(("wsmux_set_display: m=%p dev=%s error=%d\n", 
				 m, m->sc->dv_xname, error));
			if (!error) {
				ok = 1;
				*m->sc_muxp = nsc;
#ifdef WSDISPLAY_COMPAT_RAWKBD
				DPRINTF(("wsmux_set_display: on %s set rawkbd=%d\n",
					 m->sc->dv_xname, sc->sc_rawkbd));
				(void)m->sc_ops->dioctl(m->sc, 
					     WSKBDIO_SETMODE, 
					     (caddr_t)&sc->sc_rawkbd,
					     0, 0);
#endif
			}
		}
	}
	if (ok)
		error = 0;

	if (displaydv == NULL)
		printf("%s: disconnecting from %s\n", 
		       sc->sc_dv.dv_xname, odisplaydv->dv_xname);

	return (error);
}

#endif /* NWSMUX > 0 || (NWSDISPLAY > 0 && NWSKBD > 0) */
