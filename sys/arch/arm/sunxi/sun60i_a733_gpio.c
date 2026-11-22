/* $NetBSD: sun60i_a733_gpio.c,v 1.1 2026/11/22 14:29:22 jmcneill Exp $ */

/*-
 * Copyright (c) 2026 Jared McNeill <jmcneill@invisible.ca>
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
__KERNEL_RCSID(0, "$NetBSD: sun60i_a733_gpio.c,v 1.1 2026/11/22 14:29:22 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/types.h>

#include <arm/sunxi/sunxi_gpio.h>

static const struct sunxi_gpio_pins a733_pins[] = {
	{ "PB0", 1, 0, { "gpio_in", "gpio_out", "uart2", "uart0", "spi2", "dsi", "lcd0", "jtag", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 0 },
	{ "PB1", 1, 1, { "gpio_in", "gpio_out", "uart2", "uart0", "spi2", NULL, "lcd0", "jtag", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 1 },
	{ "PB2", 1, 2, { "gpio_in", "gpio_out", "uart2", NULL, "spi2", "hdmi", "lcd0", "jtag", "twi0", NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 2 },
	{ "PB3", 1, 3, { "gpio_in", "gpio_out", "uart2", NULL, "spi2", "hdmi", "lcd0", "jtag", "twi0", NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 3 },
	{ "PB4", 1, 4, { "gpio_in", "gpio_out", "pwm0_0", "i2s0_mclk", "spi2", "hdmi", "lcd0", "trace", "twi1", NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 4 },
	{ "PB5", 1, 5, { "gpio_in", "gpio_out", NULL, "i2s0_bclk", "spi2", "pwm0_1", "lcd0", "trace", "twi1", NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 5 },
	{ "PB6", 1, 6, { "gpio_in", "gpio_out", "clk", "i2s0_lrck", "spi2", "pwm0_2", "pwm0_8", "trace", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 6 },
	{ "PB7", 1, 7, { "gpio_in", "gpio_out", "clk", "i2s0_dout0", "i2s0_din1", "pwm0_9", "owa0", "trace", "twi1", NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 7 },
	{ "PB8", 1, 8, { "gpio_in", "gpio_out", "clk", "i2s0_din0", "i2s0_dout1", "pwm1_0", "owa0", "trace", "twi1", NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 8 },
	{ "PB9", 1, 9, { "gpio_in", "gpio_out", "uart0", "i2s0_din2", "i2s0_dout2", "pwm1_1", "watchdog", "lcd0", "twi8", "twi0", "test", NULL, NULL, NULL, "irq" }, 0, 9 },
	{ "PB10", 1, 10, { "gpio_in", "gpio_out", "uart0", "i2s0_din3", "i2s0_dout3", "pwm1_2", "pll", "lcd0", "twi8", "twi0", "test", NULL, NULL, NULL, "irq" }, 0, 10 },

	{ "PC0", 2, 0, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 0 },
	{ "PC1", 2, 1, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 1 },
	{ "PC2", 2, 2, { "gpio_in", "gpio_out", "nand", NULL, NULL, "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 2 },
	{ "PC3", 2, 3, { "gpio_in", "gpio_out", "nand", NULL, NULL, "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 3 },
	{ "PC4", 2, 4, { "gpio_in", "gpio_out", "nand", NULL, NULL, "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 4 },
	{ "PC5", 2, 5, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 5 },
	{ "PC6", 2, 6, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 6 },
	{ "PC7", 2, 7, { "gpio_in", "gpio_out", "nand", NULL, NULL, "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 7 },
	{ "PC8", 2, 8, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 8 },
	{ "PC9", 2, 9, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 9 },
	{ "PC10", 2, 10, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 10 },
	{ "PC11", 2, 11, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 11 },
	{ "PC12", 2, 12, { "gpio_in", "gpio_out", "nand", NULL, NULL, "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 12 },
	{ "PC13", 2, 13, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 13 },
	{ "PC14", 2, 14, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 14 },
	{ "PC15", 2, 15, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 15 },
	{ "PC16", 2, 16, { "gpio_in", "gpio_out", "nand", "sdc2", "sdc3", "spi0", "spif0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 16 },

	{ "PD0", 3, 0, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_0", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 0 },
	{ "PD1", 3, 1, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 1 },
	{ "PD2", 3, 2, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_2", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 2 },
	{ "PD3", 3, 3, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_3", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 3 },
	{ "PD4", 3, 4, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_4", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 4 },
	{ "PD5", 3, 5, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_5", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 5 },
	{ "PD6", 3, 6, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_6", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 6 },
	{ "PD7", 3, 7, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_7", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 7 },
	{ "PD8", 3, 8, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_8", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 8 },
	{ "PD9", 3, 9, { "gpio_in", "gpio_out", "lcd0", "lvds0", "dsi0", "eink", NULL, "pwm0_9", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 9 },
	{ "PD10", 3, 10, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "pwm1_0", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 10 },
	{ "PD11", 3, 11, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "pwm1_1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 11 },
	{ "PD12", 3, 12, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "pwm1_2", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 12 },
	{ "PD13", 3, 13, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "pwm1_3", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 13 },
	{ "PD14", 3, 14, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 14 },
	{ "PD15", 3, 15, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 15 },
	{ "PD16", 3, 16, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "twi2", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 16 },
	{ "PD17", 3, 17, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "twi2", "uart3", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 17 },
	{ "PD18", 3, 18, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "spi1", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 18 },
	{ "PD19", 3, 19, { "gpio_in", "gpio_out", "lcd0", "lvds1", "dsi1", "eink", "pwm1_5", "uart4", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 2, 19 },
	{ "PD20", 3, 20, { "gpio_in", "gpio_out", "lcd0", "dsi", "twi0", "eink", "pcie", "uart4", "twi3", "pwm0_2", NULL, NULL, NULL, NULL, "irq" }, 2, 20 },
	{ "PD21", 3, 21, { "gpio_in", "gpio_out", "lcd0", NULL, "twi0", "eink", "pcie", "uart4", "twi3", "pwm0_3", NULL, NULL, NULL, NULL, "irq" }, 2, 21 },
	{ "PD22", 3, 22, { "gpio_in", "gpio_out", "pwm0_4", NULL, NULL, "eink", "pcie", "twi0", "twi2", "pwm1_4", NULL, NULL, NULL, NULL, "irq" }, 2, 22 },
	{ "PD23", 3, 23, { "gpio_in", "gpio_out", "pwm0_5", NULL, NULL, NULL, "pcie", "twi0", "twi2", "pwm1_5", NULL, NULL, NULL, NULL, "irq" }, 2, 23 },

	{ "PE0", 4, 0, { "gpio_in", "gpio_out", "mcsi0", "csi0", NULL, "spi3", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 0 },
	{ "PE1", 4, 1, { "gpio_in", "gpio_out", "twi2", "csi1", NULL, "spi3", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 1 },
	{ "PE2", 4, 2, { "gpio_in", "gpio_out", "twi2", "csi0", NULL, "spi3", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 2 },
	{ "PE3", 4, 3, { "gpio_in", "gpio_out", "twi3", "csi1", "pwm0_0", "spi3", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 3 },
	{ "PE4", 4, 4, { "gpio_in", "gpio_out", "twi3", "ledc", "pwm0_1", "spi3", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 4 },
	{ "PE5", 4, 5, { "gpio_in", "gpio_out", "mcsi1", "pll", NULL, "pwm0_2", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 5 },
	{ "PE6", 4, 6, { "gpio_in", "gpio_out", "clk", "hdmi", "i2s3_din0", "i2s3_dout1", "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 6 },
	{ "PE7", 4, 7, { "gpio_in", "gpio_out", "clk", "hdmi", "i2s3_bclk", NULL, "lpc", "ncsi0", "twi11", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 7 },
	{ "PE8", 4, 8, { "gpio_in", "gpio_out", "clk", "hdmi", "i2s3_lrck", NULL, "lpc", "ncsi0", "twi11", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 8 },
	{ "PE9", 4, 9, { "gpio_in", "gpio_out", "mcsi2", "tcon", "i2s3_mclk", NULL, "lpc", "ncsi0", "uart6", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 9 },
	{ "PE10", 4, 10, { "gpio_in", "gpio_out", "twi4", "tcon", "i2s3_din3", "i2s3_dout3", "lpc", "ncsi0", "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 10 },
	{ "PE11", 4, 11, { "gpio_in", "gpio_out", "twi4", "spi3", "i2s3_din2", "i2s3_dout2", "pcie", "ncsi0", "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 11 },
	{ "PE12", 4, 12, { "gpio_in", "gpio_out", NULL, "spi3", "i2s3_dout0", "i2s3_din1", "pcie", NULL, "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 12 },
	{ "PE13", 4, 13, { "gpio_in", "gpio_out", NULL, "spi3", NULL, NULL, "pcie", NULL, "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 13 },
	{ "PE14", 4, 14, { "gpio_in", "gpio_out", NULL, "spi3", NULL, "pwm1_8", "pcie", "ir", "twi9", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 14 },
	{ "PE15", 4, 15, { "gpio_in", "gpio_out", NULL, "spi3", NULL, "pwm1_9", "ir", NULL, "twi9", NULL, NULL, NULL, NULL, NULL, "irq" }, 3, 15 },

	{ "PF0", 5, 0, { "gpio_in", "gpio_out", "sdc0", "jtag", "trace", "uart5", "twi2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 0 },
	{ "PF1", 5, 1, { "gpio_in", "gpio_out", "sdc0", "jtag", "trace", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 1 },
	{ "PF2", 5, 2, { "gpio_in", "gpio_out", "sdc0", "uart0", "trace", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 2 },
	{ "PF3", 5, 3, { "gpio_in", "gpio_out", "sdc0", "jtag", "trace", "uart5", "twi2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 3 },
	{ "PF4", 5, 4, { "gpio_in", "gpio_out", "sdc0", "uart0", NULL, "uart5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 4 },
	{ "PF5", 5, 5, { "gpio_in", "gpio_out", "sdc0", "jtag", "trace", "uart5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 5 },
	{ "PF6", 5, 6, { "gpio_in", "gpio_out", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 4, 6 },

	{ "PG0", 6, 0, { "gpio_in", "gpio_out", "sdc1", "lcd0", "pwm1_1", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 0 },
	{ "PG1", 6, 1, { "gpio_in", "gpio_out", "sdc1", "lcd0", "pwm1_2", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 1 },
	{ "PG2", 6, 2, { "gpio_in", "gpio_out", "sdc1", "lcd0", "pwm1_3", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 2 },
	{ "PG3", 6, 3, { "gpio_in", "gpio_out", "sdc1", "lcd0", "pwm1_4", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 3 },
	{ "PG4", 6, 4, { "gpio_in", "gpio_out", "sdc1", "lcd0", "pwm1_5", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 4 },
	{ "PG5", 6, 5, { "gpio_in", "gpio_out", "sdc1", "lcd0", "pwm1_6", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 5 },
	{ "PG6", 6, 6, { "gpio_in", "gpio_out", "uart1", "uart4", "pwm1_7", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 6 },
	{ "PG7", 6, 7, { "gpio_in", "gpio_out", "uart1", "uart4", "pwm1_8", "lpc", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 7 },
	{ "PG8", 6, 8, { "gpio_in", "gpio_out", "uart1", "uart4", "uart5", "lpc", "twi0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 8 },
	{ "PG9", 6, 9, { "gpio_in", "gpio_out", "uart1", "uart4", "uart5", "lpc", "twi0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 9 },
	{ "PG10", 6, 10, { "gpio_in", "gpio_out", "i2s1_mclk", "clk", "pwm1_9", "lpc", "dmic", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 10 },
	{ "PG11", 6, 11, { "gpio_in", "gpio_out", "i2s1_bclk", "twi12", "twi9", "sgpio", "dmic", "hdmi", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 11 },
	{ "PG12", 6, 12, { "gpio_in", "gpio_out", "i2s1_lrck", "twi12", "twi9", "sgpio", "dmic", "hdmi", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 12 },
	{ "PG13", 6, 13, { "gpio_in", "gpio_out", "i2s1_dout0", "i2s1_din1", "twi6", "sgpio", "dmic", "hdmi", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 13 },
	{ "PG14", 6, 14, { "gpio_in", "gpio_out", "i2s1_din0", "i2s1_dout1", "twi6", "sgpio", "dmic", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 5, 14 },

	{ "PH0", 7, 0, { "gpio_in", "gpio_out", "twi0", "i2s2_dout2", "i2s2_din2", "rgmii0", "twi3", "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 0 },
	{ "PH1", 7, 1, { "gpio_in", "gpio_out", "twi0", "i2s2_dout3", "i2s2_din3", "rgmii0", "twi3", "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 1 },
	{ "PH2", 7, 2, { "gpio_in", "gpio_out", "twi1", NULL, "i2s2_mclk", "rgmii0", NULL, "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 2 },
	{ "PH3", 7, 3, { "gpio_in", "gpio_out", "twi1", NULL, "i2s2_bclk", "rgmii0", NULL, "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 3 },
	{ "PH4", 7, 4, { "gpio_in", "gpio_out", "uart3", "spi2", "i2s2_lrck", "rgmii0", NULL, "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 4 },
	{ "PH5", 7, 5, { "gpio_in", "gpio_out", "uart3", "i2s2_din1", "i2s2_dout0", "rgmii0", NULL, "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 5 },
	{ "PH6", 7, 6, { "gpio_in", "gpio_out", "uart3", "i2s2_dout1", "i2s2_din0", "rgmii0", NULL, "spi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 6 },
	{ "PH7", 7, 7, { "gpio_in", "gpio_out", "uart3", "spi2", "dsi", "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 7 },
	{ "PH8", 7, 8, { "gpio_in", "gpio_out", "dmic", "spi2", NULL, "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 8 },
	{ "PH9", 7, 9, { "gpio_in", "gpio_out", "dmic", "spi2", NULL, "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 9 },
	{ "PH10", 7, 10, { "gpio_in", "gpio_out", "dmic", "spi2", NULL, "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 10 },
	{ "PH11", 7, 11, { "gpio_in", "gpio_out", "dmic", "spi2", NULL, "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 11 },
	{ "PH12", 7, 12, { "gpio_in", "gpio_out", "dmic", "spi2", NULL, "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 12 },
	{ "PH13", 7, 13, { "gpio_in", "gpio_out", "uart5", NULL, NULL, "rgmii0", "twi7", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 13 },
	{ "PH14", 7, 14, { "gpio_in", "gpio_out", "uart5", NULL, NULL, "rgmii0", "twi7", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 14 },
	{ "PH15", 7, 15, { "gpio_in", "gpio_out", "uart5", "ledc", "dsi", "rgmii0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 15 },
	{ "PH16", 7, 16, { "gpio_in", "gpio_out", "uart5", NULL, "spi2", "ir", "twi5", "twi6", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 16 },
	{ "PH17", 7, 17, { "gpio_in", "gpio_out", NULL, NULL, "spi2", "ir", "twi5", "twi6", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 17 },
	{ "PH18", 7, 18, { "gpio_in", "gpio_out", NULL, NULL, "spi2", "pwm1_8", NULL, "twi7", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 18 },
	{ "PH19", 7, 19, { "gpio_in", "gpio_out", NULL, NULL, "spi2", "pwm1_9", NULL, "twi7", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 6, 19 },

	{ "PI0", 8, 0, { "gpio_in", "gpio_out", NULL, "uart4", NULL, "pwm0_1", "twi3", "twi4", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 0 },
	{ "PI1", 8, 1, { "gpio_in", "gpio_out", NULL, "uart4", NULL, "pwm0_2", "twi3", "twi4", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 1 },
	{ "PI2", 8, 2, { "gpio_in", "gpio_out", "uart6", NULL, "spi4", "pwm0_3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 2 },
	{ "PI3", 8, 3, { "gpio_in", "gpio_out", "uart6", NULL, "spi4", "pwm0_4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 3 },
	{ "PI4", 8, 4, { "gpio_in", "gpio_out", "uart6", NULL, "spi4", "pwm0_5", "twi11", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 4 },
	{ "PI5", 8, 5, { "gpio_in", "gpio_out", "uart6", NULL, "spi4", "pwm0_6", "twi11", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 5 },
	{ "PI6", 8, 6, { "gpio_in", "gpio_out", "uart6", "uart0", "spi4", "pwm0_7", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 6 },
	{ "PI7", 8, 7, { "gpio_in", "gpio_out", "uart6", "uart0", "spi4", "pwm0_8", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 7 },
	{ "PI8", 8, 8, { "gpio_in", "gpio_out", "uart6", "uart5", "ir", "pwm0_9", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 8 },
	{ "PI9", 8, 9, { "gpio_in", "gpio_out", "uart6", "uart5", "spi4", "pwm1_0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 9 },
	{ "PI10", 8, 10, { "gpio_in", "gpio_out", "owa0", NULL, "pcie", "pwm1_1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 10 },
	{ "PI11", 8, 11, { "gpio_in", "gpio_out", "owa0", "uart5", "uart3", "pwm1_2", "twi5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 11 },
	{ "PI12", 8, 12, { "gpio_in", "gpio_out", "ir", "uart5", "uart3", "pwm1_3", "twi5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 12 },
	{ "PI13", 8, 13, { "gpio_in", "gpio_out", "uart2", "uart3", "pcie", "pwm1_4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 13 },
	{ "PI14", 8, 14, { "gpio_in", "gpio_out", "uart2", "uart3", "pcie", "pwm1_5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 14 },
	{ "PI15", 8, 15, { "gpio_in", "gpio_out", "uart3", "uart1", NULL, "pwm1_6", "twi8", "twi2", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 15 },
	{ "PI16", 8, 16, { "gpio_in", "gpio_out", "uart3", "uart1", NULL, "pwm1_7", "twi8", "twi2", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 7, 16 },

	{ "PJ0", 9, 0, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 0 },
	{ "PJ1", 9, 1, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 1 },
	{ "PJ2", 9, 2, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 2 },
	{ "PJ3", 9, 3, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 3 },
	{ "PJ4", 9, 4, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 4 },
	{ "PJ5", 9, 5, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 5 },
	{ "PJ6", 9, 6, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 6 },
	{ "PJ7", 9, 7, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 7 },
	{ "PJ8", 9, 8, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 8 },
	{ "PJ9", 9, 9, { "gpio_in", "gpio_out", "lcd1", "lvds2", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 9 },
	{ "PJ10", 9, 10, { "gpio_in", "gpio_out", "lcd1", "lvds3", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 10 },
	{ "PJ11", 9, 11, { "gpio_in", "gpio_out", "lcd1", "lvds3", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 11 },
	{ "PJ12", 9, 12, { "gpio_in", "gpio_out", "lcd1", "lvds3", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 12 },
	{ "PJ13", 9, 13, { "gpio_in", "gpio_out", "lcd1", "lvds3", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 13 },
	{ "PJ14", 9, 14, { "gpio_in", "gpio_out", "lcd1", "lvds3", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 14 },
	{ "PJ15", 9, 15, { "gpio_in", "gpio_out", "lcd1", "lvds3", NULL, "rgmii1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 15 },
	{ "PJ16", 9, 16, { "gpio_in", "gpio_out", "lcd1", "lvds3", "uart5", "i2s4_bclk", NULL, "twi7", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 16 },
	{ "PJ17", 9, 17, { "gpio_in", "gpio_out", "lcd1", "lvds3", "uart5", "i2s4_mclk", NULL, "twi7", "spi3", NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 17 },
	{ "PJ18", 9, 18, { "gpio_in", "gpio_out", "lcd1", "lvds3", "uart5", "i2s4_lrck", "pwm0_0", NULL, "twi4", NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 18 },
	{ "PJ19", 9, 19, { "gpio_in", "gpio_out", "lcd1", "lvds3", "uart5", "i2s4_din0", "i2s4_dout1", NULL, "twi4", NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 19 },
	{ "PJ20", 9, 20, { "gpio_in", "gpio_out", "lcd1", NULL, "uart3", "i2s4_dout0", "i2s4_din1", "twi12", "twi10", "lcd0", NULL, NULL, NULL, NULL, "irq" }, 8, 20 },
	{ "PJ21", 9, 21, { "gpio_in", "gpio_out", "lcd1", "pwm1_3", "uart3", NULL, "spi3", "twi12", "twi10", "lcd0", NULL, NULL, NULL, NULL, "irq" }, 8, 21 },
	{ "PJ22", 9, 22, { "gpio_in", "gpio_out", "lcd1", "pwm1_4", "uart3", "uart2", "twi7", "twi3", "twi11", "lcd0", NULL, NULL, NULL, NULL, "irq" }, 8, 22 },
	{ "PJ23", 9, 23, { "gpio_in", "gpio_out", "lcd1", "pwm1_5", "uart3", "uart2", "twi7", "twi3", "twi11", "lcd0", NULL, NULL, NULL, NULL, "irq" }, 8, 23 },
	{ "PJ24", 9, 24, { "gpio_in", "gpio_out", "lcd1", "pwm1_6", "uart4", NULL, NULL, "twi4", "spi3", NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 24 },
	{ "PJ25", 9, 25, { "gpio_in", "gpio_out", "lcd1", "pwm1_7", "uart4", NULL, NULL, "twi4", "spi3", NULL, NULL, NULL, NULL, NULL, "irq" }, 8, 25 },
	{ "PJ26", 9, 26, { "gpio_in", "gpio_out", "lcd1", "pwm1_8", "uart4", "uart2", NULL, "twi5", "spi3", "lcd0", NULL, NULL, NULL, NULL, "irq" }, 8, 26 },
	{ "PJ27", 9, 27, { "gpio_in", "gpio_out", "lcd1", "pwm1_9", "uart4", "uart2", "dsi", "twi5", "spi3", "lcd0", NULL, NULL, NULL, NULL, "irq" }, 8, 27 },

	{ "PK0", 10, 0, { "gpio_in", "gpio_out", "mcsia", "uart6", "i2s4_bclk", "hdmi", "twi1", "ncsi1", "sgpio", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 0 },
	{ "PK1", 10, 1, { "gpio_in", "gpio_out", "mcsia", "uart6", "i2s4_mclk", "hdmi", "twi1", "ncsi1", "sgpio", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 1 },
	{ "PK2", 10, 2, { "gpio_in", "gpio_out", "mcsia", "uart6", "i2s4_lrck", "hdmi", "twi5", "ncsi1", "sgpio", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 2 },
	{ "PK3", 10, 3, { "gpio_in", "gpio_out", "mcsia", "uart6", "i2s4_din0", "i2s4_dout1", "twi5", "ncsi1", "sgpio", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 3 },
	{ "PK4", 10, 4, { "gpio_in", "gpio_out", "mcsia", "pcie", "i2s4_dout0", "i2s4_din1", "spi3", "ncsi1", "jtag", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 4 },
	{ "PK5", 10, 5, { "gpio_in", "gpio_out", "mcsia", "pcie", "pwm1_8", "pwm1_9", "spi3", "ncsi1", "jtag", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 5 },
	{ "PK6", 10, 6, { "gpio_in", "gpio_out", "mcsia", "twi2", "uart4", "uart2", "spi3", "ncsi1", "jtag", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 6 },
	{ "PK7", 10, 7, { "gpio_in", "gpio_out", "mcsia", "twi2", "uart4", "uart2", "spi3", "ncsi1", "jtag", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 7 },
	{ "PK8", 10, 8, { "gpio_in", "gpio_out", "mcsia", "mcsi0", "uart4", "uart2", "spi3", "ncsi1", "jtag", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 8 },
	{ "PK9", 10, 9, { "gpio_in", "gpio_out", "mcsia", "mcsi2", "uart4", "uart2", NULL, "ncsi1", "jtag", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 9 },
	{ "PK10", 10, 10, { "gpio_in", "gpio_out", "mcsib", "uart6", NULL, "pwm0_3", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 10 },
	{ "PK11", 10, 11, { "gpio_in", "gpio_out", "mcsib", "uart6", NULL, "pwm0_4", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 11 },
	{ "PK12", 10, 12, { "gpio_in", "gpio_out", "mcsib", "uart6", NULL, "pwm0_5", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 12 },
	{ "PK13", 10, 13, { "gpio_in", "gpio_out", "mcsib", "uart6", NULL, "pwm0_6", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 13 },
	{ "PK14", 10, 14, { "gpio_in", "gpio_out", "mcsib", "pcie", NULL, "pwm0_7", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 14 },
	{ "PK15", 10, 15, { "gpio_in", "gpio_out", "mcsib", NULL, NULL, "pwm0_8", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 15 },
	{ "PK16", 10, 16, { "gpio_in", "gpio_out", "mcsib", "twi3", "uart2", "pwm0_9", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 16 },
	{ "PK17", 10, 17, { "gpio_in", "gpio_out", "mcsib", "twi3", "uart2", "pwm1_0", NULL, "ncsi1", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 17 },
	{ "PK18", 10, 18, { "gpio_in", "gpio_out", "mcsib", "mcsi1", "uart2", "pwm1_1", NULL, "ncsi1", "twi9", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 18 },
	{ "PK19", 10, 19, { "gpio_in", "gpio_out", "mcsib", "mcsi2", "uart2", "pwm1_2", NULL, "ncsi1", "twi9", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 19 },
	{ "PK20", 10, 20, { "gpio_in", "gpio_out", "mcsic", "twi2", "uart3", "pwm0_1", NULL, "ncsi1", "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 20 },
	{ "PK21", 10, 21, { "gpio_in", "gpio_out", "mcsic", "twi2", "uart3", "pwm0_2", NULL, "ncsi1", "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 21 },
	{ "PK22", 10, 22, { "gpio_in", "gpio_out", "mcsic", "twi3", "uart3", "pwm1_6", NULL, "ncsi1", "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 22 },
	{ "PK23", 10, 23, { "gpio_in", "gpio_out", "mcsic", "twi3", "uart3", "pwm1_7", NULL, "ncsi1", "uart1", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 23 },
	{ "PK24", 10, 24, { "gpio_in", "gpio_out", "mcsic", "mcsi0", "pwm0_6", "twi12", NULL, NULL, "twi10", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 24 },
	{ "PK25", 10, 25, { "gpio_in", "gpio_out", "mcsic", "mcsi1", "pwm0_7", "twi12", NULL, NULL, "twi10", NULL, NULL, NULL, NULL, NULL, "irq" }, 9, 25 },
};

static const struct sunxi_gpio_pins a733_r_pins[] = {
	{ "PL0", 0, 0, { "gpio_in", "gpio_out", "s_twi0", "s_twi1", "s_twi2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 0 },
	{ "PL1", 0, 1, { "gpio_in", "gpio_out", "s_twi0", "s_twi1", "s_twi2", "s_ir_rx", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 1 },
	{ "PL2", 0, 2, { "gpio_in", "gpio_out", "s_uart1", "s_uart0", "s_twi1", NULL, "s_pwm0_0", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 2 },
	{ "PL3", 0, 3, { "gpio_in", "gpio_out", "s_uart1", "s_uart0", "s_twi1", "s_ir_rx", "s_pwm0_1", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 3 },
	{ "PL4", 0, 4, { "gpio_in", "gpio_out", "s_jtag_ms", "s_twi2", "s_spi0", "s_ir_rx", "s_pwm0_2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 4 },
	{ "PL5", 0, 5, { "gpio_in", "gpio_out", "s_jtag_ck", "s_twi2", "s_spi0", NULL, "s_pwm0_3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 5 },
	{ "PL6", 0, 6, { "gpio_in", "gpio_out", "s_jtag_do", "s_uart0", "s_spi0", "s_ir_rx", "s_pwm0_4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 6 },
	{ "PL7", 0, 7, { "gpio_in", "gpio_out", "s_jtag_di", "s_uart0", "s_spi0", NULL, "s_pwm0_5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 7 },
	{ "PL8", 0, 8, { "gpio_in", "gpio_out", "s_twi1", "s_uart1", "s_twi0", "s_twi2", "s_pwm0_6", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 8 },
	{ "PL9", 0, 9, { "gpio_in", "gpio_out", "s_twi1", "s_uart1", "s_twi0", "s_twi2", "s_pwm0_7", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 9 },
	{ "PL10", 0, 10, { "gpio_in", "gpio_out", "s_uart0", "s_twi2", "s_uart1", NULL, "s_pwm0_8", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 10 },
	{ "PL11", 0, 11, { "gpio_in", "gpio_out", "s_uart0", "s_twi2", "s_uart1", "s_ir_rx", "s_pwm0_9", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 11 },
	{ "PL12", 0, 12, { "gpio_in", "gpio_out", "s_uart1", "s_twi1", "s_twi2", "s_ir_rx", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 12 },
	{ "PL13", 0, 13, { "gpio_in", "gpio_out", "s_uart1", "s_twi1", "s_twi2", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 0, 13 },

	{ "PM0", 1, 0, { "gpio_in", "gpio_out", "s_jtag_ms", "s_spi0", "s_rjtag_ms", "s_twi1", "s_pwm0_2", "s_ir_rx", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 0 },
	{ "PM1", 1, 1, { "gpio_in", "gpio_out", "s_jtag_ck", "s_spi0", "s_rjtag_ck", "s_twi1", "s_pwm0_3", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 1 },
	{ "PM2", 1, 2, { "gpio_in", "gpio_out", "s_jtag_do", "s_spi0", "s_uart0", "s_uart1", "s_pwm0_4", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 2 },
	{ "PM3", 1, 3, { "gpio_in", "gpio_out", "s_jtag_di", "s_spi0", "s_uart0", "s_uart1", "s_pwm0_5", NULL, NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 3 },
	{ "PM4", 1, 4, { "gpio_in", "gpio_out", "s_uart0", "s_twi2", "s_twi1", "s_uart1", "s_pwm0_0", "s_ir_rx", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 4 },
	{ "PM5", 1, 5, { "gpio_in", "gpio_out", "s_uart0", "s_twi2", "s_twi1", "s_uart1", "s_pwm0_1", "s_ir_rx", NULL, NULL, NULL, NULL, NULL, NULL, "irq" }, 1, 5 },
};

const struct sunxi_gpio_padconf sun60i_a733_padconf = {
	.npins = __arraycount(a733_pins),
	.pins = a733_pins,
};

const struct sunxi_gpio_padconf sun60i_a733_r_padconf = {
	.npins = __arraycount(a733_r_pins),
	.pins = a733_r_pins,
};
