/*	$NetBSD: sni_gpio.c,v 1.1 2020/03/18 08:51:08 nisimura Exp $	*/

/*-
 * Copyright (c) 2020 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Tohru Nishimura.
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
 * Socionext SC2A11 SynQuacer GPIO driver
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sni_gpio.c,v 1.1 2020/03/18 08:51:08 nisimura Exp $");

#include <sys/param.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/gpio.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <machine/endian.h>
#include <sys/bus.h>
#include <sys/intr.h>

#include <dev/gpio/gpiovar.h>
#include <dev/fdt/fdtvar.h>

static int snigpio_match(device_t, struct cfdata *, void *);
static void snigpio_attach(device_t, device_t, void *);

struct snigpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_iot;
	bus_space_handle_t	sc_ioh;
	bus_addr_t		sc_iob;
	bus_size_t		sc_ios;
	kmutex_t		sc_lock;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[32];
	int			sc_maxpins;
	int			sc_phandle;
};

CFATTACH_DECL_NEW(snigpio, sizeof(struct snigpio_softc),
    snigpio_match, snigpio_attach, NULL, NULL);

static int
snigpio_match(device_t parent, struct cfdata *match, void *aux)
{
	static const char * compatible[] = {
		"socionext,synquacer-gpio",
		"fujitsu,mb86s70-gpio",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
snigpio_attach(device_t parent, device_t self, void *aux)
{
	struct snigpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	prop_dictionary_t dict = device_properties(self);
	const int phandle = faa->faa_phandle;
	struct gpiobus_attach_args gba;
	bus_space_handle_t ioh;
	bus_addr_t addr;
	bus_size_t size;
	_Bool disable;
	int error;

	prop_dictionary_get_bool(dict, "disable", &disable);
	if (disable) {
		aprint_naive(": disabled\n");
		aprint_normal(": disabled\n");
		return;
	}
	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	error = bus_space_map(faa->faa_bst, addr, size, 0, &ioh);
	if (error) {
		aprint_error(": unable to map device\n");
		return;
	}

	aprint_naive(": GPIO controller\n");
	aprint_normal(": GPIO controller\n");

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_iot = faa->faa_bst;
	sc->sc_ioh = ioh;
	sc->sc_iob = addr;
	sc->sc_ios = size;
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	sc->sc_maxpins = 32;

	/* create controller tag */
	sc->sc_gpio_gc.gp_cookie = sc;
	sc->sc_gpio_gc.gp_pin_read = NULL; /* AAA */
	sc->sc_gpio_gc.gp_pin_write = NULL; /* AAA */
	sc->sc_gpio_gc.gp_pin_ctl = NULL; /* AAA */
	sc->sc_gpio_gc.gp_intr_establish = NULL; /* AAA */
	sc->sc_gpio_gc.gp_intr_disestablish = NULL; /* AAA */
	sc->sc_gpio_gc.gp_intr_str = NULL; /* AAA */

	gba.gba_gc = &sc->sc_gpio_gc;
	gba.gba_pins = &sc->sc_gpio_pins[0];
	gba.gba_npins = sc->sc_maxpins;
	(void) config_found_ia(self, "gpiobus", &gba, gpiobus_print);

	return;
}
