/*	$NetBSD: netwalker_backlight.c,v 1.1.10.3 2017/12/03 11:36:06 jdolecek Exp $	*/

/*
 * Copyright (c) 2014  Genetec Corporation.  All rights reserved.
 * Written by Hashimoto Kenichi for Genetec Corporation.
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
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netwalker_backlight.c,v 1.1.10.3 2017/12/03 11:36:06 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/pmf.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsdisplayvar.h>
#include <dev/wsfb/genfbvar.h>

#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <arm/imx/imx51reg.h>
#include <arm/imx/imx51var.h>
#include <arm/imx/imx51_ccmvar.h>
#include <arm/imx/imxpwmvar.h>

#include <evbarm/netwalker/netwalker_backlightvar.h>

#define BRIGHTNESS_MAX	255

struct netwalker_backlight_softc {
	struct imxpwm_softc sc_imxpwm;

	int sc_brightness;
	bool sc_islit;
};

static struct netwalker_backlight_softc *netwalker_backlight_sc;

static struct genfb_parameter_callback gpc_backlight;
static struct genfb_parameter_callback gpc_brightness;

static int netwalker_backlight_match(device_t, cfdata_t, void *);
static void netwalker_backlight_attach(device_t, device_t, void *);
static int netwalker_backlight_detach(device_t, int);

CFATTACH_DECL_NEW(netwalker_backlight, sizeof(struct netwalker_backlight_softc),
    netwalker_backlight_match, netwalker_backlight_attach, netwalker_backlight_detach, NULL);

static bool netwalker_backlight_resume(device_t, const pmf_qual_t *);
static bool netwalker_backlight_suspend(device_t, const pmf_qual_t *);
static void netwalker_backlight_on(device_t);
static void netwalker_backlight_off(device_t);
static void netwalker_brightness_up(device_t);
static void netwalker_brightness_down(device_t);
static void netwalker_set_brightness(struct netwalker_backlight_softc *, int);

static int
netwalker_backlight_match(device_t parent, cfdata_t cf, void * aux)
{
	return imxpwm_match(parent, cf, aux);
}

static void
netwalker_backlight_attach(device_t parent, device_t self, void *aux)
{
	struct netwalker_backlight_softc *sc = device_private(self);
	struct imxpwm_softc *imxpwm = &sc->sc_imxpwm;

	imxpwm->sc_dev = self;
	imxpwm->sc_hz = 1000; /* 1000 Hz */
	imxpwm->sc_handler = NULL;
	imxpwm->sc_cookie = sc;
	imxpwm_attach(imxpwm, aux);

	aprint_normal(": LCD BackLight Control\n");
	aprint_naive(": LCD BackLight Control\n");

	netwalker_backlight_sc = sc;

	/* BackLight 100% On */
	sc->sc_brightness = BRIGHTNESS_MAX;
	sc->sc_islit = true;
	imxpwm_set_pwm(imxpwm, 1000);

	if (!pmf_device_register(self, netwalker_backlight_suspend,
		netwalker_backlight_resume))
		aprint_error_dev(self,
		    "couldn't establish backlight handler\n");

	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_UP,
		netwalker_brightness_up, true))
		aprint_error_dev(self,
		    "couldn't register BRIGHTNESS UP event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_BRIGHTNESS_DOWN,
		netwalker_brightness_down, true))
		aprint_error_dev(self,
		    "couldn't register BRIGHTNESS DOWN event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_ON,
		netwalker_backlight_on, true))
		aprint_error_dev(self,
		    "couldn't register DISPLAY ON event handler\n");
	if (!pmf_event_register(self, PMFE_DISPLAY_OFF,
		netwalker_backlight_off, true))
		aprint_error_dev(self,
		    "couldn't register DISPLAY OFF event handler\n");
	if (!pmf_event_register(self, PMFE_CHASSIS_LID_OPEN,
		netwalker_backlight_on, true))
		aprint_error_dev(self,
		    "couldn't register LID OPEN event handler\n");
	if (!pmf_event_register(self, PMFE_CHASSIS_LID_CLOSE,
		netwalker_backlight_off, true))
		aprint_error_dev(self,
		    "couldn't register LID CLOSE event handler\n");
}

static int
netwalker_backlight_detach(device_t self, int flags)
{
	struct netwalker_backlight_softc *sc = device_private(self);
	struct imxpwm_softc *imxpwm = &sc->sc_imxpwm;

	imxpwm_set_pwm(imxpwm, 0);
	pmf_device_deregister(self);
	return 0;
}

static int
netwalker_backlight_get_backlight(void *cookie, int *state)
{
	struct netwalker_backlight_softc *sc = *(struct netwalker_backlight_softc **)cookie;
	*state = sc->sc_islit;
	return 0;
}

static int
netwalker_backlight_set_backlight(void *cookie, int state)
{
	struct netwalker_backlight_softc *sc = *(struct netwalker_backlight_softc **)cookie;

	KASSERT(state >= 0 && state <= 1);

	sc->sc_islit = state;
	netwalker_set_brightness(sc, sc->sc_brightness);

	return 0;
}

static int
netwalker_backlight_get_brightness(void *cookie, int *level)
{
	struct netwalker_backlight_softc *sc = *(struct netwalker_backlight_softc **)cookie;

	if (sc->sc_brightness < 0)
		return ENODEV;

	*level = sc->sc_brightness;
	return 0;
}

