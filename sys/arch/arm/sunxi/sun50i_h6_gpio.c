/* $NetBSD: sun50i_h6_gpio.c,v 1.1.2.1 2018/04/07 04:12:12 pgoyette Exp $ */

/*-
 * Copyright (c) 2016 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sun50i_h6_gpio.c,v 1.1.2.1 2018/04/07 04:12:12 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

static const struct sunxi_gpio_pins h6_pins[] = {
	{ "PC0",  2, 0,   { "gpio_in", "gpio_out", "nand", NULL, "spi0" } },
	{ "PC1",  2, 1,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC2",  2, 2,   { "gpio_in", "gpio_out", "nand", NULL, "spi0" } },
	{ "PC3",  2, 3,   { "gpio_in", "gpio_out", "nand", NULL, "spi0" } },
	{ "PC4",  2, 4,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC5",  2, 5,   { "gpio_in", "gpio_out", "nand", "mmc2", "spi0" } },
	{ "PC6",  2, 6,   { "gpio_in", "gpio_out", "nand", "mmc2", "spi0" } },
	{ "PC7",  2, 7,   { "gpio_in", "gpio_out", "nand", "mmc2", "spi0" } },
	{ "PC8",  2, 8,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC9",  2, 9,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC10", 2, 10,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC11", 2, 11,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC12", 2, 12,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC13", 2, 13,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC14", 2, 14,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC15", 2, 15,  { "gpio_in", "gpio_out", "nand" } },
	{ "PC16", 2, 16,  { "gpio_in", "gpio_out", "nand" } },

	{ "PD0",  3, 0,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD1",  3, 1,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD2",  3, 2,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD3",  3, 3,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD4",  3, 4,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD5",  3, 5,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD6",  3, 6,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD7",  3, 7,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD8",  3, 8,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD9",  3, 9,   { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD10", 3, 10,  { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD11", 3, 11,  { "gpio_in", "gpio_out", "lcd0", "ts0", "csi", "emac" } },
	{ "PD12", 3, 12,  { "gpio_in", "gpio_out", "lcd0", "ts1", "csi", "emac" } },
	{ "PD13", 3, 13,  { "gpio_in", "gpio_out", "lcd0", "ts1", "csi", "emac" } },
	{ "PD14", 3, 14,  { "gpio_in", "gpio_out", "lcd0", "ts1", "dmic", "csi" } },
	{ "PD15", 3, 15,  { "gpio_in", "gpio_out", "lcd0", "ts1", "dmic", "csi" } },
	{ "PD16", 3, 16,  { "gpio_in", "gpio_out", "lcd0", "ts1", "dmic" } },
	{ "PD17", 3, 17,  { "gpio_in", "gpio_out", "lcd0", "ts2", "dmic" } },
	{ "PD18", 3, 18,  { "gpio_in", "gpio_out", "lcd0", "ts2", "dmic" } },
	{ "PD19", 3, 19,  { "gpio_in", "gpio_out", "lcd0", "ts2", "uart2", "emac" } },
	{ "PD20", 3, 20,  { "gpio_in", "gpio_out", "lcd0", "ts2", "uart2", "emac" } },
	{ "PD21", 3, 21,  { "gpio_in", "gpio_out", "lcd0", "ts2", "uart2" } },
	{ "PD22", 3, 22,  { "gpio_in", "gpio_out", "pwm0", "ts3", "uart2" } },
	{ "PD23", 3, 23,  { "gpio_in", "gpio_out", "i2c2", "ts3", "uart3", "jtag" } }, 
	{ "PD24", 3, 24,  { "gpio_in", "gpio_out", "i2c2", "ts3", "uart3", "jtag" } },
	{ "PD25", 3, 25,  { "gpio_in", "gpio_out", "i2c0", "ts3", "uart3", "jtag" } },
	{ "PD26", 3, 26,  { "gpio_in", "gpio_out", "i2c0", "ts3", "uart3", "jtag" } },

	{ "PF0",  5, 0,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF1",  5, 1,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF2",  5, 2,   { "gpio_in", "gpio_out", "mmc0", "uart0" } },
	{ "PF3",  5, 3,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF4",  5, 4,   { "gpio_in", "gpio_out", "mmc0", "uart0" } },
	{ "PF5",  5, 5,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF6",  5, 6,   { "gpio_in", "gpio_out" } },

	{ "PG0",  6, 0,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 0 },
	{ "PG1",  6, 1,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 1 },
	{ "PG2",  6, 2,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PG3",  6, 3,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 3 },
	{ "PG4",  6, 4,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 4 },
	{ "PG5",  6, 5,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 5 },
	{ "PG6",  6, 6,   { "gpio_in", "gpio_out", "uart1", NULL, NULL, NULL, "irq" }, 6, 6 },
	{ "PG7",  6, 7,   { "gpio_in", "gpio_out", "uart1", NULL, NULL, NULL, "irq" }, 6, 7 },
	{ "PG8",  6, 8,   { "gpio_in", "gpio_out", "uart1", "sim0", NULL, NULL, "irq" }, 6, 8 },
	{ "PG9",  6, 9,   { "gpio_in", "gpio_out", "uart1", "sim0", NULL, NULL, "irq" }, 6, 9 },
	{ "PG10", 6, 10,  { "gpio_in", "gpio_out", "i2s2", "h_i2s2", "sim0", NULL, "irq" }, 6, 10 },
	{ "PG11", 6, 11,  { "gpio_in", "gpio_out", "i2s2", "h_i2s2", "sim0", NULL, "irq" }, 6, 11 },
	{ "PG12", 6, 12,  { "gpio_in", "gpio_out", "i2s2", "h_i2s2", "sim0", NULL, "irq" }, 6, 12 },
	{ "PG13", 6, 13,  { "gpio_in", "gpio_out", "i2s2", "h_i2s2", "sim0", NULL, "irq" }, 6, 13 },
	{ "PG14", 6, 14,  { "gpio_in", "gpio_out", "i2s2", "h_i2s2", "sim0", NULL, "irq" }, 6, 13 },

	{ "PH0",  7, 0,   { "gpio_in", "gpio_out", "uart0", "i2s0", "h_i2s0", "sim1", "irq" }, 6, 0 },
	{ "PH1",  7, 1,   { "gpio_in", "gpio_out", "uart0", "i2s0", "h_i2s0", "sim1", "irq" }, 6, 1 },
	{ "PH2",  7, 2,   { "gpio_in", "gpio_out", "cir", "i2s0", "h_i2s0", "sim1", "irq" }, 6, 2 },
	{ "PH3",  7, 3,   { "gpio_in", "gpio_out", "spi1", "i2s0", "h_i2s0", "sim1", "irq" }, 6, 3 },
	{ "PH4",  7, 4,   { "gpio_in", "gpio_out", "spi1", "i2s0", "h_i2s0", "sim1", "irq" }, 6, 4 },
	{ "PH5",  7, 5,   { "gpio_in", "gpio_out", "spi1", "spdif", "i2c1", "sim1", "irq" }, 6, 5 },
	{ "PH6",  7, 6,   { "gpio_in", "gpio_out", "spi1", "spdif", "i2c1", "sim1", "irq" }, 6, 6 },
	{ "PH7",  7, 7,   { "gpio_in", "gpio_out", NULL, "spdif", NULL, NULL, "irq" }, 6, 7 },
	{ "PH8",  7, 8,   { "gpio_in", "gpio_out", "hdmi", NULL, NULL, NULL, "irq" }, 6, 8 },
	{ "PH9",  7, 9,   { "gpio_in", "gpio_out", "hdmi", NULL, NULL, NULL, "irq" }, 6, 9 },
	{ "PH10", 7, 10,  { "gpio_in", "gpio_out", "hdmi", NULL, NULL, NULL, "irq" }, 6, 10 },
};

static const struct sunxi_gpio_pins h6_r_pins[] = {
	{ "PL0",   0, 0,  { "gpio_in", "gpio_out", NULL, "s_i2c", NULL, NULL, "irq" }, 6, 0 },
	{ "PL1",   0, 1,  { "gpio_in", "gpio_out", NULL, "s_i2c", NULL, NULL, "irq" }, 6, 1 },
	{ "PL2",   0, 2,  { "gpio_in", "gpio_out", "s_uart", NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PL3",   0, 3,  { "gpio_in", "gpio_out", "s_uart", NULL, NULL, NULL, "irq" }, 6, 3 },
	{ "PL4",   0, 4,  { "gpio_in", "gpio_out", "s_jtag", NULL, NULL, NULL, "irq" }, 6, 4 },
	{ "PL5",   0, 5,  { "gpio_in", "gpio_out", "s_jtag", NULL, NULL, NULL, "irq" }, 6, 5 },
	{ "PL6",   0, 6,  { "gpio_in", "gpio_out", "s_jtag", NULL, NULL, NULL, "irq" }, 6, 6 },
	{ "PL7",   0, 7,  { "gpio_in", "gpio_out", "s_jtag", NULL, NULL, NULL, "irq" }, 6, 7 },
	{ "PL8",   0, 8,  { "gpio_in", "gpio_out", "s_i2s", NULL, NULL, NULL, "irq" }, 6, 8 },
	{ "PL9",   0, 9,  { "gpio_in", "gpio_out", "s_cir", NULL, NULL, NULL, "irq" }, 6, 9 },
	{ "PL10",  0, 10, { "gpio_in", "gpio_out", "s_spdif", NULL, NULL, NULL, "irq" }, 6, 10 },

	{ "PM0",   1, 0,  { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "irq" }, 6, 0 },
	{ "PM1",   1, 1,  { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "irq" }, 6, 1 },
	{ "PM2",   1, 2,  { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PM3",   1, 3,  { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "irq" }, 6, 3 },
	{ "PM4",   1, 4,  { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "irq" }, 6, 4 },
};

const struct sunxi_gpio_padconf sun50i_h6_padconf = {
	.npins = __arraycount(h6_pins),
	.pins = h6_pins,
};

const struct sunxi_gpio_padconf sun50i_h6_r_padconf = {
	.npins = __arraycount(h6_r_pins),
	.pins = h6_r_pins,
};
