/* $NetBSD: sun5i_a13_gpio.c,v 1.3 2018/04/03 16:01:25 bouyer Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: sun5i_a13_gpio.c,v 1.3 2018/04/03 16:01:25 bouyer Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

static const struct sunxi_gpio_pins a13_pins[] = {
	{ "PB0",  1, 0,   { "gpio_in", "gpio_out", "i2c0" } },
	{ "PB1",  1, 1,   { "gpio_in", "gpio_out", "i2c0" } },
	{ "PB2",  1, 2,   { "gpio_in", "gpio_out", "pwm", NULL, NULL, NULL, "irq" }, 6, 16 },
	{ "PB3",  1, 3,   { "gpio_in", "gpio_out", "ir0", NULL, NULL, NULL, "irq" }, 6, 17 },
	{ "PB4",  1, 4,   { "gpio_in", "gpio_out", "ir0", NULL, NULL, NULL, "irq" }, 6, 18 },
	{ "PB10", 1, 10,  { "gpio_in", "gpio_out", "spi2", NULL, NULL, NULL, "irq" }, 6, 24 },
	{ "PB15", 1, 15,  { "gpio_in", "gpio_out", "i2c1" } },
	{ "PB16", 1, 16,  { "gpio_in", "gpio_out", "i2c1" } },
	{ "PB17", 1, 17,  { "gpio_in", "gpio_out", "i2c2" } },
	{ "PB18", 1, 18,  { "gpio_in", "gpio_out", "i2c2" } },

	{ "PC0",  2, 0,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
	{ "PC1",  2, 1,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
	{ "PC2",  2, 2,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
	{ "PC3",  2, 3,   { "gpio_in", "gpio_out", "nand0", "spi0" } },
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
	{ "PC19", 2, 19,  { "gpio_in", "gpio_out", "nand0" } },

	{ "PD2",  3, 2,   { "gpio_in", "gpio_out", "lcd0", "uart2" } },
	{ "PD3",  3, 3,   { "gpio_in", "gpio_out", "lcd0", "uart2" } },
	{ "PD4",  3, 4,   { "gpio_in", "gpio_out", "lcd0", "uart2" } },
	{ "PD5",  3, 5,   { "gpio_in", "gpio_out", "lcd0", "uart2" } },
	{ "PD6",  3, 6,   { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD7",  3, 7,   { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD10", 3, 10,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD11", 3, 11,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD12", 3, 12,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD13", 3, 13,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD14", 3, 14,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD15", 3, 15,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD18", 3, 18,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD19", 3, 19,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD20", 3, 20,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD21", 3, 21,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD22", 3, 22,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD23", 3, 23,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD24", 3, 24,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD25", 3, 25,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD26", 3, 26,  { "gpio_in", "gpio_out", "lcd0", "emac" } },
	{ "PD27", 3, 27,  { "gpio_in", "gpio_out", "lcd0", "emac" } },

	{ "PE0",  4, 0,   { "gpio_in", NULL, "ts0", "csi0", "spi2", NULL, "irq" }, 6, 14 },
	{ "PE1",  4, 1,   { "gpio_in", NULL, "ts0", "csi0", "spi2", NULL, "irq" }, 6, 15 },
	{ "PE2",  4, 2,   { "gpio_in", NULL, "ts0", "csi0", "spi2" } },
	{ "PE3",  4, 3,   { "gpio_in", "gpio_out", "ts0", "csi0", "spi2" } },
	{ "PE4",  4, 4,   { "gpio_in", "gpio_out", "ts0", "csi0", "mmc2" } },
	{ "PE5",  4, 5,   { "gpio_in", "gpio_out", "ts0", "csi0", "mmc2" } },
	{ "PE6",  4, 6,   { "gpio_in", "gpio_out", "ts0", "csi0", "mmc2" } },
	{ "PE7",  4, 7,   { "gpio_in", "gpio_out", "ts0", "csi0", "mmc2" } },
	{ "PE8",  4, 8,   { "gpio_in", "gpio_out", "ts0", "csi0", "mmc2" } },
	{ "PE9",  4, 9,   { "gpio_in", "gpio_out", "ts0", "csi0", "mmc2" } },
	{ "PE10", 4, 10,  { "gpio_in", "gpio_out", "ts0", "csi0", "uart1" } },
	{ "PE11", 4, 11,  { "gpio_in", "gpio_out", "ts0", "csi0", "uart1" } },

	{ "PF0",  5, 0,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF1",  5, 1,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF2",  5, 2,   { "gpio_in", "gpio_out", "mmc0", "uart0" } },
	{ "PF3",  5, 3,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },
	{ "PF4",  5, 4,   { "gpio_in", "gpio_out", "mmc0", "uart0" } },
	{ "PF5",  5, 5,   { "gpio_in", "gpio_out", "mmc0", "jtag" } },

	{ "PG0",  6, 0,   { "gpio_in", NULL, "gps", NULL, NULL, NULL, "irq" }, 6, 0 },
	{ "PG1",  6, 1,   { "gpio_in", NULL, "gps", NULL, NULL, NULL, "irq" }, 6, 1 },
	{ "PG2",  6, 2,   { "gpio_in", NULL, "gps", NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PG3",  6, 3,   { "gpio_in", "gpio_out", NULL, NULL, "uart1", NULL, "irq" }, 6, 3 },
	{ "PG4",  6, 4,   { "gpio_in", "gpio_out", NULL, NULL, "uart1", NULL, "irq" }, 6, 4 },
	{ "PG9",  6, 9,   { "gpio_in", "gpio_out", "spi1", "uart3", NULL, NULL, "irq" }, 6, 9 },
	{ "PG10", 6, 10,  { "gpio_in", "gpio_out", "spi1", "uart3", NULL, NULL, "irq" }, 6, 10 },
	{ "PG11", 6, 11,  { "gpio_in", "gpio_out", "spi1", "uart3", NULL, NULL, "irq" }, 6, 11 },
	{ "PG12", 6, 12,  { "gpio_in", "gpio_out", "spi1", "uart3", NULL, NULL, "irq" }, 6, 12 },
};

const struct sunxi_gpio_padconf sun5i_a13_padconf = {
	.npins = __arraycount(a13_pins),
	.pins = a13_pins,
};
