/* $NetBSD: mesongxbb_pinctrl.c,v 1.3 2021/11/17 11:31:12 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mesongxbb_pinctrl.c,v 1.3 2021/11/17 11:31:12 jmcneill Exp $");

#include <sys/param.h>

#include <arm/amlogic/meson_pinctrl.h>

/* CBUS pinmux registers */
#define	CBUS_REG(n)	((n) << 2)
#define	REG0		CBUS_REG(0)
#define	REG1		CBUS_REG(1)
#define	REG2		CBUS_REG(2)
#define	REG3		CBUS_REG(3)
#define	REG4		CBUS_REG(4)
#define	REG5		CBUS_REG(5)
#define	REG6		CBUS_REG(6)
#define	REG7		CBUS_REG(7)
#define	REG8		CBUS_REG(8)
#define	REG9		CBUS_REG(9)

/* AO pinmux registers */
#define	AOREG0		0x00
#define	AOREG1		0x04

/*
 * GPIO banks. The values must match those in dt-bindings/gpio/meson-gxbb-gpio.h
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

	BOOT_0 = 20,
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
	BOOT_16,
	BOOT_17,

	CARD_0 = 38,
	CARD_1,
	CARD_2,
	CARD_3,
	CARD_4,
	CARD_5,
	CARD_6,

	GPIODV_0 = 45,
	GPIODV_1,
	GPIODV_2,
	GPIODV_3,
	GPIODV_4,
	GPIODV_5,
	GPIODV_6,
	GPIODV_7,
	GPIODV_8,
	GPIODV_9,
	GPIODV_10,
	GPIODV_11,
	GPIODV_12,
	GPIODV_13,
	GPIODV_14,
	GPIODV_15,
	GPIODV_16,
	GPIODV_17,
	GPIODV_18,
	GPIODV_19,
	GPIODV_20,
	GPIODV_21,
	GPIODV_22,
	GPIODV_23,
	GPIODV_24,
	GPIODV_25,
	GPIODV_26,
	GPIODV_27,
	GPIODV_28,
	GPIODV_29,

	GPIOY_0 = 75,
	GPIOY_1,
	GPIOY_2,
	GPIOY_3,
	GPIOY_4,
	GPIOY_5,
	GPIOY_6,
	GPIOY_7,
	GPIOY_8,
	GPIOY_9,
	GPIOY_10,
	GPIOY_11,
	GPIOY_12,
	GPIOY_13,
	GPIOY_14,
	GPIOY_15,
	GPIOY_16,

	GPIOX_0 = 92,
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
	GPIOX_20,
	GPIOX_21,
	GPIOX_22,

	GPIOCLK_0 = 115,
	GPIOCLK_1,
	GPIOCLK_2,
	GPIOCLK_3,

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
	GPIOAO_12,
	GPIOAO_13,
	GPIO_TEST_N,
};

#define	CBUS_GPIO(_id, _off, _bit)	\
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

static const struct meson_pinctrl_gpio mesongxbb_periphs_gpios[] = {
	/* GPIODV */
	CBUS_GPIO(GPIODV_0, 0, 0),
	CBUS_GPIO(GPIODV_1, 0, 1),
	CBUS_GPIO(GPIODV_2, 0, 2),
	CBUS_GPIO(GPIODV_3, 0, 3),
	CBUS_GPIO(GPIODV_4, 0, 4),
	CBUS_GPIO(GPIODV_5, 0, 5),
	CBUS_GPIO(GPIODV_6, 0, 6),
	CBUS_GPIO(GPIODV_7, 0, 7),
	CBUS_GPIO(GPIODV_8, 0, 8),
	CBUS_GPIO(GPIODV_9, 0, 9),
	CBUS_GPIO(GPIODV_10, 0, 10),
	CBUS_GPIO(GPIODV_11, 0, 11),
	CBUS_GPIO(GPIODV_12, 0, 12),
	CBUS_GPIO(GPIODV_13, 0, 13),
	CBUS_GPIO(GPIODV_14, 0, 14),
	CBUS_GPIO(GPIODV_15, 0, 15),
	CBUS_GPIO(GPIODV_16, 0, 16),
	CBUS_GPIO(GPIODV_17, 0, 17),
	CBUS_GPIO(GPIODV_18, 0, 18),
	CBUS_GPIO(GPIODV_19, 0, 19),
	CBUS_GPIO(GPIODV_20, 0, 20),
	CBUS_GPIO(GPIODV_21, 0, 21),
	CBUS_GPIO(GPIODV_22, 0, 22),

	/* GPIOY */
	CBUS_GPIO(GPIOY_0, 1, 0),
	CBUS_GPIO(GPIOY_1, 1, 1),
	CBUS_GPIO(GPIOY_2, 1, 2),
	CBUS_GPIO(GPIOY_3, 1, 3),
	CBUS_GPIO(GPIOY_4, 1, 4),
	CBUS_GPIO(GPIOY_5, 1, 5),
	CBUS_GPIO(GPIOY_6, 1, 6),
	CBUS_GPIO(GPIOY_7, 1, 7),
	CBUS_GPIO(GPIOY_8, 1, 8),
	CBUS_GPIO(GPIOY_9, 1, 9),
	CBUS_GPIO(GPIOY_10, 1, 10),
	CBUS_GPIO(GPIOY_11, 1, 11),
	CBUS_GPIO(GPIOY_12, 1, 12),
	CBUS_GPIO(GPIOY_13, 1, 13),
	CBUS_GPIO(GPIOY_14, 1, 14),
	CBUS_GPIO(GPIOY_15, 1, 15),
	CBUS_GPIO(GPIOY_16, 1, 16),

	/* GPIOH */
	CBUS_GPIO(GPIOH_0, 1, 20),
	CBUS_GPIO(GPIOH_1, 1, 21),
	CBUS_GPIO(GPIOH_2, 1, 22),
	CBUS_GPIO(GPIOH_3, 1, 23),

	/* BOOT */
	CBUS_GPIO(BOOT_0, 2, 0),
	CBUS_GPIO(BOOT_1, 2, 1),
	CBUS_GPIO(BOOT_2, 2, 2),
	CBUS_GPIO(BOOT_3, 2, 3),
	CBUS_GPIO(BOOT_4, 2, 4),
	CBUS_GPIO(BOOT_5, 2, 5),
	CBUS_GPIO(BOOT_6, 2, 6),
	CBUS_GPIO(BOOT_7, 2, 7),
	CBUS_GPIO(BOOT_8, 2, 8),
	CBUS_GPIO(BOOT_9, 2, 9),
	CBUS_GPIO(BOOT_10, 2, 10),
	CBUS_GPIO(BOOT_11, 2, 11),
	CBUS_GPIO(BOOT_12, 2, 12),
	CBUS_GPIO(BOOT_13, 2, 13),
	CBUS_GPIO(BOOT_14, 2, 14),
	CBUS_GPIO(BOOT_15, 2, 15),
	CBUS_GPIO(BOOT_16, 2, 16),
	CBUS_GPIO(BOOT_17, 2, 17),

	/* CARD */
	CBUS_GPIO(CARD_0, 2, 20),
	CBUS_GPIO(CARD_1, 2, 21),
	CBUS_GPIO(CARD_2, 2, 22),
	CBUS_GPIO(CARD_3, 2, 23),
	CBUS_GPIO(CARD_4, 2, 24),
	CBUS_GPIO(CARD_5, 2, 25),
	CBUS_GPIO(CARD_6, 2, 26),

	/* GPIOZ */
	CBUS_GPIO(GPIOZ_0, 3, 0),
	CBUS_GPIO(GPIOZ_1, 3, 1),
	CBUS_GPIO(GPIOZ_2, 3, 2),
	CBUS_GPIO(GPIOZ_3, 3, 3),
	CBUS_GPIO(GPIOZ_4, 3, 4),
	CBUS_GPIO(GPIOZ_5, 3, 5),
	CBUS_GPIO(GPIOZ_6, 3, 6),
	CBUS_GPIO(GPIOZ_7, 3, 7),
	CBUS_GPIO(GPIOZ_8, 3, 8),
	CBUS_GPIO(GPIOZ_9, 3, 9),
	CBUS_GPIO(GPIOZ_10, 3, 10),
	CBUS_GPIO(GPIOZ_11, 3, 11),
	CBUS_GPIO(GPIOZ_12, 3, 12),
	CBUS_GPIO(GPIOZ_13, 3, 13),
	CBUS_GPIO(GPIOZ_14, 3, 14),
	CBUS_GPIO(GPIOZ_15, 3, 15),

	/* CLK */
	CBUS_GPIO(GPIOCLK_0, 3, 28),
	CBUS_GPIO(GPIOCLK_1, 3, 29),
	CBUS_GPIO(GPIOCLK_2, 3, 30),
	CBUS_GPIO(GPIOCLK_3, 3, 31),

	/* GPIOX */
	CBUS_GPIO(GPIOX_0, 4, 0),
	CBUS_GPIO(GPIOX_1, 4, 1),
	CBUS_GPIO(GPIOX_2, 4, 2),
	CBUS_GPIO(GPIOX_3, 4, 3),
	CBUS_GPIO(GPIOX_4, 4, 4),
	CBUS_GPIO(GPIOX_5, 4, 5),
	CBUS_GPIO(GPIOX_6, 4, 6),
	CBUS_GPIO(GPIOX_7, 4, 7),
	CBUS_GPIO(GPIOX_8, 4, 8),
	CBUS_GPIO(GPIOX_9, 4, 9),
	CBUS_GPIO(GPIOX_10, 4, 10),
	CBUS_GPIO(GPIOX_11, 4, 11),
	CBUS_GPIO(GPIOX_12, 4, 12),
	CBUS_GPIO(GPIOX_13, 4, 13),
	CBUS_GPIO(GPIOX_14, 4, 14),
	CBUS_GPIO(GPIOX_15, 4, 15),
	CBUS_GPIO(GPIOX_16, 4, 16),
	CBUS_GPIO(GPIOX_17, 4, 17),
	CBUS_GPIO(GPIOX_18, 4, 18),
	CBUS_GPIO(GPIOX_19, 4, 19),
	CBUS_GPIO(GPIOX_20, 4, 20),
	CBUS_GPIO(GPIOX_21, 4, 21),
	CBUS_GPIO(GPIOX_22, 4, 22),
};

