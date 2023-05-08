/*	$NetBSD: gfrtc_fdt.c,v 1.1 2023/05/08 07:51:44 skrll Exp $	*/

/*-
 * Copyright (c) 2023 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Nick Hudson
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: gfrtc_fdt.c,v 1.1 2023/05/08 07:51:44 skrll Exp $");

#include <sys/param.h>

#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

/*
 * https://android.googlesource.com/platform/external/qemu/+/master/docs/GOLDFISH-VIRTUAL-HARDWARE.TXT
 */

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "google,goldfish-rtc" },
	DEVICE_COMPAT_EOL
};

struct gfrtc_fdt_softc {
	device_t sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;

	struct todr_chip_handle	sc_todr;
};

#define	GOLDFISH_RTC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	GOLDFISH_RTC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))


#define	GOLDFISH_RTC_TIME_LOW	0x00
#define	GOLDFISH_RTC_TIME_HIGH	0x04


static int
gfrtc_gettime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct gfrtc_fdt_softc *sc = ch->cookie;
	const uint64_t lo = GOLDFISH_RTC_READ(sc, GOLDFISH_RTC_TIME_LOW);
	const uint64_t hi = GOLDFISH_RTC_READ(sc, GOLDFISH_RTC_TIME_HIGH);

	uint64_t nsec = (hi << 32) | lo;

	tv->tv_sec = nsec / 1000000000;
	tv->tv_usec = (nsec - tv->tv_sec) / 1000;	// Always 0?

	return 0;
}

static int
gfrtc_settime(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct gfrtc_fdt_softc *sc = ch->cookie;

	uint64_t nsec = (tv->tv_sec * 1000000 + tv->tv_usec) * 1000;
	uint32_t hi = nsec >> 32;
	uint32_t lo = nsec;

	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_TIME_HIGH, hi);
	GOLDFISH_RTC_WRITE(sc, GOLDFISH_RTC_TIME_HIGH, lo);

	return 0;
}


static int
gfrtc_fdt_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
gfrtc_fdt_attach(device_t parent, device_t self, void *aux)
{
	struct gfrtc_fdt_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Google Goldfish RTC\n");

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = gfrtc_gettime;
	sc->sc_todr.todr_settime = gfrtc_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

CFATTACH_DECL_NEW(gfrtc_fdt, sizeof(struct gfrtc_fdt_softc),
	gfrtc_fdt_match, gfrtc_fdt_attach, NULL, NULL);
