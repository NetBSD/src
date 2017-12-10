/* $NetBSD: bcm2835_cprman.c,v 1.1 2017/12/10 21:38:26 skrll Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: bcm2835_cprman.c,v 1.1 2017/12/10 21:38:26 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kmem.h>
#include <sys/bus.h>

#include <dev/clk/clk_backend.h>

#include <dev/fdt/fdtvar.h>

#include <arm/broadcom/bcm2835var.h>

enum {
	CPRMAN_CLOCK_TIMER = 17,
	CPRMAN_CLOCK_UART = 19,
	CPRMAN_CLOCK_VPU = 20,
	CPRMAN_CLOCK_V3D = 21,
	CPRMAN_CLOCK_ISP = 22,
	CPRMAN_CLOCK_H264 = 23,
	CPRMAN_CLOCK_VEC = 24,
	CPRMAN_CLOCK_HSM = 25,
	CPRMAN_CLOCK_SDRAM = 26,
	CPRMAN_CLOCK_TSENS = 27,
	CPRMAN_CLOCK_EMMC = 28,
	CPRMAN_CLOCK_PERIIMAGE = 29,
	CPRMAN_CLOCK_PWM = 30,
	CPRMAN_CLOCK_PCM = 31,
	CPRMAN_NCLOCK
};

struct cprman_clk {
	struct clk	base;
	u_int		id;
};

struct cprman_softc {
	device_t	sc_dev;
	int		sc_phandle;

	struct clk_domain sc_clkdom;
	struct cprman_clk sc_clk[CPRMAN_NCLOCK];
};


static struct clk *
cprman_decode(device_t dev, const void *data, size_t len)
{
	struct cprman_softc * const sc = device_private(dev);
	struct cprman_clk *clk;
	const u_int *spec = data;
	u_int id;

	if (len != 4)
		return NULL;

	id = be32toh(spec[0]);

	if (id >= CPRMAN_NCLOCK)
		return NULL;
	clk = &sc->sc_clk[id];
	if (clk->base.name == NULL)
		return NULL;

	return &clk->base;
}

static const struct fdtbus_clock_controller_func cprman_fdt_funcs = {
	.decode = cprman_decode
};

static struct clk *
cprman_get(void *priv, const char *name)
{
	struct cprman_softc * const sc = priv;
	u_int n;

	for (n = 0; n < __arraycount(sc->sc_clk); n++) {
		if (sc->sc_clk[n].base.name == NULL)
			continue;
		if (strcmp(sc->sc_clk[n].base.name, name) == 0)
			return &sc->sc_clk[n].base;
	}

	return NULL;
}

static void
cprman_put(void *priv, struct clk *clk)
{
}

static u_int
cprman_get_rate(void *priv, struct clk *baseclk)
{
	//struct cprman_softc * const sc = priv;
	struct cprman_clk *clk = container_of(baseclk, struct cprman_clk, base);

	switch (clk->id) {
	case CPRMAN_CLOCK_UART:
		return bcm283x_clk_get_rate_uart();
	case CPRMAN_CLOCK_VPU:
		return bcm283x_clk_get_rate_vpu();
	case CPRMAN_CLOCK_EMMC:
		return bcm283x_clk_get_rate_emmc();
	default:
		panic("unsupported clock id %d\n", clk->id);
	}
}

static const struct clk_funcs cprman_clk_funcs = {
	.get = cprman_get,
	.put = cprman_put,
	.get_rate = cprman_get_rate,
};

static void
cprman_add_clock(struct cprman_softc *sc, u_int id, const char *name)
{
	sc->sc_clk[id].base.domain = &sc->sc_clkdom;
	sc->sc_clk[id].base.name = name;
	sc->sc_clk[id].id = id;
}

static int
cprman_match(device_t parent, cfdata_t cf, void *aux)
{
	const char * const compatible[] = { "brcm,bcm2835-cprman", NULL };
	const struct fdt_attach_args *faa = aux;

	return of_match_compatible(faa->faa_phandle, compatible);
}

static void
cprman_attach(device_t parent, device_t self, void *aux)
{
	struct cprman_softc * const sc = device_private(self);
	const struct fdt_attach_args *faa = aux;
	const int phandle = faa->faa_phandle;

	sc->sc_dev = self;
	sc->sc_phandle = phandle;
	sc->sc_clkdom.funcs = &cprman_clk_funcs;
	sc->sc_clkdom.priv = sc;

	cprman_add_clock(sc, CPRMAN_CLOCK_UART, "uart");
	cprman_add_clock(sc, CPRMAN_CLOCK_VPU, "vpu");
	cprman_add_clock(sc, CPRMAN_CLOCK_EMMC, "emmc");

	aprint_naive("\n");
	aprint_normal(": BCM283x Clock Controller\n");

	fdtbus_register_clock_controller(self, phandle, &cprman_fdt_funcs);
}

CFATTACH_DECL_NEW(bcmcprman_fdt, sizeof(struct cprman_softc),
    cprman_match, cprman_attach, NULL, NULL);
