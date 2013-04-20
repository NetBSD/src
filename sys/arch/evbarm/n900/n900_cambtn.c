/*	$NetBSD: n900_cambtn.c,v 1.1 2013/04/20 03:37:55 khorben Exp $ */

/*
 * Camera button driver for the Nokia N900.
 *
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Pierre Pronchery (khorben@defora.org).
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
__KERNEL_RCSID(0, "$NetBSD: n900_cambtn.c,v 1.1 2013/04/20 03:37:55 khorben Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/kmem.h>

#include <dev/gpio/gpiovar.h>
#include <dev/sysmon/sysmonvar.h>

#include <arm/omap/omap2_gpio.h>


/* The base interrupt for the corresponding GPIO device where this driver
 * attaches. This is an ugly workaround the current limitations of gpio(4),
 * which does not seem to allow a better way to locate the interrupt yet. */
#define N900CAMBTN_GPIO_BASE	160

#define N900CAMBTN_PIN_FOCUS	0
#define N900CAMBTN_PIN_CAPTURE	1
#define N900CAMBTN_NPINS	2


struct n900cambtn_softc
{
	device_t		sc_dev;
	void *			sc_gpio;
	void *			sc_intr[N900CAMBTN_NPINS];

	struct gpio_pinmap	sc_map;
	int			sc_map_pins[N900CAMBTN_NPINS];

	struct sysmon_pswitch	sc_smpsw[N900CAMBTN_NPINS];
	int			sc_state[N900CAMBTN_NPINS];
};

static int	n900cambtn_match(device_t, cfdata_t, void *);
static void	n900cambtn_attach(device_t, device_t, void *);
static int	n900cambtn_detach(device_t, int);

CFATTACH_DECL_NEW(n900cambtn, sizeof(struct n900cambtn_softc),
	n900cambtn_match, n900cambtn_attach, n900cambtn_detach, NULL);

static void	n900cambtn_refresh_pin(struct n900cambtn_softc *, int);

static int	n900cambtn_intr(void *v);


static int
n900cambtn_match(device_t parent, cfdata_t cf, void * aux)
{
	struct gpio_attach_args *ga = aux;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return 0;

	if (ga->ga_offset == -1)
		return 0;

	/* check that we have enough pins */
	if (gpio_npins(ga->ga_mask) != N900CAMBTN_NPINS) {
		aprint_debug("%s: invalid pin mask 0x%02x\n", cf->cf_name,
				ga->ga_mask);
		return 0;
	}

	return 1;
}

static void
n900cambtn_attach(device_t parent, device_t self, void *aux)
{
	struct n900cambtn_softc *sc = device_private(self);
	struct gpio_attach_args *ga = aux;
	int caps;

	sc->sc_dev = self;
	sc->sc_gpio = ga->ga_gpio;

	/* map pins */
	sc->sc_map.pm_map = sc->sc_map_pins;
	if (gpio_pin_map(sc->sc_gpio, ga->ga_offset, ga->ga_mask,
				&sc->sc_map)) {
		aprint_error(": couldn't map the pins\n");
		return;
	}

	/* configure the capture pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, N900CAMBTN_PIN_CAPTURE);
	if (!(caps & GPIO_PIN_INPUT)) {
		aprint_error(": pin is unable to read input\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, N900CAMBTN_PIN_CAPTURE,
			GPIO_PIN_INPUT);

	/* configure the focus pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, N900CAMBTN_PIN_FOCUS);
	if (!(caps & GPIO_PIN_INPUT)) {
		aprint_error(": pin is unable to read input\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, N900CAMBTN_PIN_FOCUS,
			GPIO_PIN_INPUT);

	sc->sc_intr[0] = intr_establish(N900CAMBTN_GPIO_BASE + ga->ga_offset,
			IPL_VM, IST_EDGE_BOTH, n900cambtn_intr, sc);
	if (sc->sc_intr[0] == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		return;
	}

	sc->sc_intr[1] = intr_establish(N900CAMBTN_GPIO_BASE + ga->ga_offset
			+ 1, IPL_VM, IST_EDGE_BOTH, n900cambtn_intr, sc);
	if (sc->sc_intr[1] == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		intr_disestablish(sc->sc_intr[0]);
		return;
	}

	aprint_normal(": N900 camera button\n");
	aprint_naive(": N900 camera button\n");

	if (!pmf_device_register(sc->sc_dev, NULL, NULL)) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");
	}

	/* focus button */
	sc->sc_smpsw[0].smpsw_name = kmem_asprintf("%s%s", device_xname(self),
			"focus");
	sc->sc_smpsw[0].smpsw_type = PSWITCH_TYPE_HOTKEY;
	sc->sc_state[0] = PSWITCH_EVENT_RELEASED;
	sysmon_pswitch_register(&sc->sc_smpsw[0]);

	/* capture button */
	sc->sc_smpsw[1].smpsw_name = kmem_asprintf("%s%s", device_xname(self),
			"capture");
	sc->sc_smpsw[1].smpsw_type = PSWITCH_TYPE_HOTKEY;
	sc->sc_state[1] = PSWITCH_EVENT_RELEASED;
	sysmon_pswitch_register(&sc->sc_smpsw[1]);
}

static int
n900cambtn_detach(device_t self, int flags)
{
	struct n900cambtn_softc *sc = device_private(self);

	if (sc->sc_intr[0] != NULL) {
		intr_disestablish(sc->sc_intr[0]);
	}

	if (sc->sc_intr[1] != NULL) {
		intr_disestablish(sc->sc_intr[1]);
	}

	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
	pmf_device_deregister(self);

	return 0;
}

static int
n900cambtn_intr(void *v)
{
	struct n900cambtn_softc *sc = v;

	n900cambtn_refresh_pin(sc, N900CAMBTN_PIN_CAPTURE);
	n900cambtn_refresh_pin(sc, N900CAMBTN_PIN_FOCUS);
	return 1;
}

static void
n900cambtn_refresh_pin(struct n900cambtn_softc *sc, int pin)
{
	int i;
	int event;

	i = gpio_pin_read(sc->sc_gpio, &sc->sc_map, pin);
	event = (i == GPIO_PIN_HIGH)
		? PSWITCH_EVENT_RELEASED : PSWITCH_EVENT_PRESSED;

	/* report the event if relevant */
	if(sc->sc_state[pin] != event) {
		sysmon_pswitch_event(&sc->sc_smpsw[pin], event);
		sc->sc_state[pin] = event;
	}
}
