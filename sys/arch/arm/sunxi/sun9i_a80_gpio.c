/* $NetBSD: sun9i_a80_gpio.c,v 1.1 2017/10/08 18:00:36 jmcneill Exp $ */

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
 *
 * $FreeBSD$
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sun9i_a80_gpio.c,v 1.1 2017/10/08 18:00:36 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

static const struct sunxi_gpio_pins a80_pins[] = {
	{ "PA0",  0, 0,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 0, 0 },
	{ "PA1",  0, 1,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 1, 0 },
	{ "PA2",  0, 2,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 2, 0 },
	{ "PA3",  0, 3,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 3, 0 },
	{ "PA4",  0, 4,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 4, 0 },
	{ "PA5",  0, 5,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 5, 0 },
	{ "PA6",  0, 6,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 6, 0 },
	{ "PA7",  0, 7,   { "gpio_in", "gpio_out", "gmac", NULL, "uart1", NULL, "eint" }, 6, 7, 0 },
	{ "PA8",  0, 8,   { "gpio_in", "gpio_out", "gmac", NULL, "eclk", NULL, "eint" }, 6, 8, 0 },
	{ "PA9",  0, 9,   { "gpio_in", "gpio_out", "gmac", NULL, "eclk", NULL, "eint" }, 6, 9, 0 },
	{ "PA10", 0, 10,  { "gpio_in", "gpio_out", "gmac", NULL, "clk_out_a", NULL, "eint" }, 6, 10, 0 },
	{ "PA11", 0, 11,  { "gpio_in", "gpio_out", "gmac", NULL, "clk_out_b", NULL, "eint" }, 6, 11, 0 },
	{ "PA12", 0, 12,  { "gpio_in", "gpio_out", "gmac", NULL, "pwm3", NULL, "eint" }, 6, 12, 0 },
	{ "PA13", 0, 13,  { "gpio_in", "gpio_out", "gmac", NULL, "pwm3", NULL, "eint" }, 6, 13, 0 },
	{ "PA14", 0, 14,  { "gpio_in", "gpio_out", "gmac", NULL, "spi1", NULL, "eint" }, 6, 14, 0 },
	{ "PA15", 0, 15,  { "gpio_in", "gpio_out", "gmac", NULL, "spi1", NULL, "eint" }, 6, 15, 0 },
	{ "PA16", 0, 16,  { "gpio_in", "gpio_out", "gmac", NULL, "spi1", NULL, "eint" }, 6, 16, 0 },
	{ "PA17", 0, 17,  { "gpio_in", "gpio_out", "gmac", NULL, "spi1", NULL, "eint" }, 6, 17, 0 },

	{ "PB5",  1, 5,   { "gpio_in", "gpio_out", NULL, "uart3", NULL, NULL, "eint" }, 6, 5, 1 },
	{ "PB6",  1, 6,   { "gpio_in", "gpio_out", NULL, "uart3", NULL, NULL, "eint" }, 6, 6, 1 },
	{ "PB14", 1, 14,  { "gpio_in", "gpio_out", NULL, "mcsi", NULL, NULL, "eint" }, 6, 14, 1 },
	{ "PB15", 1, 15,  { "gpio_in", "gpio_out", NULL, "mcsi", "i2c4", NULL, "eint" }, 6, 15, 1 },
	{ "PB16", 1, 16,  { "gpio_in", "gpio_out", NULL, "mcsi", "i2c4", NULL, "eint" }, 6, 16, 1 },

	{ "PC0",  2, 0,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
	{ "PC1",  2, 1,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
	{ "PC2",  2, 2,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
	{ "PC3",  2, 3,   { "gpio_in", "gpio_out", "nand0" } },
	{ "PC4",  2, 4,   { "gpio_in", "gpio_out", "nand0" } },
	{ "PC5",  2, 5,   { "gpio_in", "gpio_out", "nand0" } },
	{ "PC6",  2, 6,   { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC7",  2, 7,   { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC8",  2, 8,   { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC9",  2, 9,   { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC10", 2, 10,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC11", 2, 11,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC12", 2, 12,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC13", 2, 13,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC14", 2, 14,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC15", 2, 15,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC16", 2, 16,  { "gpio_in", "gpio_out", "nand0", "mmc2" } },
	{ "PC17", 2, 17,  { "gpio_in", "gpio_out", "nand0", "nand0_b" } },
	{ "PC18", 2, 18,  { "gpio_in", "gpio_out", "nand0", "nand0_b" } },
	{ "PC19", 2, 19,  { "gpio_in", "gpio_out", NULL, "spi0" } },

	{ "PD0",  3, 0,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD1",  3, 1,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD2",  3, 2,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD3",  3, 3,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD4",  3, 4,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD5",  3, 5,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD6",  3, 6,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD7",  3, 7,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD8",  3, 8,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD9",  3, 9,   { "gpio_in", "gpio_out", "lcd0", "lvds0" } },
	{ "PD10", 3, 10,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD11", 3, 11,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD12", 3, 12,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD13", 3, 13,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD14", 3, 14,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD15", 3, 15,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD16", 3, 16,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD17", 3, 17,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD18", 3, 18,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD19", 3, 19,  { "gpio_in", "gpio_out", "lcd0", "lvds1" } },
	{ "PD20", 3, 20,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD21", 3, 21,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD22", 3, 22,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD23", 3, 23,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD24", 3, 24,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD25", 3, 25,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD26", 3, 26,  { "gpio_in", "gpio_out", "lcd0" } },
	{ "PD27", 3, 27,  { "gpio_in", "gpio_out", "lcd0" } },

	{ "PE0",  4, 0,   { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 0, 2 },
	{ "PE1",  4, 1,   { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 1, 2 },
	{ "PE2",  4, 2,   { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 2, 2 },
	{ "PE3",  4, 3,   { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 3, 2 },
	{ "PE4",  4, 4,   { "gpio_in", "gpio_out", "csi", "spi2", "uart5", NULL, "eint" }, 6, 4, 2 },
	{ "PE5",  4, 5,   { "gpio_in", "gpio_out", "csi", "spi2", "uart5", NULL, "eint" }, 6, 5, 2 },
	{ "PE6",  4, 6,   { "gpio_in", "gpio_out", "csi", "spi2", "uart5", NULL, "eint" }, 6, 6, 2 },
	{ "PE7",  4, 7,   { "gpio_in", "gpio_out", "csi", "spi2", "uart5", NULL, "eint" }, 6, 7, 2 },
	{ "PE8",  4, 8,   { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 8, 2 },
	{ "PE9",  4, 9,   { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 9, 2 },
	{ "PE10", 4, 10,  { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 10, 2 },
	{ "PE11", 4, 11,  { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 11, 2 },
	{ "PE12", 4, 12,  { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 12, 2 },
	{ "PE13", 4, 13,  { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 13, 2 },
	{ "PE14", 4, 14,  { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 14, 2 },
	{ "PE15", 4, 15,  { "gpio_in", "gpio_out", "csi", "ts", NULL, NULL, "eint" }, 6, 15, 2 },
	{ "PE16", 4, 16,  { "gpio_in", "gpio_out", "csi", "i2c4", NULL, NULL, "eint" }, 6, 16, 2 },
	{ "PE17", 4, 17,  { "gpio_in", "gpio_out", "csi", "i2c4", NULL, NULL, "eint" }, 6, 17, 2 },

	{ "PF0",  5, 0,   { "gpio_in", "gpio_out", "mmc0" } },
	{ "PF1",  5, 1,   { "gpio_in", "gpio_out", "mmc0" } },
	{ "PF2",  5, 2,   { "gpio_in", "gpio_out", "mmc0", "uart0" } },
	{ "PF3",  5, 3,   { "gpio_in", "gpio_out", "mmc0" } },
	{ "PF4",  5, 4,   { "gpio_in", "gpio_out", "mmc0", "uart0" } },
	{ "PF5",  5, 5,   { "gpio_in", "gpio_out", "mmc0" } },

	{ "PG0",  6, 0,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "eint" }, 6, 0, 3 },
	{ "PG1",  6, 1,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "eint" }, 6, 1, 3 },
	{ "PG2",  6, 2,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "eint" }, 6, 2, 3 },
	{ "PG3",  6, 3,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "eint" }, 6, 3, 3 },
	{ "PG4",  6, 4,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "eint" }, 6, 4, 3 },
	{ "PG5",  6, 5,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "eint" }, 6, 5, 3 },
	{ "PG6",  6, 6,   { "gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "eint" }, 6, 6, 3 },
	{ "PG7",  6, 7,   { "gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "eint" }, 6, 7, 3 },
	{ "PG8",  6, 8,   { "gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "eint" }, 6, 8, 3 },
	{ "PG9",  6, 9,   { "gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "eint" }, 6, 9, 3 },
	{ "PG10", 6, 10,  { "gpio_in", "gpio_out", "i2c3", NULL, NULL, NULL, "eint" }, 6, 10, 3 },
	{ "PG11", 6, 11,  { "gpio_in", "gpio_out", "i2c3", NULL, NULL, NULL, "eint" }, 6, 11, 3 },
	{ "PG12", 6, 12,  { "gpio_in", "gpio_out", "uart4", NULL, NULL, NULL, "eint" }, 6, 12, 3 },
	{ "PG13", 6, 13,  { "gpio_in", "gpio_out", "uart4", NULL, NULL, NULL, "eint" }, 6, 13, 3 },
	{ "PG14", 6, 14,  { "gpio_in", "gpio_out", "uart4", NULL, NULL, NULL, "eint" }, 6, 14, 3 },
	{ "PG15", 6, 15,  { "gpio_in", "gpio_out", "uart4", NULL, NULL, NULL, "eint" }, 6, 15, 3 },

	{ "PH0",  7, 0,   { "gpio_in", "gpio_out", "i2c0" } },
	{ "PH1",  7, 1,   { "gpio_in", "gpio_out", "i2c0" } },
	{ "PH2",  7, 2,   { "gpio_in", "gpio_out", "i2c1" } },
	{ "PH3",  7, 3,   { "gpio_in", "gpio_out", "i2c1" } },
	{ "PH4",  7, 4,   { "gpio_in", "gpio_out", "i2c2" } },
	{ "PH5",  7, 5,   { "gpio_in", "gpio_out", "i2c2" } },
	{ "PH6",  7, 6,   { "gpio_in", "gpio_out", "pwm0" } },
	{ "PH8",  7, 8,   { "gpio_in", "gpio_out", NULL, "pwm1", NULL, NULL, "eint" }, 6, 8, 4 },
	{ "PH9",  7, 9,   { "gpio_in", "gpio_out", NULL, "pwm1", NULL, NULL, "eint" }, 6, 9, 4 },
	{ "PH10", 7, 10,  { "gpio_in", "gpio_out", NULL, "pwm2", NULL, NULL, "eint" }, 6, 10, 4 },
	{ "PH11", 7, 11,  { "gpio_in", "gpio_out", NULL, "pwm2", NULL, NULL, "eint" }, 6, 11, 4 },
	{ "PH12", 7, 12,  { "gpio_in", "gpio_out", "uart0", "spi3", NULL, NULL, "eint" }, 6, 12, 4 },
	{ "PH13", 7, 13,  { "gpio_in", "gpio_out", "uart0", "spi3", NULL, NULL, "eint" }, 6, 13, 4 },
	{ "PH14", 7, 14,  { "gpio_in", "gpio_out", "spi3", NULL, NULL, NULL, "eint" }, 6, 14, 4 },
	{ "PH15", 7, 15,  { "gpio_in", "gpio_out", "spi3", NULL, NULL, NULL, "eint" }, 6, 15, 4 },
	{ "PH16", 7, 16,  { "gpio_in", "gpio_out", "spi3", NULL, NULL, NULL, "eint" }, 6, 16, 4 },
	{ "PH17", 7, 17,  { "gpio_in", "gpio_out", "spi3", NULL, NULL, NULL, "eint" }, 6, 17, 4 },
	{ "PH18", 7, 18,  { "gpio_in", "gpio_out", "spi3", NULL, NULL, NULL, "eint" }, 6, 18, 4 },
	{ "PH19", 7, 19,  { "gpio_in", "gpio_out", "hdmi" } },
	{ "PH20", 7, 20,  { "gpio_in", "gpio_out", "hdmi" } },
	{ "PH21", 7, 21,  { "gpio_in", "gpio_out", "hdmi" } },
};

static const struct sunxi_gpio_pins a80_r_pins[] = {
	{ "PL0",  0, 0,   { "gpio_in", "gpio_out", NULL, "s_uart", NULL, NULL, "eint" }, 6, 0, 0 },
	{ "PL1",  0, 1,   { "gpio_in", "gpio_out", NULL, "s_uart", NULL, NULL, "eint" }, 6, 1, 0 },
	{ "PL2",  0, 2,   { "gpio_in", "gpio_out", NULL, "s_jtag", NULL, NULL, "eint" }, 6, 2, 0 },
	{ "PL3",  0, 3,   { "gpio_in", "gpio_out", NULL, "s_jtag", NULL, NULL, "eint" }, 6, 3, 0 },
	{ "PL4",  0, 4,   { "gpio_in", "gpio_out", NULL, "s_jtag", NULL, NULL, "eint" }, 6, 4, 0 },
	{ "PL5",  0, 5,   { "gpio_in", "gpio_out", NULL, "s_jtag", NULL, NULL, "eint" }, 6, 5, 0 },
	{ "PL6",  0, 6,   { "gpio_in", "gpio_out", NULL, "s_cir_rx", NULL, NULL, "eint" }, 6, 6, 0 },
	{ "PL7",  0, 7,   { "gpio_in", "gpio_out", NULL, "1wire", NULL, NULL, "eint" }, 6, 7, 0 },
	{ "PL8",  0, 8,   { "gpio_in", "gpio_out", "s_ps2", NULL, NULL, NULL, "eint" }, 6, 8, 0 },
	{ "PL9",  0, 9,   { "gpio_in", "gpio_out", "s_ps2", NULL, NULL, NULL, "eint" }, 6, 9, 0 },

	{ "PM0",  1, 0,   { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "eint" }, 6, 0, 1 },
	{ "PM1",  1, 1,   { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "eint" }, 6, 1, 1 },
	{ "PM2",  1, 2,   { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "eint" }, 6, 2, 1 },
	{ "PM3",  1, 3,   { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "eint" }, 6, 3, 1 },
	{ "PM4",  1, 4,   { "gpio_in", "gpio_out", NULL, "s_i2s1", NULL, NULL, "eint" }, 6, 4, 1 },
	{ "PM8",  1, 8,   { "gpio_in", "gpio_out", NULL, "s_i2c1", NULL, NULL, "eint" }, 6, 8, 1 },
	{ "PM9",  1, 9,   { "gpio_in", "gpio_out", NULL, "s_i2c1", NULL, NULL, "eint" }, 6, 9, 1 },
	{ "PM10", 1, 10,  { "gpio_in", "gpio_out", "s_i2s0", "s_i2s1" } },
	{ "PM11", 1, 11,  { "gpio_in", "gpio_out", "s_i2s0", "s_i2s1" } },
	{ "PM12", 1, 12,  { "gpio_in", "gpio_out", "s_i2s0", "s_i2s1" } },
	{ "PM13", 1, 13,  { "gpio_in", "gpio_out", "s_i2s0", "s_i2s1" } },
	{ "PM14", 1, 14,  { "gpio_in", "gpio_out", "s_i2s0", "s_i2s1" } },
	{ "PM15", 1, 15,  { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "eint" }, 6, 15, 1 },

	{ "PN0",  2, 0,   { "gpio_in", "gpio_out", "s_i2c0", "s_rsb" } },
	{ "PN1",  2, 1,   { "gpio_in", "gpio_out", "s_i2c0", "s_rsb" } },
};

const struct sunxi_gpio_padconf sun9i_a80_padconf = {
	.npins = __arraycount(a80_pins),
	.pins = a80_pins,
};

const struct sunxi_gpio_padconf sun9i_a80_r_padconf = {
	.npins = __arraycount(a80_r_pins),
	.pins = a80_r_pins,
};
