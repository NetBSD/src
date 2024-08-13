/* $NetBSD: sun20i_d1_gpio.c,v 1.1 2024/08/13 07:20:23 skrll Exp $ */

/*-
 * Copyright (c) 2024 Rui-Xiang Guo
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sun20i_d1_gpio.c,v 1.1 2024/08/13 07:20:23 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

static const struct sunxi_gpio_pins d1_pins[] = {
	{"PB0",  1,  0, {"gpio_in", "gpio_out", "pwm3", "ir", "i2c2", "spi1", "uart0", "uart2", "spdif", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 0},
	{"PB1",  1,  1, {"gpio_in", "gpio_out", "pwm4", "i2s2", "i2c2", "i2s2", "uart0", "uart2", "ir", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 1},
	{"PB2",  1,  2, {"gpio_in", "gpio_out", "lcd0", "i2s2", "i2c0", "i2s2", "lcd0", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 2},
	{"PB3",  1,  3, {"gpio_in", "gpio_out", "lcd0", "i2s2", "i2c0", "i2s2", "lcd0", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 3},
	{"PB4",  1,  4, {"gpio_in", "gpio_out", "lcd0", "i2s2", "i2c1", "i2s2", "lcd0", "uart5", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 4},
	{"PB5",  1,  5, {"gpio_in", "gpio_out", "lcd0", "i2s2", "i2c1", "pwm0", "lcd0", "uart5", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 5},
	{"PB6",  1,  6, {"gpio_in", "gpio_out", "lcd0", "i2s2", "i2c3", "pwm1", "lcd0", "uart3", "cpu", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 6},
	{"PB7",  1,  7, {"gpio_in", "gpio_out", "lcd0", "i2s2", "i2c3", "ir", "lcd0", "uart3", "cpu", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 7},
	{"PB8",  1,  8, {"gpio_in", "gpio_out", "dmic", "pwm5", "i2c2", "spi1", "uart0", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 8},
	{"PB9",  1,  9, {"gpio_in", "gpio_out", "dmic", "pwm6", "i2c2", "spi1", "uart0", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 9},
	{"PB10", 1, 10, {"gpio_in", "gpio_out", "dmic", "pwm7", "i2c0", "spi1", "clk", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 10},
	{"PB11", 1, 11, {"gpio_in", "gpio_out", "dmic", "pwm2", "i2c0", "spi1", "clk", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 11},
	{"PB12", 1, 12, {"gpio_in", "gpio_out", "dmic", "pwm0", "spdif", "spi1", "clk", "ir", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 12},

	{"PC0",  2,  0, {"gpio_in", "gpio_out", "uart2", "i2c2", "ledc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 0},
	{"PC1",  2,  1, {"gpio_in", "gpio_out", "uart2", "i2c2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 1},
	{"PC2",  2,  2, {"gpio_in", "gpio_out", "spi0", "mmc2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 2},
	{"PC3",  2,  3, {"gpio_in", "gpio_out", "spi0", "mmc2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 3},
	{"PC4",  2,  4, {"gpio_in", "gpio_out", "spi0", "mmc2", "boot", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 4},
	{"PC5",  2,  5, {"gpio_in", "gpio_out", "spi0", "mmc2", "boot", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 5},
	{"PC6",  2,  6, {"gpio_in", "gpio_out", "spi0", "mmc2", "uart3", "i2c3", "dbg", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 6},
	{"PC7",  2,  7, {"gpio_in", "gpio_out", "spi0", "mmc2", "uart3", "i2c3", "tcon", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 7},

	{"PD0",  3,  0, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "i2c0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 0},
	{"PD1",  3,  1, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 1},
	{"PD2",  3,  2, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 2},
	{"PD3",  3,  3, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 3},
	{"PD4",  3,  4, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 4},
	{"PD5",  3,  5, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 5},
	{"PD6",  3,  6, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 6},
	{"PD7",  3,  7, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 7},
	{"PD8",  3,  8, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 8},
	{"PD9",  3,  9, {"gpio_in", "gpio_out", "lcd0", "lvds0", "dsi", "pwm6", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 9},
	{"PD10", 3, 10, {"gpio_in", "gpio_out", "lcd0", "lvds1", "spi1", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 10},
	{"PD11", 3, 11, {"gpio_in", "gpio_out", "lcd0", "lvds1", "spi1", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 11},
	{"PD12", 3, 12, {"gpio_in", "gpio_out", "lcd0", "lvds1", "spi1", "i2c0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 12},
	{"PD13", 3, 13, {"gpio_in", "gpio_out", "lcd0", "lvds1", "spi1", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 13},
	{"PD14", 3, 14, {"gpio_in", "gpio_out", "lcd0", "lvds1", "spi1", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 14},
	{"PD15", 3, 15, {"gpio_in", "gpio_out", "lcd0", "lvds1", "spi1", "ir", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 15},
	{"PD16", 3, 16, {"gpio_in", "gpio_out", "lcd0", "lvds1", "dmic", "pwm0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 16},
	{"PD17", 3, 17, {"gpio_in", "gpio_out", "lcd0", "lvds1", "dmic", "pwm1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 17},
	{"PD18", 3, 18, {"gpio_in", "gpio_out", "lcd0", "lvds1", "dmic", "pwm2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 18},
	{"PD19", 3, 19, {"gpio_in", "gpio_out", "lcd0", "lvds1", "dmic", "pwm3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 19},
	{"PD20", 3, 20, {"gpio_in", "gpio_out", "lcd0", "i2c2", "dmic", "pwm4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 20},
	{"PD21", 3, 21, {"gpio_in", "gpio_out", "lcd0", "i2c2", "uart1", "pwm5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 21},
	{"PD22", 3, 22, {"gpio_in", "gpio_out", "spdif", "ir", "uart1", "pwm7", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 22},

	{"PE0",  4,  0, {"gpio_in", "gpio_out", "ncsi0", "uart2", "i2c1", "lcd0", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 0},
	{"PE1",  4,  1, {"gpio_in", "gpio_out", "ncsi0", "uart2", "i2c1", "lcd0", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 1},
	{"PE2",  4,  2, {"gpio_in", "gpio_out", "ncsi0", "uart2", "i2c0", "clk", "uart0", NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 2},
	{"PE3",  4,  3, {"gpio_in", "gpio_out", "ncsi0", "uart2", "i2c0", "clk", "uart0", NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 3},
	{"PE4",  4,  4, {"gpio_in", "gpio_out", "ncsi0", "uart4", "i2c2", "clk", "jtag", "jtag", "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 4},
	{"PE5",  4,  5, {"gpio_in", "gpio_out", "ncsi0", "uart4", "i2c2", "ledc", "jtag", "jtag", "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 5},
	{"PE6",  4,  6, {"gpio_in", "gpio_out", "ncsi0", "uart5", "i2c3", "spdif", "jtag", "jtag", "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 6},
	{"PE7",  4,  7, {"gpio_in", "gpio_out", "ncsi0", "uart5", "i2c3", "spdif", "jtag", "jtag", "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 7},
	{"PE8",  4,  8, {"gpio_in", "gpio_out", "ncsi0", "uart1", "pwm2", "uart3", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 8},
	{"PE9",  4,  9, {"gpio_in", "gpio_out", "ncsi0", "uart1", "pwm3", "uart3", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 9},
	{"PE10", 4, 10, {"gpio_in", "gpio_out", "ncsi0", "uart1", "pwm4", "ir", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 10},
	{"PE11", 4, 11, {"gpio_in", "gpio_out", "ncsi0", "uart1", "i2s0", "i2s0", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 11},
	{"PE12", 4, 12, {"gpio_in", "gpio_out", "i2c2", "ncsi0", "i2s0", "i2s0", NULL, NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 12},
	{"PE13", 4, 13, {"gpio_in", "gpio_out", "i2c2", "pwm5", "i2s0", "i2s0", "dmic", NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 13},
	{"PE14", 4, 14, {"gpio_in", "gpio_out", "i2c1", "jtag", "i2s0", "i2s0", "dmic", NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 14},
	{"PE15", 4, 15, {"gpio_in", "gpio_out", "i2c1", "jtag", "pwm6", "i2s0", "dmic", NULL, "emac", NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 15},
	{"PE16", 4, 16, {"gpio_in", "gpio_out", "i2c3", "jtag", "pwm7", "i2s0", "dmic", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 16},
	{"PE17", 4, 17, {"gpio_in", "gpio_out", "i2c3", "jtag", "ir", "i2s0", "dmic", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 17},

	{"PF0",  5,  0, {"gpio_in", "gpio_out", "mmc0", NULL, "jtag", "i2s2", "i2s2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 0},
	{"PF1",  5,  1, {"gpio_in", "gpio_out", "mmc0", NULL, "jtag", "i2s2", "i2s2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 1},
	{"PF2",  5,  2, {"gpio_in", "gpio_out", "mmc0", "uart0", "i2c0", "ledc", "spdif", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 2},
	{"PF3",  5,  3, {"gpio_in", "gpio_out", "mmc0", NULL, "jtag", "i2s2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 3},
	{"PF4",  5,  4, {"gpio_in", "gpio_out", "mmc0", "uart0", "i2c0", "pwm6", "ir", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 4},
	{"PF5",  5,  5, {"gpio_in", "gpio_out", "mmc0", NULL, "jtag", "i2s2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 5},
	{"PF6",  5,  6, {"gpio_in", "gpio_out", NULL, "spdif", "ir", "i2s2", "pwm5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 6},

	{"PG0",  6,  0, {"gpio_in", "gpio_out", "mmc1", "uart3", "emac", "pwm7", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 0},
	{"PG1",  6,  1, {"gpio_in", "gpio_out", "mmc1", "uart3", "emac", "pwm6", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 1},
	{"PG2",  6,  2, {"gpio_in", "gpio_out", "mmc1", "uart3", "emac", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 2},
	{"PG3",  6,  3, {"gpio_in", "gpio_out", "mmc1", "uart3", "emac", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 3},
	{"PG4",  6,  4, {"gpio_in", "gpio_out", "mmc1", "uart5", "emac", "pwm5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 4},
	{"PG5",  6,  5, {"gpio_in", "gpio_out", "mmc1", "uart5", "emac", "pwm4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 5},
	{"PG6",  6,  6, {"gpio_in", "gpio_out", "uart1", "i2c2", "emac", "pwm1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 6},
	{"PG7",  6,  7, {"gpio_in", "gpio_out", "uart1", "i2c2", "emac", "spdif", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 7},
	{"PG8",  6,  8, {"gpio_in", "gpio_out", "uart1", "i2c1", "emac", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 8},
	{"PG9",  6,  9, {"gpio_in", "gpio_out", "uart1", "i2c1", "emac", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 9},
	{"PG10", 6, 10, {"gpio_in", "gpio_out", "pwm3", "i2c3", "emac", "clk", "ir", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 10},
	{"PG11", 6, 11, {"gpio_in", "gpio_out", "i2s1", "i2c3", "emac", "clk", "tcon", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 11},
	{"PG12", 6, 12, {"gpio_in", "gpio_out", "i2s1", "i2c0", "emac", "clk", "pwm0", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 12},
	{"PG13", 6, 13, {"gpio_in", "gpio_out", "i2s1", "i2c0", "emac", "pwm2", "ledc", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 13},
	{"PG14", 6, 14, {"gpio_in", "gpio_out", "i2s1", "i2c2", "emac", "i2s1", "spi0", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 14},
	{"PG15", 6, 15, {"gpio_in", "gpio_out", "i2s1", "i2c2", "emac", "i2s1", "spi0", "uart1", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 15},
	{"PG16", 6, 16, {"gpio_in", "gpio_out", "ir", "tcon", "pwm5", "clk", "spdif", "ledc", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 16},
	{"PG17", 6, 17, {"gpio_in", "gpio_out", "uart2", "i2c3", "pwm7", "clk", "ir", "uart0", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 17},
	{"PG18", 6, 18, {"gpio_in", "gpio_out", "uart2", "i2c3", "pwm6", "clk", "spdif", "uart0", NULL, NULL, NULL, NULL, NULL, NULL, "irq"}, 14, 18},
};

const struct sunxi_gpio_padconf sun20i_d1_padconf = {
	.npins = __arraycount(d1_pins),
	.pins = d1_pins,
};
