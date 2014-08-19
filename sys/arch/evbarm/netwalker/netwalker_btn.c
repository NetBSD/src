/*	$NetBSD: netwalker_btn.c,v 1.2.8.2 2014/08/20 00:02:55 tls Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netwalker_btn.c,v 1.2.8.2 2014/08/20 00:02:55 tls Exp $");

#include "opt_imxspi.h"
#include "opt_mousebtn.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/lock.h>
#include <sys/callout.h>
#include <sys/gpio.h>
#include <sys/bus.h>
#include <sys/mutex.h>

#include <dev/wscons/wsconsio.h>
#include <dev/wscons/wsmousevar.h>

#include <dev/gpio/gpiovar.h>

#define GPIO1_BASE		160

#define MOUSEBTN_PIN_LEFT	0
#define MOUSEBTN_PIN_RIGHT	1
#define MOUSEBTN_NPINS		2

#define POLLRATE (hz/10)

struct mousebtn_softc
{
	device_t sc_dev;
	void *sc_gpio;
	void *sc_intr[MOUSEBTN_NPINS];

	struct gpio_pinmap sc_map;
	int sc_map_pins[MOUSEBTN_NPINS];

	int sc_buttons;

	struct callout sc_c;
	kmutex_t sc_lock;

	int sc_enabled;
	struct device *sc_wsmousedev;
};

static int mousebtn_match(device_t, cfdata_t, void *);
static void mousebtn_attach(device_t, device_t, void *);
static int mousebtn_detach(device_t, int);

CFATTACH_DECL_NEW(netwalker_btn, sizeof(struct mousebtn_softc),
    mousebtn_match, mousebtn_attach, mousebtn_detach, NULL);

static void mousebtn_poll(void *);
static int mousebtn_intr(void *);

static int mousebtn_enable(void *v);
static void mousebtn_disable(void *v);
static int mousebtn_ioctl(void *, u_long, void *, int, struct lwp *);

static bool mousebtn_resume(device_t, const pmf_qual_t *);
static bool mousebtn_suspend(device_t, const pmf_qual_t *);

static const struct wsmouse_accessops mousebtn_accessops = {
	.enable	 = mousebtn_enable,
	.ioctl	 = mousebtn_ioctl,
	.disable = mousebtn_disable
};

static int
mousebtn_match(device_t parent, cfdata_t cf, void * aux)
{
	struct gpio_attach_args *ga = aux;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return 0;
	if (ga->ga_offset == -1)
		return 0;

	/* check that we have enough pins */
	if (gpio_npins(ga->ga_mask) != MOUSEBTN_NPINS) {
		aprint_debug("%s: invalid pin mask 0x%02x\n", cf->cf_name,
		    ga->ga_mask);
		return 0;
	}

	return 1;
}

static void
mousebtn_attach(device_t parent, device_t self, void *aux)
{
	struct mousebtn_softc *sc = device_private(self);
	struct gpio_attach_args *ga = aux;
	int caps;
	struct wsmousedev_attach_args a;

	sc->sc_dev = self;
	sc->sc_gpio = ga->ga_gpio;

	/* map pins */
	sc->sc_map.pm_map = sc->sc_map_pins;
	if (gpio_pin_map(sc->sc_gpio, ga->ga_offset, ga->ga_mask,
		&sc->sc_map)) {
		aprint_error(": couldn't map the pins\n");
		return;
	}

	/* configure left pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, MOUSEBTN_PIN_LEFT);
	if (!(caps & GPIO_PIN_INPUT)) {
		aprint_error(": pin is unable to read input\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, MOUSEBTN_PIN_LEFT,
	    GPIO_PIN_INPUT);

	/* configure right pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, MOUSEBTN_PIN_RIGHT);
	if (!(caps & GPIO_PIN_INPUT)) {
		aprint_error(": pin is unable to read input\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, MOUSEBTN_PIN_RIGHT,
	    GPIO_PIN_INPUT);

	/* interrupt settings */
	sc->sc_intr[0] = intr_establish(GPIO1_BASE + ga->ga_offset,
	    IPL_VM, IST_EDGE_BOTH, mousebtn_intr, sc);
	if (sc->sc_intr[0] == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		return;
	}
	sc->sc_intr[1] = intr_establish(GPIO1_BASE + ga->ga_offset + 1,
	     IPL_VM, IST_EDGE_BOTH, mousebtn_intr, sc);
	if (sc->sc_intr[1] == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		intr_disestablish(sc->sc_intr[0]);
		return;
	}

	aprint_normal(": NetWalker mouse button\n");
	aprint_naive(": NetWalker mouse button\n");

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);
	callout_init(&sc->sc_c, 0);

	a.accessops = &mousebtn_accessops;
	a.accesscookie = sc;

	sc->sc_buttons = 0;
	sc->sc_wsmousedev = config_found(self, &a, wsmousedevprint);
}

