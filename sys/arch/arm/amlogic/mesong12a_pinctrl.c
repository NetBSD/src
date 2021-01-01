/* $NetBSD: mesong12a_pinctrl.c,v 1.1 2021/01/01 07:21:58 ryo Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mesong12a_pinctrl.c,v 1.1 2021/01/01 07:21:58 ryo Exp $");

#include <sys/param.h>
#include <sys/types.h>
#include <sys/bus.h>
#include <sys/device.h>

#include <arm/amlogic/meson_pinctrl.h>

/* CBUS pinmux registers */
#define	CBUS_REG(n)	((n) << 2)

/*
 * GPIO banks.
 * The values must match those in dt-bindings/gpio/meson-g12a-gpio.h
 */
enum {
	GPIOZ_0 = 0,
	GPIOZ_1,
	GPIOZ_2,
	GPIOZ_3,
	GPIOZ_4,
	GPIOZ_5,
	GPIOZ_6,
	GPIOZ_7,
	GPIOZ_8,
	GPIOZ_9,
	GPIOZ_10,
	GPIOZ_11,
	GPIOZ_12,
	GPIOZ_13,
	GPIOZ_14,
	GPIOZ_15,

	GPIOH_0 = 16,
	GPIOH_1,
	GPIOH_2,
	GPIOH_3,
	GPIOH_4,
	GPIOH_5,
	GPIOH_6,
	GPIOH_7,
	GPIOH_8,

	BOOT_0 = 25,
	BOOT_1,
	BOOT_2,
	BOOT_3,
	BOOT_4,
	BOOT_5,
	BOOT_6,
	BOOT_7,
	BOOT_8,
	BOOT_9,
	BOOT_10,
	BOOT_11,
	BOOT_12,
	BOOT_13,
	BOOT_14,
	BOOT_15,

	GPIOC_0 = 41,
	GPIOC_1,
	GPIOC_2,
	GPIOC_3,
	GPIOC_4,
	GPIOC_5,
	GPIOC_6,
	GPIOC_7,

	GPIOA_0 = 49,
	GPIOA_1,
	GPIOA_2,
	GPIOA_3,
	GPIOA_4,
	GPIOA_5,
	GPIOA_6,
	GPIOA_7,
	GPIOA_8,
	GPIOA_9,
	GPIOA_10,
	GPIOA_11,
	GPIOA_12,
	GPIOA_13,
	GPIOA_14,
	GPIOA_15,

	GPIOX_0 = 65,
	GPIOX_1,
	GPIOX_2,
	GPIOX_3,
	GPIOX_4,
	GPIOX_5,
	GPIOX_6,
	GPIOX_7,
	GPIOX_8,
	GPIOX_9,
	GPIOX_10,
	GPIOX_11,
	GPIOX_12,
	GPIOX_13,
	GPIOX_14,
	GPIOX_15,
	GPIOX_16,
	GPIOX_17,
	GPIOX_18,
	GPIOX_19,

	GPIOAO_0 = 0,
	GPIOAO_1,
	GPIOAO_2,
	GPIOAO_3,
	GPIOAO_4,
	GPIOAO_5,
	GPIOAO_6,
	GPIOAO_7,
	GPIOAO_8,
	GPIOAO_9,
	GPIOAO_10,
	GPIOAO_11,
	GPIOE_0 = 12,
	GPIOE_1,
	GPIOE_2,
};

#define	CBUS_GPIO(_id, _off, _bit)					\
	[_id] = {							\
		.id = (_id),						\
		.name = __STRING(_id),					\
		.oen = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) * 3 + 0),		\
			.mask = __BIT(_bit)				\
		},							\
		.out = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) * 3 + 1),		\
			.mask = __BIT(_bit)				\
		},							\
		.in = {							\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) * 3 + 2),		\
			.mask = __BIT(_bit)				\
		},							\
		.pupden = {						\
			.type = MESON_PINCTRL_REGTYPE_PULL_ENABLE,	\
			.reg = CBUS_REG(_off),				\
			.mask = __BIT(_bit)				\
		},							\
		.pupd = {						\
			.type = MESON_PINCTRL_REGTYPE_PULL,		\
			.reg = CBUS_REG(_off),				\
			.mask = __BIT(_bit)				\
		},							\
	}