#define	AO_GPIO(_id, _bit)						\
	[_id] = {							\
		.id = (_id),						\
		.name = __STRING(_id),					\
		.oen = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = 0,					\
			.mask = __BIT(_bit)				\
		},							\
		.out = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = 0,					\
			.mask = __BIT(_bit + 16)			\
		},							\
		.in = {							\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = 4,					\
			.mask = __BIT(_bit)				\
		},							\
		.pupden = {						\
			.type = MESON_PINCTRL_REGTYPE_PULL,		\
			.reg = 0,					\
			.mask = __BIT(_bit)				\
		},							\
		.pupd = {						\
			.type = MESON_PINCTRL_REGTYPE_PULL,		\
			.reg = 0,					\
			.mask = __BIT(_bit + 16)			\
		},							\
	}

static const struct meson_pinctrl_gpio mesongxbb_aobus_gpios[] = {
	/* GPIOAO */
	AO_GPIO(GPIOAO_0, 0),
	AO_GPIO(GPIOAO_1, 1),
	AO_GPIO(GPIOAO_2, 2),
	AO_GPIO(GPIOAO_3, 3),
	AO_GPIO(GPIOAO_4, 4),
	AO_GPIO(GPIOAO_5, 5),
	AO_GPIO(GPIOAO_6, 6),
	AO_GPIO(GPIOAO_7, 7),
	AO_GPIO(GPIOAO_8, 8),
	AO_GPIO(GPIOAO_9, 9),
	AO_GPIO(GPIOAO_10, 10),
	AO_GPIO(GPIOAO_11, 11),
	AO_GPIO(GPIOAO_12, 12),
	AO_GPIO(GPIOAO_13, 13),
};

