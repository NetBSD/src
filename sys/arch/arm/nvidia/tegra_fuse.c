/* $NetBSD: tegra_fuse.c,v 1.5.8.2 2017/12/03 11:35:54 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: tegra_fuse.c,v 1.5.8.2 2017/12/03 11:35:54 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kernel.h>

#include <arm/nvidia/tegra_reg.h>
#include <arm/nvidia/tegra_var.h>

#include <dev/fdt/fdtvar.h>

static int	tegra_fuse_match(device_t, cfdata_t, void *);
static void	tegra_fuse_attach(device_t, device_t, void *);

struct tegra_fuse_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;

	struct clk		*sc_clk;
	struct fdtbus_reset	*sc_rst;
};

static struct tegra_fuse_softc *fuse_softc = NULL;

CFATTACH_DECL_NEW(tegra_fuse, sizeof(struct tegra_fuse_softc),
	tegra_fuse_match, tegra_fuse_attach, NULL, NULL);

static int
tegra_fuse_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = {
		"nvidia,tegra210-efuse",
		"nvidia,tegra124-efuse",
		NULL
	};
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
tegra_fuse_attach(device_t parent, device_t self, void *aux)
{
	struct tegra_fuse_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	if (fdtbus_get_reg(faa->faa_phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	sc->sc_clk = fdtbus_clock_get(faa->faa_phandle, "fuse");
	if (sc->sc_clk == NULL) {
		aprint_error(": couldn't get clock fuse\n");
		return;
	}
	sc->sc_rst = fdtbus_reset_get(faa->faa_phandle, "fuse");
	if (sc->sc_rst == NULL) {
		aprint_error(": couldn't get reset fuse\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_bst = faa->faa_bst;
	error = bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#llx: %d", (uint64_t)addr, error);
		return;
	}

	KASSERT(fuse_softc == NULL);
	fuse_softc = sc;

	aprint_naive("\n");
	aprint_normal(": FUSE\n");
}

uint32_t
tegra_fuse_read(u_int offset)
{
	bus_space_tag_t bst;
	bus_space_handle_t bsh;

	KASSERT(fuse_softc != NULL);

	bst = fuse_softc->sc_bst;
	bsh = fuse_softc->sc_bsh;

	clk_enable(fuse_softc->sc_clk);
	const uint32_t v = bus_space_read_4(bst, bsh, 0x100 + offset);
	clk_disable(fuse_softc->sc_clk);

	return v;
}