static const struct meson_pinctrl_gpio mesong12a_periphs_gpios[] = {
	/* BOOT */
	CBUS_GPIO(BOOT_0,   0, 0),
	CBUS_GPIO(BOOT_1,   0, 1),
	CBUS_GPIO(BOOT_2,   0, 2),
	CBUS_GPIO(BOOT_3,   0, 3),
	CBUS_GPIO(BOOT_4,   0, 4),
	CBUS_GPIO(BOOT_5,   0, 5),
	CBUS_GPIO(BOOT_6,   0, 6),
	CBUS_GPIO(BOOT_7,   0, 7),
	CBUS_GPIO(BOOT_8,   0, 8),
	CBUS_GPIO(BOOT_9,   0, 9),
	CBUS_GPIO(BOOT_10,  0, 10),
	CBUS_GPIO(BOOT_11,  0, 11),
	CBUS_GPIO(BOOT_12,  0, 12),
	CBUS_GPIO(BOOT_13,  0, 13),
	CBUS_GPIO(BOOT_14,  0, 14),
	CBUS_GPIO(BOOT_15,  0, 15),

	/* GPIOC */
	CBUS_GPIO(GPIOC_0,  1, 0),
	CBUS_GPIO(GPIOC_1,  1, 1),
	CBUS_GPIO(GPIOC_2,  1, 2),
	CBUS_GPIO(GPIOC_3,  1, 3),
	CBUS_GPIO(GPIOC_4,  1, 4),
	CBUS_GPIO(GPIOC_5,  1, 5),
	CBUS_GPIO(GPIOC_6,  1, 6),
	CBUS_GPIO(GPIOC_7,  1, 7),

	/* GPIOX */
	CBUS_GPIO(GPIOX_0,  2, 0),
	CBUS_GPIO(GPIOX_1,  2, 1),
	CBUS_GPIO(GPIOX_2,  2, 2),
	CBUS_GPIO(GPIOX_3,  2, 3),
	CBUS_GPIO(GPIOX_4,  2, 4),
	CBUS_GPIO(GPIOX_5,  2, 5),
	CBUS_GPIO(GPIOX_6,  2, 6),
	CBUS_GPIO(GPIOX_7,  2, 7),
	CBUS_GPIO(GPIOX_8,  2, 8),
	CBUS_GPIO(GPIOX_9,  2, 9),
	CBUS_GPIO(GPIOX_10, 2, 10),
	CBUS_GPIO(GPIOX_11, 2, 11),
	CBUS_GPIO(GPIOX_12, 2, 12),
	CBUS_GPIO(GPIOX_13, 2, 13),
	CBUS_GPIO(GPIOX_14, 2, 14),
	CBUS_GPIO(GPIOX_15, 2, 15),
	CBUS_GPIO(GPIOX_16, 2, 16),
	CBUS_GPIO(GPIOX_17, 2, 17),
	CBUS_GPIO(GPIOX_18, 2, 18),
	CBUS_GPIO(GPIOX_19, 2, 19),

	/* GPIOH */
	CBUS_GPIO(GPIOH_0,  3, 0),
	CBUS_GPIO(GPIOH_1,  3, 1),
	CBUS_GPIO(GPIOH_2,  3, 2),
	CBUS_GPIO(GPIOH_3,  3, 3),
	CBUS_GPIO(GPIOH_4,  3, 4),
	CBUS_GPIO(GPIOH_5,  3, 5),
	CBUS_GPIO(GPIOH_6,  3, 6),
	CBUS_GPIO(GPIOH_7,  3, 7),
	CBUS_GPIO(GPIOH_8,  3, 8),

	/* GPIOZ */
	CBUS_GPIO(GPIOZ_0,  4, 0),
	CBUS_GPIO(GPIOZ_1,  4, 1),
	CBUS_GPIO(GPIOZ_2,  4, 2),
	CBUS_GPIO(GPIOZ_3,  4, 3),
	CBUS_GPIO(GPIOZ_4,  4, 4),
	CBUS_GPIO(GPIOZ_5,  4, 5),
	CBUS_GPIO(GPIOZ_6,  4, 6),
	CBUS_GPIO(GPIOZ_7,  4, 7),
	CBUS_GPIO(GPIOZ_8,  4, 8),
	CBUS_GPIO(GPIOZ_9,  4, 9),
	CBUS_GPIO(GPIOZ_10, 4, 10),
	CBUS_GPIO(GPIOZ_11, 4, 11),
	CBUS_GPIO(GPIOZ_12, 4, 12),
	CBUS_GPIO(GPIOZ_13, 4, 13),
	CBUS_GPIO(GPIOZ_14, 4, 14),
	CBUS_GPIO(GPIOZ_15, 4, 15),

	/* GPIOA */
	CBUS_GPIO(GPIOA_0,  5, 0),
	CBUS_GPIO(GPIOA_1,  5, 1),
	CBUS_GPIO(GPIOA_2,  5, 2),
	CBUS_GPIO(GPIOA_3,  5, 3),
	CBUS_GPIO(GPIOA_4,  5, 4),
	CBUS_GPIO(GPIOA_5,  5, 5),
	CBUS_GPIO(GPIOA_6,  5, 6),
	CBUS_GPIO(GPIOA_7,  5, 7),
	CBUS_GPIO(GPIOA_8,  5, 8),
	CBUS_GPIO(GPIOA_9,  5, 9),
	CBUS_GPIO(GPIOA_10, 5, 10),
	CBUS_GPIO(GPIOA_11, 5, 11),
	CBUS_GPIO(GPIOA_12, 5, 12),
	CBUS_GPIO(GPIOA_13, 5, 13),
	CBUS_GPIO(GPIOA_14, 5, 14),
	CBUS_GPIO(GPIOA_15, 5, 15),
};

#define	AO_GPIO(_id, _off, _bit)					\
	[_id] = {							\
		.id = (_id),						\
		.name = __STRING(_id),					\
		.oen = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG(_off),				\
			.mask = __BIT(_bit)				\
		},							\
		.out = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) + 4),			\
			.mask = __BIT(_bit)				\
		},							\
		.in = {							\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) + 1),			\
			.mask = __BIT(_bit)				\
		},							\
		.pupden = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) + 3),			\
			.mask = __BIT(_bit)				\
		},							\
		.pupd = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_off) + 2),			\
			.mask = __BIT(_bit)				\
		},							\
	}

