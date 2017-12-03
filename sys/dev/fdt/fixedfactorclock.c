/* $NetBSD: fixedfactorclock.c,v 1.1.8.2 2017/12/03 11:37:01 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fixedfactorclock.c,v 1.1.8.2 2017/12/03 11:37:01 jdolecek Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static int	fixedfactorclock_match(device_t, cfdata_t, void *);
static void	fixedfactorclock_attach(device_t, device_t, void *);

static struct clk *fixedfactorclock_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func fixedfactorclock_fdt_funcs = {
	.decode = fixedfactorclock_decode
};

static struct clk *fixedfactorclock_get(void *, const char *);
static void	fixedfactorclock_put(void *, struct clk *);
static u_int	fixedfactorclock_get_rate(void *, struct clk *);

static const struct clk_funcs fixedfactorclock_clk_funcs = {
	.get = fixedfactorclock_get,
	.put = fixedfactorclock_put,
	.get_rate = fixedfactorclock_get_rate,
};

struct fixedfactorclock_clk {
	struct clk	base;

	u_int		div;
	u_int		mult;
};

struct fixedfactorclock_softc {
	device_t	sc_dev;
	int		sc_phandle;

	struct clk_domain sc_clkdom;
	struct fixedfactorclock_clk sc_clk;
};

CFATTACH_DECL_NEW(ffclock, sizeof(struct fixedfactorclock_softc),
    fixedfactorclock_match, fixedfactorclock_attach, NULL, NULL);

static int
fixedfactorclock_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "fixed-factor-clock", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
fixedfactorclock_attach(device_t parent, device_t self, void *aux)
{
	struct fixedfactorclock_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	const char *name;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_clkdom.funcs = &fixedfactorclock_clk_funcs;
	sc->sc_clkdom.priv = sc;

	of_getprop_uint32(phandle, "clock-div", &sc->sc_clk.div);
	of_getprop_uint32(phandle, "clock-mult", &sc->sc_clk.mult);

	if (sc->sc_clk.div == 0 || sc->sc_clk.mult == 0) {
		aprint_error(": invalid properties\n");
		return;
	}

	name = fdtbus_get_string(phandle, "clock-output-names");
	if (name == NULL)
		name = faa->faa_name;

	sc->sc_clk.base.domain = &sc->sc_clkdom;
	sc->sc_clk.base.name = name;

	aprint_naive("\n");
	aprint_normal(": x%u /%u fixed-factor clock\n",
	    sc->sc_clk.mult, sc->sc_clk.div);

	fdtbus_register_clock_controller(self, phandle,
	    &fixedfactorclock_fdt_funcs);
}

static struct clk *
fixedfactorclock_decode(device_t dev, const void *data, size_t len)
{
	struct fixedfactorclock_softc * const sc = device_private(dev);

	/* #clock-cells for a fixed factor clock is always 0 */
	if (len != 0)
		return NULL;

	return &sc->sc_clk.base;
}

static struct clk *
fixedfactorclock_get(void *priv, const char *name)
{
	struct fixedfactorclock_softc * const sc = priv;

	if (strcmp(name, sc->sc_clk.base.name) != 0)
		return NULL;

	return &sc->sc_clk.base;
}

static void
fixedfactorclock_put(void *priv, struct clk *clk)
{
}

static u_int
fixedfactorclock_get_rate(void *priv, struct clk *clk)
{
	struct fixedfactorclock_softc * const sc = priv;
	struct fixedfactorclock_clk *fclk = (struct fixedfactorclock_clk *)clk;
	struct clk *clkp_parent;

	clkp_parent = fdtbus_clock_get_index(sc->sc_phandle, 0);
	if (clkp_parent == NULL)
		return 0;

	return (clk_get_rate(clkp_parent) * fclk->mult) / fclk->div;
}
