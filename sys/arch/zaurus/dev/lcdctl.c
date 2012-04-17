/*	$NetBSD: lcdctl.c,v 1.3.6.2 2012/04/17 00:07:12 yamt Exp $	*/

/*-
 * Copyright (C) 2012 NONAKA Kimihiro <nonaka@netbsd.org>
 * All rights reserved.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: lcdctl.c,v 1.3.6.2 2012/04/17 00:07:12 yamt Exp $");

#include "ioexp.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>

#include <zaurus/zaurus/zaurus_var.h>
#include <zaurus/dev/lcdctlvar.h>
#include <zaurus/dev/zsspvar.h>
#include <zaurus/dev/scoopvar.h>
#include <zaurus/dev/ioexpvar.h>

struct lcdctl_backlight {
	int	duty;		/* LZ9JG18 DAC value */
	int	cont;		/* BACKLIGHT_CONT signal */
	int	on;		/* BACKLIGHT_ON signal */
};

struct lcdctl_softc {
	device_t	sc_dev;

	int		sc_brightness;
	int		sc_brightnesscurval;
	bool		sc_islit;
	bool		sc_isblank;

	int		sc_nbacklighttbl;
	const struct lcdctl_backlight *sc_backlighttbl;
};

/* for SL-C1000/SL-C3x00 */
static const struct lcdctl_backlight lcdctl_backlight_c3000[] = {
	{ 0x00, 0,  0 },	/* 0:  Off */
	{ 0x00, 0,  1 },	/* 1:   0% */
	{ 0x01, 0,  1 },	/* 2:  20% */
	{ 0x07, 0,  1 },	/* 3:  40% */
	{ 0x01, 1,  1 },	/* 4:  60% */
	{ 0x07, 1,  1 },	/* 5:  80% */
	{ 0x11, 1,  1 }		/* 6: 100% */
};

static int	lcdctl_match(device_t, cfdata_t, void *);
static void	lcdctl_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(lcdctl, sizeof(struct lcdctl_softc),
	lcdctl_match, lcdctl_attach, NULL, NULL);

static void lcdctl_brightness_up(device_t);
static void lcdctl_brightness_down(device_t);
static void lcdctl_display_on(device_t);
static void lcdctl_display_off(device_t);

static void lcdctl_set_brightness(struct lcdctl_softc *, int);
static void lcdctl_set_blank(struct lcdctl_softc *, bool);
static void lcdctl_set_backlight(struct lcdctl_softc *, bool);

static struct lcdctl_softc *lcdctl_sc;

static int
lcdctl_match(device_t parent, cfdata_t cf, void *aux)
{
	struct zssp_attach_args *aa = aux;

	if (strcmp("lcdctl", aa->zaa_name))
		return 0;

	return 1;
}

static void
lcdctl_attach(device_t parent, device_t self, void *aux)
{
	struct lcdctl_softc *sc = device_private(self);

	sc->sc_dev = self;

	aprint_normal("\n");
	aprint_naive("\n");

	sc->sc_brightness = sc->sc_brightnesscurval = 1;
	sc->sc_islit = true;
	sc->sc_isblank = false;

	if (ZAURUS_ISC1000 || ZAURUS_ISC3000) {
		sc->sc_nbacklighttbl = __arraycount(lcdctl_backlight_c3000);
		sc->sc_backlighttbl = lcdctl_backlight_c3000;
	} else {
		/* XXX: Is this okay for C7x0/860? */
		sc->sc_nbacklighttbl = __arraycount(lcdctl_backlight_c3000);
		sc->sc_backlighttbl = lcdctl_backlight_c3000;
	}

	/* Start with approximately 40% of full brightness. */
	lcdctl_set_brightness(sc, 3);

	lcdctl_sc = sc;

	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
	    lcdctl_brightness_up, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
	    lcdctl_brightness_down, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_ON,
	    lcdctl_display_on, true))
		aprint_error_dev(self, "couldn't register event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_OFF,
	    lcdctl_display_off, true))
		aprint_error_dev(self, "couldn't register event handler\n");
}

static void
lcdctl_brightness_up(device_t dv)
{
	struct lcdctl_softc *sc = device_private(dv);

	lcdctl_set_brightness(sc, sc->sc_brightness + 1);
}

static void
lcdctl_brightness_down(device_t dv)
{
	struct lcdctl_softc *sc = device_private(dv);

	lcdctl_set_brightness(sc, sc->sc_brightness - 1);
}

static void
lcdctl_display_on(device_t dv)
{
	struct lcdctl_softc *sc = device_private(dv);

	lcdctl_set_blank(sc, false);
	lcdctl_set_backlight(sc, true);
}

