/*	$NetBSD: mms.c,v 1.1.4.3 2002/06/23 17:35:35 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

#include <sys/param.h>
#include <sys/device.h>
#include <sys/proc.h>
#include <sys/systm.h>

#include "wsmouse.h"

#include <dev/wscons/wsconsio.h> 
#include <dev/wscons/wsmousevar.h>

#include <dreamcast/dev/maple/maple.h>
#include <dreamcast/dev/maple/mapleconf.h>

struct mms_condition {
	uint32_t buttons;
	uint16_t axis1;		/* X */
	uint16_t axis2;		/* Y */
	uint16_t axis3;		/* wheel */
	uint16_t axis4;
	uint16_t axis5;
	uint16_t axis6;
	uint16_t axis7;
	uint16_t axis8;
};

#define	MMS_BUTTON_C		0x01	/* middle */
#define	MMS_BUTTON_B		0x02	/* right */
#define	MMS_BUTTON_A		0x04	/* left */
#define	MMS_BUTTON_START	0x08	/* thumb */
#define	MMS_BUTTON_MASK		0x0f

#define	MMS_MOVEMENT_BASE	0x200
#define	MMS_MOVEMENT_MAX	0x3ff

#define	MMS_FUNCDATA_AXIS1	0x001
#define	MMS_FUNCDATA_AXIS2	0x002
#define	MMS_FUNCDATA_AXIS3	0x004
#define	MMS_FUNCDATA_AXIS4	0x008
#define	MMS_FUNCDATA_AXIS5	0x010
#define	MMS_FUNCDATA_AXIS6	0x020
#define	MMS_FUNCDATA_AXIS7	0x040
#define	MMS_FUNCDATA_AXIS8	0x080
#define	MMS_FUNCDATA_C		0x100
#define	MMS_FUNCDATA_B		0x200
#define	MMS_FUNCDATA_A		0x400
#define	MMS_FUNCDATA_START	0x800

struct mms_softc {
	struct device sc_dev;

	int sc_port;
	int sc_subunit;

	uint32_t sc_oldbuttons;

	struct device *sc_wsmousedev;
};

int	mms_match(struct device *, struct cfdata *, void *);
void	mms_attach(struct device *, struct device *, void *);

struct cfattach mms_ca = {
	sizeof(struct mms_softc), mms_match, mms_attach,
};

int	mms_enable(void *);
int	mms_ioctl(void *, u_long, caddr_t, int, struct proc *);
void	mms_disable(void *);

const struct wsmouse_accessops mms_accessops = {
	mms_enable,
	mms_ioctl,
	mms_disable,
};

void	mms_intr(void *, void *, int);

int
mms_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct maple_attach_args *ma = aux;

	return ((ma->ma_function & MAPLE_FUNC_MOUSE) != 0);
}

void
mms_attach(struct device *parent, struct device *self, void *aux)
{
	struct mms_softc *sc = (void *) self;
	struct maple_attach_args *ma = aux;
	struct wsmousedev_attach_args a;
	uint32_t data;

	printf(": SEGA Dreamcast Mouse\n");

	sc->sc_port = ma->ma_port;
	sc->sc_subunit = ma->ma_subunit;

	data = maple_get_function_data(ma->ma_devinfo,
	    MAPLE_FUNC_MOUSE);

	printf("%s: buttons:", sc->sc_dev.dv_xname);
	if (data & MMS_FUNCDATA_A)
		printf(" left");
	if (data & MMS_FUNCDATA_C)
		printf(" middle");
	if (data & MMS_FUNCDATA_B)
		printf(" right");
	if (data & MMS_FUNCDATA_START)
		printf(" thumb");
	printf("\n");

	sc->sc_oldbuttons = 0;

	a.accessops = &mms_accessops;
	a.accesscookie = sc;

	/*
	 * Attach the mouse, saving a handle to it.
	 */
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
	if (sc->sc_wsmousedev == NULL) {
		/* Nothing more to do here. */
		return;
	}

	maple_set_condition_callback(parent, sc->sc_port, sc->sc_subunit,
	    MAPLE_FUNC_MOUSE, mms_intr, sc);
}

int
mms_enable(void *v)
{

	return (0);
}

void
mms_disable(void *v)
{

	/* Nothing to do here. */
}

int
mms_ioctl(void *v, u_long cmd, caddr_t data, int flag, struct proc *p)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *) data = WSMOUSE_TYPE_USB;	/* XXX */
		break;

	case WSMOUSEIO_SRES:
		/* XXX */
		return (EOPNOTSUPP);

	default:
		return (EPASSTHROUGH);
	}

	return (0);
}

void
mms_intr(void *arg, void *buf, int size)
{
	struct mms_softc *sc = arg;
	struct mms_condition *data = buf;
	int dx = 0, dy = 0, dz = 0, buttons = 0;
	uint32_t buttonchg;

	if (size < sizeof(*data))
		return;

	data->buttons &= MMS_BUTTON_MASK;
	buttonchg = sc->sc_oldbuttons ^ data->buttons;
	sc->sc_oldbuttons = data->buttons;

	dx = (data->axis1 & MMS_MOVEMENT_MAX) - MMS_MOVEMENT_BASE;
	dy = (data->axis2 & MMS_MOVEMENT_MAX) - MMS_MOVEMENT_BASE;
	dz = (data->axis3 & MMS_MOVEMENT_MAX) - MMS_MOVEMENT_BASE;

	if (dx || dy || dz || buttonchg) {
		if ((data->buttons & MMS_BUTTON_A) == 0)
			buttons |= 0x01;
		if ((data->buttons & MMS_BUTTON_C) == 0)
			buttons |= 0x02;
		if ((data->buttons & MMS_BUTTON_B) == 0)
			buttons |= 0x04;
		if ((data->buttons & MMS_BUTTON_START) == 0)
			buttons |= 0x08;

		wsmouse_input(sc->sc_wsmousedev, buttons,
		    dx, dy, dz, WSMOUSE_INPUT_DELTA);
	}
}
