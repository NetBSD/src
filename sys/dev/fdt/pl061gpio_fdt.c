/* $NetBSD: pl061gpio_fdt.c,v 1.1.2.4 2018/10/20 06:58:31 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: pl061gpio_fdt.c,v 1.1.2.4 2018/10/20 06:58:31 pgoyette Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/gpio.h>

#include <dev/gpio/gpiovar.h>

#include <dev/ic/pl061reg.h>
#include <dev/ic/pl061var.h>

#include <dev/fdt/fdtvar.h>

static int	plgpio_fdt_match(device_t, cfdata_t, void *);
static void	plgpio_fdt_attach(device_t, device_t, void *);

static void *	plgpio_fdt_acquire(device_t, const void *,
		    size_t, int);
static void	plgpio_fdt_release(device_t, void *);
static int	plgpio_fdt_read(device_t, void *, bool);
static void	plgpio_fdt_write(device_t, void *, int, bool);

struct fdtbus_gpio_controller_func plgpio_fdt_funcs = {
	.acquire = plgpio_fdt_acquire,
	.release = plgpio_fdt_release,
	.read = plgpio_fdt_read,
	.write = plgpio_fdt_write
};

struct plgpio_fdt_pin {
	struct plgpio_softc *	pin_sc;
	int			pin_no;
	u_int			pin_flags;
	bool			pin_actlo;
};

CFATTACH_DECL_NEW(plgpio_fdt, sizeof(struct plgpio_softc),
	plgpio_fdt_match, plgpio_fdt_attach, NULL, NULL);

static int
plgpio_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"arm,pl061",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
plgpio_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct plgpio_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#"PRIx64": %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": GPIO\n");

	plgpio_attach(sc);

	fdtbus_register_gpio_controller(self, faa->faa_phandle,
	    &plgpio_fdt_funcs);
}

static void *
plgpio_fdt_acquire(device_t dev, const void *data, size_t len, int flags)
{
	struct plgpio_softc * const sc = device_private(dev);
	struct plgpio_fdt_pin *gpin;
	const u_int *gpio = data;

	if (len != 12)
		return NULL;

	const u_int pin = be32toh(gpio[1]);
	const bool actlo = be32toh(gpio[2]) & 1;

	if (pin > 8)
		return NULL;

	const uint32_t cnf = PLGPIO_READ(sc, PL061_GPIOAFSEL_REG);
	if ((cnf & __BIT(pin)) != 0)
		PLGPIO_WRITE(sc, PL061_GPIOAFSEL_REG, cnf & ~__BIT(pin));

	gpin = kmem_zalloc(sizeof(*gpin), KM_SLEEP);
	gpin->pin_sc = sc;
	gpin->pin_no = pin;
	gpin->pin_flags = flags;
	gpin->pin_actlo = actlo;

	plgpio_pin_ctl(gpin->pin_sc, gpin->pin_no, gpin->pin_flags);

	return gpin;
}

static void
plgpio_fdt_release(device_t dev, void *priv)
{
	struct plgpio_fdt_pin * const gpin = priv;

	plgpio_pin_ctl(gpin->pin_sc, gpin->pin_no, GPIO_PIN_INPUT);
	kmem_free(gpin, sizeof(*gpin));
}

static int
plgpio_fdt_read(device_t dev, void *priv, bool raw)
{
	struct plgpio_fdt_pin * const gpin = priv;
	int val;

	val = plgpio_pin_read(gpin->pin_sc, gpin->pin_no);

	if (!raw && gpin->pin_actlo)
		val = !val;

	return val;
}

static void
plgpio_fdt_write(device_t dev, void *priv, int val, bool raw)
{
	struct plgpio_fdt_pin * const gpin = priv;

	if (!raw && gpin->pin_actlo)
		val = !val;

	plgpio_pin_write(gpin->pin_sc, gpin->pin_no, val);
}