static const struct meson_pinctrl_gpio mesong12a_aobus_gpios[] = {
	/* GPIOAO */
	AO_GPIO(GPIOAO_0,  0, 0),
	AO_GPIO(GPIOAO_1,  0, 1),
	AO_GPIO(GPIOAO_2,  0, 2),
	AO_GPIO(GPIOAO_3,  0, 3),
	AO_GPIO(GPIOAO_4,  0, 4),
	AO_GPIO(GPIOAO_5,  0, 5),
	AO_GPIO(GPIOAO_6,  0, 6),
	AO_GPIO(GPIOAO_7,  0, 7),
	AO_GPIO(GPIOAO_8,  0, 8),
	AO_GPIO(GPIOAO_9,  0, 9),
	AO_GPIO(GPIOAO_10, 0, 10),
	AO_GPIO(GPIOAO_11, 0, 11),

	/* GPIOE */
	AO_GPIO(GPIOE_0,   0, 16),
	AO_GPIO(GPIOE_1,   0, 17),
	AO_GPIO(GPIOE_2,   0, 18),
};

#define GPIO_MUX_PINCTRL_GROUP(_name, _reg, _off, _group, _func)	\
	{ .name = _name, .reg = CBUS_REG(_reg), .bit = (_func),		\
	    .bank = { _group }, .nbank = 1,				\
	    .func = (_func), .mask = (0xf << (4 * ((_off) & 7))) }