static int
netwalker_backlight_set_brightness(void *cookie, int level)
{
	struct netwalker_backlight_softc *sc = *(struct netwalker_backlight_softc **)cookie;

	KASSERT(level >= 0 && level <= 255);

	sc->sc_brightness = level;
	netwalker_set_brightness(sc, sc->sc_brightness);

	return 0;
}

static int
netwalker_backlight_upd_brightness(void *cookie, int delta)
{
	struct netwalker_backlight_softc *sc = *(struct netwalker_backlight_softc **)cookie;

	if (sc->sc_brightness < 0)
		return ENODEV;

	sc->sc_brightness += delta;
	if (sc->sc_brightness < 0) sc->sc_brightness = 0;
	if (sc->sc_brightness > 255) sc->sc_brightness = 255;
	netwalker_set_brightness(sc, sc->sc_brightness);

	return 0;
}

void
netwalker_backlight_genfb_parameter_set(prop_dictionary_t dict)
{
	gpc_backlight.gpc_cookie = (void *)&netwalker_backlight_sc;
	gpc_backlight.gpc_set_parameter = netwalker_backlight_set_backlight;
	gpc_backlight.gpc_get_parameter = netwalker_backlight_get_backlight;
	gpc_backlight.gpc_upd_parameter = NULL;
	prop_dictionary_set_uint64(dict, "backlight_callback",
	    (uint64_t)(uintptr_t)&gpc_backlight);

	gpc_brightness.gpc_cookie = (void *)&netwalker_backlight_sc;
	gpc_brightness.gpc_set_parameter = netwalker_backlight_set_brightness;
	gpc_brightness.gpc_get_parameter = netwalker_backlight_get_brightness;
	gpc_brightness.gpc_upd_parameter = netwalker_backlight_upd_brightness;
	prop_dictionary_set_uint64(dict, "brightness_callback",
	    (uint64_t)(uintptr_t)&gpc_brightness);
}

/*
 * Power management
 */
static bool
netwalker_backlight_suspend(device_t dv, const pmf_qual_t *qual)
{
	netwalker_backlight_off(dv);
 	return true;
}

static bool
netwalker_backlight_resume(device_t dv, const pmf_qual_t *qual)
{
	netwalker_backlight_on(dv);
	return true;
}

static void
netwalker_backlight_on(device_t dv)
{
	struct netwalker_backlight_softc *sc = device_private(dv);
	sc->sc_islit = true;
	netwalker_set_brightness(sc, sc->sc_brightness);
}

static void
netwalker_backlight_off(device_t dv)
{
	struct netwalker_backlight_softc *sc = device_private(dv);
	sc->sc_islit = false;
	netwalker_set_brightness(sc, sc->sc_brightness);
}

static void
netwalker_brightness_up(device_t dv)
{
	struct netwalker_backlight_softc *sc = device_private(dv);
	netwalker_set_brightness(sc, sc->sc_brightness + 1);
}

static void
netwalker_brightness_down(device_t dv)
{
	struct netwalker_backlight_softc *sc = device_private(dv);
	netwalker_set_brightness(sc, sc->sc_brightness - 1);
}

static void
netwalker_set_brightness(struct netwalker_backlight_softc *sc, int newval)
{
	struct imxpwm_softc *imxpwm = &sc->sc_imxpwm;

	if (newval < 0)
		newval = 0;
	else if (newval > BRIGHTNESS_MAX)
		newval = BRIGHTNESS_MAX;
	sc->sc_brightness = newval;

	if (sc->sc_islit)
		imxpwm_set_pwm(imxpwm, 1000 * sc->sc_brightness / BRIGHTNESS_MAX);
	else
		imxpwm_set_pwm(imxpwm, 0);
}

int
netwalker_lcd_param_ioctl(u_long cmd, struct wsdisplay_param *dp)
{
	struct netwalker_backlight_softc *sc = netwalker_backlight_sc;
	int rv = EINVAL;

	switch (dp->param) {
	case WSDISPLAYIO_PARAM_BACKLIGHT:
		if (cmd == WSDISPLAYIO_GETPARAM) {
			dp->min = 0;
			dp->max = 1;
			dp->curval = sc->sc_islit ? 1 : 0;
			rv = 0;
		} else if (cmd == WSDISPLAYIO_SETPARAM) {
			if (dp->curval != 0)
				netwalker_backlight_on(sc->sc_imxpwm.sc_dev);
			else
				netwalker_backlight_off(sc->sc_imxpwm.sc_dev);
			rv = 0;
		}
		break;

	case WSDISPLAYIO_PARAM_CONTRAST:
		/* unsupported */
		rv = ENOTSUP;
		break;

	case WSDISPLAYIO_PARAM_BRIGHTNESS:
		if (cmd == WSDISPLAYIO_GETPARAM) {
			dp->min = 0;
			dp->max = BRIGHTNESS_MAX;
			dp->curval = sc->sc_brightness;
			rv = 0;
		} else if (cmd == WSDISPLAYIO_SETPARAM) {
			netwalker_set_brightness(sc, dp->curval);
			rv = 0;
		}
		break;
	}

	return rv;
}
