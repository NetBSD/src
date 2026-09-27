/* $NetBSD: am18xx_pupd.c,v 1.1 2026/09/27 19:55:02 yurix Exp $ */

/*-
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Yuri Honegger.
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
 * Pull-up/pull-down pin configuration on the TI AM18XX SoC.
 */

#include <sys/param.h>
#include <sys/cdefs.h>
#include <sys/device.h>
#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>

struct am18xx_pupd_softc {
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
	kmutex_t sc_lock;
};

static int 	am18xx_pupd_match(device_t, cfdata_t, void *);
static void	am18xx_pupd_attach(device_t, device_t, void *);
static int	am18xx_pupd_set_config(device_t, const void *, size_t);
static void	am18xx_pupd_configure_group(struct am18xx_pupd_softc *, int);

CFATTACH_DECL_NEW(am18xxpupd, sizeof(struct am18xx_pupd_softc),
		  am18xx_pupd_match, am18xx_pupd_attach, NULL, NULL);

#define AM18XX_PUPD_ENA	0
#define AM18XX_PUPD_SEL	4

#define	PUPD_READ(sc, reg)					\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, reg)
#define	PUPD_WRITE(sc, reg, val)				\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, reg, val)

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "ti,da850-pupd" },
	DEVICE_COMPAT_EOL
};

static struct fdtbus_pinctrl_controller_func am18xx_pupd_pinctrl_funcs = {
	.set_config = am18xx_pupd_set_config,
};

static int
am18xx_pupd_set_config(device_t dev, const void *data, size_t len)
{
	struct am18xx_pupd_softc * const sc = device_private(dev);

	if (len != 4) {
		return -1;
	}

	const int phandle = fdtbus_get_phandle_from_native(be32dec(data));

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		am18xx_pupd_configure_group(sc, child);
	}

	return 0;
}

static void
am18xx_pupd_configure_group(struct am18xx_pupd_softc *sc, int phandle)
{
	int groups_len;
	const char *groups = fdtbus_pinctrl_parse_groups(phandle, &groups_len);

	if (groups == NULL) {
		return;
	}

	uint32_t active_groups = 0;
	while (groups_len > 0) {
		for (int j = 0; j < 32; j++) {
			char buf[16];
			snprintf(buf, sizeof(buf), "cp%d", j);
			if (strcmp(groups, buf) == 0) {
				active_groups |= __BIT(j);
			}
		}
		int len = strlen(groups) + 1;
		groups += len;
		groups_len -= len;
	}

	int pull_strength;
	int bias = fdtbus_pinctrl_parse_bias(phandle, &pull_strength);

	mutex_enter(&sc->sc_lock);

	if (bias == 0) {
		/* no bias */
		uint32_t ena = PUPD_READ(sc, AM18XX_PUPD_ENA);
		ena &= ~active_groups;
		PUPD_WRITE(sc, AM18XX_PUPD_ENA, ena);
	} else if (bias == GPIO_PIN_PULLDOWN) {
		uint32_t sel = PUPD_READ(sc, AM18XX_PUPD_SEL);
		uint32_t ena = PUPD_READ(sc, AM18XX_PUPD_ENA);
		sel &= ~active_groups;
		ena |= active_groups;
		PUPD_WRITE(sc, AM18XX_PUPD_SEL, sel);
		PUPD_WRITE(sc, AM18XX_PUPD_ENA, ena);
	} else if (bias == GPIO_PIN_PULLUP) {
		uint32_t sel = PUPD_READ(sc, AM18XX_PUPD_SEL);
		uint32_t ena = PUPD_READ(sc, AM18XX_PUPD_ENA);
		sel |= active_groups;
		ena |= active_groups;
		PUPD_WRITE(sc, AM18XX_PUPD_SEL, sel);
		PUPD_WRITE(sc, AM18XX_PUPD_ENA, ena);
	}

	mutex_exit(&sc->sc_lock);
}

int
am18xx_pupd_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

void
am18xx_pupd_attach(device_t parent, device_t self, void *aux)
{
	struct am18xx_pupd_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	sc->sc_bst = faa->faa_bst;

	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_NONE);

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh)) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	for (int child = OF_child(phandle); child; child = OF_peer(child)) {
		fdtbus_register_pinctrl_config(self, child,
		    &am18xx_pupd_pinctrl_funcs);
	}

	aprint_naive("\n");
	aprint_normal("\n");
}

