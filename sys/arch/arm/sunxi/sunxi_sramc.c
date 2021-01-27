/* $NetBSD: sunxi_sramc.c,v 1.11 2021/01/27 03:10:20 thorpej Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_sramc.c,v 1.11 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>
#include <sys/mutex.h>

#include <dev/fdt/fdtvar.h>
#include <dev/fdt/syscon.h>

#include <arm/sunxi/sunxi_sramc.h>

static const struct device_compatible_entry compat_data[] = {
		/* old compat string */
	{ .compat = "allwinner,sun4i-a10-sram-controller" },
	{ .compat = "allwinner,sun4i-a10-system-control" },
	{ .compat = "allwinner,sun8i-h3-system-control" },
	{ .compat = "allwinner,sun50i-a64-system-control" },
	{ .compat = "allwinner,sun50i-h5-system-control" },
	{ .compat = "allwinner,sun50i-h6-system-control" },
	DEVICE_COMPAT_EOL
};

struct sunxi_sramc_area {
	const char			*desc;
	bus_size_t			reg;
	uint32_t			mask;
	u_int				flags;
#define	SUNXI_SRAMC_F_SWAP		__BIT(0)
};

static const struct sunxi_sramc_area sunxi_sramc_area_a3_a4 = {
	.desc = "SRAM A3/A4",
	.reg = 0x04,
	.mask = __BITS(5,4),
	.flags = 0,
};

static const struct sunxi_sramc_area sunxi_sramc_area_d = {
	.desc = "SRAM D",
	.reg = 0x04,
	.mask = __BIT(0),
	.flags = 0,
};

static const struct sunxi_sramc_area sunxi_sramc_area_c = {
	.desc = "SRAM C",
	.reg = 0x04,
	.mask = __BIT(24),
	.flags = SUNXI_SRAMC_F_SWAP,
};

static const struct device_compatible_entry sunxi_sramc_areas[] = {
	{ .compat = "allwinner,sun4i-a10-sram-a3-a4",
	  .data = &sunxi_sramc_area_a3_a4 },

	{ .compat = "allwinner,sun4i-a10-sram-d",
	  .data = &sunxi_sramc_area_d },

	{ .compat = "allwinner,sun50i-a64-sram-c",
	  .data = &sunxi_sramc_area_c },

	DEVICE_COMPAT_EOL
};

struct sunxi_sramc_node {
	int				phandle;
	const struct sunxi_sramc_area	*area;
	TAILQ_ENTRY(sunxi_sramc_node)	nodes;
};

struct sunxi_sramc_softc {
	device_t			sc_dev;
	int				sc_phandle;
	bus_space_tag_t			sc_bst;
	bus_space_handle_t		sc_bsh;
	kmutex_t			sc_lock;
	struct syscon			sc_syscon;
	TAILQ_HEAD(, sunxi_sramc_node)	sc_nodes;
};

static struct sunxi_sramc_softc *sramc_softc = NULL;

#define SRAMC_READ(sc, reg) \
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define SRAMC_WRITE(sc, reg, val) \
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static void
sunxi_sramc_init_mmio(struct sunxi_sramc_softc *sc, int phandle)
{
	const struct device_compatible_entry *dce;
	struct sunxi_sramc_node *node;
	int child;

	for (child = OF_child(phandle); child; child = OF_peer(child)) {
		dce = of_compatible_lookup(child, sunxi_sramc_areas);
		if (dce != NULL) {
			node = kmem_alloc(sizeof(*node), KM_SLEEP);
			node->phandle = child;
			node->area = dce->data;
			TAILQ_INSERT_TAIL(&sc->sc_nodes, node, nodes);
			aprint_verbose_dev(sc->sc_dev, "area: %s\n",
			    node->area->desc);
		}
	}
}

static void
sunxi_sramc_init(struct sunxi_sramc_softc *sc)
{
	const struct device_compatible_entry mmio_compat_data[] = {
		{ .compat = "mmio-sram" },
		DEVICE_COMPAT_EOL
	};
	int child;

	for (child = OF_child(sc->sc_phandle); child; child = OF_peer(child)) {
		if (!of_compatible_match(child, mmio_compat_data))
			continue;
		sunxi_sramc_init_mmio(sc, child);
	}
}

static void
sunxi_sramc_lock(void *priv)
{
	struct sunxi_sramc_softc * const sc = priv;

	mutex_enter(&sc->sc_lock);
}

static void
sunxi_sramc_unlock(void *priv)
{
	struct sunxi_sramc_softc * const sc = priv;

	mutex_exit(&sc->sc_lock);
}

static uint32_t
sunxi_sramc_read_4(void *priv, bus_size_t reg)
{
	struct sunxi_sramc_softc * const sc = priv;

	KASSERT(mutex_owned(&sc->sc_lock));

	return SRAMC_READ(sc, reg);
}

static void
sunxi_sramc_write_4(void *priv, bus_size_t reg, uint32_t val)
{
	struct sunxi_sramc_softc * const sc = priv;

	KASSERT(mutex_owned(&sc->sc_lock));

	SRAMC_WRITE(sc, reg, val);
}

static int
sunxi_sramc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_sramc_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_sramc_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}
	mutex_init(&sc->sc_lock, MUTEX_DEFAULT, IPL_VM);
	TAILQ_INIT(&sc->sc_nodes);

	aprint_naive("\n");
	aprint_normal(": SRAM Controller\n");

	sunxi_sramc_init(sc);

	KASSERT(sramc_softc == NULL);
	sramc_softc = sc;

	sc->sc_syscon.priv = sc;
	sc->sc_syscon.lock = sunxi_sramc_lock;
	sc->sc_syscon.unlock = sunxi_sramc_unlock;
	sc->sc_syscon.read_4 = sunxi_sramc_read_4;
	sc->sc_syscon.write_4 = sunxi_sramc_write_4;
	fdtbus_register_syscon(self, phandle, &sc->sc_syscon);
}

CFATTACH_DECL_NEW(sunxi_sramc, sizeof(struct sunxi_sramc_softc),
	sunxi_sramc_match, sunxi_sramc_attach, NULL, NULL);

static int
sunxi_sramc_map(const int node_phandle, u_int config)
{
	struct sunxi_sramc_softc * const sc = sramc_softc;
	struct sunxi_sramc_node *node;
	uint32_t val;

	if (sc == NULL)
		return ENXIO;

	TAILQ_FOREACH(node, &sc->sc_nodes, nodes)
		if (node->phandle == node_phandle) {
			if (config > __SHIFTOUT_MASK(node->area->mask))
				return ERANGE;
			if ((node->area->flags & SUNXI_SRAMC_F_SWAP) != 0)
				config = !config;
			val = SRAMC_READ(sc, node->area->reg);
			val &= ~node->area->mask;
			val |= __SHIFTIN(config, node->area->mask);
			SRAMC_WRITE(sc, node->area->reg, val);
			return 0;
		}

	return EINVAL;
}

int
sunxi_sramc_claim(const int phandle)
{
	const u_int *data;
	int len;

	data = fdtbus_get_prop(phandle, "allwinner,sram", &len);
	if (data == NULL)
		return ENOENT;
	if (len != 8)
		return EIO;

	const int node_phandle = fdtbus_get_phandle_from_native(be32toh(data[0]));
	const u_int config = be32toh(data[1]);

	return sunxi_sramc_map(node_phandle, config);
}
