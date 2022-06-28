/* $NetBSD: sun8i_v3s_gpio.c,v 1.1 2022/06/28 05:19:03 skrll Exp $ */

/*-
 * Copyright (c) 2021 Rui-Xiang Guo
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
__KERNEL_RCSID(0, "$NetBSD: sun8i_v3s_gpio.c,v 1.1 2022/06/28 05:19:03 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

static const struct sunxi_gpio_pins v3s_pins[] = {
	{"PB0",  1, 0,  {"gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "irq", NULL}, 6, 0},
	{"PB1",  1, 1,  {"gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "irq", NULL}, 6, 1},
	{"PB2",  1, 2,  {"gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "irq", NULL}, 6, 2},
	{"PB3",  1, 3,  {"gpio_in", "gpio_out", "uart2", NULL, NULL, NULL, "irq", NULL}, 6, 3},
	{"PB4",  1, 4,  {"gpio_in", "gpio_out", "pwm0", NULL, NULL, NULL, "irq", NULL}, 6, 4},
	{"PB5",  1, 5,  {"gpio_in", "gpio_out", "pwm1", NULL, NULL, NULL, "irq", NULL}, 6, 5},
	{"PB6",  1, 6,  {"gpio_in", "gpio_out", "i2c0", NULL, NULL, NULL, "irq", NULL}, 6, 6},
	{"PB7",  1, 7,  {"gpio_in", "gpio_out", "i2c0", NULL, NULL, NULL, "irq", NULL}, 6, 7},
	{"PB8",  1, 8,  {"gpio_in", "gpio_out", "i2c1", "uart0", NULL, NULL, "irq", NULL}, 6, 8},
	{"PB9",  1, 9,  {"gpio_in", "gpio_out", "i2c1", "uart0", NULL, NULL, "irq", NULL}, 6, 9},

	{"PC0",  2, 0,  {"gpio_in", "gpio_out", "mmc2", "spi0", NULL, NULL, NULL, NULL}},
	{"PC1",  2, 1,  {"gpio_in", "gpio_out", "mmc2", "spi0", NULL, NULL, NULL, NULL}},
	{"PC2",  2, 2,  {"gpio_in", "gpio_out", "mmc2", "spi0", NULL, NULL, NULL, NULL}},
	{"PC3",  2, 3,  {"gpio_in", "gpio_out", "mmc2", "spi0", NULL, NULL, NULL, NULL}},

	{"PE0",  4, 0,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE1",  4, 1,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE2",  4, 2,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE3",  4, 3,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE4",  4, 4,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE5",  4, 5,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE6",  4, 6,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE7",  4, 7,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE8",  4, 8,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE9",  4, 9,  {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE10", 4, 10, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE11", 4, 11, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE12", 4, 12, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE13", 4, 13, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE14", 4, 14, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE15", 4, 15, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE16", 4, 16, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE17", 4, 17, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE18", 4, 18, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE19", 4, 19, {"gpio_in", "gpio_out", "csi", "lcd", NULL, NULL, NULL, NULL}},
	{"PE20", 4, 20, {"gpio_in", "gpio_out", "csi", "csi_mipi", NULL, NULL, NULL, NULL}},
	{"PE21", 4, 21, {"gpio_in", "gpio_out", "csi", "i2c1", "uart1", NULL, NULL, NULL}},
	{"PE22", 4, 22, {"gpio_in", "gpio_out", "csi", "i2c1", "uart1", NULL, NULL, NULL}},
	{"PE23", 4, 23, {"gpio_in", "gpio_out", "lcd", "uart1", NULL, NULL, NULL, NULL}},
	{"PE24", 4, 24, {"gpio_in", "gpio_out", "lcd", "uart1", NULL, NULL, NULL, NULL}},

	{"PF0",  5, 0,  {"gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, NULL, NULL}},
	{"PF1",  5, 1,  {"gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, NULL, NULL}},
	{"PF2",  5, 2,  {"gpio_in", "gpio_out", "mmc0", "uart0", NULL, NULL, NULL, NULL}},
	{"PF3",  5, 3,  {"gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, NULL, NULL}},
	{"PF4",  5, 4,  {"gpio_in", "gpio_out", "mmc0", "uart0", NULL, NULL, NULL, NULL}},
	{"PF5",  5, 5,  {"gpio_in", "gpio_out", "mmc0", "jtag", NULL, NULL, NULL, NULL}},
	{"PF6",  5, 6,  {"gpio_in", "gpio_out", NULL, NULL, NULL, NULL, NULL, NULL}},

	{"PG0",  6, 0,  {"gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq", NULL}, 6, 0},
	{"PG1",  6, 1,  {"gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq", NULL}, 6, 1},
	{"PG2",  6, 2,  {"gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq", NULL}, 6, 2},
	{"PG3",  6, 3,  {"gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq", NULL}, 6, 3},
	{"PG4",  6, 4,  {"gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq", NULL}, 6, 4},
	{"PG5",  6, 5,  {"gpio_in", "gpio_out", "mmc1", NULL, NULL, NULL, "irq", NULL}, 6, 5},
};

const struct sunxi_gpio_padconf sun8i_v3s_padconf = {
	.npins = __arraycount(v3s_pins),
	.pins = v3s_pins,
};