static const struct meson_pinctrl_group mesong12a_periphs_groups[] = {
	/* BOOT (PERIPHS_PIN_MUX_0..1) */
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d0",		0x0, 0, BOOT_0,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d1",		0x0, 1, BOOT_1,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d2",		0x0, 2, BOOT_2,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d3",		0x0, 3, BOOT_3,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d4",		0x0, 4, BOOT_4,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d5",		0x0, 5, BOOT_5,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d6",		0x0, 6, BOOT_6,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_d7",		0x0, 7, BOOT_7,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_clk",		0x1, 0, BOOT_8,   1),
	GPIO_MUX_PINCTRL_GROUP("emmc_cmd",		0x1, 2, BOOT_10,  1),
	GPIO_MUX_PINCTRL_GROUP("emmc_nand_ds",		0x1, 5, BOOT_13,  1),
	GPIO_MUX_PINCTRL_GROUP("nand_ce0",		0x1, 3, BOOT_11,  2),
	GPIO_MUX_PINCTRL_GROUP("nand_ale",		0x1, 1, BOOT_9,   2),
	GPIO_MUX_PINCTRL_GROUP("nand_cle",		0x1, 2, BOOT_10,  2),
	GPIO_MUX_PINCTRL_GROUP("nand_wen_clk",		0x1, 0, BOOT_8,   2),
	GPIO_MUX_PINCTRL_GROUP("nand_ren_wr",		0x1, 4, BOOT_12,  2),
	GPIO_MUX_PINCTRL_GROUP("nand_rb0",		0x1, 6, BOOT_14,  2),
	GPIO_MUX_PINCTRL_GROUP("nand_ce1",		0x1, 7, BOOT_15,  2),
	GPIO_MUX_PINCTRL_GROUP("nor_hold",		0x0, 3, BOOT_3,   3),
	GPIO_MUX_PINCTRL_GROUP("nor_d",			0x0, 4, BOOT_4,   3),
	GPIO_MUX_PINCTRL_GROUP("nor_q",			0x0, 5, BOOT_5,   3),
	GPIO_MUX_PINCTRL_GROUP("nor_c",			0x0, 6, BOOT_6,   3),
	GPIO_MUX_PINCTRL_GROUP("nor_wp",		0x0, 7, BOOT_7,   3),
	GPIO_MUX_PINCTRL_GROUP("nor_cs",		0x1, 6, BOOT_14,  3),

	/* GPIOZ (PERIPHS_PIN_MUX_6..7) */
	GPIO_MUX_PINCTRL_GROUP("sdcard_d0_z",		0x6, 2, GPIOZ_2,  5),
	GPIO_MUX_PINCTRL_GROUP("sdcard_d1_z",		0x6, 3, GPIOZ_3,  5),
	GPIO_MUX_PINCTRL_GROUP("sdcard_d2_z",		0x6, 4, GPIOZ_4,  5),
	GPIO_MUX_PINCTRL_GROUP("sdcard_d3_z",		0x6, 5, GPIOZ_5,  5),
	GPIO_MUX_PINCTRL_GROUP("sdcard_clk_z",		0x6, 6, GPIOZ_6,  5),
	GPIO_MUX_PINCTRL_GROUP("sdcard_cmd_z",		0x6, 7, GPIOZ_7,  5),
	GPIO_MUX_PINCTRL_GROUP("i2c0_sda_z0",		0x6, 0, GPIOZ_0,  4),
	GPIO_MUX_PINCTRL_GROUP("i2c0_sck_z1",		0x6, 1, GPIOZ_1,  4),
	GPIO_MUX_PINCTRL_GROUP("i2c0_sda_z7",		0x6, 7, GPIOZ_7,  7),
	GPIO_MUX_PINCTRL_GROUP("i2c0_sck_z8",		0x7, 0, GPIOZ_8,  7),
	GPIO_MUX_PINCTRL_GROUP("i2c2_sda_z",		0x7, 6, GPIOZ_14, 3),
	GPIO_MUX_PINCTRL_GROUP("i2c2_sck_z",		0x7, 7, GPIOZ_15, 3),
	GPIO_MUX_PINCTRL_GROUP("iso7816_clk_z",		0x6, 0, GPIOZ_0,  3),
	GPIO_MUX_PINCTRL_GROUP("iso7816_data_z",	0x6, 1, GPIOZ_1,  3),
	GPIO_MUX_PINCTRL_GROUP("eth_mdio",		0x6, 0, GPIOZ_0,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_mdc",		0x6, 1, GPIOZ_1,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rgmii_rx_clk",	0x6, 2, GPIOZ_2,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rx_dv",		0x6, 3, GPIOZ_3,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rxd0",		0x6, 4, GPIOZ_4,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rxd1",		0x6, 5, GPIOZ_5,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rxd2_rgmii",	0x6, 6, GPIOZ_6,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rxd3_rgmii",	0x6, 7, GPIOZ_7,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_rgmii_tx_clk",	0x7, 0, GPIOZ_8,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_txen",		0x7, 1, GPIOZ_9,  1),
	GPIO_MUX_PINCTRL_GROUP("eth_txd0",		0x7, 2, GPIOZ_10, 1),
	GPIO_MUX_PINCTRL_GROUP("eth_txd1",		0x7, 3, GPIOZ_11, 1),
	GPIO_MUX_PINCTRL_GROUP("eth_txd2_rgmii",	0x7, 4, GPIOZ_12, 1),
	GPIO_MUX_PINCTRL_GROUP("eth_txd3_rgmii",	0x7, 5, GPIOZ_13, 1),
	GPIO_MUX_PINCTRL_GROUP("eth_link_led",		0x7, 6, GPIOZ_14, 1),
	GPIO_MUX_PINCTRL_GROUP("eth_act_led",		0x7, 7, GPIOZ_15, 1),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_vs",		0x6, 0, GPIOZ_0,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_hs",		0x6, 1, GPIOZ_1,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_clk",		0x6, 3, GPIOZ_3,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din0",		0x6, 4, GPIOZ_4,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din1",		0x6, 5, GPIOZ_5,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din2",		0x6, 6, GPIOZ_6,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din3",		0x6, 7, GPIOZ_7,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din4",		0x7, 0, GPIOZ_8,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din5",		0x7, 1, GPIOZ_9,  2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din6",		0x7, 2, GPIOZ_10, 2),
	GPIO_MUX_PINCTRL_GROUP("bt565_a_din7",		0x7, 3, GPIOZ_11, 2),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_valid_z",	0x6, 2, GPIOZ_2,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_sop_z",		0x6, 3, GPIOZ_3,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din0_z",		0x6, 4, GPIOZ_4,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_clk_z",		0x6, 5, GPIOZ_5,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_fail",		0x6, 6, GPIOZ_6,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din1",		0x6, 7, GPIOZ_7,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din2",		0x7, 0, GPIOZ_8,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din3",		0x7, 1, GPIOZ_9,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din4",		0x7, 2, GPIOZ_10, 3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din5",		0x7, 3, GPIOZ_11, 3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din6",		0x7, 4, GPIOZ_12, 3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din7",		0x7, 5, GPIOZ_13, 3),
	GPIO_MUX_PINCTRL_GROUP("pdm_din0_z",		0x6, 2, GPIOZ_2,  7),
	GPIO_MUX_PINCTRL_GROUP("pdm_din1_z",		0x6, 3, GPIOZ_3,  7),
	GPIO_MUX_PINCTRL_GROUP("pdm_din2_z",		0x6, 4, GPIOZ_4,  7),
	GPIO_MUX_PINCTRL_GROUP("pdm_din3_z",		0x6, 5, GPIOZ_5,  7),
	GPIO_MUX_PINCTRL_GROUP("pdm_dclk_z",		0x6, 6, GPIOZ_6,  7),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_slv_sclk_z",	0x6, 7, GPIOZ_7,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_slv_fs_z",	0x6, 6, GPIOZ_6,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din0_z",		0x6, 2, GPIOZ_2,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din1_z",		0x6, 3, GPIOZ_3,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din2_z",		0x6, 4, GPIOZ_4,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din3_z",		0x6, 5, GPIOZ_5,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_sclk_z",		0x6, 7, GPIOZ_7,  4),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_fs_z",		0x6, 6, GPIOZ_6,  4),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout0_z",		0x6, 2, GPIOZ_2,  4),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout1_z",		0x6, 3, GPIOZ_3,  4),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout2_z",		0x6, 4, GPIOZ_4,  4),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout3_z",		0x6, 5, GPIOZ_5,  4),
	GPIO_MUX_PINCTRL_GROUP("mclk1_z",		0x7, 0, GPIOZ_8,  4),

	/* GPIOX (PERIPHS_PIN_MUX_3..5) */
	GPIO_MUX_PINCTRL_GROUP("sdio_d0",		0x3, 0, GPIOX_0,  1),
	GPIO_MUX_PINCTRL_GROUP("sdio_d1",		0x3, 1, GPIOX_1,  1),
	GPIO_MUX_PINCTRL_GROUP("sdio_d2",		0x3, 2, GPIOX_2,  1),
	GPIO_MUX_PINCTRL_GROUP("sdio_d3",		0x3, 3, GPIOX_3,  1),
	GPIO_MUX_PINCTRL_GROUP("sdio_clk",		0x3, 4, GPIOX_4,  1),
	GPIO_MUX_PINCTRL_GROUP("sdio_cmd",		0x3, 5, GPIOX_5,  1),
	GPIO_MUX_PINCTRL_GROUP("spi0_mosi_x",		0x4, 0, GPIOX_8,  4),
	GPIO_MUX_PINCTRL_GROUP("spi0_miso_x",		0x4, 1, GPIOX_9,  4),
	GPIO_MUX_PINCTRL_GROUP("spi0_ss0_x",		0x4, 2, GPIOX_10, 4),
	GPIO_MUX_PINCTRL_GROUP("spi0_clk_x",		0x4, 3, GPIOX_11, 4),
	GPIO_MUX_PINCTRL_GROUP("i2c1_sda_x",		0x4, 2, GPIOX_10, 5),
	GPIO_MUX_PINCTRL_GROUP("i2c1_sck_x",		0x4, 3, GPIOX_11, 5),
	GPIO_MUX_PINCTRL_GROUP("i2c2_sda_x",		0x5, 1, GPIOX_17, 1),
	GPIO_MUX_PINCTRL_GROUP("i2c2_sck_x",		0x5, 2, GPIOX_18, 1),
	GPIO_MUX_PINCTRL_GROUP("uart_a_tx",		0x4, 4, GPIOX_12, 1),
	GPIO_MUX_PINCTRL_GROUP("uart_a_rx",		0x4, 5, GPIOX_13, 1),
	GPIO_MUX_PINCTRL_GROUP("uart_a_cts",		0x4, 6, GPIOX_14, 1),
	GPIO_MUX_PINCTRL_GROUP("uart_a_rts",		0x4, 7, GPIOX_15, 1),
	GPIO_MUX_PINCTRL_GROUP("uart_b_tx",		0x3, 6, GPIOX_6,  2),
	GPIO_MUX_PINCTRL_GROUP("uart_b_rx",		0x3, 7, GPIOX_7,  2),
	GPIO_MUX_PINCTRL_GROUP("iso7816_clk_x",		0x4, 0, GPIOX_8,  6),
	GPIO_MUX_PINCTRL_GROUP("iso7816_data_x",	0x4, 1, GPIOX_9,  6),
	GPIO_MUX_PINCTRL_GROUP("pwm_a",			0x3, 6, GPIOX_6,  1),
	GPIO_MUX_PINCTRL_GROUP("pwm_b_x7",		0x3, 7, GPIOX_7,  4),
	GPIO_MUX_PINCTRL_GROUP("pwm_b_x19",		0x5, 3, GPIOX_19, 1),
	GPIO_MUX_PINCTRL_GROUP("pwm_c_x5",		0x3, 5, GPIOX_5,  4),
	GPIO_MUX_PINCTRL_GROUP("pwm_c_x8",		0x4, 0, GPIOX_8,  5),
	GPIO_MUX_PINCTRL_GROUP("pwm_d_x3",		0x3, 3, GPIOX_3,  4),
	GPIO_MUX_PINCTRL_GROUP("pwm_d_x6",		0x3, 6, GPIOX_6,  4),
	GPIO_MUX_PINCTRL_GROUP("pwm_e",			0x5, 0, GPIOX_16, 1),
	GPIO_MUX_PINCTRL_GROUP("pwm_f_x",		0x3, 7, GPIOX_7,  1),
	GPIO_MUX_PINCTRL_GROUP("tsin_a_valid",		0x3, 2, GPIOX_2,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_a_sop",		0x3, 1, GPIOX_1,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_a_din0",		0x3, 0, GPIOX_0,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_a_clk",		0x3, 3, GPIOX_3,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_valid_x",	0x4, 1, GPIOX_9,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_sop_x",		0x4, 0, GPIOX_8,  3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_din0_x",		0x4, 2, GPIOX_10, 3),
	GPIO_MUX_PINCTRL_GROUP("tsin_b_clk_x",		0x4, 3, GPIOX_11, 3),
	GPIO_MUX_PINCTRL_GROUP("pdm_din0_x",		0x3, 0, GPIOX_0,  2),
	GPIO_MUX_PINCTRL_GROUP("pdm_din1_x",		0x3, 1, GPIOX_1,  2),
	GPIO_MUX_PINCTRL_GROUP("pdm_din2_x",		0x3, 2, GPIOX_2,  2),
	GPIO_MUX_PINCTRL_GROUP("pdm_din3_x",		0x3, 3, GPIOX_3,  2),
	GPIO_MUX_PINCTRL_GROUP("pdm_dclk_x",		0x3, 4, GPIOX_4,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_slv_sclk",	0x4, 3, GPIOX_11, 2),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_slv_fs",		0x4, 2, GPIOX_10, 2),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_din0",		0x4, 1, GPIOX_9,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_din1",		0x4, 0, GPIOX_8,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_sclk",		0x4, 3, GPIOX_11, 1),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_fs",		0x4, 2, GPIOX_10, 1),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_dout0",		0x4, 1, GPIOX_9,  1),
	GPIO_MUX_PINCTRL_GROUP("tdm_a_dout1",		0x4, 0, GPIOX_8,  1),
	GPIO_MUX_PINCTRL_GROUP("mclk1_x",		0x3, 5, GPIOX_5,  2),

	/* GPIOC (PERIPHS_PIN_MUX_9) */
	GPIO_MUX_PINCTRL_GROUP("sdcard_d0_c",		0x9, 0, GPIOC_0,  1),
	GPIO_MUX_PINCTRL_GROUP("sdcard_d1_c",		0x9, 1, GPIOC_1,  1),
	GPIO_MUX_PINCTRL_GROUP("sdcard_d2_c",		0x9, 2, GPIOC_2,  1),
	GPIO_MUX_PINCTRL_GROUP("sdcard_d3_c",		0x9, 3, GPIOC_3,  1),
	GPIO_MUX_PINCTRL_GROUP("sdcard_clk_c",		0x9, 4, GPIOC_4,  1),
	GPIO_MUX_PINCTRL_GROUP("sdcard_cmd_c",		0x9, 5, GPIOC_5,  1),
	GPIO_MUX_PINCTRL_GROUP("spi0_mosi_c",		0x9, 0, GPIOC_0,  5),
	GPIO_MUX_PINCTRL_GROUP("spi0_miso_c",		0x9, 1, GPIOC_1,  5),
	GPIO_MUX_PINCTRL_GROUP("spi0_ss0_c",		0x9, 2, GPIOC_2,  5),
	GPIO_MUX_PINCTRL_GROUP("spi0_clk_c",		0x9, 3, GPIOC_3,  5),
	GPIO_MUX_PINCTRL_GROUP("i2c0_sda_c",		0x9, 5, GPIOC_5,  3),
	GPIO_MUX_PINCTRL_GROUP("i2c0_sck_c",		0x9, 6, GPIOC_6,  3),
	GPIO_MUX_PINCTRL_GROUP("uart_ao_a_rx_c",	0x9, 2, GPIOC_2,  2),
	GPIO_MUX_PINCTRL_GROUP("uart_ao_a_tx_c",	0x9, 3, GPIOC_3,  2),
	GPIO_MUX_PINCTRL_GROUP("iso7816_clk_c",		0x9, 5, GPIOC_5,  5),
	GPIO_MUX_PINCTRL_GROUP("iso7816_data_c",	0x9, 6, GPIOC_6,  5),
	GPIO_MUX_PINCTRL_GROUP("pwm_c_c",		0x9, 4, GPIOC_4,  5),
	GPIO_MUX_PINCTRL_GROUP("jtag_b_tdo",		0x9, 0, GPIOC_0,  2),
	GPIO_MUX_PINCTRL_GROUP("jtag_b_tdi",		0x9, 1, GPIOC_1,  2),
	GPIO_MUX_PINCTRL_GROUP("jtag_b_clk",		0x9, 4, GPIOC_4,  2),
	GPIO_MUX_PINCTRL_GROUP("jtag_b_tms",		0x9, 5, GPIOC_5,  2),
	GPIO_MUX_PINCTRL_GROUP("pdm_din0_c",		0x9, 0, GPIOC_0,  4),
	GPIO_MUX_PINCTRL_GROUP("pdm_din1_c",		0x9, 1, GPIOC_1,  4),
	GPIO_MUX_PINCTRL_GROUP("pdm_din2_c",		0x9, 2, GPIOC_2,  4),
	GPIO_MUX_PINCTRL_GROUP("pdm_din3_c",		0x9, 3, GPIOC_3,  4),
	GPIO_MUX_PINCTRL_GROUP("pdm_dclk_c",		0x9, 4, GPIOC_4,  4),

	/* GPIOH (PERIPHS_PIN_MUX_B..C) */
	GPIO_MUX_PINCTRL_GROUP("spi1_mosi",		0xb, 4, GPIOH_4,  3),
	GPIO_MUX_PINCTRL_GROUP("spi1_miso",		0xb, 5, GPIOH_5,  3),
	GPIO_MUX_PINCTRL_GROUP("spi1_ss0",		0xb, 6, GPIOH_6,  3),
	GPIO_MUX_PINCTRL_GROUP("spi1_clk",		0xb, 7, GPIOH_7,  3),
	GPIO_MUX_PINCTRL_GROUP("i2c1_sda_h2",		0xb, 2, GPIOH_2,  2),
	GPIO_MUX_PINCTRL_GROUP("i2c1_sck_h3",		0xb, 3, GPIOH_3,  2),
	GPIO_MUX_PINCTRL_GROUP("i2c1_sda_h6",		0xb, 6, GPIOH_6,  4),
	GPIO_MUX_PINCTRL_GROUP("i2c1_sck_h7",		0xb, 7, GPIOH_7,  4),
	GPIO_MUX_PINCTRL_GROUP("i2c3_sda_h",		0xb, 0, GPIOH_0,  2),
	GPIO_MUX_PINCTRL_GROUP("i2c3_sck_h",		0xb, 1, GPIOH_1,  2),
	GPIO_MUX_PINCTRL_GROUP("uart_c_tx",		0xb, 7, GPIOH_7,  2),
	GPIO_MUX_PINCTRL_GROUP("uart_c_rx",		0xb, 6, GPIOH_6,  2),
	GPIO_MUX_PINCTRL_GROUP("uart_c_cts",		0xb, 5, GPIOH_5,  2),
	GPIO_MUX_PINCTRL_GROUP("uart_c_rts",		0xb, 4, GPIOH_4,  2),
	GPIO_MUX_PINCTRL_GROUP("iso7816_clk_h",		0xb, 6, GPIOH_6,  1),
	GPIO_MUX_PINCTRL_GROUP("iso7816_data_h",	0xb, 7, GPIOH_7,  1),
	GPIO_MUX_PINCTRL_GROUP("pwm_f_h",		0xb, 5, GPIOH_5,  4),
	GPIO_MUX_PINCTRL_GROUP("cec_ao_a_h",		0xb, 3, GPIOH_3,  4),
	GPIO_MUX_PINCTRL_GROUP("cec_ao_b_h",		0xb, 3, GPIOH_3,  5),
	GPIO_MUX_PINCTRL_GROUP("hdmitx_sda",		0xb, 0, GPIOH_0,  1),
	GPIO_MUX_PINCTRL_GROUP("hdmitx_sck",		0xb, 1, GPIOH_1,  1),
	GPIO_MUX_PINCTRL_GROUP("hdmitx_hpd_in",		0xb, 2, GPIOH_2,  1),
	GPIO_MUX_PINCTRL_GROUP("spdif_out_h",		0xb, 4, GPIOH_4,  1),
	GPIO_MUX_PINCTRL_GROUP("spdif_in_h",		0xb, 5, GPIOH_5,  1),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_din3_h",		0xb, 5, GPIOH_5,  6),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_dout3_h",		0xb, 5, GPIOH_5,  5),

	/* GPIOA (PERIPHS_PIN_MUX_D..E) */
	GPIO_MUX_PINCTRL_GROUP("i2c3_sda_a",		0xe, 6, GPIOA_14, 2),
	GPIO_MUX_PINCTRL_GROUP("i2c3_sck_a",		0xe, 7, GPIOA_15, 2),
	GPIO_MUX_PINCTRL_GROUP("pdm_din0_a",		0xe, 0, GPIOA_8,  1),
	GPIO_MUX_PINCTRL_GROUP("pdm_din1_a",		0xe, 1, GPIOA_9,  1),
	GPIO_MUX_PINCTRL_GROUP("pdm_din2_a",		0xd, 6, GPIOA_6,  1),
	GPIO_MUX_PINCTRL_GROUP("pdm_din3_a",		0xd, 5, GPIOA_5,  1),
	GPIO_MUX_PINCTRL_GROUP("pdm_dclk_a",		0xd, 7, GPIOA_7,  1),
	GPIO_MUX_PINCTRL_GROUP("spdif_in_a10",		0xe, 2, GPIOA_10, 1),
	GPIO_MUX_PINCTRL_GROUP("spdif_in_a12",		0xe, 4, GPIOA_12, 1),
	GPIO_MUX_PINCTRL_GROUP("spdif_out_a11",		0xe, 3, GPIOA_11, 1),
	GPIO_MUX_PINCTRL_GROUP("spdif_out_a13",		0xe, 5, GPIOA_13, 1),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_slv_sclk",	0xd, 1, GPIOA_1,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_slv_fs",		0xd, 2, GPIOA_2,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_din0",		0xd, 3, GPIOA_3,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_din1",		0xd, 4, GPIOA_4,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_din2",		0xd, 5, GPIOA_5,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_din3_a",		0xd, 6, GPIOA_6,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_sclk",		0xd, 1, GPIOA_1,  1),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_fs",		0xd, 2, GPIOA_2,  1),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_dout0",		0xd, 3, GPIOA_3,  1),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_dout1",		0xd, 4, GPIOA_4,  1),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_dout2",		0xd, 5, GPIOA_5,  3),
	GPIO_MUX_PINCTRL_GROUP("tdm_b_dout3_a",		0xd, 6, GPIOA_6,  3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_slv_sclk_a",	0xe, 4, GPIOA_12, 3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_slv_fs_a",	0xe, 5, GPIOA_13, 3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din0_a",		0xe, 2, GPIOA_10, 3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din1_a",		0xe, 1, GPIOA_9,  3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din2_a",		0xe, 0, GPIOA_8,  3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_din3_a",		0xd, 7, GPIOA_7,  3),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_sclk_a",		0xe, 4, GPIOA_12, 2),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_fs_a",		0xe, 5, GPIOA_13, 2),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout0_a",		0xe, 2, GPIOA_10, 2),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout1_a",		0xe, 1, GPIOA_9,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout2_a",		0xe, 0, GPIOA_8,  2),
	GPIO_MUX_PINCTRL_GROUP("tdm_c_dout3_a",		0xd, 7, GPIOA_7,  2),
	GPIO_MUX_PINCTRL_GROUP("mclk0_a",		0xd, 0, GPIOA_0,  1),
	GPIO_MUX_PINCTRL_GROUP("mclk1_a",		0xe, 3, GPIOA_11, 2),
};

