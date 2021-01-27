/* $NetBSD: mesongxbb_aoclkc.c,v 1.3 2021/01/27 03:10:18 thorpej Exp $ */

/*-
 * Copyright (c) 2019 Jared McNeill <jmcneill@invisible.ca>
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

__KERNEL_RCSID(1, "$NetBSD: mesongxbb_aoclkc.c,v 1.3 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/bus.h>
#include <sys/device.h>
#include <sys/systm.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_clk.h>
#include <arm/amlogic/mesongxbb_aoclkc.h>

#define	AO_RTI_GEN_CTNL_REG0	0x40

#define	GATE_PARENT		"mpeg-clk"

static int mesongxbb_aoclkc_match(device_t, cfdata_t, void *);
static void mesongxbb_aoclkc_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-gxbb-aoclkc" },
	{ .compat = "amlogic,meson-gxl-aoclkc" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(mesongxbb_aoclkc, sizeof(struct meson_clk_softc),
	mesongxbb_aoclkc_match, mesongxbb_aoclkc_attach, NULL, NULL);

static struct meson_clk_reset mesongxbb_aoclkc_resets[] = {
	MESON_CLK_RESET(MESONGXBB_RESET_AO_REMOTE, AO_RTI_GEN_CTNL_REG0, 16),
	MESON_CLK_RESET(MESONGXBB_RESET_AO_UART1, AO_RTI_GEN_CTNL_REG0, 17),
	MESON_CLK_RESET(MESONGXBB_RESET_AO_I2C_MASTER, AO_RTI_GEN_CTNL_REG0, 18),
	MESON_CLK_RESET(MESONGXBB_RESET_AO_I2C_SLAVE, AO_RTI_GEN_CTNL_REG0, 19),
	MESON_CLK_RESET(MESONGXBB_RESET_AO_UART2, AO_RTI_GEN_CTNL_REG0, 22),
	MESON_CLK_RESET(MESONGXBB_RESET_AO_IR_BLASTER, AO_RTI_GEN_CTNL_REG0, 23),
};

static struct meson_clk_clk mesongxbb_aoclkc_clks[] = {
	MESON_CLK_GATE(MESONGXBB_CLOCK_AO_REMOTE, "remote_ao", GATE_PARENT, AO_RTI_GEN_CTNL_REG0, 0),
	MESON_CLK_GATE(MESONGXBB_CLOCK_AO_I2C_MASTER, "i2c_master_ao", GATE_PARENT, AO_RTI_GEN_CTNL_REG0, 1),
	MESON_CLK_GATE(MESONGXBB_CLOCK_AO_I2C_SLAVE, "i2c_slave_ao", GATE_PARENT, AO_RTI_GEN_CTNL_REG0, 2),
	MESON_CLK_GATE(MESONGXBB_CLOCK_AO_UART1, "uart1_ao", GATE_PARENT, AO_RTI_GEN_CTNL_REG0, 3),
	MESON_CLK_GATE(MESONGXBB_CLOCK_AO_UART2, "uart2_ao", GATE_PARENT, AO_RTI_GEN_CTNL_REG0, 5),
	MESON_CLK_GATE(MESONGXBB_CLOCK_AO_IR_BLASTER, "ir_blaster_ao", GATE_PARENT, AO_RTI_GEN_CTNL_REG0, 6),
};

static int
mesongxbb_aoclkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesongxbb_aoclkc_attach(device_t parent, device_t self, void *aux)
{
	struct meson_clk_softc * const sc = device_private(self);
	struct fdt_attach_args * const faa = aux;

	sc->sc_dev = self;
	sc->sc_phandle = faa->faa_phandle;
	sc->sc_syscon = fdtbus_syscon_lookup(OF_parent(sc->sc_phandle));
	if (sc->sc_syscon == NULL) {
		aprint_error(": couldn't get syscon registers\n");
		return;
	}

	sc->sc_resets = mesongxbb_aoclkc_resets;
	sc->sc_nresets = __arraycount(mesongxbb_aoclkc_resets);

	sc->sc_clks = mesongxbb_aoclkc_clks;
	sc->sc_nclks = __arraycount(mesongxbb_aoclkc_clks);

	meson_clk_attach(sc);

	aprint_naive("\n");
	aprint_normal(": Meson GX AO clock controller\n");

	meson_clk_print(sc);
}
