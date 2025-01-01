/* $NetBSD: jh7110_pciephy.c,v 1.3 2025/01/01 17:35:44 skrll Exp $ */

/*-
 * Copyright (c) 2024 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: jh7110_pciephy.c,v 1.3 2025/01/01 17:35:44 skrll Exp $");

#include <sys/param.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>


struct jh7110_pciephy_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;
};

/* Register definitions */
#define PCIE_KVCO_LEVEL			0x28
#define  PCEI_PHY_KVCO_FINE_TUNE_LEVEL	0x91

#define PCIE_USB3_PHY_PLL_CTL		0x7c

#define PCIE_KVCO_TUNE_SIGNAL		0x80
#define	 PCIE_KVO_FINE_TUNE_SIGNALS	0x0c

#define RD4(sc, reg)							      \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define WR4(sc, reg, val)						      \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void *
jh7110pciephy_acquire(device_t dev, const void *data, size_t len)
{
	struct jh7110_pciephy_softc * const sc = device_private(dev);

	if (len != 0) {
		aprint_verbose("phy acquire with len %zu", len);
		return NULL;
	}

	return sc;
}

static void
jh7110pciephy_release(device_t dev, void *data)
{
}

static int
jh7110pciephy_enable(device_t dev, void *priv, bool enable)
{

	return 0;
}

const struct fdtbus_phy_controller_func jh7110pciephy_funcs = {
	.acquire = jh7110pciephy_acquire,
	.release = jh7110pciephy_release,
	.enable = jh7110pciephy_enable,
};

/* Compat string(s) */
static const struct device_compatible_entry compat_data[] = {
	{ .compat = "starfive,jh7110-pcie-phy" },
	DEVICE_COMPAT_EOL
};

static int
jh7110_pciephy_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
jh7110_pciephy_attach(device_t parent, device_t self, void *aux)
{
	struct jh7110_pciephy_softc *sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const bus_space_tag_t bst = faa->faa_bst;
	bus_addr_t addr;
	bus_size_t size;
	int error;

	error = fdtbus_get_reg(phandle, 0, &addr, &size);
	if (error) {
		aprint_error(": couldn't get registers\n");
		return;
	}
	error = bus_space_map(bst, addr, size, 0, &sc->sc_bsh);
	if (error) {
		aprint_error(": couldn't map %#" PRIxBUSADDR ": %d", addr,
		    error);
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = bst;

	aprint_naive("\n");
	aprint_normal(": JH7110 PCIe PHY\n");

	WR4(sc, PCIE_KVCO_LEVEL, PCEI_PHY_KVCO_FINE_TUNE_LEVEL);
	WR4(sc, PCIE_KVCO_TUNE_SIGNAL, PCIE_KVO_FINE_TUNE_SIGNALS);

	fdtbus_register_phy_controller(self, faa->faa_phandle,
	    &jh7110pciephy_funcs);
}

CFATTACH_DECL_NEW(jh7110_pciephy, sizeof(struct jh7110_pciephy_softc),
	jh7110_pciephy_match, jh7110_pciephy_attach, NULL, NULL);
