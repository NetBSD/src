/*	$NetBSD: mms.c,v 1.15 2010/10/17 14:13:44 tsutsui Exp $	*/

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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mms.c,v 1.15 2010/10/17 14:13:44 tsutsui Exp $");

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
	uint32_t func_code;	/* function code (big endian) */
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
	device_t sc_dev;

	device_t sc_parent;
	struct maple_unit *sc_unit;

	uint32_t sc_oldbuttons;

	struct device *sc_wsmousedev;
};

int	mms_match(device_t, cfdata_t, void *);
void	mms_attach(device_t, device_t, void *);
int	mms_detach(device_t, int);

CFATTACH_DECL_NEW(mms, sizeof(struct mms_softc),
    mms_match, mms_attach, mms_detach, NULL);

int	mms_enable(void *);
int	mms_ioctl(void *, u_long, void *, int, struct lwp *);
void	mms_disable(void *);

const struct wsmouse_accessops mms_accessops = {
	mms_enable,
	mms_ioctl,
	mms_disable,
};

void	mms_intr(void *, struct maple_response *, int, int);

int
mms_match(device_t parent, cfdata_t cf, void *aux)
{
	struct maple_attach_args *ma = aux;

	return ma->ma_function == MAPLE_FN_MOUSE ? MAPLE_MATCH_FUNC : 0;
}

void
mms_attach(device_t parent, device_t self, void *aux)
{
	struct mms_softc *sc = device_private(self);
	struct maple_attach_args *ma = aux;
	struct wsmousedev_attach_args a;
	uint32_t data;

	printf(": SEGA Dreamcast Mouse\n");

	sc->sc_dev = self;
	sc->sc_parent = parent;
	sc->sc_unit = ma->ma_unit;

	data = maple_get_function_data(ma->ma_devinfo,
	    MAPLE_FN_MOUSE);

	printf("%s: buttons:", device_xname(self));
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

	maple_set_callback(parent, sc->sc_unit, MAPLE_FN_MOUSE, mms_intr, sc);
}

int
mms_detach(device_t self, int flags)
{
	struct mms_softc *sc = device_private(self);
	int rv = 0;

	if (sc->sc_wsmousedev != NULL)
		rv = config_detach(sc->sc_wsmousedev, flags);

	return rv;
}

int
mms_enable(void *v)
{
	struct mms_softc *sc = v;

	maple_enable_periodic(sc->sc_parent, sc->sc_unit, MAPLE_FN_MOUSE, 1);
	return 0;
}

void
mms_disable(void *v)
{
	struct mms_softc *sc = v;

	maple_enable_periodic(sc->sc_parent, sc->sc_unit, MAPLE_FN_MOUSE, 0);
}

int
mms_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *) data = WSMOUSE_TYPE_MAPLE;
		break;

	case WSMOUSEIO_SRES:
		/* XXX */
		return EOPNOTSUPP;

	default:
		return EPASSTHROUGH;
	}

	return 0;
}

void
mms_intr(void *arg, struct maple_response *response, int size, int flags)
{
	struct mms_softc *sc = arg;
	struct mms_condition *data = (void *) response->data;
	int dx = 0, dy = 0, dz = 0, buttons = 0;
	uint32_t buttonchg;

	if ((flags & MAPLE_FLAG_PERIODIC) == 0 ||
	    size < sizeof(*data))
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

		wsmouse_input(sc->sc_wsmousedev,
				buttons,
				dx, -dy, dz, 0,
				WSMOUSE_INPUT_DELTA);
	}
}
