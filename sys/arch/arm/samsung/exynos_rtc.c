/*	$NetBSD: exynos_rtc.c,v 1.2.18.2 2017/12/03 11:35:56 jdolecek Exp $ */

/*-
* Copyright (c) 2015 The NetBSD Foundation, Inc.
* All rights reserved.
*
* This code is derived from software contributed to The NetBSD Foundation
* by Marty Fouts
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

#include "opt_exynos.h"
#include "opt_arm_debug.h"
#include "gpio.h"

#include <sys/cdefs.h>
__KERNEL_RCSID(1, "$NetBSD: exynos_rtc.c,v 1.2.18.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/kmem.h>

#include <dev/clock_subr.h>

#include <arm/samsung/exynos_reg.h>
#include <arm/samsung/exynos_intr.h>

#include <dev/fdt/fdtvar.h>

struct exynos_rtc_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct todr_chip_handle sc_todr;
};

static int exynos_rtc_match(device_t, cfdata_t, void *);
static void exynos_rtc_attach(device_t, device_t, void *);

static int	exynos_rtc_gettime(todr_chip_handle_t, struct timeval *);
static int	exynos_rtc_settime(todr_chip_handle_t, struct timeval *);

CFATTACH_DECL_NEW(exynos_rtc, sizeof(struct exynos_rtc_softc),
	exynos_rtc_match, exynos_rtc_attach, NULL, NULL);

#define RTC_READ(sc, reg)	\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define RTC_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static int
exynos_rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "samsung,s3c6410-rtc",
					    NULL };
	struct fdt_attach_args * const faa = aux;
	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
exynos_rtc_attach(device_t parent, device_t self, void *aux)
{
	struct exynos_rtc_softc * const sc
		= kmem_zalloc(sizeof(*sc), KM_SLEEP);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	aprint_normal(" @ 0x%08x", (uint)addr);
	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d",
			     (uint64_t)addr, error);
		return;
	}

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	sc->sc_todr.todr_gettime = exynos_rtc_gettime;
	sc->sc_todr.todr_settime = exynos_rtc_settime;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);

}

static int
exynos_rtc_gettime(todr_chip_handle_t tch, struct timeval *tv)
{
	struct exynos_rtc_softc * const sc = tch->cookie;

	tv->tv_sec = RTC_READ(sc, EXYNOS5_RTC_OFFSET);
	tv->tv_usec = 0;

	return 0;
}

static int
exynos_rtc_settime(todr_chip_handle_t tch, struct timeval *tv)
{
	struct exynos_rtc_softc * const sc = tch->cookie;
	int retry = 500;

	while (--retry > 0) {
		if (!RTC_READ(sc, EXYNOS5_RTC_OFFSET))
			break;
		delay(1);
	}
	if (retry == 0) {
		device_printf(sc->sc_dev, "RTC write failed (BUSY)\n");
		return ETIMEDOUT;
	}

	RTC_WRITE(sc, EXYNOS5_RTC_OFFSET, tv->tv_sec);

	return 0;
}
