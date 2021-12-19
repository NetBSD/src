/* $NetBSD: fdt_panel.c,v 1.6 2021/12/19 11:01:10 riastradh Exp $ */

/*-
 * Copyright (c) 2019 Jonathan A. Kollasch <jakllsch@kollasch.net>
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: fdt_panel.c,v 1.6 2021/12/19 11:01:10 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/gpio.h>
#include <sys/systm.h>

#include <dev/fdt/fdt_port.h>
#include <dev/fdt/fdtvar.h>

#include <dev/i2c/ddcvar.h>

#include <drm/drm_drv.h>
#include <drm/drm_edid.h>
#include <drm/drm_panel.h>

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "simple-panel" },
	{ .compat = "boe,nv140fhmn49" },
	DEVICE_COMPAT_EOL
};

struct panel_fdt_softc {
	device_t			sc_dev;
	struct fdt_device_ports		sc_ports;
	struct drm_panel		sc_panel;
	struct fdtbus_gpio_pin *	sc_enable;
	struct fdtbus_regulator *	sc_regulator;
};

#define	to_panel_fdt(x)	container_of(x, struct panel_fdt_softc, sc_panel)

static int
panel_fdt_enable(struct drm_panel * panel)
{
	struct panel_fdt_softc * const sc = to_panel_fdt(panel);

        if (sc->sc_enable) {
		fdtbus_gpio_write(sc->sc_enable, true);
        }

	if (!cold)
		kpause("panele", false, mstohz(200), NULL);

	return 0;
}

static int
panel_fdt_prepare(struct drm_panel * panel)
{
	struct panel_fdt_softc * const sc = to_panel_fdt(panel);

        if (sc->sc_regulator) {
                fdtbus_regulator_enable(sc->sc_regulator);
        }

	if (cold)
		delay(210000);
	else
		kpause("panelp", false, mstohz(210), NULL);

	return 0;
}

static const struct drm_panel_funcs panel_fdt_funcs = {
	.disable = NULL,
	.enable = panel_fdt_enable,
	.get_modes = NULL,
	.get_timings = NULL,
	.prepare = panel_fdt_prepare,
	.unprepare = NULL,
};

static int
panel_fdt_ep_activate(device_t dev, struct fdt_endpoint *ep, bool activate)
{
	struct fdt_endpoint *rep = fdt_endpoint_remote(ep);

	if (fdt_endpoint_port_index(ep) != 0)
		return EINVAL;

	if (fdt_endpoint_type(rep) != EP_DRM_ENCODER)
		return EINVAL;

	return 0;
}

static void *
panel_fdt_ep_get_data(device_t dev, struct fdt_endpoint *ep)
{
	struct panel_fdt_softc * const sc = device_private(dev);

	return &sc->sc_panel;
}

static int
panel_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
panel_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct panel_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	aprint_naive("\n");
	aprint_normal(": panel\n");

	sc->sc_dev = self;

	/* required for "simple-panel" */
        sc->sc_regulator = fdtbus_regulator_acquire(phandle, "power-supply");
        if (sc->sc_regulator == NULL) {
		aprint_error_dev(self, "regulator not found\n");
		return;
	}

	/* optional for "simple-panel" */
	sc->sc_enable = fdtbus_gpio_acquire_index(phandle,
	    "enable-gpios", 0, GPIO_PIN_OUTPUT);

	sc->sc_ports.dp_ep_activate = panel_fdt_ep_activate;
	sc->sc_ports.dp_ep_get_data = panel_fdt_ep_get_data;
	fdt_ports_register(&sc->sc_ports, self, phandle, EP_DRM_PANEL);

	drm_panel_init(&sc->sc_panel, self, &panel_fdt_funcs, DRM_MODE_CONNECTOR_DPI);
	sc->sc_panel.funcs = &panel_fdt_funcs;

	drm_panel_add(&sc->sc_panel);
}

CFATTACH_DECL_NEW(panel_fdt, sizeof(struct panel_fdt_softc),
	panel_fdt_match, panel_fdt_attach, NULL, NULL);