static const struct meson_pinctrl_group mesongxbb_periphs_groups[] = {
	/* GPIOX */
	{ "sdio_d0",		REG8,	5,	{ GPIOX_0 }, 1 },
	{ "sdio_d1",		REG8,	4,	{ GPIOX_1 }, 1 },
	{ "sdio_d2",		REG8,	3,	{ GPIOX_2 }, 1 },
	{ "sdio_d3",		REG8,	2,	{ GPIOX_3 }, 1 },
	{ "sdio_cmd",		REG8,	1,	{ GPIOX_4 }, 1 },
	{ "sdio_clk",		REG8,	0,	{ GPIOX_5 }, 1 },
	{ "sdio_irq",		REG8,	11,	{ GPIOX_7 }, 1 },
	{ "uart_tx_a",		REG4,	13,	{ GPIOX_12 }, 1 },
	{ "uart_rx_a",		REG4,	12,	{ GPIOX_13 }, 1 },
	{ "uart_cts_a",		REG4,	11,	{ GPIOX_14 }, 1 },
	{ "uart_rts_a",		REG4,	10,	{ GPIOX_15 }, 1 },
	{ "pwm_a_x",		REG3,	17,	{ GPIOX_6 }, 1 },
	{ "pwm_e",		REG2,	30,	{ GPIOX_19 }, 1 },
	{ "pwm_f_x",		REG3,	18,	{ GPIOX_7 }, 1 },

	/* GPIOY */
	{ "uart_cts_c",		REG1,	19,	{ GPIOY_11 }, 1 },
	{ "uart_rts_c",		REG1,	18,	{ GPIOY_12 }, 1 },
	{ "uart_tx_c",		REG1,	17,	{ GPIOY_13 }, 1 },
	{ "uart_rx_c",		REG1,	16,	{ GPIOY_14 }, 1 },
	{ "pwm_a_y",		REG1,	21,	{ GPIOY_16 }, 1 },
	{ "pwm_f_y",		REG1,	20,	{ GPIOY_15 }, 1 },
	{ "i2s_out_ch23_y",	REG1,	5,	{ GPIOY_8 }, 1 },
	{ "i2s_out_ch45_y",	REG1,	6,	{ GPIOY_9 }, 1 },
	{ "i2s_out_ch67_y",	REG1,	7,	{ GPIOY_10 }, 1 },
	{ "spdif_out_y",	REG1,	9,	{ GPIOY_12 }, 1 },
	{ "gen_clk_out",	REG6,	15,	{ GPIOY_15 }, 1 },

	/* GPIOZ */
	{ "eth_mdio",		REG6,	1,	{ GPIOZ_0 }, 1 },
	{ "eth_mdc",		REG6,	0,	{ GPIOZ_1 }, 1 },
	{ "eth_clk_rx_clk",	REG6,	13,	{ GPIOZ_2 }, 1 },
	{ "eth_rx_dv",		REG6,	12,	{ GPIOZ_3 }, 1 },
	{ "eth_rxd0",		REG6,	11,	{ GPIOZ_4 }, 1 },
	{ "eth_rxd1",		REG6,	10,	{ GPIOZ_5 }, 1 },
	{ "eth_rxd2",		REG6,	9,	{ GPIOZ_6 }, 1 },
	{ "eth_rxd3",		REG6,	8,	{ GPIOZ_7 }, 1 },
	{ "eth_rgmii_tx_clk",	REG6,	7,	{ GPIOZ_8 }, 1 },
	{ "eth_tx_en",		REG6,	6,	{ GPIOZ_9 }, 1 },
	{ "eth_txd0",		REG6,	5,	{ GPIOZ_10 }, 1 },
	{ "eth_txd1",		REG6,	4,	{ GPIOZ_11 }, 1 },
	{ "eth_txd2",		REG6,	3,	{ GPIOZ_12 }, 1 },
	{ "eth_txd3",		REG6,	2,	{ GPIOZ_13 }, 1 },
	{ "spi_ss0",		REG5,	26,	{ GPIOZ_7 }, 1 },
	{ "spi_sclk",		REG5,	27,	{ GPIOZ_6 }, 1 },
	{ "spi_miso",		REG5,	28,	{ GPIOZ_12 }, 1 },
	{ "spi_mosi",		REG5,	29,	{ GPIOZ_13 }, 1 },

	/* GPIOH */
	{ "hdmi_hpd",		REG1,	26,	{ GPIOH_0 }, 1 },
	{ "hdmi_sda",		REG1,	25,	{ GPIOH_1 }, 1 },
	{ "hdmi_scl",		REG1,	24,	{ GPIOH_2 }, 1 },

	/* GPIODV */
	{ "uart_tx_b",		REG2,	29,	{ GPIODV_24 }, 1 },
	{ "uart_rx_b",		REG2,	28,	{ GPIODV_25 }, 1 },
	{ "uart_cts_b",		REG2,	27,	{ GPIODV_26 }, 1 },
	{ "uart_rts_b",		REG2,	26,	{ GPIODV_27 }, 1 },
	{ "pwm_b",		REG3,	21,	{ GPIODV_29 }, 1 },
	{ "pwm_d",		REG3,	20,	{ GPIODV_28 }, 1 },
	{ "i2c_sck_a",		REG7,	27,	{ GPIODV_25 }, 1 },
	{ "i2c_sda_a",		REG7,	26,	{ GPIODV_24 }, 1 },
	{ "i2c_sck_b",		REG7,	25,	{ GPIODV_27 }, 1 },
	{ "i2c_sda_b",		REG7,	24,	{ GPIODV_26 }, 1 },
	{ "i2c_sck_c",		REG7,	23,	{ GPIODV_29 }, 1 },
	{ "i2c_sda_c",		REG7,	22,	{ GPIODV_28 }, 1 },

	/* BOOT */
	{ "emmc_nand_d07",	REG4,	30,	{ BOOT_0, BOOT_1, BOOT_2, BOOT_3, BOOT_4, BOOT_5, BOOT_6, BOOT_7 }, 8 },
	{ "emmc_clk",		REG4,	18,	{ BOOT_8 }, 1 },
	{ "emmc_cmd",		REG4,	19,	{ BOOT_10 }, 1 },
	{ "emmc_ds",		REG4,	31,	{ BOOT_15 }, 1 },
	{ "nor_d",		REG5,	1,	{ BOOT_11 }, 1 },
	{ "nor_q",		REG5,	3,	{ BOOT_12 }, 1 },
	{ "nor_c",		REG5,	2,	{ BOOT_13 }, 1 },
	{ "nor_cs",		REG5,	0,	{ BOOT_15 }, 1 },
	{ "nand_ce0",		REG4,	26,	{ BOOT_8 }, 1 },
	{ "nand_ce1",		REG4,	27,	{ BOOT_9 }, 1 },
	{ "nand_rb0",		REG4,	25,	{ BOOT_10 }, 1 },
	{ "nand_ale",		REG4,	24,	{ BOOT_11 }, 1 },
	{ "nand_cle",		REG4,	23,	{ BOOT_12 }, 1 },
	{ "nand_wen_clk",	REG4,	22,	{ BOOT_13 }, 1 },
	{ "nand_ren_wr",	REG4,	21,	{ BOOT_14 }, 1 },
	{ "nand_dqs",		REG4,	20,	{ BOOT_15 }, 1 },

	/* CARD */
	{ "sdcard_d1",		REG2,	14,	{ CARD_0 }, 1 },
	{ "sdcard_d0",		REG2,	15,	{ CARD_1 }, 1 },
	{ "sdcard_d3",		REG2,	12,	{ CARD_4 }, 1 },
	{ "sdcard_d2",		REG2,	13,	{ CARD_5 }, 1 },
	{ "sdcard_cmd",		REG2,	10,	{ CARD_3 }, 1 },
	{ "sdcard_clk",		REG2,	11,	{ CARD_2 }, 1 },
};