static int
mousebtn_detach(device_t self, int flags)
{
	struct mousebtn_softc *sc = device_private(self);

	if (sc->sc_intr[0] != NULL)
		intr_disestablish(sc->sc_intr[0]);
	if (sc->sc_intr[1] != NULL)
		intr_disestablish(sc->sc_intr[1]);

	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);

	return 0;
}

static void
mousebtn_poll(void *arg)
{
	struct mousebtn_softc *sc = (struct mousebtn_softc *)arg;
	uint32_t buttons = 0;
	int s;
	int left;
	int right;

	mutex_enter(&sc->sc_lock);

	left = !gpio_pin_read(sc->sc_gpio, &sc->sc_map, MOUSEBTN_PIN_LEFT);
	right = !gpio_pin_read(sc->sc_gpio, &sc->sc_map, MOUSEBTN_PIN_RIGHT);
	buttons = (right << 2) | left;

	if (sc->sc_buttons != buttons) {
		s = spltty();
		wsmouse_input(sc->sc_wsmousedev, buttons, 0, 0, 0, 0,
		    WSMOUSE_INPUT_DELTA);
		sc->sc_buttons = buttons;
		splx(s);
	}

	mutex_exit(&sc->sc_lock);

#if defined(MOUSEBTN_POLLING)
	if (sc->sc_enabled)
		callout_reset(&sc->sc_c, POLLRATE, mousebtn_poll, sc);
#endif
	return;
}

static int
mousebtn_intr(void *v)
{
	struct mousebtn_softc *sc = v;

	if (sc->sc_enabled)
		callout_reset(&sc->sc_c, POLLRATE, mousebtn_poll, sc);

	return 1;
}


int
mousebtn_ioctl(void *v, u_long cmd, void *data, int flag, struct lwp *l)
{
	struct wsmouse_id *id;

	switch (cmd) {
	case WSMOUSEIO_GTYPE:
		*(u_int *)data = WSMOUSE_TYPE_PS2;
		return 0;
	case WSMOUSEIO_GETID:
		id = (struct wsmouse_id *)data;
		if (id->type != WSMOUSE_ID_TYPE_UIDSTR)
			return EINVAL;
		strlcpy(id->data, "NetWalker Mouse Button", WSMOUSE_ID_MAXLEN);
		id->length = strlen(id->data);
		return 0;
	}

	return EPASSTHROUGH;
}

int
mousebtn_enable(void *v)
{
	struct mousebtn_softc *sc = (struct mousebtn_softc *)v;

	if (sc->sc_enabled)
		return EBUSY;

	if (!pmf_device_register(sc->sc_dev, mousebtn_suspend, mousebtn_resume))
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");

	sc->sc_enabled = 1;
#if defined(MOUSEBTN_POLLING)
	callout_reset(&sc->sc_c, POLLRATE, mousebtn_poll, sc);
#endif

	return 0;
}

void
mousebtn_disable(void *v)
{
	struct mousebtn_softc *sc = (struct mousebtn_softc *)v;

	if (!sc->sc_enabled)
		return;

	pmf_device_deregister(sc->sc_dev);

	sc->sc_enabled = 0;

	return;
}

static bool
mousebtn_suspend(device_t dv, const pmf_qual_t *qual)
{
	struct mousebtn_softc *sc = device_private(dv);

#if defined(MOUSEBTN_POLLING)
	callout_stop(&sc->sc_c);
#endif
	sc->sc_enabled = 0;

	return true;
}

static bool
mousebtn_resume(device_t dv, const pmf_qual_t *qual)
{
	struct mousebtn_softc *sc = device_private(dv);

	sc->sc_enabled = 1;
#if defined(MOUSEBTN_POLLING)
	callout_reset(&sc->sc_c, POLLRATE, mousebtn_poll, sc);
#endif
	return true;
}

