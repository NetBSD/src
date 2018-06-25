/* $NetBSD: pl061gpio_fdt.c,v 1.1.2.2 2018/06/25 07:25:49 pgoyette Exp $ */

/*
 * Copyright (c) 2018 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pl061gpio_fdt.c,v 1.1.2.2 2018/06/25 07:25:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>
#include "gpio.h"

#include <dev/ic/pl061reg.h>

#include <dev/fdt/fdtvar.h>

static int	pl061_gpio_match(device_t, cfdata_t, void *);
static void	pl061_gpio_attach(device_t, device_t, void *);

static void *	pl061_gpio_fdt_acquire(device_t, const void *,
		    size_t, int);
static void	pl061_gpio_fdt_release(device_t, void *);
static int	pl061_gpio_fdt_read(device_t, void *, bool);
static void	pl061_gpio_fdt_write(device_t, void *, int, bool);

struct fdtbus_gpio_controller_func pl061_gpio_funcs = {
	.acquire = pl061_gpio_fdt_acquire,
	.release = pl061_gpio_fdt_release,
	.read = pl061_gpio_fdt_read,
	.write = pl061_gpio_fdt_write
};

struct pl061_gpio_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct gpio_chipset_tag	sc_gc;
	gpio_pin_t		sc_pins[8];
};

struct pl061_gpio_pin {
	struct pl061_gpio_softc *pin_sc;
	int			pin_no;
	u_int			pin_flags;
	bool			pin_actlo;
};

static int	pl061_gpio_pin_read(void *, int);
static void	pl061_gpio_pin_write(void *, int, int);
static void	pl061_gpio_pin_ctl(void *, int, int);

CFATTACH_DECL_NEW(pl061gpio_fdt, sizeof(struct pl061_gpio_softc),
	pl061_gpio_match, pl061_gpio_attach, NULL, NULL);

#define PL061_WRITE(sc, reg, val) \
	bus_space_write_1((sc)->sc_bst, (sc)->sc_bsh, (reg)/4, (val))
#define PL061_READ(sc, reg) \
	bus_space_read_1((sc)->sc_bst, (sc)->sc_bsh, (reg)/4)

static int
pl061_gpio_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"arm,pl061",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
pl061_gpio_attach(device_t parent, device_t self, void *aux)
{
	struct pl061_gpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;
	struct gpiobus_attach_args gba;
	u_int pin;


	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_a4x_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GPIO\n");

	sc->sc_gc.gp_cookie = sc;
	sc->sc_gc.gp_pin_read = pl061_gpio_pin_read;
	sc->sc_gc.gp_pin_write = pl061_gpio_pin_write;
	sc->sc_gc.gp_pin_ctl = pl061_gpio_pin_ctl;

	const uint32_t cnf = PL061_READ(sc, PL061_GPIOAFSEL_REG);

	for (pin = 0; pin < 8; pin++) {
		sc->sc_pins[pin].pin_num = pin;
		/* skip pins in hardware control mode */
		if ((cnf & __BIT(pin)) != 0)
			continue;
		sc->sc_pins[pin].pin_caps =
		    GPIO_PIN_INPUT | GPIO_PIN_OUTPUT |
		    GPIO_PIN_TRISTATE;
		sc->sc_pins[pin].pin_state =
		    pl061_gpio_pin_read(sc, pin);
	}

	memset(&gba, 0, sizeof(gba));
	gba.gba_gc = &sc->sc_gc;
	gba.gba_pins = sc->sc_pins;
	gba.gba_npins = 8;

#if NGPIO > 0
	(void)config_found_ia(sc->sc_dev, "gpiobus", &gba,
	    gpiobus_print);
#endif

	fdtbus_register_gpio_controller(self, faa->faa_phandle,
	    &pl061_gpio_funcs);

}

static int
pl061_gpio_pin_read(void *priv, int pin)
{
	struct pl061_gpio_softc * const sc = priv;

	const uint32_t v = PL061_READ(sc, PL061_GPIODATA_REG(1<<pin));

	return (v >> pin) & 1;
}

static void
pl061_gpio_pin_write(void *priv, int pin, int val)
{
	struct pl061_gpio_softc * const sc = priv;

	PL061_WRITE(sc, PL061_GPIODATA_REG(1 << pin), val << pin);
}

static void
pl061_gpio_pin_ctl(void *priv, int pin, int flags)
{
	struct pl061_gpio_softc * const sc = priv;
	uint32_t v;

	if (flags & GPIO_PIN_INPUT) {
		v = PL061_READ(sc, PL061_GPIODIR_REG);
		v &= ~(1 << pin);
		PL061_WRITE(sc, PL061_GPIODIR_REG, v);
	} else if (flags & GPIO_PIN_OUTPUT) {
		v = PL061_READ(sc, PL061_GPIODIR_REG);
		v |= (1 << pin);
		PL061_WRITE(sc, PL061_GPIODIR_REG, v);
	}
}

static void *
pl061_gpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct pl061_gpio_softc * const sc = device_private(dev);
	struct pl061_gpio_pin *gpin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const u_int pin = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin > 8)
		return NULL;

	const uint32_t cnf = PL061_READ(sc, PL061_GPIOAFSEL_REG);
	if ((cnf & __BIT(pin)) != 0)
		PL061_WRITE(sc, PL061_GPIOAFSEL_REG, cnf & ~__BIT(pin));

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	pl061_gpio_pin_ctl(gpin->pin_sc, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
pl061_gpio_fdt_release(device_t dev, void *priv)
{
	struct pl061_gpio_pin * const gpin = priv;

	pl061_gpio_pin_ctl(gpin->pin_sc, gpin->pin_no, GPIO_PIN_INPUT);
	kmem_free(gpin, sizeof(*gpin));
}

static int
pl061_gpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct pl061_gpio_pin * const gpin = priv;
	int val;

	val = pl061_gpio_pin_read(gpin->pin_sc, gpin->pin_no);

	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
pl061_gpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct pl061_gpio_pin * const gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	pl061_gpio_pin_write(gpin->pin_sc, gpin->pin_no, val);
}
