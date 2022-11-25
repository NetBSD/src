/* $NetBSD: fu540_prci.c,v 1.1 2022/11/25 12:35:44 jmcneill Exp $ */

/*-
 * Copyright (c) 2022 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(0, "$NetBSD: fu540_prci.c,v 1.1 2022/11/25 12:35:44 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/kmem.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#define	COREPLLCFG0	0x04
#define	DDRPLLCFG0	0x0c
#define	DDRPLLCFG1	0x10
#define	GEMGXLPLLCFG0	0x1c
#define	GEMGXLPLLCFG1	0x20
#define	 PLL0_DIVQ	__BITS(17,15)
#define	 PLL0_DIVF	__BITS(14,6)
#define	 PLL0_DIVR	__BITS(5,0)
#define	 PLL1_CKE	__BIT(24)
#define	CORECLKSEL	0x24

enum fu540_clkid {
	clkid_corepll,
	clkid_ddrpll,
	clkid_gemgxlpll,
	clkid_tlclk,
	num_clkid
};
CTASSERT(num_clkid == 4);

static int fu540_prci_match(device_t, cfdata_t, void *);
static void fu540_prci_attach(device_t, device_t, void *);

static u_int fu540_prci_clk_get_rate(void *, struct clk *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "sifive,fu540-c000-prci" },
	DEVICE_COMPAT_EOL
};

struct fu540_prci_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	struct clk_domain	sc_clkdom;
	struct clk		sc_clk[num_clkid];

	u_int			sc_hfclk;
	u_int			sc_rtcclk;
};

#define	RD4(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	WR4(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(fu540_prci, sizeof(struct fu540_prci_softc),
	fu540_prci_match, fu540_prci_attach, NULL, NULL);

static struct clk *
fu540_prci_clk_get(void *priv, const char *name)
{
	struct fu540_prci_softc * const sc = priv;
	u_int clkid;

	for (clkid = 0; clkid < num_clkid; clkid++) {
		if (strcmp(name, sc->sc_clk[clkid].name) == 0) {
			return &sc->sc_clk[clkid];
		}
	}

	return NULL;
}

static void
fu540_prci_clk_put(void *priv, struct clk *clk)
{
}

static u_int
fu540_prci_clk_get_rate_pll(struct fu540_prci_softc *sc, u_int reg)
{
	uint32_t val;
	u_int rate, divr, divf, divq;

	val = RD4(sc, reg);
	divr = __SHIFTOUT(val, PLL0_DIVR) + 1;
	divf = __SHIFTOUT(val, PLL0_DIVF) + 1;
	divq = __SHIFTOUT(val, PLL0_DIVQ);
	rate = sc->sc_hfclk * divr * divf;
	rate <<= divq;

	return rate;
}

static u_int
fu540_prci_clk_get_rate(void *priv, struct clk *clk)
{
	struct fu540_prci_softc * const sc = priv;
	u_int rate;

	if (clk == &sc->sc_clk[clkid_corepll] ||
	    clk == &sc->sc_clk[clkid_tlclk]) {
		rate = fu540_prci_clk_get_rate_pll(sc, COREPLLCFG0);
		if (clk == &sc->sc_clk[clkid_tlclk]) {
			rate /= 2;
		}
		return rate;
	} else if (clk == &sc->sc_clk[clkid_ddrpll]) {
		return fu540_prci_clk_get_rate_pll(sc, DDRPLLCFG0);
	} else if (clk == &sc->sc_clk[clkid_gemgxlpll]) {
		return fu540_prci_clk_get_rate_pll(sc, GEMGXLPLLCFG0);
	} else {
		/* Not implemented. */
		return 0;
	}
}

static int
fu540_prci_clk_enable(void *priv, struct clk *clk)
{
	struct fu540_prci_softc * const sc = priv;
	uint32_t val;

	if (clk == &sc->sc_clk[clkid_corepll] ||
	    clk == &sc->sc_clk[clkid_tlclk]) {
		/* Always enabled. */
		return 0;
	} else if (clk == &sc->sc_clk[clkid_ddrpll]) {
		val = RD4(sc, DDRPLLCFG1);
		WR4(sc, DDRPLLCFG1, val | PLL1_CKE);
		return 0;
	} else if (clk == &sc->sc_clk[clkid_gemgxlpll]) {
		val = RD4(sc, GEMGXLPLLCFG1);
		WR4(sc, GEMGXLPLLCFG1, val | PLL1_CKE);
		return 0;
	} else {
		/* Not implemented. */
		return ENXIO;
	}
}

static int
fu540_prci_clk_disable(void *priv, struct clk *clk)
{
	return ENXIO;
}

static const struct clk_funcs fu540_prci_clk_funcs = {
	.get = fu540_prci_clk_get,
	.put = fu540_prci_clk_put,
	.get_rate = fu540_prci_clk_get_rate,
	.enable = fu540_prci_clk_enable,
	.disable = fu540_prci_clk_disable,
};

static struct clk *
fu540_prci_fdt_decode(device_t dev, int cc_phandle, const void *data, size_t len)
{
	struct fu540_prci_softc * const sc = device_private(dev);
	u_int clkid;

	if (len != 4) {
		return NULL;
	}

	clkid = be32dec(data);
	if (clkid >= num_clkid) {
		return NULL;
	}

	return &sc->sc_clk[clkid];
}

static const struct fdtbus_clock_controller_func fu540_prci_fdt_funcs = {
	.decode = fu540_prci_fdt_decode
};

static int
fu540_prci_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
fu540_prci_attach(device_t parent, device_t self, void *aux)
{
	struct fu540_prci_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	const char *clkname;
	struct clk *clk;
	bus_addr_t addr;
	bus_size_t size;
	u_int clkid;

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

	clk = fdtbus_clock_get(phandle, "hfclk");
	if (clk == NULL) {
		aprint_error(": couldn't get hfclk\n");
		return;
	}
	sc->sc_hfclk = clk_get_rate(clk);

	clk = fdtbus_clock_get(phandle, "rtcclk");
	if (clk == NULL) {
		aprint_error(": couldn't get rtcclk\n");
		return;
	}
	sc->sc_rtcclk = clk_get_rate(clk);

	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &fu540_prci_clk_funcs;
	sc->sc_clkdom.priv = sc;
	for (clkid = 0; clkid < num_clkid; clkid++) {
		clkname = fdtbus_get_string_index(phandle,
		    "clock-output-names", clkid);
		sc->sc_clk[clkid].domain = &sc->sc_clkdom;
		if (clkname != NULL) {
			sc->sc_clk[clkid].name = kmem_asprintf("%s", clkname);
		}
		clk_attach(&sc->sc_clk[clkid]);
	}

	aprint_naive("\n");
	aprint_normal(": FU540 PRCI (HF %u Hz, RTC %u Hz)\n",
	    sc->sc_hfclk, sc->sc_rtcclk);

	for (clkid = 0; clkid < num_clkid; clkid++) {
		aprint_debug_dev(self, "clkid %u [%s]: %u Hz\n", clkid,
		    sc->sc_clk[clkid].name ? sc->sc_clk[clkid].name : "<none>",
		    clk_get_rate(&sc->sc_clk[clkid]));
	}

	fdtbus_register_clock_controller(self, phandle, &fu540_prci_fdt_funcs);
}
