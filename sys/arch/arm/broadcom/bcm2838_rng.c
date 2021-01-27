/*	$NetBSD: bcm2838_rng.c,v 1.2 2021/01/27 03:10:19 thorpej Exp $ */

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jared D. McNeill
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
__KERNEL_RCSID(0, "$NetBSD: bcm2838_rng.c,v 1.2 2021/01/27 03:10:19 thorpej Exp $");

#include <sys/param.h>
#include <sys/device.h>

#include <dev/ic/rng200var.h>
#include <dev/fdt/fdtvar.h>

struct bcm2838rng_softc {
	device_t		sc_dev;
	struct rng200_softc	sc_rng200;
};

static int bcm2838rng_match(device_t, cfdata_t, void *);
static void bcm2838rng_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(bcm2838rng_fdt, sizeof(struct bcm2838rng_softc),
    bcm2838rng_match, bcm2838rng_attach, NULL, NULL);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "brcm,bcm2838-rng200" },
	DEVICE_COMPAT_EOL
};

/* ARGSUSED */
static int
bcm2838rng_match(device_t parent, cfdata_t match, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
bcm2838rng_attach(device_t parent, device_t self, void *aux)
{
	struct bcm2838rng_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	bus_space_handle_t bsh;
	int error;

	sc->sc_dev = self;

	error = fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size);
	if (error) {
		aprint_error_dev(sc->sc_dev, ": couldn't get registers\n");
		return;
	}

	if (bus_space_map(faa->faa_bst, addr, size, 0, &bsh)) {
		aprint_error_dev(sc->sc_dev, ": unable to map device\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Hardware RNG\n");

	sc->sc_rng200.sc_bst = faa->faa_bst;
	sc->sc_rng200.sc_bsh = bsh;
	sc->sc_rng200.sc_name = device_xname(sc->sc_dev);
	rng200_attach(&sc->sc_rng200);
}
