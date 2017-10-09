/* $NetBSD: sunxi_sramc.c,v 1.1 2017/10/09 15:53:28 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_sramc.c,v 1.1 2017/10/09 15:53:28 jmcneill Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <arm/sunxi/sunxi_sramc.h>

static const char * compatible[] = {
	"allwinner,sun4i-a10-sram-controller",
	NULL
};

static const struct sunxi_sramc_area {
	const char			*compatible;
	const char			*desc;
	bus_size_t			reg;
	uint32_t			mask;
} sunxi_sramc_areas[] = {
	{ "allwinner,sun4i-a10-sram-a3-a4",
	  "SRAM A3/A4",
	  0x04, __BITS(5,4) },
	{ "allwinner,sun4i-a10-sram-d",
	  "SRAM D",
	  0x04, __BIT(0) }
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
	struct sunxi_sramc_node *node;
	int child, i;

	for (child = OF_child(phandle); child; child = OF_peer(child))
		for (i = 0; i < __arraycount(sunxi_sramc_areas); i++) {
			const char * area_compatible[] = { sunxi_sramc_areas[i].compatible, NULL };
			if (of_match_compatible(child, area_compatible)) {
				node = kmem_alloc(sizeof(*node), KM_SLEEP);
				node->phandle = child;
				node->area = &sunxi_sramc_areas[i];
				TAILQ_INSERT_TAIL(&sc->sc_nodes, node, nodes);
				aprint_verbose_dev(sc->sc_dev, "area: %s\n", node->area->desc);
				break;
			}
		}
}

static void
sunxi_sramc_init(struct sunxi_sramc_softc *sc)
{
	const char * mmio_compatible[] = { "mmio-sram", NULL };
	int child;

	for (child = OF_child(sc->sc_phandle); child; child = OF_peer(child)) {
		if (!of_match_compatible(child, mmio_compatible))
			continue;
		sunxi_sramc_init_mmio(sc, child);
	}
}

static int
sunxi_sramc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
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
	TAILQ_INIT(&sc->sc_nodes);

	aprint_naive("\n");
	aprint_normal(": SRAM Controller\n");

	sunxi_sramc_init(sc);

	KASSERT(sramc_softc == NULL);
	sramc_softc = sc;
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
