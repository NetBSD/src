/* $NetBSD: sunxi_twi.c,v 1.17 2021/01/27 03:10:20 thorpej Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: sunxi_twi.c,v 1.17 2021/01/27 03:10:20 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/intr.h>
#include <sys/systm.h>
#include <sys/time.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/gttwsivar.h>
#include <dev/i2c/gttwsireg.h>

#include <dev/fdt/fdtvar.h>

#define	TWI_CCR_REG	0x14
#define	 TWI_CCR_CLK_M	__BITS(6,3)
#define	 TWI_CCR_CLK_N	__BITS(2,0)

static const bus_size_t sunxi_twi_regmap[] = {
	[TWSI_SLAVEADDR]	= TWSI_ALLWINNER_SLAVEADDR,
	[TWSI_EXTEND_SLAVEADDR]	= TWSI_ALLWINNER_EXTEND_SLAVEADDR,
	[TWSI_DATA]		= TWSI_ALLWINNER_DATA,
	[TWSI_CONTROL]		= TWSI_ALLWINNER_CONTROL,
	[TWSI_STATUS]		= TWSI_ALLWINNER_STATUS,
	[TWSI_BAUDRATE]		= TWSI_ALLWINNER_BAUDRATE,
	[TWSI_SOFTRESET]	= TWSI_ALLWINNER_SOFTRESET,
};

static int sunxi_twi_match(device_t, cfdata_t, void *);
static void sunxi_twi_attach(device_t, device_t, void *);

struct sunxi_twi_config {
	bool		iflg_rwc;
};

static const struct sunxi_twi_config sun4i_a10_i2c_config = {
	.iflg_rwc = false,
};

static const struct sunxi_twi_config sun6i_a31_i2c_config = {
	.iflg_rwc = true,
};

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "allwinner,sun4i-a10-i2c",	.data = &sun4i_a10_i2c_config },
	{ .compat = "allwinner,sun6i-a31-i2c",	.data = &sun6i_a31_i2c_config },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(sunxi_twi, sizeof(struct gttwsi_softc),
	sunxi_twi_match, sunxi_twi_attach, NULL, NULL);

static u_int
sunxi_twi_calc_rate(u_int parent_rate, u_int n, u_int m)
{
	return parent_rate / (10 * (m + 1) * (1 << n));
}

static void
sunxi_twi_set_clock(struct gttwsi_softc *sc, u_int parent_rate, u_int rate)
{
	uint32_t baud;
	u_int n, m, best_rate;

	baud = gttwsi_read_4(sc, TWSI_BAUDRATE);

	for (best_rate = 0, n = 0; n < 8; n++) {
		for (m = 0; m < 16; m++) {
			const u_int tmp_rate =
			    sunxi_twi_calc_rate(parent_rate, n, m);
			if (tmp_rate <= rate && tmp_rate > best_rate) {
				best_rate = tmp_rate;
				baud = __SHIFTIN(n, TWI_CCR_CLK_N) |
				       __SHIFTIN(m, TWI_CCR_CLK_M);
			}
		}
	}

	gttwsi_write_4(sc, TWSI_BAUDRATE, baud);
	delay(10000);
}

static int
sunxi_twi_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
sunxi_twi_attach(device_t parent, device_t self, void *aux)
{
	struct gttwsi_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;
	const struct sunxi_twi_config *conf;
	const int phandle = faa->faa_phandle;
	bus_space_tag_t bst = faa->faa_bst;
	bus_space_handle_t bsh;
	struct fdtbus_reset *rst;
	struct clk *clk;
	char intrstr[128];
	bus_addr_t addr;
	bus_size_t size;
	void *ih;

	if (fdtbus_get_reg(phandle, 0, &addr, &size) != 0) {
		aprint_error(": couldn't get registers\n");
		return;
	}

	if (bus_space_map(bst, addr, size, 0, &bsh) != 0) {
		aprint_error(": couldn't map registers\n");
		return;
	}

	if (!fdtbus_intr_str(phandle, 0, intrstr, sizeof(intrstr))) {
		aprint_error(": failed to decode interrupt\n");
		return;
	}

	if ((clk = fdtbus_clock_get_index(phandle, 0)) != NULL)
		if (clk_enable(clk) != 0) {
			aprint_error(": couldn't enable clock\n");
			return;
		}
	if ((rst = fdtbus_reset_get_index(phandle, 0)) != NULL)
		if (fdtbus_reset_deassert(rst) != 0) {
			aprint_error(": couldn't de-assert reset\n");
			return;
		}

	conf = of_compatible_lookup(phandle, compat_data)->data;
	prop_dictionary_set_bool(device_properties(self), "iflg-rwc",
	    conf->iflg_rwc);

	/* Attach gttwsi core */
	gttwsi_attach_subr(self, bst, bsh, sunxi_twi_regmap);

	/*
	 * Set clock rate to 100kHz.
	 */
	if (clk != NULL)
		sunxi_twi_set_clock(sc, clk_get_rate(clk), 100000);

	ih = fdtbus_intr_establish_xname(phandle, 0, IPL_VM, 0, gttwsi_intr,
	    sc, device_xname(self));
	if (ih == NULL) {
		aprint_error_dev(self, "couldn't establish interrupt on %s\n",
		    intrstr);
		return;
	}
	aprint_normal_dev(self, "interrupting on %s\n", intrstr);

	fdtbus_register_i2c_controller(&sc->sc_i2c, phandle);

	fdtbus_attach_i2cbus(self, phandle, &sc->sc_i2c, iicbus_print);
}
