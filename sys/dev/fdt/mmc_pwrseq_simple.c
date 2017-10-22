/* $NetBSD: mmc_pwrseq_simple.c,v 1.1 2017/10/22 13:56:49 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mmc_pwrseq_simple.c,v 1.1 2017/10/22 13:56:49 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>

#define	MMCPWRSEQ_MAX_PINS	32

static const char * const compatible[] = {
	"mmc-pwrseq-simple",
	NULL
};

struct mmcpwrseq_softc {
	device_t sc_dev;
	int sc_phandle;
	struct clk *sc_clk;
	struct fdtbus_gpio_pin *sc_pins[MMCPWRSEQ_MAX_PINS];
	u_int sc_npins;
	u_int sc_post_power_on_delay_ms;
	u_int sc_power_off_delay_us;
};

static void
mmcpwrseq_pre_power_on(device_t dev)
{
	struct mmcpwrseq_softc * const sc = device_private(dev);
	int error;

	if (sc->sc_clk) {
		error = clk_enable(sc->sc_clk);
		if (error != 0) {
			aprint_error_dev(dev, "failed to enable clock: %d\n",
			    error);
		}
	}

	for (u_int n = 0; n < sc->sc_npins; n++)
		fdtbus_gpio_write(sc->sc_pins[n], 1);
}

static void
mmcpwrseq_post_power_on(device_t dev)
{
	struct mmcpwrseq_softc * const sc = device_private(dev);

	for (u_int n = 0; n < sc->sc_npins; n++)
		fdtbus_gpio_write(sc->sc_pins[n], 0);

	if (sc->sc_post_power_on_delay_ms > 0)
		kpause("mmcpwrseq", false,
		    mstohz(sc->sc_post_power_on_delay_ms), NULL);
}

static void
mmcpwrseq_power_off(device_t dev)
{
	struct mmcpwrseq_softc * const sc = device_private(dev);

	for (u_int n = 0; n < sc->sc_npins; n++)
		fdtbus_gpio_write(sc->sc_pins[n], 1);

	if (sc->sc_power_off_delay_us > 0)
		delay(sc->sc_power_off_delay_us);
}

static const struct fdtbus_mmc_pwrseq_func mmcpwrseq_funcs = {
	.pre_power_on = mmcpwrseq_pre_power_on,
	.post_power_on = mmcpwrseq_post_power_on,
	.power_off = mmcpwrseq_power_off,
};

static int
mmcpwrseq_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mmcpwrseq_attach(device_t parent, device_t self, void *aux)
{
	struct mmcpwrseq_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	/* Optional external clock provider */
	if (of_hasprop(phandle, "clocks")) {
		sc->sc_clk = fdtbus_clock_get(phandle, "ext_clock");
		if (sc->sc_clk == NULL) {
			aprint_error(": couldn't acquire ext_clock\n");
			return;
		}
	}

	/* Optional reset GPIOs */
	if (of_hasprop(phandle, "reset-gpios")) {
		for (sc->sc_npins = 0;
		     sc->sc_npins < MMCPWRSEQ_MAX_PINS;
		     sc->sc_npins++) {
			sc->sc_pins[sc->sc_npins] =
			    fdtbus_gpio_acquire_index(phandle,
				"reset-gpios", sc->sc_npins, GPIO_PIN_OUTPUT);
			if (sc->sc_pins[sc->sc_npins] == NULL)
				break;
		}
		if (sc->sc_npins == 0) {
			aprint_error(": couldn't get reset GPIOs\n");
			return;
		}
	}

	/* Delay in ms after powering the card and de-asserting reset GPIOs */
	of_getprop_uint32(phandle, "post-power-on-delay-ms",
	    &sc->sc_post_power_on_delay_ms);
	/* Delay in us after asserting the reset GPIOs during power off */
	of_getprop_uint32(phandle, "power-off-delay-us",
	    &sc->sc_power_off_delay_us);

	aprint_naive("\n");
	aprint_normal("\n");

	fdtbus_register_mmc_pwrseq(self, phandle, &mmcpwrseq_funcs);
}

CFATTACH_DECL_NEW(mmcpwrseq, sizeof(struct mmcpwrseq_softc),
	mmcpwrseq_match, mmcpwrseq_attach, NULL, NULL);
