/*	$NetBSD: qvkbd.c,v 1.6 2022/02/12 17:09:43 riastradh Exp $	*/

/* Copyright (c) 2015 Charles H. Dickman. All rights reserved.
 * Derived from dzkbd.c
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
 *	@(#)kbd.c	8.2 (Berkeley) 10/30/93
 */

/*
 * LK200/LK400 keyboard attached to line 0 of the QVSS aux port
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/kmem.h>
#include <sys/intr.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wskbdvar.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsksymvar.h>
#include <dev/dec/wskbdmap_lk201.h>

#include <sys/bus.h>

#include <vax/uba/qvareg.h>
#include <vax/uba/qvavar.h>
#include <vax/uba/qvkbdvar.h>
#include <dev/dec/lk201reg.h>
#include <dev/dec/lk201var.h>

#include "locators.h"

struct qvkbd_internal {
	struct qvaux_linestate *qvi_ls;
	struct lk201_state qvi_ks;
};

struct qvkbd_internal qvkbd_console_internal;

struct qvkbd_softc {
	struct qvkbd_internal *sc_itl;

	int sc_enabled;
	int kbd_type;

	device_t sc_wskbddev;
};

static int	qvkbd_input(void *, int);

static int	qvkbd_match(device_t, cfdata_t, void *);
static void	qvkbd_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(qvkbd, sizeof(struct qvkbd_softc),
    qvkbd_match, qvkbd_attach, NULL, NULL);

static int	qvkbd_enable(void *, int);
static void	qvkbd_set_leds(void *, int);
static int	qvkbd_ioctl(void *, u_long, void *, int, struct lwp *);

const struct wskbd_accessops qvkbd_accessops = {
	qvkbd_enable,
	qvkbd_set_leds,
	qvkbd_ioctl,
};

static void	qvkbd_cngetc(void *, u_int *, int *);
static void	qvkbd_cnpollc(void *, int);
int             qvkbd_cnattach(struct qvaux_linestate *);

const struct wskbd_consops qvkbd_consops = {
	qvkbd_cngetc,
	qvkbd_cnpollc,
};

static int qvkbd_sendchar(void *, u_char);

const struct wskbd_mapdata qvkbd_keymapdata = {
	lkkbd_keydesctab,
#ifdef DZKBD_LAYOUT
	DZKBD_LAYOUT,
#else
	KB_US | KB_LK401,
#endif
};

/*
 * kbd_match: how is this qvaux line configured?
 */
static int
qvkbd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct qvauxkm_attach_args *daa = aux;

	/* Exact match is better than wildcard. */
	if (cf->cf_loc[QVAUXCF_LINE] == daa->daa_line) {
	        //printf("qvkbd_match: return 2\n");
		return 2;
        }
	/* This driver accepts wildcard. */
	if (cf->cf_loc[QVAUXCF_LINE] == QVAUXCF_LINE_DEFAULT) {
	        //printf("qvkbd_match: return 1\n");
 		return 1;
       }
        //printf("qvkbd_match: return 0\n");
	return 0;
}

static void
qvkbd_attach(device_t parent, device_t self, void *aux)
{
	struct qvaux_softc *qvaux = device_private(parent);
	struct qvkbd_softc *qvkbd = device_private(self);
	struct qvauxkm_attach_args *daa = aux;
	struct qvaux_linestate *ls;
	struct qvkbd_internal *qvi;
	struct wskbddev_attach_args a;
	int isconsole;

	qvaux->sc_qvaux[daa->daa_line].qvaux_catch = qvkbd_input;
	qvaux->sc_qvaux[daa->daa_line].qvaux_private = qvkbd;
	ls = &qvaux->sc_qvaux[daa->daa_line];

	isconsole = (daa->daa_flags & QVAUXKBD_CONSOLE);

	if (isconsole) {
		qvi = &qvkbd_console_internal;
	} else {
		qvi = kmem_alloc(sizeof(struct qvkbd_internal), KM_SLEEP);
		qvi->qvi_ks.attmt.sendchar = qvkbd_sendchar;
		qvi->qvi_ks.attmt.cookie = ls;
	}
	qvi->qvi_ls = ls;
	qvkbd->sc_itl = qvi;

	printf("\n");

	if (!isconsole) {
		DELAY(100000);
		lk201_init(&qvi->qvi_ks);
	}

	/* XXX should identify keyboard ID here XXX */
	/* XXX layout and the number of LED is varying XXX */
        /* XXX ID done during kb init XXX */

	// qvkbd->kbd_type = WSKBD_TYPE_LK201;

	qvkbd->sc_enabled = 1;

	a.console = isconsole;
	a.keymap = &qvkbd_keymapdata;
	a.accessops = &qvkbd_accessops;
	a.accesscookie = qvkbd;

	qvkbd->sc_wskbddev = config_found(self, &a, wskbddevprint, CFARGS_NONE);
}