#define AOBUS_MUX_PINCTRL_GROUP(_name, _bit, _group, _func)		\
	{ .name = _name, .reg = CBUS_REG((_bit) / 8), .bit = (_func),	\
	    .bank = { _group }, .nbank = 1,				\
	    .func = (_func), .mask = (0xf << (4 * ((_bit) & 7))) }

static const struct meson_pinctrl_group mesong12a_aobus_groups[] = {
	/* GPIOAO and GPIOE (AO_RTI_PINMUX_REG0..1) */
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_a_rx",		 1, GPIOAO_1,  1),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_a_tx",		 0, GPIOAO_0,  1),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_a_cts",	16, GPIOE_0,   1),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_a_rts",	17, GPIOE_1,   1),

	AOBUS_MUX_PINCTRL_GROUP("uart_ao_b_tx_2",	 2, GPIOAO_2,  2),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_b_rx_3",	 3, GPIOAO_3,  2),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_b_tx_8",	 8, GPIOAO_8,  3),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_b_rx_9",	 9, GPIOAO_9,  3),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_b_cts",	12, GPIOE_0,   2),
	AOBUS_MUX_PINCTRL_GROUP("uart_ao_b_rts",	13, GPIOE_1,   2),

	AOBUS_MUX_PINCTRL_GROUP("i2c_ao_sck",		 2, GPIOAO_2,  1),
	AOBUS_MUX_PINCTRL_GROUP("i2c_ao_sda",		 3, GPIOAO_3,  1),
	AOBUS_MUX_PINCTRL_GROUP("i2c_ao_sck_e",		12, GPIOE_0,   4),
	AOBUS_MUX_PINCTRL_GROUP("i2c_ao_sda_e",		13, GPIOE_1,   4),
	AOBUS_MUX_PINCTRL_GROUP("i2c_ao_slave_sck",	 2, GPIOAO_2,  3),
	AOBUS_MUX_PINCTRL_GROUP("i2c_ao_slave_sda",	 3, GPIOAO_3,  3),

	AOBUS_MUX_PINCTRL_GROUP("remote_ao_input",	 5, GPIOAO_5,  1),
	AOBUS_MUX_PINCTRL_GROUP("remote_ao_out",	 4, GPIOAO_4,  1),

	AOBUS_MUX_PINCTRL_GROUP("pwm_a_e",		14, GPIOE_2,   3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_a",		11, GPIOAO_11, 3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_a_hiz",		11, GPIOAO_11, 2),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_b",		12, GPIOE_0,   3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_c_4",		 4, GPIOAO_4,  3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_c_hiz",		 4, GPIOAO_4,  4),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_c_6",		 6, GPIOAO_6,  3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_d_5",		 5, GPIOAO_5,  3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_d_10",		10, GPIOAO_10, 3),
	AOBUS_MUX_PINCTRL_GROUP("pwm_ao_d_e",		13, GPIOE_1,   3),

	AOBUS_MUX_PINCTRL_GROUP("jtag_a_tdi",		 8, GPIOAO_8,  1),
	AOBUS_MUX_PINCTRL_GROUP("jtag_a_tdo",		 9, GPIOAO_9,  1),
	AOBUS_MUX_PINCTRL_GROUP("jtag_a_clk",		 6, GPIOAO_6,  1),
	AOBUS_MUX_PINCTRL_GROUP("jtag_a_tms",		 7, GPIOAO_7,  1),

	AOBUS_MUX_PINCTRL_GROUP("cec_ao_a",		10, GPIOAO_10, 1),
	AOBUS_MUX_PINCTRL_GROUP("cec_ao_b",		10, GPIOAO_10, 2),

	AOBUS_MUX_PINCTRL_GROUP("tsin_ao_asop",		 6, GPIOAO_6,  4),
	AOBUS_MUX_PINCTRL_GROUP("tsin_ao_adin0",	 7, GPIOAO_7,  4),
	AOBUS_MUX_PINCTRL_GROUP("tsin_ao_aclk",		 8, GPIOAO_8,  4),
	AOBUS_MUX_PINCTRL_GROUP("tsin_ao_a_valid",	 9, GPIOAO_9,  4),

	AOBUS_MUX_PINCTRL_GROUP("spdif_ao_out",		10, GPIOAO_10, 4),

	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_dout0",	 4, GPIOAO_4,  5),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_dout1",	10, GPIOAO_10, 5),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_dout2",	 6, GPIOAO_6,  5),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_fs",		 7, GPIOAO_7,  5),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_sclk",	 8, GPIOAO_8,  5),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_din0",	 4, GPIOAO_4,  6),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_din1",	10, GPIOAO_10, 6),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_din2",	 6, GPIOAO_6,  6),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_slv_fs",	 7, GPIOAO_7,  6),
	AOBUS_MUX_PINCTRL_GROUP("tdm_ao_b_slv_sclk",	 8, GPIOAO_8,  6),

	AOBUS_MUX_PINCTRL_GROUP("mclk0_ao",		 9, GPIOAO_9,  5),
};

const struct meson_pinctrl_config mesong12a_periphs_pinctrl_config = {
	.name = "Meson G12A periphs GPIO",
	.groups = mesong12a_periphs_groups,
	.ngroups = __arraycount(mesong12a_periphs_groups),
	.gpios = mesong12a_periphs_gpios,
	.ngpios = __arraycount(mesong12a_periphs_gpios),
};

const struct meson_pinctrl_config mesong12a_aobus_pinctrl_config = {
	.name = "Meson G12A AO GPIO",
	.groups = mesong12a_aobus_groups,
	.ngroups = __arraycount(mesong12a_aobus_groups),
	.gpios = mesong12a_aobus_gpios,
	.ngpios = __arraycount(mesong12a_aobus_gpios),
};
