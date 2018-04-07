/*	$NetBSD: panel_fdt.c,v 1.1.2.2 2018/04/07 04:12:14 pgoyette Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Manuel Bouyer.
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

/*
 * lvds panel driver.
 * specified in linux/Documentation/devicetree/bindings/display/panel/
 * Simple RGB panels could be added as well
 * registers an endpoint for use by graphic controller drivers
 *
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(1, "$NetBSD: panel_fdt.c,v 1.1.2.2 2018/04/07 04:12:14 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/panel_fdt.h>

static int fdt_panel_match(device_t, cfdata_t, void *);
static void fdt_panel_attach(device_t, device_t, void *);
static void *fdt_panel_get_data(device_t, struct fdt_endpoint *);
static int fdt_panel_enable(device_t, struct fdt_endpoint *, bool);

struct fdt_panel_softc {
	device_t sc_dev;
	int sc_phandle;
	struct fdt_panel sc_panel;
	struct fdt_device_ports sc_ports;
#define MAX_GPIO_ENABLES 8
	struct fdtbus_gpio_pin *sc_gpios_enable[MAX_GPIO_ENABLES];
};


CFATTACH_DECL_NEW(fdt_panel, sizeof(struct fdt_panel_softc),
    fdt_panel_match, fdt_panel_attach, NULL, NULL);

static const struct of_compat_data compat_data[] = {
	{"panel-lvds", PANEL_LVDS},
	{"panel-dual-lvds", PANEL_DUAL_LVDS},
	{ NULL }
};

static int
fdt_panel_match(device_t parent, cfdata_t cf, void *aux)
{
	const struct fdt_attach_args *faa = aux;

	return of_match_compat_data(faa->faa_phandle, compat_data);
}

static void
fdt_panel_attach(device_t parent, device_t self, void *aux)
{
	struct fdt_panel_softc *sc = device_private(self);
	const struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const struct display_timing * const timing =
	    &sc->sc_panel.panel_timing;
	int child;
	char buf[16];
	const char *val;
	int i;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_panel.panel_type =
	    of_search_compatible(phandle, compat_data)->data;

	if (of_getprop_uint32(phandle, "width-mm", &sc->sc_panel.panel_width) ||
	    of_getprop_uint32(phandle, "height-mm", &sc->sc_panel.panel_height)){
		aprint_error(": missing width-mm or height-mm properties\n");
		return;
	}
	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		if (OF_getprop(child, "name", buf, sizeof(buf)) <= 0)
			continue;
		if (strcmp(buf, "panel-timing") != 0)
		    continue;

		if (display_timing_parse(child,
		    &sc->sc_panel.panel_timing) != 0) {
			aprint_error(": failed to parse panel-timing\n");
			return;
		}
	}
	if (sc->sc_panel.panel_timing.clock_freq == 0) {
		aprint_error(": missing panel-timing\n");
		return;
	}
	switch(sc->sc_panel.panel_type) {
	case PANEL_LVDS:
	case PANEL_DUAL_LVDS:
		val = fdtbus_get_string(phandle, "data-mapping");
		if (val == NULL) {
			aprint_error(": missing data-mapping\n");
			return;
		}
		if (strcmp(val, "jeida-18") == 0)
			sc->sc_panel.panel_lvds_format = LVDS_JEIDA_18;
		else if (strcmp(val, "jeida-24") == 0)
			sc->sc_panel.panel_lvds_format = LVDS_JEIDA_24;
		else if (strcmp(val, "vesa-24") == 0)
			sc->sc_panel.panel_lvds_format = LVDS_VESA_24;
		else {
			aprint_error(": unkown data-mapping \"%s\"\n", val);
			return;
		}
		break;
	default:
		panic("unknown panel type %d", sc->sc_panel.panel_type);
	}

	aprint_naive("\n");
	aprint_normal(": %dx%d", timing->hactive, timing->vactive);
	switch(sc->sc_panel.panel_type) {
	case PANEL_LVDS:
		aprint_normal(" LVDS");
		break;
	case PANEL_DUAL_LVDS:
		aprint_normal(" dual-link LVDS");
		break;
	default:
		panic(" unknown panel type %d", sc->sc_panel.panel_type);
	}
	aprint_normal(" panel\n");

	for (i = 0; i < MAX_GPIO_ENABLES ; i++) {
		sc->sc_gpios_enable[i] = fdtbus_gpio_acquire_index(phandle,
		    "enable-gpios", i, GPIO_PIN_OUTPUT);
		if (sc->sc_gpios_enable[i] == NULL)
			break;
	}

	aprint_verbose_dev(self, "%d enable GPIO%c\n", i, i > 1 ? 's' : ' ');

	sc->sc_ports.dp_ep_get_data = fdt_panel_get_data;
	sc->sc_ports.dp_ep_enable = fdt_panel_enable;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_PANEL);
}

static void *
fdt_panel_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct fdt_panel_softc *sc = device_private(dev);
	return &sc->sc_panel;
}

static int
fdt_panel_enable(device_t dev, struct fdt_endpoint *ep, bool enable)
{
	struct fdt_panel_softc *sc = device_private(dev);
	for (int i = 0; i < MAX_GPIO_ENABLES ; i++) {
		if (sc->sc_gpios_enable[i] == NULL)
			break;
		fdtbus_gpio_write(sc->sc_gpios_enable[i], enable);
	}
	return 0;
}
