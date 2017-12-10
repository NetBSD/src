/* $NetBSD: bcm2835_aux.c,v 1.1 2017/12/10 21:38:26 skrll Exp $ */

/*-
 * Copyright (c) 2017 Jared D. McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_aux.c,v 1.1 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

/* Registers */
#define	BCMAUX_AUXIRQ_REG	0x00
#define	BCMAUX_AUXENB_REG	0x04

/* Clock IDs */
#define	BCMAUX_CLOCK_UART	0
#define	BCMAUX_CLOCK_SPI1	1
#define	BCMAUX_CLOCK_SPI2	2
#define	BCMAUX_NCLOCK		3

static int	bcmaux_match(device_t, cfdata_t, void *);
static void	bcmaux_attach(device_t, device_t, void *);

static struct clk *bcmaux_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func bcmaux_fdt_funcs = {
	.decode = bcmaux_decode
};

static struct clk *bcmaux_get(void *, const char *);
static void	bcmaux_put(void *, struct clk *);
static u_int	bcmaux_get_rate(void *, struct clk *);
static int	bcmaux_enable(void *, struct clk *);
static int	bcmaux_disable(void *, struct clk *);

static const struct clk_funcs bcmaux_clk_funcs = {
	.get = bcmaux_get,
	.put = bcmaux_put,
	.get_rate = bcmaux_get_rate,
	.enable = bcmaux_enable,
	.disable = bcmaux_disable,
};

struct bcmaux_clk {
	struct clk	base;
	uint32_t	mask;
};

struct bcmaux_softc {
	device_t	sc_dev;
	int		sc_phandle;
	bus_space_tag_t	sc_bst;
	bus_space_handle_t sc_bsh;

	struct clk	*sc_pclk;

	struct clk_domain sc_clkdom;
	struct bcmaux_clk sc_clk[BCMAUX_NCLOCK];
};

#define	BCMAUX_READ(sc, reg)		\
	bus_space_read_4((sc)->sc_bst, (sc)->sc_bsh, (reg))
#define	BCMAUX_WRITE(sc, reg, val)	\
	bus_space_write_4((sc)->sc_bst, (sc)->sc_bsh, (reg), (val))

CFATTACH_DECL_NEW(bcmaux_fdt, sizeof(struct bcmaux_softc),
    bcmaux_match, bcmaux_attach, NULL, NULL);

static int
bcmaux_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-aux", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
bcmaux_attach(device_t parent, device_t self, void *aux)
{
	struct bcmaux_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	bus_addr_t addr;
	bus_size_t size;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_clkdom.funcs = &bcmaux_clk_funcs;
	sc->sc_clkdom.priv = sc;
	sc->sc_pclk = fdtbus_clock_get_index(phandle, 0);
	if (sc->sc_pclk == NULL) {
		aprint_error(": couldn't get parent clock\n");
		return;
	}
	sc->sc_bst = faa->faa_bst;
	if (bus_space_map(sc->sc_bst, addr, size, 0, &sc->sc_bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	sc->sc_clk[BCMAUX_CLOCK_UART].base.domain = &sc->sc_clkdom;
	sc->sc_clk[BCMAUX_CLOCK_UART].base.name = "aux_uart";
	sc->sc_clk[BCMAUX_CLOCK_UART].mask = __BIT(0);

	sc->sc_clk[BCMAUX_CLOCK_SPI1].base.domain = &sc->sc_clkdom;
	sc->sc_clk[BCMAUX_CLOCK_SPI1].base.name = "aux_spi1";
	sc->sc_clk[BCMAUX_CLOCK_SPI1].mask = __BIT(1);

	sc->sc_clk[BCMAUX_CLOCK_SPI2].base.domain = &sc->sc_clkdom;
	sc->sc_clk[BCMAUX_CLOCK_SPI2].base.name = "aux_spi2";
	sc->sc_clk[BCMAUX_CLOCK_SPI2].mask = __BIT(2);

	aprint_naive("\n");
	aprint_normal("\n");

	fdtbus_register_clock_controller(self, phandle, &bcmaux_fdt_funcs);
}

static struct clk *
bcmaux_decode(device_t dev, const void *data, size_t len)
{
	struct bcmaux_softc * const sc = device_private(dev);
	u_int clkid;

	if (len != 4)
		return NULL;

	clkid = be32dec(data);
	if (clkid >= BCMAUX_NCLOCK)
		return NULL;

	return &sc->sc_clk[clkid].base;
}

static struct clk *
bcmaux_get(void *priv, const char *name)
{
	struct bcmaux_softc * const sc = priv;

	for (size_t i = 0; i < BCMAUX_NCLOCK; i++) {
		if (strcmp(name, sc->sc_clk[i].base.name) == 0)
			return &sc->sc_clk[i].base;
	}

	return NULL;
}

static void
bcmaux_put(void *priv, struct clk *clk)
{
}

static u_int
bcmaux_get_rate(void *priv, struct clk *clk)
{
	struct bcmaux_softc * const sc = priv;

	return clk_get_rate(sc->sc_pclk);
}

static int
bcmaux_enable(void *priv, struct clk *clk)
{
	struct bcmaux_softc * const sc = priv;
	struct bcmaux_clk *auxclk = (struct bcmaux_clk *)clk;
	uint32_t val;

	val = BCMAUX_READ(sc, BCMAUX_AUXENB_REG);
	val |= auxclk->mask;
	BCMAUX_WRITE(sc, BCMAUX_AUXENB_REG, val);

	return 0;
}

static int
bcmaux_disable(void *priv, struct clk *clk)
{
	struct bcmaux_softc * const sc = priv;
	struct bcmaux_clk *auxclk = (struct bcmaux_clk *)clk;
	uint32_t val;

	val = BCMAUX_READ(sc, BCMAUX_AUXENB_REG);
	val &= ~auxclk->mask;
	BCMAUX_WRITE(sc, BCMAUX_AUXENB_REG, val);

	return 0;
}
