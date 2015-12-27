/* $NetBSD: fixedregulator.c,v 1.3.2.2 2015/12/27 12:09:49 skrll Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: fixedregulator.c,v 1.3.2.2 2015/12/27 12:09:49 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>

static int	fixedregulator_match(device_t, cfdata_t, void *);
static void	fixedregulator_attach(device_t, device_t, void *);

static int	fixedregulator_acquire(device_t);
static void 	fixedregulator_release(device_t);
static int	fixedregulator_enable(device_t, bool);

struct fdtbus_regulator_controller_func fixedregulator_funcs = {
	.acquire = fixedregulator_acquire,
	.release = fixedregulator_release,
	.enable = fixedregulator_enable
};

struct fixedregulator_softc {
	device_t	sc_dev;
	int		sc_phandle;

	struct fdtbus_gpio_pin *sc_pin;
	bool		sc_always_on;
	bool		sc_enable_val;
};

CFATTACH_DECL_NEW(fregulator, sizeof(struct fixedregulator_softc),
    fixedregulator_match, fixedregulator_attach, NULL, NULL);

static int
fixedregulator_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "regulator-fixed", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
fixedregulator_attach(device_t parent, device_t self, void *aux)
{
	struct fixedregulator_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	char *name;
	int len, gpioflags;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	aprint_naive("\n");

	len = OF_getproplen(phandle, "regulator-name");
	if (len > 0) {
		name = kmem_zalloc(len, KM_SLEEP);
		if (OF_getprop(phandle, "regulator-name", name, len) == len) {
			aprint_normal(": %s\n", name);
		} else {
			aprint_normal("\n");
		}
		kmem_free(name, len);
	} else {
		aprint_normal("\n");
	}

	gpioflags = GPIO_PIN_OUTPUT;
	if (of_getprop_bool(phandle, "gpio-open-drain"))
		gpioflags |= GPIO_PIN_OPENDRAIN;

	sc->sc_always_on = of_getprop_bool(phandle, "regulator-always-on");
	sc->sc_pin = fdtbus_gpio_acquire(phandle, "gpio", gpioflags);
	if (sc->sc_pin == NULL)
		sc->sc_always_on = true;
	sc->sc_enable_val = of_getprop_bool(phandle, "enable-active-high");

	fdtbus_register_regulator_controller(self, phandle,
	    &fixedregulator_funcs);
}

static int
fixedregulator_acquire(device_t dev)
{
	return 0;
}

static void 
fixedregulator_release(device_t dev)
{
}

static int
fixedregulator_enable(device_t dev, bool enable)
{
	struct fixedregulator_softc * const sc = device_private(dev);

	if (enable) {
		if (sc->sc_always_on) {
			return 0;
		} else {
			fdtbus_gpio_write_raw(sc->sc_pin, sc->sc_enable_val);
			return 0;
		}
	} else {
		if (sc->sc_always_on) {
			return EIO;
		} else {
			fdtbus_gpio_write_raw(sc->sc_pin, !sc->sc_enable_val);
			return 0;
		}
	}
}