static const struct meson_pinctrl_group mesongxbb_aobus_groups[] = {
	/* GPIOAO */
	{ "uart_tx_ao_b",	AOREG0,	24,	{ GPIOAO_4 }, 1 },
	{ "uart_rx_ao_b",	AOREG0,	25,	{ GPIOAO_5 }, 1 },
	{ "uart_tx_ao_a",	AOREG0,	12,	{ GPIOAO_0 }, 1 },
	{ "uart_rx_ao_a",	AOREG0,	11,	{ GPIOAO_1 }, 1 },
	{ "uart_cts_ao_a",	AOREG0,	10,	{ GPIOAO_2 }, 1 },
	{ "uart_rts_ao_a",	AOREG0,	9,	{ GPIOAO_3 }, 1 },
	{ "uart_cts_ao_b",	AOREG0,	8,	{ GPIOAO_2 }, 1 },
	{ "uart_rts_ao_b",	AOREG0,	7,	{ GPIOAO_3 }, 1 },
	{ "i2c_sck_ao",		AOREG0,	6,	{ GPIOAO_4 }, 1 },
	{ "i2c_sda_ao",		AOREG0,	5,	{ GPIOAO_5 }, 1 },
	{ "i2c_slave_sck_ao",	AOREG0,	2,	{ GPIOAO_4 }, 1 },
	{ "i2c_slave_sda_ao",	AOREG0,	1,	{ GPIOAO_5 }, 1 },
	{ "remote_input_ao",	AOREG0,	0,	{ GPIOAO_7 }, 1 },
	{ "pwm_ao_a_3",		AOREG0,	22,	{ GPIOAO_3 }, 1 },
	{ "pwm_ao_a_6",		AOREG0,	18,	{ GPIOAO_6 }, 1 },
	{ "pwm_ao_a_12",	AOREG0,	17,	{ GPIOAO_12 }, 1 },
	{ "pwm_ao_b",		AOREG0,	3,	{ GPIOAO_13 }, 1 },
	{ "i2s_am_clk",		AOREG0,	30,	{ GPIOAO_8 }, 1 },
	{ "i2s_out_ao_clk",	AOREG0,	29,	{ GPIOAO_9 }, 1 },
	{ "i2s_out_lr_clk",	AOREG0,	28,	{ GPIOAO_10 }, 1 },
	{ "i2s_out_ch01_ao",	AOREG0,	27,	{ GPIOAO_11 }, 1 },
	{ "i2s_out_ch23_ao",	AOREG1,	0,	{ GPIOAO_12 }, 1 },
	{ "i2s_out_ch45_ao",	AOREG1,	1,	{ GPIOAO_13 }, 1 },
	{ "spdif_out_ao_6",	AOREG0,	16,	{ GPIOAO_6 }, 1 },
	{ "spdif_out_ao_13",	AOREG0,	4,	{ GPIOAO_13 }, 1 },
	{ "ao_cec",		AOREG0,	15,	{ GPIOAO_12 }, 1 },
	{ "ee_cec",		AOREG0,	14,	{ GPIOAO_12 }, 1 },

	/* TEST_N */
	{ "i2s_out_ch67_ao",	AOREG1,	2,	{ GPIO_TEST_N }, 1 },

};

const struct meson_pinctrl_config mesongxbb_periphs_pinctrl_config = {
	.name = "Meson GXBB periphs GPIO",
	.groups = mesongxbb_periphs_groups,
	.ngroups = __arraycount(mesongxbb_periphs_groups),
	.gpios = mesongxbb_periphs_gpios,
	.ngpios = __arraycount(mesongxbb_periphs_gpios),
};

const struct meson_pinctrl_config mesongxbb_aobus_pinctrl_config = {
	.name = "Meson GXBB AO GPIO",
	.groups = mesongxbb_aobus_groups,
	.ngroups = __arraycount(mesongxbb_aobus_groups),
	.gpios = mesongxbb_aobus_gpios,
	.ngpios = __arraycount(mesongxbb_aobus_gpios),
};
