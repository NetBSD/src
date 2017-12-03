/* $NetBSD: tegra_rtc.c,v 1.4.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_rtc.c,v 1.4.8.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/clock_subr.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_rtcreg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra_rtc_match(device_t, cfdata_t, void *);
static void	tegra_rtc_attach(device_t, device_t, void *);

struct tegra_rtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct todr_chip_handle	sc_todr;
};

static int	tegra_rtc_gettime(todr_chip_handle_t, struct timeval *);
static int	tegra_rtc_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(tegra_rtc, sizeof(struct tegra_rtc_softc),
	tegra_rtc_match, tegra_rtc_attach, NULL, NULL);

#define RTC_READ(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RTC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
tegra_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-rtc",
		"nvidia,tegra124-rtc",
		"nvidia,tegra20-rtc",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_rtc_softc * const sc = device_private(self);
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
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	sc->sc_todr.todr_gettime = tegra_rtc_gettime;
	sc->sc_todr.todr_settime = tegra_rtc_settime;
	sc->sc_todr.cookie = sc;
	fdtbus_todr_attach(self, faa->faa_phandle, &sc->sc_todr);
}

static int
tegra_rtc_gettime(todr_chip_handle_t tch, struct timeval *tv)
{
	struct tegra_rtc_softc * const sc = tch->cookie;

	tv->tv_sec = RTC_READ(sc, RTC_SECONDS_REG);
	tv->tv_usec = 0;

	return 0;
}

static int
tegra_rtc_settime(todr_chip_handle_t tch, struct timeval *tv)
{
	struct tegra_rtc_softc * const sc = tch->cookie;
	int retry = 500;

	while (--retry > 0) {
		if ((RTC_READ(sc, RTC_BUSY_REG) & RTC_BUSY_STATUS) == 0)
			break;
		delay(1);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "RTC write failed (BUSY)\n");
		return ETIMEDOUT;
	}

	RTC_WRITE(sc, RTC_SECONDS_REG, tv->tv_sec);

	return 0;
}
