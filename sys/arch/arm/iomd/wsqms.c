/* $NetBSD: wsqms.c,v 1.6 2002/06/19 23:12:14 bjh21 Exp $ */

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
 *
 *
 * Quadratic mouse driver for the wscons as used in the IOMD but is in
 * principle more generic.
 *
 */


#include <sys/param.h>

#include <sys/callout.h>
#include <sys/device.h>
#include <sys/errno.h> 
#include <sys/ioctl.h>
#include <sys/kernel.h>
#include <sys/malloc.h>
#include <sys/proc.h>
#include <sys/syslog.h> 
#include <sys/systm.h>

#include <machine/bus.h>
#include <machine/intr.h>
 
#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsksymdef.h>
#include <dev/wscons/wsmousevar.h>

#include <arm/iomd/wsqmsvar.h>


/* Offsets of hardware registers */
#define QMS_MOUSEX	0		/* 16 bits X register */
#define QMS_MOUSEY	1		/* 16 bits Y register */
#define QMS_BUTTONS	0 		/* mouse buttons in bits 4,5,6 */

/* forward declarations */

static int wsqms_enable(void *);
static int wsqms_ioctl(void *, u_long, caddr_t, int, struct proc *);
static void wsqms_disable(void *cookie);
static void wsqms_intr(void *arg);


static struct wsmouse_accessops wsqms_accessops = {
	wsqms_enable, wsqms_ioctl, wsqms_disable
};


void
wsqms_attach(struct wsqms_softc *sc, struct device *self)
{
	struct wsmousedev_attach_args wsmouseargs;

	/* set up wsmouse attach arguments */
	wsmouseargs.accessops = &wsqms_accessops;
	wsmouseargs.accesscookie = self;

	printf("\n");

	sc->sc_wsmousedev = config_found(self, &wsmouseargs, wsmousedevprint);

	callout_init(&sc->sc_callout);
}


static int
wsqms_enable(void *cookie)
{
	struct wsqms_softc *sc = cookie;

	sc->sc_flags |= WSQMS_ENABLED;

	callout_reset(&sc->sc_callout, hz / 100, wsqms_intr, sc);
	return 0;
}


static void
wsqms_disable(void *cookie)
{
	struct wsqms_softc *sc = cookie;

	sc->sc_flags &= ~WSQMS_ENABLED;

	callout_stop(&sc->sc_callout);
}


static int
wsqms_ioctl(void *cookie, u_long cmd, caddr_t data, int flag, struct proc *p)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(int *)data = WSMOUSE_TYPE_ARCHIMEDES;
		return 0;
	}

	return EPASSTHROUGH;
}


static void
wsqms_intr(void *arg)
{
	struct wsqms_softc *sc = arg;
	int x, y, b;
	int dx, dy;

	x = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX) & 0xffff;
	y = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY) & 0xffff;
	b = bus_space_read_1(sc->sc_iot, sc->sc_butioh, QMS_BUTTONS) & 0x70;

	/* patch up the buttons */
	b >>= 4;
	b = ~( ((b & 1)<<2) | (b & 2) | ((b & 4)>>2));

	if ((x != sc->lastx) || (y != sc->lasty) || (b != sc->lastb)) {
		dx = (x - sc->lastx) & 0xffff;
		if (dx >= 0x8000) dx -= 0x10000;
		dy = (y - sc->lasty) & 0xffff;
		if (dy >= 0x8000) dy -= 0x10000;
		wsmouse_input(sc->sc_wsmousedev, b, dx, dy, 0,
		    WSMOUSE_INPUT_DELTA);

		/* save old values */
		sc->lastx = x;
		sc->lasty = y;
		sc->lastb = b;
	};
	callout_reset(&sc->sc_callout, hz / 100, wsqms_intr, sc);
}


/* end of wsqms.c */
