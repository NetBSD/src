/* $NetBSD: wsqms.c,v 1.1.4.3 2002/04/01 07:39:12 nathanw Exp $ */

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

#include <sys/device.h>
#include <sys/errno.h> 
#include <sys/ioctl.h>
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

#define MAX_XYREG	4096

/* forward declarations */

static int wsqms_enable		__P((void *cookie));
static int wsqms_ioctl		__P((void *cookie, u_long cmd, caddr_t data, int flag, struct proc *p));
static void wsqms_disable	__P((void *cookie));


static struct wsmouse_accessops wsqms_accessops = {
	wsqms_enable, wsqms_ioctl, wsqms_disable
};


void
wsqms_attach(sc, self)
	struct wsqms_softc *sc;
	struct device *self;
{
	struct wsmousedev_attach_args wsmouseargs;

	/* set up wsmouse attach arguments */
	wsmouseargs.accessops = &wsqms_accessops;
	wsmouseargs.accesscookie = self;

	printf("\n");

	sc->sc_wsmousedev = config_found(self, &wsmouseargs, wsmousedevprint);
}


static int
wsqms_enable(cookie)
	void *cookie;
{
	struct wsqms_softc *sc = cookie;

	sc->sc_flags |= WSQMS_ENABLED;

	/* enable interrupts */
	sc->sc_intenable(sc, 1);

	return 0;
}


static void
wsqms_disable(cookie)
	void *cookie;
{
	struct wsqms_softc *sc = cookie;

	sc->sc_flags &= ~WSQMS_ENABLED;

	/* disable interrupts */
	sc->sc_intenable(sc, 0);
}


static int
wsqms_ioctl(cookie, cmd, data, flag, p)
	void *cookie;
	u_long cmd;
	caddr_t data;
	int flag;
	struct proc *p;
{
	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(int *)data = WSMOUSE_TYPE_ARCHIMEDES;
		return 0;
	}

	return EPASSTHROUGH;
}


/* We can really put in the mouse XY as absolutes ? */
int
wsqms_intr(arg)
	void *arg;
{
	struct wsqms_softc *sc = arg;
	int x, y, b;

	x = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX) & 0xffff;
	y = bus_space_read_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY) & 0xffff;
	b = bus_space_read_1(sc->sc_iot, sc->sc_butioh, QMS_BUTTONS) & 0x70;
	b >>= 4;
	if (x & 0x8000) x |= 0xffff0000;
	if (y & 0x8000) y |= 0xffff0000;

	/* patch up the buttons */
	b = ~( ((b & 1)<<2) | (b & 2) | ((b & 4)>>2));

	if ((x != sc->lastx) || (y != sc->lasty) || (b != sc->lastb)) {
		/* save old values */
		sc->lastx = x;
		sc->lasty = y;
		sc->lastb = b;

		/* do we have to bound x and y ? => yes */
		if (x < -MAX_XYREG) x = -MAX_XYREG;
		if (x >  MAX_XYREG) x =  MAX_XYREG;
		if (y < -MAX_XYREG) y = -MAX_XYREG;
		if (y >  MAX_XYREG) y =  MAX_XYREG;

		/* write the bounded values back */
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEX, x);
		bus_space_write_4(sc->sc_iot, sc->sc_ioh, QMS_MOUSEY, y);

		wsmouse_input(sc->sc_wsmousedev, b, x, y, 0,
				WSMOUSE_INPUT_ABSOLUTE_X | WSMOUSE_INPUT_ABSOLUTE_Y);
	};

	return (0);	/* pass on */
}


/* end of wsqms.c */
