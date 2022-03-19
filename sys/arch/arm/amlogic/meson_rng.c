/* $NetBSD: meson_rng.c,v 1.5 2022/03/19 11:36:43 riastradh Exp $ */

/*-
 * Copyright (c) 2015-2019 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: meson_rng.c,v 1.5 2022/03/19 11:36:43 riastradh Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/mutex.h>
#include <sys/rndsource.h>

#include <dev/fdt/fdtvar.h>

static int	meson_rng_match(device_t, cfdata_t, void *);
static void	meson_rng_attach(device_t, device_t, void *);

static void	meson_rng_get(size_t, void *);

struct meson_rng_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	krndsource_t		sc_rndsource;
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-rng" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(meson_rng, sizeof(struct meson_rng_softc),
	meson_rng_match, meson_rng_attach, NULL, NULL);

static int
meson_rng_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
meson_rng_attach(device_t parent, device_t self, void *aux)
{
	struct meson_rng_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct clk *clk;
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

	/* Core clock is optional */
	clk = fdtbus_clock_get(phandle, "core");
	if (clk != NULL && clk_enable(clk) != 0) {
		aprint_error(": couldn't enable core clock\n");
		return;
	}

	aprint_naive("\n");
	aprint_normal(": Hardware RNG\n");

	rndsource_setcb(&sc->sc_rndsource, meson_rng_get, sc);
	rnd_attach_source(&sc->sc_rndsource, device_xname(self), RND_TYPE_RNG,
	    RND_FLAG_COLLECT_VALUE|RND_FLAG_HASCB);
}

static void
meson_rng_get(size_t bytes_wanted, void *priv)
{
	struct meson_rng_softc * const sc = priv;
	uint32_t data;

	while (bytes_wanted) {
		data = bus_space_read_4(sc->sc_bst, sc->sc_bsh, 0);
		rnd_add_data_sync(&sc->sc_rndsource, &data, sizeof(data),
		    sizeof(data) * NBBY);
		bytes_wanted -= MIN(bytes_wanted, sizeof(data));
	}
	explicit_memset(&data, 0, sizeof(data));
}
