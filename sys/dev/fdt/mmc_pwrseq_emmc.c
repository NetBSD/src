/* $NetBSD: mmc_pwrseq_emmc.c,v 1.1 2019/03/03 12:54:07 jmcneill Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: mmc_pwrseq_emmc.c,v 1.1 2019/03/03 12:54:07 jmcneill Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/gpio.h>

#include <dev/fdt/fdtvar.h>

#define	MMCPWRSEQ_MAX_PINS	32

static const char * const compatible[] = {
	"mmc-pwrseq-emmc",
	NULL
};

struct mmcpwrseq_emmc_softc {
	device_t sc_dev;
	int sc_phandle;
	struct fdtbus_gpio_pin *sc_pin;
};

static void
mmcpwrseq_emmc_reset(device_t dev)
{
	struct mmcpwrseq_emmc_softc * const sc = device_private(dev);

	fdtbus_gpio_write(sc->sc_pin, 1);
	delay(1);
	fdtbus_gpio_write(sc->sc_pin, 0);
	delay(200);
}

static const struct fdtbus_mmc_pwrseq_func mmcpwrseq_emmc_funcs = {
	.reset = mmcpwrseq_emmc_reset,
};

static int
mmcpwrseq_emmc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
mmcpwrseq_emmc_attach(device_t parent, device_t self, void *aux)
{
	struct mmcpwrseq_emmc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;

	sc->sc_pin = fdtbus_gpio_acquire_index(phandle, "reset-gpios", 0, GPIO_PIN_OUTPUT);
	if (sc->sc_pin == NULL) {
		aprint_error(": couldn't get reset GPIO\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": eMMC hardware reset provider\n");

	fdtbus_register_mmc_pwrseq(self, phandle, &mmcpwrseq_emmc_funcs);
}

static int
mmcpwrseq_emmc_detach(device_t self, int flags)
{
	struct mmcpwrseq_emmc_softc * const sc = device_private(self);

	if (sc->sc_pin != NULL)
		mmcpwrseq_emmc_reset(self);

	return 0;
}

CFATTACH_DECL_NEW(mmcpwrseq_emmc, sizeof(struct mmcpwrseq_emmc_softc),
	mmcpwrseq_emmc_match, mmcpwrseq_emmc_attach, mmcpwrseq_emmc_detach, NULL);
