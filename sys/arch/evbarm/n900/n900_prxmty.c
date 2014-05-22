/*	$NetBSD: n900_prxmty.c,v 1.2.10.2 2014/05/22 11:39:42 yamt Exp $ */

/*
 * Proximity sensor driver for the Nokia N900.
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
__KERNEL_RCSID(0, "$NetBSD: n900_prxmty.c,v 1.2.10.2 2014/05/22 11:39:42 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>
#include <dev/sysmon/sysmonvar.h>
#include <dev/sysmon/sysmon_taskq.h>

#include <arm/omap/omap2_gpio.h>


/* The base interrupt for the corresponding GPIO device where this driver
 * attaches. This is an ugly workaround the current limitations of gpio(4),
 * which does not seem to allow a better way to locate the interrupt yet. */
#define N900PRXMTY_GPIO_BASE	160

#define N900PRXMTY_PIN_INPUT	0
#define N900PRXMTY_NPINS	1


struct n900prxmty_softc
{
	device_t		sc_dev;
	void *			sc_gpio;
	void *			sc_intr;

	struct gpio_pinmap	sc_map;
	int			sc_map_pins[N900PRXMTY_NPINS];

	struct sysmon_pswitch	sc_smpsw;
};

static int	n900prxmty_match(device_t, cfdata_t, void *);
static void	n900prxmty_attach(device_t, device_t, void *);
static int	n900prxmty_detach(device_t, int);

CFATTACH_DECL_NEW(n900prxmty, sizeof(struct n900prxmty_softc),
	n900prxmty_match, n900prxmty_attach, n900prxmty_detach, NULL);

static void	n900prxmty_refresh(void *);

static int	n900prxmty_intr(void *);


static int
n900prxmty_match(device_t parent, cfdata_t cf, void * aux)
{
	struct gpio_attach_args *ga = aux;

	if (strcmp(ga->ga_dvname, cf->cf_name))
		return 0;

	if (ga->ga_offset == -1)
		return 0;

	/* check that we have enough pins */
	if (gpio_npins(ga->ga_mask) != N900PRXMTY_NPINS) {
		aprint_debug("%s: invalid pin mask 0x%02x\n", cf->cf_name,
				ga->ga_mask);
		return 0;
	}

	return 1;
}

static void
n900prxmty_attach(device_t parent, device_t self, void *aux)
{
	struct n900prxmty_softc *sc = device_private(self);
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

	/* configure the input pin */
	caps = gpio_pin_caps(sc->sc_gpio, &sc->sc_map, N900PRXMTY_PIN_INPUT);
	if (!(caps & GPIO_PIN_INPUT)) {
		aprint_error(": pin is unable to read input\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}
	gpio_pin_ctl(sc->sc_gpio, &sc->sc_map, N900PRXMTY_PIN_INPUT,
			GPIO_PIN_INPUT);

	sc->sc_intr = intr_establish(N900PRXMTY_GPIO_BASE + ga->ga_offset,
			IPL_VM, IST_EDGE_BOTH, n900prxmty_intr, sc);
	if (sc->sc_intr == NULL) {
		aprint_error(": couldn't establish interrupt\n");
		gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
		return;
	}

	aprint_normal(": N900 proximity sensor\n");
	aprint_naive(": N900 proximity sensor\n");

	if (!pmf_device_register(sc->sc_dev, NULL, NULL)) {
		aprint_error_dev(sc->sc_dev,
		    "couldn't establish power handler\n");
	}

	sysmon_task_queue_init();
	sc->sc_smpsw.smpsw_name = device_xname(self);
	sc->sc_smpsw.smpsw_type = PSWITCH_TYPE_HOTKEY;
	sysmon_pswitch_register(&sc->sc_smpsw);

	n900prxmty_refresh(sc);
}

static int
n900prxmty_detach(device_t self, int flags)
{
	struct n900prxmty_softc *sc = device_private(self);

	if (sc->sc_intr != NULL) {
		intr_disestablish(sc->sc_intr);
	}

	gpio_pin_unmap(sc->sc_gpio, &sc->sc_map);
	pmf_device_deregister(self);
	sysmon_task_queue_fini();

	return 0;
}

static int
n900prxmty_intr(void *v)
{
	struct n900prxmty_softc *sc = v;

	sysmon_task_queue_sched(0, n900prxmty_refresh, sc);
	return 1;
}

static void
n900prxmty_refresh(void *v)
{
	struct n900prxmty_softc *sc = v;
	int i;
	int event;

	i = gpio_pin_read(sc->sc_gpio, &sc->sc_map, N900PRXMTY_PIN_INPUT);
	event = (i == GPIO_PIN_HIGH)
		? PSWITCH_EVENT_PRESSED : PSWITCH_EVENT_RELEASED;

	/* report the event */
	sysmon_pswitch_event(&sc->sc_smpsw, event);
}
