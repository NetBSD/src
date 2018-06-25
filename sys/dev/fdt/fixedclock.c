/* $NetBSD: fixedclock.c,v 1.2.16.2 2018/06/25 07:25:49 pgoyette Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: fixedclock.c,v 1.2.16.2 2018/06/25 07:25:49 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

static int	fixedclock_match(device_t, cfdata_t, void *);
static void	fixedclock_attach(device_t, device_t, void *);

static struct clk *fixedclock_decode(device_t, const void *, size_t);

static const struct fdtbus_clock_controller_func fixedclock_fdt_funcs = {
	.decode = fixedclock_decode
};

static struct clk *fixedclock_get(void *, const char *);
static void	fixedclock_put(void *, struct clk *);
static u_int	fixedclock_get_rate(void *, struct clk *);

static const struct clk_funcs fixedclock_clk_funcs = {
	.get = fixedclock_get,
	.put = fixedclock_put,
	.get_rate = fixedclock_get_rate,
};

struct fixedclock_clk {
	struct clk	base;
	u_int		rate;
};

struct fixedclock_softc {
	device_t	sc_dev;
	int		sc_phandle;

	struct clk_domain sc_clkdom;
	struct fixedclock_clk sc_clk;
};

CFATTACH_DECL_NEW(fclock, sizeof(struct fixedclock_softc),
    fixedclock_match, fixedclock_attach, NULL, NULL);

static int
fixedclock_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "fixed-clock", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
fixedclock_attach(device_t parent, device_t self, void *aux)
{
	struct fixedclock_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;
	const char *clkname;

	clkname = fdtbus_get_string(phandle, "clock-output-names");
	if (!clkname)
		clkname = faa->faa_name;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_clkdom.name = device_xname(self);
	sc->sc_clkdom.funcs = &fixedclock_clk_funcs;
	sc->sc_clkdom.priv = sc;
	if (of_getprop_uint32(phandle, "clock-frequency",
	    &sc->sc_clk.rate) != 0) {
		aprint_error(": couldn't determine frequency\n");
		return;
	}
	sc->sc_clk.base.domain = &sc->sc_clkdom;
	sc->sc_clk.base.name = kmem_asprintf("%s", clkname);
	clk_attach(&sc->sc_clk.base);

	aprint_naive("\n");
	aprint_normal(": %u Hz fixed clock (%s)\n", sc->sc_clk.rate, clkname);

	fdtbus_register_clock_controller(self, phandle, &fixedclock_fdt_funcs);
}

static struct clk *
fixedclock_decode(device_t dev, const void *data, size_t len)
{
	struct fixedclock_softc * const sc = device_private(dev);

	/* #clock-cells for a fixed clock is always 0 */
	if (len != 0)
		return NULL;

	return &sc->sc_clk.base;
}

static struct clk *
fixedclock_get(void *priv, const char *name)
{
	struct fixedclock_softc * const sc = priv;

	if (strcmp(name, sc->sc_clk.base.name) != 0)
		return NULL;

	return &sc->sc_clk.base;
}

static void
fixedclock_put(void *priv, struct clk *clk)
{
}

static u_int
fixedclock_get_rate(void *priv, struct clk *clk)
{
	struct fixedclock_clk *fclk = (struct fixedclock_clk *)clk;

	return fclk->rate;
}
