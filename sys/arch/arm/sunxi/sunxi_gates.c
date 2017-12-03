/* $NetBSD: sunxi_gates.c,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sunxi_gates.c,v 1.1.8.2 2017/12/03 11:35:56 jdolecek Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kmem.h>

#include <dev/fdt/fdtvar.h>

#include <dev/clk/clk_backend.h>

#define	GATE_REG(index)		(((index) / 32) * 4)
#define	GATE_MASK(index)	__BIT((index) % 32)

static const char * compatible[] = {
	"allwinner,sun4i-a10-gates-clk",
	NULL
};

struct sunxi_gate {
	struct clk		base;
	u_int			index;

	TAILQ_ENTRY(sunxi_gate)	gates;
};

struct sunxi_gates_softc {
	device_t		sc_dev;
	bus_space_tag_t		sc_bst;
	bus_space_handle_t	sc_bsh;
	int			sc_phandle;

	struct clk_domain	sc_clkdom;
	TAILQ_HEAD(, sunxi_gate) sc_gates;
};

#define	GATE_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	GATE_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

static struct clk *
sunxi_gates_clock_decode(device_t dev, const void *data, size_t len)
{
	struct sunxi_gates_softc * const sc = device_private(dev);
	struct sunxi_gate *gate;

	if (len != 4)
		return NULL;

	const u_int index = be32dec(data);

	TAILQ_FOREACH(gate, &sc->sc_gates, gates)
		if (gate->index == index)
			return &gate->base;

	return NULL;
}

static const struct fdtbus_clock_controller_func sunxi_gates_fdtclock_funcs = {
	.decode = sunxi_gates_clock_decode,
};

static struct clk *
sunxi_gates_clock_get(void *priv, const char *name)
{
	struct sunxi_gates_softc * const sc = priv;
	struct sunxi_gate *gate;

	TAILQ_FOREACH(gate, &sc->sc_gates, gates)
		if (strcmp(gate->base.name, name) == 0)
			return &gate->base;

	return NULL;
}

static void
sunxi_gates_clock_put(void *priv, struct clk *clk)
{
}

static u_int
sunxi_gates_clock_get_rate(void *priv, struct clk *clkp)
{
	struct sunxi_gates_softc * const sc = priv;
	struct sunxi_gate *gate = (struct sunxi_gate *)clkp;
	struct clk *clkp_parent;

	clkp_parent = clk_get_parent(clkp);
	if (clkp_parent == NULL)
		return 0;

	const bus_size_t gate_reg = GATE_REG(gate->index);
	const uint32_t gate_mask = GATE_MASK(gate->index);

	if ((GATE_READ(sc, gate_reg) & gate_mask) == 0)
		return 0;

	return clk_get_rate(clkp_parent);
}

static int
sunxi_gates_clock_enable(void *priv, struct clk *clkp)
{
	struct sunxi_gates_softc * const sc = priv;
	struct sunxi_gate *gate = (struct sunxi_gate *)clkp;
	uint32_t val;

	const bus_size_t gate_reg = GATE_REG(gate->index);
	const uint32_t gate_mask = GATE_MASK(gate->index);

	val = GATE_READ(sc, gate_reg);
	val |= gate_mask;
	GATE_WRITE(sc, gate_reg, val);

	return 0;
}

static int
sunxi_gates_clock_disable(void *priv, struct clk *clkp)
{
	struct sunxi_gates_softc * const sc = priv;
	struct sunxi_gate *gate = (struct sunxi_gate *)clkp;
	uint32_t val;

	const bus_size_t gate_reg = GATE_REG(gate->index);
	const uint32_t gate_mask = GATE_MASK(gate->index);

	val = GATE_READ(sc, gate_reg);
	val &= ~gate_mask;
	GATE_WRITE(sc, gate_reg, val);

	return 0;
}

static struct clk *
sunxi_gates_clock_get_parent(void *priv, struct clk *clkp)
{
	struct sunxi_gates_softc * const sc = priv;

	return fdtbus_clock_get_index(sc->sc_phandle, 0);
}

static const struct clk_funcs sunxi_gates_clock_funcs = {
	.get = sunxi_gates_clock_get,
	.put = sunxi_gates_clock_put,
	.get_rate = sunxi_gates_clock_get_rate,
	.enable = sunxi_gates_clock_enable,
	.disable = sunxi_gates_clock_disable,
	.get_parent = sunxi_gates_clock_get_parent,
};

static void
sunxi_gates_print(struct sunxi_gates_softc *sc)
{
	struct sunxi_gate *gate;
	struct clk *clkp_parent;

	TAILQ_FOREACH(gate, &sc->sc_gates, gates) {
		clkp_parent = clk_get_parent(&gate->base);

        	aprint_debug_dev(sc->sc_dev,
		    "%3d %-12s %2s %-12s %-7s ",
		    gate->index,
        	    gate->base.name,
        	    clkp_parent ? "<-" : "",
        	    clkp_parent ? clkp_parent->name : "",
        	    "gate");
		aprint_debug("%10d Hz\n", clk_get_rate(&gate->base));
	}
}

static int
sunxi_gates_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
sunxi_gates_attach(device_t parent, device_t self, void *aux)
{
	struct sunxi_gates_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const int phandle = faa->faa_phandle;
	struct sunxi_gate *gate;
	const u_int *indices;
	bus_addr_t addr;
	bus_size_t size;
	int len, i;

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
	TAILQ_INIT(&sc->sc_gates);

	aprint_naive("\n");
	aprint_normal("\n");

	sc->sc_clkdom.funcs = &sunxi_gates_clock_funcs;
	sc->sc_clkdom.priv = sc;

	indices = fdtbus_get_prop(phandle, "clock-indices", &len);
	if (indices == NULL) {
		aprint_error_dev(self, "no clock-indices property\n");
		return;
	}

	for (i = 0;
	     len >= sizeof(u_int);
	     len -= sizeof(u_int), i++, indices++) {
		const u_int index = be32dec(indices);
		const char *name = fdtbus_get_string_index(phandle,
		    "clock-output-names", i);

		if (name == NULL) {
			aprint_error_dev(self, "no name for clk index %d\n",
			    index);
			continue;
		}

		gate = kmem_zalloc(sizeof(*gate), KM_SLEEP);
		gate->base.domain = &sc->sc_clkdom;
		gate->base.name = name;
		gate->index = index;

		TAILQ_INSERT_TAIL(&sc->sc_gates, gate, gates);
	}
	
	fdtbus_register_clock_controller(sc->sc_dev, phandle,
	    &sunxi_gates_fdtclock_funcs);

	sunxi_gates_print(sc);
}

CFATTACH_DECL_NEW(sunxi_gates, sizeof(struct sunxi_gates_softc),
    sunxi_gates_match, sunxi_gates_attach, NULL, NULL);
