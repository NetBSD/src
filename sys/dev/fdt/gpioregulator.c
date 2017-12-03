/* $NetBSD: gpioregulator.c,v 1.1.4.2 2017/12/03 11:37:01 jdolecek Exp $ */

/*-
 * Copyright (c) 2017 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: gpioregulator.c,v 1.1.4.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>

static int	gpioregulator_match(device_t, cfdata_t, void *);
static void	gpioregulator_attach(device_t, device_t, void *);

static int	gpioregulator_acquire(device_t);
static void 	gpioregulator_release(device_t);
static int	gpioregulator_enable(device_t, bool);
static int	gpioregulator_set_voltage(device_t, u_int, u_int);
static int	gpioregulator_get_voltage(device_t, u_int *);

static const struct fdtbus_regulator_controller_func gpioregulator_funcs = {
	.acquire = gpioregulator_acquire,
	.release = gpioregulator_release,
	.enable = gpioregulator_enable,
	.set_voltage = gpioregulator_set_voltage,
	.get_voltage = gpioregulator_get_voltage,
};

struct gpioregulator_state {
	u_int		st_val;
	u_int		st_mask;
};

struct gpioregulator_softc {
	device_t	sc_dev;
	int		sc_phandle;

	struct fdtbus_gpio_pin *sc_pin_enable;

	struct fdtbus_gpio_pin **sc_pins;
	u_int		sc_npins;

	struct gpioregulator_state *sc_states;
	u_int		sc_nstates;

	bool		sc_always_on;
	bool		sc_boot_on;
	bool		sc_enable_val;
	uint32_t	sc_delay;

	int		sc_gpioflags;
};

CFATTACH_DECL_NEW(gregulator, sizeof(struct gpioregulator_softc),
    gpioregulator_match, gpioregulator_attach, NULL, NULL);

static int
gpioregulator_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "regulator-gpio", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
gpioregulator_attach(device_t parent, device_t self, void *aux)
{
	struct gpioregulator_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	const uint32_t *pstates;
	uint32_t mask;
	char *name;
	int len, n;

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

	pstates = fdtbus_get_prop(phandle, "states", &len);
	if (pstates == NULL || len < 8 || len % 8 != 0) {
		aprint_error_dev(self, "invalid 'states' property\n");
		return;
	}

	mask = 0;
	sc->sc_nstates = len / (sizeof(uint32_t) * 2);
	sc->sc_states = kmem_zalloc(
	    sc->sc_nstates * sizeof(struct gpioregulator_state), KM_SLEEP);
	for (n = 0; n < sc->sc_nstates; n++) {
		sc->sc_states[n].st_val = be32toh(pstates[n * 2 + 0]);
		sc->sc_states[n].st_mask = be32toh(pstates[n * 2 + 1]);
		mask |= sc->sc_states[n].st_mask;
	}

	sc->sc_gpioflags = GPIO_PIN_OUTPUT;
	if (of_getprop_bool(phandle, "gpio-open-drain"))
		sc->sc_gpioflags |= GPIO_PIN_OPENDRAIN;

	sc->sc_always_on = of_getprop_bool(phandle, "regulator-always-on");
	sc->sc_boot_on = of_getprop_bool(phandle, "regulator-boot-on");
	sc->sc_enable_val = of_getprop_bool(phandle, "enable-active-high");
	if (of_getprop_uint32(phandle, "startup-delay-us", &sc->sc_delay) != 0)
		sc->sc_delay = 0;

	/* "enable-gpio" property (optional) */
	sc->sc_pin_enable = fdtbus_gpio_acquire(phandle, "enable-gpio",
	    sc->sc_gpioflags);

	/* "gpios" property */
	sc->sc_npins = 32 - __builtin_clz(mask);
	sc->sc_pins = kmem_zalloc(sc->sc_npins * sizeof(sc->sc_pins), KM_SLEEP);
	for (n = 0; n < sc->sc_npins; n++) {
		sc->sc_pins[n] = fdtbus_gpio_acquire_index(phandle, "gpios",
		    n, sc->sc_gpioflags);
		if (sc->sc_pins[n] == NULL) {
			aprint_error_dev(self, "cannot get pin %d\n", n);
			return;
		}
	}

	fdtbus_register_regulator_controller(self, phandle,
	    &gpioregulator_funcs);

	/*
	 * If the regulator is flagged as always on or enabled at boot,
	 * ensure that it is enabled
	 */
	if (sc->sc_always_on || sc->sc_boot_on)
		gpioregulator_enable(self, true);
}

static int
gpioregulator_acquire(device_t dev)
{
	return 0;
}

static void 
gpioregulator_release(device_t dev)
{
}

static int
gpioregulator_enable(device_t dev, bool enable)
{
	struct gpioregulator_softc * const sc = device_private(dev);

	if (enable) {
		if (sc->sc_pin_enable != NULL)
			fdtbus_gpio_write_raw(sc->sc_pin_enable, sc->sc_enable_val);
		if (sc->sc_delay > 0)
			delay(sc->sc_delay);
	} else {
		if (sc->sc_always_on)
			return EIO;
		fdtbus_gpio_write_raw(sc->sc_pin_enable, !sc->sc_enable_val);
	}
	return 0;
}

static int
gpioregulator_set_voltage(device_t dev, u_int min_uvolt, u_int max_uvolt)
{
	struct gpioregulator_softc * const sc = device_private(dev);
	const struct gpioregulator_state *state = NULL;
	int n;

	for (n = 0; n < sc->sc_nstates; n++)
		if (sc->sc_states[n].st_val >= min_uvolt &&
		    sc->sc_states[n].st_val <= max_uvolt) {
			state = &sc->sc_states[n];
			break;
		}
	if (state == NULL)
		return EINVAL;

	for (n = 0; n < sc->sc_npins; n++)
		fdtbus_gpio_write(sc->sc_pins[n], (state->st_mask >> n) & 1);

	if (sc->sc_delay > 0)
		delay(sc->sc_delay);

	return 0;
}

static int
gpioregulator_get_voltage(device_t dev, u_int *puvolt)
{
	struct gpioregulator_softc * const sc = device_private(dev);
	uint32_t mask = 0;
	int n, val;

	for (n = 0; n < sc->sc_npins; n++) {
		val = fdtbus_gpio_read(sc->sc_pins[n]);
		mask |= (val << n);
	}

	for (n = 0; n < sc->sc_nstates; n++)
		if (sc->sc_states[n].st_mask == mask) {
			*puvolt = sc->sc_states[n].st_val;
			return 0;
		}

	return EIO;
}
