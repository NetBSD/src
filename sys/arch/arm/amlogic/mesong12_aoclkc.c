/* $NetBSD: mesong12_aoclkc.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $ */

/*
 * Copyright (c) 2021 Ryo Shimizu <ryo@nerv.org>
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mesong12_aoclkc.c,v 1.2 2021/01/27 03:10:18 thorpej Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <dev/fdt/fdtvar.h>

#include <arm/amlogic/meson_clk.h>
#include <arm/amlogic/mesong12_aoclkc.h>

#define	AO_SOFT_RESET_REG		0x40
#define	AO_DOMAIN_CLOCK_GATEING0_REG	0x4c
#define	AO_DOMAIN_CLOCK_GATEING1_REG	0x50

#define	GATE_PARENT		"mpeg-clk"

static int mesong12_aoclkc_match(device_t, cfdata_t, void *);
static void mesong12_aoclkc_attach(device_t, device_t, void *);

static const struct device_compatible_entry compat_data[] = {
	{ .compat = "amlogic,meson-g12a-aoclkc" },
	DEVICE_COMPAT_EOL
};

CFATTACH_DECL_NEW(mesong12_aoclkc, sizeof(struct meson_clk_softc),
    mesong12_aoclkc_match, mesong12_aoclkc_attach, NULL, NULL);

static struct meson_clk_reset mesong12_aoclkc_resets[] = {
	MESON_CLK_RESET(MESONG12_RESET_AO_IR_IN,   AO_SOFT_RESET_REG, 16),
	MESON_CLK_RESET(MESONG12_RESET_AO_UART,    AO_SOFT_RESET_REG, 17),
	MESON_CLK_RESET(MESONG12_RESET_AO_I2C_M,   AO_SOFT_RESET_REG, 18),
	MESON_CLK_RESET(MESONG12_RESET_AO_I2C_S,   AO_SOFT_RESET_REG, 19),
	MESON_CLK_RESET(MESONG12_RESET_AO_SAR_ADC, AO_SOFT_RESET_REG, 20),
	MESON_CLK_RESET(MESONG12_RESET_AO_UART2,   AO_SOFT_RESET_REG, 22),
	MESON_CLK_RESET(MESONG12_RESET_AO_IR_OUT,  AO_SOFT_RESET_REG, 23),
};

static struct meson_clk_clk mesong12_aoclkc_clks[] = {
	MESON_CLK_GATE(MESONG12_CLOCK_AO_AHB, "ahb_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 0),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_IR_IN,"ir_in_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 1),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_I2C_M0, "i2c_m0_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 2),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_I2C_S0, "i2c_s0_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 3),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_UART, "uart_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 4),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_PROD_I2C, "prod_i2c_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 5),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_UART2, "uart2_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 6),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_IR_OUT, "ir_out_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 7),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_SAR_ADC, "sar_adc_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING0_REG, 8),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_MAILBOX, "mailbox_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING1_REG, 0),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_M3, "m3_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING1_REG, 1),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_AHB_SRAM, "ahb_sram_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING1_REG, 2),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_RTI, "rti_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING1_REG, 3),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_M4_FCLK, "m4_fclk_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING1_REG, 4),
	MESON_CLK_GATE(MESONG12_CLOCK_AO_M4_HCLK, "m4_hclk_ao",
	    GATE_PARENT, AO_DOMAIN_CLOCK_GATEING1_REG, 5),
};

static int
mesong12_aoclkc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct fdt_attach_args * const faa = aux;

	return of_compatible_match(faa->faa_phandle, compat_data);
}

static void
mesong12_aoclkc_attach(device_t parent, device_t self, void *aux)
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

	sc->sc_resets = mesong12_aoclkc_resets;
	sc->sc_nresets = __arraycount(mesong12_aoclkc_resets);

	sc->sc_clks = mesong12_aoclkc_clks;
	sc->sc_nclks = __arraycount(mesong12_aoclkc_clks);

	aprint_naive("\n");
	aprint_normal(": Meson G12A AO clock controller\n");

	meson_clk_attach(sc);
	meson_clk_print(sc);
}
