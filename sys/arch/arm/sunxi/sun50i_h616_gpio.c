/* $NetBSD$ */

/*-
 * Copyright (c) 2026
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
__KERNEL_RCSID(0, "$NetBSD$");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

/*
 * pin definitions derived from linux pinctrl-sun50i-h616.c
 * function index mapping: 0=gpio_in, 1=gpio_out, 2-5=mux functions, 6=irq
 */
static const struct sunxi_gpio_pins h616_pins[] = {
	/* port a: emac1, i2c, i2s */
	{ "PA0",  0, 0,   { "gpio_in", "gpio_out", "emac1", NULL, "i2c0", NULL, "irq" }, 6, 0 },
	{ "PA1",  0, 1,   { "gpio_in", "gpio_out", "emac1", NULL, "i2c0", NULL, "irq" }, 6, 1 },
	{ "PA2",  0, 2,   { "gpio_in", "gpio_out", "emac1", NULL, "i2c1", NULL, "irq" }, 6, 2 },
	{ "PA3",  0, 3,   { "gpio_in", "gpio_out", "emac1", NULL, "i2c1", NULL, "irq" }, 6, 3 },
	{ "PA4",  0, 4,   { "gpio_in", "gpio_out", "emac1", NULL, NULL, NULL, "irq" }, 6, 4 },
	{ "PA5",  0, 5,   { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 5 },
	{ "PA6",  0, 6,   { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 6 },
	{ "PA7",  0, 7,   { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 7 },
	{ "PA8",  0, 8,   { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 8 },
	{ "PA9",  0, 9,   { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 9 },
	{ "PA10", 0, 10,  { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 10 },
	{ "PA11", 0, 11,  { "gpio_in", "gpio_out", "emac1", "i2s0", NULL, NULL, "irq" }, 6, 11 },
	{ "PA12", 0, 12,  { "gpio_in", "gpio_out", "emac1", NULL, NULL, NULL, "irq" }, 6, 12 },

	/* port c: nand, mmc2, spi0 */
	{ "PC0",  2, 0,   { "gpio_in", "gpio_out", "nand", NULL, "spi0" } },
	{ "PC1",  2, 1,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC2",  2, 2,   { "gpio_in", "gpio_out", "nand", NULL, "spi0" } },
	{ "PC3",  2, 3,   { "gpio_in", "gpio_out", "nand", NULL, "spi0" } },
	{ "PC4",  2, 4,   { "gpio_in", "gpio_out", "nand", "mmc2", "spi0" } },
	{ "PC5",  2, 5,   { "gpio_in", "gpio_out", "nand", "mmc2", "spi0" } },
	{ "PC6",  2, 6,   { "gpio_in", "gpio_out", "nand", "mmc2", "spi0" } },
	{ "PC7",  2, 7,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC8",  2, 8,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC9",  2, 9,   { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC10", 2, 10,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC11", 2, 11,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC12", 2, 12,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC13", 2, 13,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC14", 2, 14,  { "gpio_in", "gpio_out", "nand", "mmc2" } },
	{ "PC15", 2, 15,  { "gpio_in", "gpio_out", "nand" } },
	{ "PC16", 2, 16,  { "gpio_in", "gpio_out", "nand" } },

	/* port f: mmc0, jtag, uart0 */
	{ "PF0",  5, 0,   { "gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, "irq" }, 6, 0 },
	{ "PF1",  5, 1,   { "gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, "irq" }, 6, 1 },
	{ "PF2",  5, 2,   { "gpio_in", "gpio_out", "mmc0", "uart0", NULL, NULL, "irq" }, 6, 2 },
	{ "PF3",  5, 3,   { "gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, "irq" }, 6, 3 },
	{ "PF4",  5, 4,   { "gpio_in", "gpio_out", "mmc0", "uart0", NULL, NULL, "irq" }, 6, 4 },
	{ "PF5",  5, 5,   { "gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, "irq" }, 6, 5 },
	{ "PF6",  5, 6,   { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, "irq" }, 6, 6 },

	/* port g: mmc1, uart, i2c */
	{ "PG0",  6, 0,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 0 },
	{ "PG1",  6, 1,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 1 },
	{ "PG2",  6, 2,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PG3",  6, 3,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 3 },
	{ "PG4",  6, 4,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 4 },
	{ "PG5",  6, 5,   { "gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq" }, 6, 5 },
	{ "PG6",  6, 6,   { "gpio_in", "gpio_out", "uart1", NULL, NULL, NULL, "irq" }, 6, 6 },
	{ "PG7",  6, 7,   { "gpio_in", "gpio_out", "uart1", NULL, NULL, NULL, "irq" }, 6, 7 },
	{ "PG8",  6, 8,   { "gpio_in", "gpio_out", "uart1", NULL, "sim0", NULL, "irq" }, 6, 8 },
	{ "PG9",  6, 9,   { "gpio_in", "gpio_out", "uart1", NULL, "sim0", NULL, "irq" }, 6, 9 },
	{ "PG10", 6, 10,  { "gpio_in", "gpio_out", "clock", "i2c3", NULL, NULL, "irq" }, 6, 10 },
	{ "PG11", 6, 11,  { "gpio_in", "gpio_out", NULL, "i2c3", NULL, NULL, "irq" }, 6, 11 },
	{ "PG12", 6, 12,  { "gpio_in", "gpio_out", "uart3", "i2c4", NULL, NULL, "irq" }, 6, 12 },
	{ "PG13", 6, 13,  { "gpio_in", "gpio_out", "uart3", "i2c4", NULL, NULL, "irq" }, 6, 13 },
	{ "PG14", 6, 14,  { "gpio_in", "gpio_out", "uart3", NULL, NULL, NULL, "irq" }, 6, 14 },
	{ "PG15", 6, 15,  { "gpio_in", "gpio_out", "uart3", NULL, NULL, NULL, "irq" }, 6, 15 },
	{ "PG16", 6, 16,  { "gpio_in", "gpio_out", "ir_rx", NULL, NULL, NULL, "irq" }, 6, 16 },
	{ "PG17", 6, 17,  { "gpio_in", "gpio_out", "uart4", NULL, NULL, NULL, "irq" }, 6, 17 },
	{ "PG18", 6, 18,  { "gpio_in", "gpio_out", "uart4", NULL, NULL, NULL, "irq" }, 6, 18 },
	{ "PG19", 6, 19,  { "gpio_in", "gpio_out", "pwm1", NULL, NULL, NULL, "irq" }, 6, 19 },

	/* port h: uart, i2c, spi1, hdmi */
	{ "PH0",  7, 0,   { "gpio_in", "gpio_out", "uart0", "pwm3", "i2c1", NULL, "irq" }, 6, 0 },
	{ "PH1",  7, 1,   { "gpio_in", "gpio_out", "uart0", "pwm4", "i2c1", NULL, "irq" }, 6, 1 },
	{ "PH2",  7, 2,   { "gpio_in", "gpio_out", "uart5", "pwm2", "i2c2", NULL, "irq" }, 6, 2 },
	{ "PH3",  7, 3,   { "gpio_in", "gpio_out", "uart5", NULL, "i2c2", NULL, "irq" }, 6, 3 },
	{ "PH4",  7, 4,   { "gpio_in", "gpio_out", "spdif", NULL, "i2c3", NULL, "irq" }, 6, 4 },
	{ "PH5",  7, 5,   { "gpio_in", "gpio_out", "spi1", "spdif", "i2c3", NULL, "irq" }, 6, 5 },
	{ "PH6",  7, 6,   { "gpio_in", "gpio_out", "spi1", NULL, "i2c4", NULL, "irq" }, 6, 6 },
	{ "PH7",  7, 7,   { "gpio_in", "gpio_out", "spi1", NULL, "i2c4", NULL, "irq" }, 6, 7 },
	{ "PH8",  7, 8,   { "gpio_in", "gpio_out", "spi1", NULL, NULL, NULL, "irq" }, 6, 8 },
	{ "PH9",  7, 9,   { "gpio_in", "gpio_out", "spi1", NULL, NULL, NULL, "irq" }, 6, 9 },
	{ "PH10", 7, 10,  { "gpio_in", "gpio_out", "ir_rx", NULL, NULL, NULL, "irq" }, 6, 10 },

	/* port i: emac0, uart, i2c, pwm, hdmi */
	{ "PI0",  8, 0,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 0 },
	{ "PI1",  8, 1,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 1 },
	{ "PI2",  8, 2,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PI3",  8, 3,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 3 },
	{ "PI4",  8, 4,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 4 },
	{ "PI5",  8, 5,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 5 },
	{ "PI6",  8, 6,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 6 },
	{ "PI7",  8, 7,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 7 },
	{ "PI8",  8, 8,   { "gpio_in", "gpio_out", "emac0", NULL, NULL, NULL, "irq" }, 6, 8 },
	{ "PI9",  8, 9,   { "gpio_in", "gpio_out", "emac0", NULL, "uart2", NULL, "irq" }, 6, 9 },
	{ "PI10", 8, 10,  { "gpio_in", "gpio_out", "emac0", NULL, "uart2", NULL, "irq" }, 6, 10 },
	{ "PI11", 8, 11,  { "gpio_in", "gpio_out", "emac0", NULL, "uart2", NULL, "irq" }, 6, 11 },
	{ "PI12", 8, 12,  { "gpio_in", "gpio_out", "emac0", NULL, "uart2", NULL, "irq" }, 6, 12 },
	{ "PI13", 8, 13,  { "gpio_in", "gpio_out", "emac0", "pwm5", "uart3", NULL, "irq" }, 6, 13 },
	{ "PI14", 8, 14,  { "gpio_in", "gpio_out", "emac0", "pwm0", "uart3", NULL, "irq" }, 6, 14 },
	{ "PI15", 8, 15,  { "gpio_in", "gpio_out", "emac0", "pwm1", "uart3", NULL, "irq" }, 6, 15 },
	{ "PI16", 8, 16,  { "gpio_in", "gpio_out", "emac0", NULL, "uart3", NULL, "irq" }, 6, 16 },
};

static const struct sunxi_gpio_pins h616_r_pins[] = {
	{ "PL0",  0, 0,   { "gpio_in", "gpio_out", "s_rsb", "s_i2c", NULL, NULL, "irq" }, 6, 0 },
	{ "PL1",  0, 1,   { "gpio_in", "gpio_out", "s_rsb", "s_i2c", NULL, NULL, "irq" }, 6, 1 },
};

const struct sunxi_gpio_padconf sun50i_h616_padconf = {
	.npins = __arraycount(h616_pins),
	.pins = h616_pins,
};

const struct sunxi_gpio_padconf sun50i_h616_r_padconf = {
	.npins = __arraycount(h616_r_pins),
	.pins = h616_r_pins,
};