int
qvkbd_cnattach(struct qvaux_linestate *ls)
{

	qvkbd_console_internal.qvi_ks.attmt.sendchar = qvkbd_sendchar;
	qvkbd_console_internal.qvi_ks.attmt.cookie = ls;
	lk201_init(&qvkbd_console_internal.qvi_ks);
	qvkbd_console_internal.qvi_ls = ls;

	wskbd_cnattach(&qvkbd_consops, &qvkbd_console_internal,
		       &qvkbd_keymapdata);

	return 0;
}

static int
qvkbd_enable(void *v, int on)
{
	struct qvkbd_softc *sc = v;

	sc->sc_enabled = on;
	return 0;
}

static int
qvkbd_sendchar(void *v, u_char c)
{
	struct qvaux_linestate *ls = v;
	int s;

	s = spltty();
	qvauxputc(ls, c);
	splx(s);
	return (0);
}

static void
qvkbd_cngetc(void *v, u_int *type, int *data)
{
	struct qvkbd_internal *qvi = v;
	int c;

	do {
		c = qvauxgetc(qvi->qvi_ls);
	} while (!lk201_decode(&qvi->qvi_ks, 0, c, type, data));
}

static void
qvkbd_cnpollc(void *v, int on)
{
#if 0
	struct qvkbd_internal *qvi = v;
#endif
}

static void
qvkbd_set_leds(void *v, int leds)
{
	struct qvkbd_softc *sc = (struct qvkbd_softc *)v;

	lk201_set_leds(&sc->sc_itl->qvi_ks, leds);
}

static int
qvkbd_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct qvkbd_softc *sc = (struct qvkbd_softc *)v;

	switch (cmd) {
	case WSKBDIO_GTYPE:
		*(int *)data = sc->kbd_type;
		return 0;
	case WSKBDIO_SETLEDS:
		lk201_set_leds(&sc->sc_itl->qvi_ks, *(int *)data);
		return 0;
	case WSKBDIO_GETLEDS:
		/* XXX don't dig in kbd internals */
		*(int *)data = sc->sc_itl->qvi_ks.leds_state;
		return 0;
	case WSKBDIO_COMPLEXBELL:
		lk201_bell(&sc->sc_itl->qvi_ks,
			   (struct wskbd_bell_data *)data);
		return 0;
	case WSKBDIO_SETKEYCLICK:
		lk201_set_keyclick(&sc->sc_itl->qvi_ks, *(int *)data);
		return 0;
	case WSKBDIO_GETKEYCLICK:
		/* XXX don't dig in kbd internals */
		*(int *)data = sc->sc_itl->qvi_ks.kcvol;
		return 0;
	}
	return (EPASSTHROUGH);
}

static int
qvkbd_input(void *v, int data)
{
	struct qvkbd_softc *sc = (struct qvkbd_softc *)v;
	u_int type;
	int val;
        int decode;

        do {
                decode = lk201_decode(&sc->sc_itl->qvi_ks, 1,
                    data, &type, &val);
                if (decode != LKD_NODATA)
                        wskbd_input(sc->sc_wskbddev, type, val);
        } while (decode == LKD_MORE);
	return(1);
}
