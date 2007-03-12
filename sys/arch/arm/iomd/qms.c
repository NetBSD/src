/*	$NetBSD: qms.c,v 1.10.4.1 2007/03/12 05:47:06 rmind Exp $	*/

/*-
 * Copyright (c) 2001 Reinoud Zandijk
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Reinoud Zandijk
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
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
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
/*
 * Quadrature mouse driver for the wscons as used in the IOMD
 */

#include <sys/param.h>

__KERNEL_RCSID(0, "$NetBSD: qms.c,v 1.10.4.1 2007/03/12 05:47:06 rmind Exp $");

#include <sys/callout.h>
#include <sys/device.h>
#include <sys/errno.h> 
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/tty.h>
#include <sys/types.h>
#include <sys/syslog.h> 
#include <sys/systm.h>
#include <sys/select.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/iomd/iomdvar.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

struct qms_softc {
	struct device  sc_dev;
	struct device *sc_wsmousedev;

	bus_space_tag_t sc_iot;		/* bus tag */
	bus_space_handle_t sc_ioh;	/* bus handle for XY */
	bus_space_handle_t sc_butioh;	/* bus handle for buttons */

	struct callout sc_callout;

	u_int16_t lastx;
	u_int16_t lasty;
	int lastb;
};

/* Offsets of hardware registers */
#define QMS_MOUSEX	0		/* 16 bits X register */
#define QMS_MOUSEY	1		/* 16 bits Y register */
#define QMS_BUTTONS	0 		/* mouse buttons in bits 4,5,6 */

static int  qms_match(struct device *, struct cfdata *, void *);
static void qms_attach(struct device *, struct device *, void *);

static int qms_enable(void *);
static int qms_ioctl(void *, u_long, void *, int, struct lwp *);
static void qms_disable(void *cookie);
static void qms_intr(void *arg);

CFATTACH_DECL(qms, sizeof(struct qms_softc),
    qms_match, qms_attach, NULL, NULL);

static struct wsmouse_accessops qms_accessops = {
	qms_enable, qms_ioctl, qms_disable
};


static int
qms_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct qms_attach_args *qa = aux;

	if (strcmp(qa->qa_name, "qms") == 0)
		return 1;

	return 0;
}


static void
qms_attach(struct device *parent, struct device *self, void *aux)
{
	struct qms_softc *sc = (void *)self;
	struct qms_attach_args *qa = aux;
	struct wsmousedev_attach_args wsmouseargs;

	sc->sc_iot = qa->qa_iot;
	sc->sc_ioh = qa->qa_ioh;
	sc->sc_butioh = qa->qa_ioh_but;

	/* set up wsmouse attach arguments */
	wsmouseargs.accessops = &qms_accessops;
	wsmouseargs.accesscookie = sc;

	printf("\n");

	sc->sc_wsmousedev =
	    config_found(&sc->sc_dev, &wsmouseargs, wsmousedevprint);

	callout_init(&sc->sc_callout);
}


static int
qms_enable(void *cookie)
{
	struct qms_softc *sc = cookie;
	int b;

	/* We don't want the mouse to warp on open. */
	sc->lastx = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX);
	sc->lasty = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY);
	b = bus_space_read_1(sc->sc_iot, sc->sc_butioh, QMS_BUTTONS) & 0x70;

	/* patch up the buttons */
	b >>= 4;
	sc->lastb = ~( ((b & 1)<<2) | (b & 2) | ((b & 4)>>2));

	callout_reset(&sc->sc_callout, hz / 100, qms_intr, sc);
	return 0;
}


static void
qms_disable(void *cookie)
{
	struct qms_softc *sc = cookie;

	callout_stop(&sc->sc_callout);
}


static int
qms_ioctl(void *cookie, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(int *)data = WSMOUSE_TYPE_ARCHIMEDES;
		return 0;
	}

	return EPASSTHROUGH;
}


static void
qms_intr(void *arg)
{
	struct qms_softc *sc = arg;
	int b;
	u_int16_t x, y;
	int16_t dx, dy;

	x = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX);
	y = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY);
	b = bus_space_read_1(sc->sc_iot, sc->sc_butioh, QMS_BUTTONS) & 0x70;

	/* patch up the buttons */
	b >>= 4;
	b = ~( ((b & 1)<<2) | (b & 2) | ((b & 4)>>2));

	if ((x != sc->lastx) || (y != sc->lasty) || (b != sc->lastb)) {
		/* This assumes that int16_t is two's complement. */
		dx = x - sc->lastx;
		dy = y - sc->lasty;
		wsmouse_input(sc->sc_wsmousedev,
				b,
				dx, dy, 0, 0,
				WSMOUSE_INPUT_DELTA);

		/* save old values */
		sc->lastx = x;
		sc->lasty = y;
		sc->lastb = b;
	}
	callout_reset(&sc->sc_callout, hz / 100, qms_intr, sc);
}


/* end of qms.c */