static void
lcdctl_display_off(device_t dv)
{
	struct lcdctl_softc *sc = device_private(dv);

	lcdctl_set_backlight(sc, false);
	lcdctl_set_blank(sc, true);
}

static __inline void
set_backlight(const struct lcdctl_backlight *bl)
{

	(void) zssp_ic_send(ZSSP_IC_LZ9JG18, bl->duty);
	if (ZAURUS_ISC1000)
		ioexp_set_backlight(bl->on, bl->cont);
	else
		scoop_set_backlight(bl->on, bl->cont);
}

static void
lcdctl_set_brightness_internal(struct lcdctl_softc *sc, int newval)
{
	int i;

	/*
	 * It appears that the C3000 backlight can draw too much power if we
	 * switch it from a low to a high brightness.  Increasing brightness
	 * in steps avoids this issue.
	 */
	if (newval > sc->sc_brightnesscurval) {
		for (i = sc->sc_brightnesscurval + 1; i <= newval; i++) {
			set_backlight(&sc->sc_backlighttbl[i]);
			delay(5000);
		}
	} else
		set_backlight(&sc->sc_backlighttbl[newval]);

	sc->sc_brightnesscurval = newval;
}

static void
lcdctl_set_brightness(struct lcdctl_softc *sc, int newval)
{
	int maxval = sc->sc_nbacklighttbl - 1;

	if (newval < 0)
		newval = 0;
	else if (newval > maxval)
		newval = maxval;

	if (sc->sc_islit && !sc->sc_isblank)
		lcdctl_set_brightness_internal(sc, newval);

	if (newval > 0)
		sc->sc_brightness = newval;
}

static void
lcdctl_set_backlight(struct lcdctl_softc *sc, bool onoff)
{

	if (!onoff) {
		lcdctl_set_brightness(sc, 0);
		sc->sc_islit = false;
	} else {
		sc->sc_islit = true;
		lcdctl_set_brightness(sc, sc->sc_brightness);
	}
}

static void
lcdctl_set_blank(struct lcdctl_softc *sc, bool isblank)
{

	if (isblank) {
		lcdctl_set_brightness(sc, 0);
		sc->sc_isblank = true;
	} else {
		sc->sc_isblank = false;
		lcdctl_set_brightness(sc, sc->sc_brightness);
	}
}

static void
lcdctl_set_onoff(struct lcdctl_softc *sc, bool onoff)
{

	if (!onoff) {
		lcdctl_set_brightness(sc, 0);
	} else {
		lcdctl_set_brightness(sc, sc->sc_brightness);
	}
}

static int
lcdctl_param_ioctl(struct lcdctl_softc *sc, u_long cmd, struct wsdisplay_param *dp)
{
	int rv = EINVAL;

	switch (dp->param) {
	case WSDISPLAYIO_PARAM_BACKLIGHT:
		if (cmd == WSDISPLAYIO_GETPARAM) {
			dp->min = 0;
			dp->max = 1;
			dp->curval = sc->sc_islit ? 1 : 0;
			rv = 0;
		} else if (cmd == WSDISPLAYIO_SETPARAM) {
			lcdctl_set_backlight(sc, dp->curval != 0);
			rv = 0;
		}
		break;

	case WSDISPLAYIO_PARAM_CONTRAST:
		/* unsupported */
		rv = ENOTTY;
		break;

	case WSDISPLAYIO_PARAM_BRIGHTNESS:
		if (cmd == WSDISPLAYIO_GETPARAM) {
			dp->min = 1;
			dp->max = sc->sc_nbacklighttbl - 1;
			dp->curval = sc->sc_brightness;
			rv = 0;
		} else if (cmd == WSDISPLAYIO_SETPARAM) {
			lcdctl_set_brightness(sc, dp->curval);
			rv = 0;
		}
		break;
	}
	return rv;
}

void
lcdctl_brightness(int newval)
{

	if (__predict_true(lcdctl_sc != NULL))
		lcdctl_set_brightness(lcdctl_sc, newval);
}

void
lcdctl_blank(bool isblank)
{

	if (__predict_true(lcdctl_sc != NULL))
		lcdctl_set_blank(lcdctl_sc, isblank);
}

void
lcdctl_onoff(bool onoff)
{

	if (__predict_true(lcdctl_sc != NULL))
		lcdctl_set_onoff(lcdctl_sc, onoff);
}

int
lcdctl_param(u_long cmd, struct wsdisplay_param *dp)
{

	if (__predict_true(lcdctl_sc != NULL))
		return lcdctl_param_ioctl(lcdctl_sc, cmd, dp);
	return ENOTTY;	/* unsupported */
}
