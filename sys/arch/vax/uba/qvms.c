/*	$NetBSD: qvms.c,v 1.1.18.2 2017/12/03 11:36:48 jdolecek Exp $	*/

/* Copyright (c) 2015 Charles H. Dickman. All rights reserved.
 * Derived from dzms.c
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
 */

/*
 * VSXXX mice attached to line 1 of the QVSS
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: qvms.c,v 1.1.18.2 2017/12/03 11:36:48 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/ioctl.h>
#include <sys/syslog.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/tty.h>

#include <sys/bus.h>

#include <vax/uba/qvareg.h>
#include <vax/uba/qvavar.h>
#include <vax/uba/qvkbdvar.h>
#include <dev/dec/lk201.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include "locators.h"

struct qvms_softc {		/* driver status information */
	struct	device qvms_dev;	/* required first: base device */
	struct	qvaux_linestate *qvms_ls;

	int sc_enabled;		/* input enabled? */
	int sc_selftest;

	int inputstate;
	u_int buttons;
	int dx, dy;

	device_t sc_wsmousedev;
};

static int  qvms_match(device_t, cfdata_t, void *);
static void qvms_attach(device_t, device_t, void *);
static int qvms_input(void *, int);

CFATTACH_DECL_NEW(qvms, sizeof(struct qvms_softc),
    qvms_match, qvms_attach, NULL, NULL);

static int  qvms_enable(void *);
static int  qvms_ioctl(void *, u_long, void *, int, struct lwp *);
static void qvms_disable(void *);

const struct wsmouse_accessops qvms_accessops = {
	qvms_enable,
	qvms_ioctl,
	qvms_disable,
};

static int
qvms_match(device_t parent, cfdata_t cf, void *aux)
{
	struct qvauxkm_attach_args *daa = aux;

	/* Exact match is better than wildcard. */
	if (cf->cf_loc[QVAUXCF_LINE] == daa->daa_line)
		return 2;

	/* This driver accepts wildcard. */
	if (cf->cf_loc[QVAUXCF_LINE] == QVAUXCF_LINE_DEFAULT)
		return 1;

	return 0;
}

static void
qvms_attach(device_t parent, device_t self, void *aux)
{
	struct qvaux_softc *qvaux = device_private(parent);
	struct qvms_softc *qvms = device_private(self);
	struct qvauxkm_attach_args *daa = aux;
	struct qvaux_linestate *ls;
	struct wsmousedev_attach_args a;

	qvaux->sc_qvaux[daa->daa_line].qvaux_catch = qvms_input;
	qvaux->sc_qvaux[daa->daa_line].qvaux_private = qvms;
	ls = &qvaux->sc_qvaux[daa->daa_line];
	qvms->qvms_ls = ls;

	printf("\n");

	a.accessops = &qvms_accessops;
	a.accesscookie = qvms;

	qvms->sc_enabled = 0;
	qvms->sc_selftest = 0;
	qvms->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}

static int
qvms_enable(void *v)
{
	struct qvms_softc *sc = v;

	if (sc->sc_enabled)
		return EBUSY;

	sc->sc_selftest = 4; /* wait for 4 byte reply upto 1/2 sec */
	qvauxputc(sc->qvms_ls, MOUSE_SELF_TEST);
	(void)tsleep(qvms_enable, TTIPRI, "qvmsopen", hz / 2);
	if (sc->sc_selftest != 0) {
		sc->sc_selftest = 0;
		return ENXIO;
	}
	DELAY(150);
	qvauxputc(sc->qvms_ls, MOUSE_INCREMENTAL);
	sc->sc_enabled = 1;
	sc->inputstate = 0;
	return 0;
}

static void
qvms_disable(void *v)
{
	struct qvms_softc *sc = v;

	sc->sc_enabled = 0;
}

static int
qvms_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	if (cmd == WSMOUSEIO_GTYPE) {
		*(u_int *)data = WSMOUSE_TYPE_VSXXX;
		return 0;
	}
	return EPASSTHROUGH;
}

static int
qvms_input(void *vsc, int data)
{
	struct qvms_softc *sc = vsc;

	if (sc->sc_enabled == 0) {
		if (sc->sc_selftest > 0) {
			sc->sc_selftest -= 1;
			if (sc->sc_selftest == 0)
				wakeup(qvms_enable);
		}
		return (1);
	}

#define WSMS_BUTTON1    0x01
#define WSMS_BUTTON2    0x02
#define WSMS_BUTTON3    0x04

	if ((data & MOUSE_START_FRAME) != 0)
		sc->inputstate = 1;
	else
		sc->inputstate++;

	if (sc->inputstate == 1) {
		sc->buttons = 0;
		if ((data & LEFT_BUTTON) != 0)
			sc->buttons |= WSMS_BUTTON1;
		if ((data & MIDDLE_BUTTON) != 0)
			sc->buttons |= WSMS_BUTTON2;
		if ((data & RIGHT_BUTTON) != 0)
			sc->buttons |= WSMS_BUTTON3;

		sc->dx = data & MOUSE_X_SIGN;
		sc->dy = data & MOUSE_Y_SIGN;
	} else if (sc->inputstate == 2) {
		if (sc->dx == 0)
			sc->dx = -data;
		else
			sc->dx = data;
	} else if (sc->inputstate == 3) {
		sc->inputstate = 0;
		if (sc->dy == 0)
			sc->dy = -data;
		else
			sc->dy = data;
		wsmouse_input(sc->sc_wsmousedev,
				sc->buttons,
		    		sc->dx, sc->dy, 0, 0,
				WSMOUSE_INPUT_DELTA);
	}

	return(1);
}

