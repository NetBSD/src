/* $NetBSD: mesongxl_pinctrl.c,v 1.1 2019/04/19 19:07:56 jmcneill Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: mesongxl_pinctrl.c,v 1.1 2019/04/19 19:07:56 jmcneill Exp $");

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
 * GPIO banks. The values must match those in dt-bindings/gpio/meson-gxl-gpio.h
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
	GPIOH_9,

	BOOT_0 = 26,
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

	CARD_0 = 42,
	CARD_1,
	CARD_2,
	CARD_3,
	CARD_4,
	CARD_5,
	CARD_6,

	GPIODV_0 = 49,
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

	GPIOX_0 = 79,
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

	GPIOCLK_0 = 98,
	GPIOCLK_1,

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

static const struct meson_pinctrl_gpio mesongxl_periphs_gpios[] = {
	/* GPIODV */
	CBUS_GPIO(GPIODV_24, 0, 24),
	CBUS_GPIO(GPIODV_25, 0, 25),
	CBUS_GPIO(GPIODV_26, 0, 26),
	CBUS_GPIO(GPIODV_27, 0, 27),
	CBUS_GPIO(GPIODV_28, 0, 28),
	CBUS_GPIO(GPIODV_29, 0, 29),

	/* GPIOH */
	CBUS_GPIO(GPIOH_0, 1, 20),
	CBUS_GPIO(GPIOH_1, 1, 21),
	CBUS_GPIO(GPIOH_2, 1, 22),
	CBUS_GPIO(GPIOH_3, 1, 23),
	CBUS_GPIO(GPIOH_4, 1, 24),
	CBUS_GPIO(GPIOH_5, 1, 25),
	CBUS_GPIO(GPIOH_6, 1, 26),
	CBUS_GPIO(GPIOH_7, 1, 27),
	CBUS_GPIO(GPIOH_8, 1, 28),
	CBUS_GPIO(GPIOH_9, 1, 29),

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

	/* CARD */
	CBUS_GPIO(CARD_0, 2, 20),
	CBUS_GPIO(CARD_1, 2, 21),
	CBUS_GPIO(CARD_2, 2, 22),
	CBUS_GPIO(CARD_3, 2, 23),
	CBUS_GPIO(CARD_4, 2, 24),
	CBUS_GPIO(CARD_5, 2, 25),
	CBUS_GPIO(CARD_6, 2, 26),

	/* GPIOCLK */
	CBUS_GPIO(GPIOCLK_0, 3, 28),
	CBUS_GPIO(GPIOCLK_1, 3, 29),

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

static const struct meson_pinctrl_gpio mesongxl_aobus_gpios[] = {
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
};

static const struct meson_pinctrl_group mesongxl_periphs_groups[] = {
	/* GPIOX */
	{ "sdio_d0",		REG5,	31,	{ GPIOX_0 }, 1 },
	{ "sdio_d1",		REG5,	30,	{ GPIOX_1 }, 1 },
	{ "sdio_d2",		REG5,	29,	{ GPIOX_2 }, 1 },
	{ "sdio_d3",		REG5,	28,	{ GPIOX_3 }, 1 },
	{ "sdio_clk",		REG5,	27,	{ GPIOX_4 }, 1 },
	{ "sdio_cmd",		REG5,	26,	{ GPIOX_5 }, 1 },
	{ "sdio_irq",		REG5,	24,	{ GPIOX_7 }, 1 },
	{ "uart_tx_a",		REG5,	19,	{ GPIOX_12 }, 1 },
	{ "uart_rx_a",		REG5,	18,	{ GPIOX_13 }, 1 },
	{ "uart_cts_a",		REG5,	17,	{ GPIOX_14 }, 1 },
	{ "uart_dts_a",		REG5,	16,	{ GPIOX_15 }, 1 },
	{ "uart_tx_c",		REG5,	13,	{ GPIOX_8 }, 1 },
	{ "uart_rx_c",		REG5,	12,	{ GPIOX_9 }, 1 },
	{ "uart_cts_c",		REG5,	11,	{ GPIOX_10 }, 1 },
	{ "uart_dts_c",		REG5,	10,	{ GPIOX_11 }, 1 },
	{ "pwm_a",		REG5,	25,	{ GPIOX_6 }, 1 },
	{ "pwm_e",		REG5,	15,	{ GPIOX_16 }, 1 },
	{ "pwm_f_x",		REG5,	14,	{ GPIOX_7 }, 1 },
	{ "spi_mosi",		REG5,	3,	{ GPIOX_8 }, 1 },
	{ "spi_miso",		REG5,	2,	{ GPIOX_9 }, 1 },
	{ "spi_ss0",		REG5,	1,	{ GPIOX_10 }, 1 },
	{ "spi_sclk",		REG5,	0,	{ GPIOX_11 }, 1 },

	/* GPIOZ */
	{ "eth_mdio",		REG4,	23,	{ GPIOZ_0 }, 1 },
	{ "eth_mdc",		REG4,	22,	{ GPIOZ_1 }, 1 },
	{ "eth_clk_rx_clk",	REG4,	21,	{ GPIOZ_2 }, 1 },
	{ "eth_rx_dv",		REG4,	20,	{ GPIOZ_3 }, 1 },
	{ "eth_rxd0",		REG4,	19,	{ GPIOZ_4 }, 1 },
	{ "eth_rxd1",		REG4,	18,	{ GPIOZ_5 }, 1 },
	{ "eth_rxd2",		REG4,	17,	{ GPIOZ_6 }, 1 },
	{ "eth_rxd3",		REG4,	16,	{ GPIOZ_7 }, 1 },
	{ "eth_rgmii_tx_clk",	REG4,	15,	{ GPIOZ_8 }, 1 },
	{ "eth_tx_en",		REG4,	14,	{ GPIOZ_9 }, 1 },
	{ "eth_txd0",		REG4,	13,	{ GPIOZ_10 }, 1 },
	{ "eth_txd1",		REG4,	12,	{ GPIOZ_11 }, 1 },
	{ "eth_txd2",		REG4,	11,	{ GPIOZ_12 }, 1 },
	{ "eth_txd3",		REG4,	10,	{ GPIOZ_13 }, 1 },
	{ "pwm_c",		REG3,	20,	{ GPIOZ_15 }, 1 },
	{ "i2s_out_ch23_z",	REG3,	26,	{ GPIOZ_5 }, 1 },
	{ "i2s_out_ch45_z",	REG3,	25,	{ GPIOZ_6 }, 1 },
	{ "i2s_out_ch67_z",	REG3,	24,	{ GPIOZ_7 }, 1 },
	{ "eth_link_led",	REG4,	25,	{ GPIOZ_14 }, 1 },
	{ "eth_act_led",	REG4,	24,	{ GPIOZ_15 }, 1 },

	/* GPIOH */
	{ "hdmi_hpd",		REG6,	31,	{ GPIOH_0 }, 1 },
	{ "hdmi_sda",		REG6,	30,	{ GPIOH_1 }, 1 },
	{ "hdmi_scl",		REG6,	29,	{ GPIOH_2 }, 1 },
	{ "i2s_am_clk",		REG6,	26,	{ GPIOH_6 }, 1 },
	{ "i2s_out_ao_clk",	REG6,	25,	{ GPIOH_7 }, 1 },
	{ "i2s_out_lr_clk",	REG6,	24,	{ GPIOH_8 }, 1 },
	{ "i2s_out_ch01",	REG6,	23,	{ GPIOH_9 }, 1 },
	{ "spdif_out_h",	REG6,	28,	{ GPIOH_4 }, 1 },

	/* GPIODV */
	{ "uart_tx_b",		REG2,	16,	{ GPIODV_24 }, 1 },
	{ "uart_rx_b",		REG2,	15,	{ GPIODV_25 }, 1 },
	{ "uart_cts_b",		REG2,	14,	{ GPIODV_26 }, 1 },
	{ "uart_rts_b",		REG2,	13,	{ GPIODV_27 }, 1 },
	{ "i2c_sda_c_dv18",	REG1,	17,	{ GPIODV_18 }, 1 },
	{ "i2c_sck_c_dv19",	REG1,	16,	{ GPIODV_19 }, 1 },
	{ "i2c_sda_a",		REG1,	15,	{ GPIODV_24 }, 1 },
	{ "i2c_sck_a",		REG1,	14,	{ GPIODV_25 }, 1 },
	{ "i2c_sda_b",		REG1,	13,	{ GPIODV_26 }, 1 },
	{ "i2c_sck_b",		REG1,	12,	{ GPIODV_27 }, 1 },
	{ "i2c_sda_c",		REG1,	11,	{ GPIODV_28 }, 1 },
	{ "i2c_sck_c",		REG1,	10,	{ GPIODV_29 }, 1 },
	{ "pwm_b",		REG2,	11,	{ GPIODV_29 }, 1 },
	{ "pwm_d",		REG2,	12,	{ GPIODV_28 }, 1 },
	{ "tsin_a_d0",		REG2,	4,	{ GPIODV_0 }, 1 },
	{ "tsin_a_dp",		REG2,	3,	{ GPIODV_1, GPIODV_2, GPIODV_3, GPIODV_4, GPIODV_5, GPIODV_6, GPIODV_7 }, 7 },
	{ "tsin_a_clk",		REG2,	2,	{ GPIODV_8 }, 1 },
	{ "tsin_a_sop",		REG2,	1,	{ GPIODV_9 }, 1 },
	{ "tsin_a_d_valid",	REG2,	0,	{ GPIODV_10 }, 1 },
	{ "tsin_a_fail",	REG1,	31,	{ GPIODV_11 }, 1 },

	/* BOOT */
	{ "emmc_nand_d07",	REG7,	31,	{ BOOT_0, BOOT_1, BOOT_2, BOOT_3, BOOT_4, BOOT_5, BOOT_6, BOOT_7 }, 8 },
	{ "emmc_clk",		REG7,	30,	{ BOOT_8 }, 1 },
	{ "emmc_cmd",		REG7,	29,	{ BOOT_10 }, 1 },
	{ "emmc_ds",		REG7,	28,	{ BOOT_15 }, 1 },
	{ "nor_d",		REG7,	13,	{ BOOT_11 }, 1 },
	{ "nor_q",		REG7,	12,	{ BOOT_12 }, 1 },
	{ "nor_c",		REG7,	11,	{ BOOT_13 }, 1 },
	{ "nor_cs",		REG7,	10,	{ BOOT_15 }, 1 },
	{ "nand_ce0",		REG7,	7,	{ BOOT_8 }, 1 },
	{ "nand_ce1",		REG7,	6,	{ BOOT_9 }, 1 },
	{ "nand_rb0",		REG7,	5,	{ BOOT_10 }, 1 },
	{ "nand_ale",		REG7,	4,	{ BOOT_11 }, 1 },
	{ "nand_cle",		REG7,	3,	{ BOOT_12 }, 1 },
	{ "nand_wen_clk",	REG7,	2,	{ BOOT_13 }, 1 },
	{ "nand_ren_wr",	REG7,	1,	{ BOOT_14 }, 1 },
	{ "nand_dqs",		REG7,	0,	{ BOOT_15 }, 1 },

	/* CARD */
	{ "sdcard_d1",		REG6,	5,	{ CARD_0 }, 1 },
	{ "sdcard_d0",		REG6,	4,	{ CARD_1 }, 1 },
	{ "sdcard_d3",		REG6,	1,	{ CARD_4 }, 1 },
	{ "sdcard_d2",		REG6,	0,	{ CARD_5 }, 1 },
	{ "sdcard_cmd",		REG6,	2,	{ CARD_3 }, 1 },
	{ "sdcard_clk",		REG6,	3,	{ CARD_2 }, 1 },

	/* GPIOCLK */
	{ "pwm_f_clk",		REG8,	30,	{ GPIOCLK_1 }, 1 },
};

static const struct meson_pinctrl_group mesongxl_aobus_groups[] = {
	/* GPIOAO */
	{ "uart_tx_ao_b_0",	AOREG0,	26,	{ GPIOAO_0 }, 1 },
	{ "uart_rx_ao_b_1",	AOREG0,	25,	{ GPIOAO_1 }, 1 },
	{ "uart_tx_ao_b",	AOREG0,	24,	{ GPIOAO_4 }, 1 },
	{ "uart_rx_ao_b",	AOREG0,	23,	{ GPIOAO_5 }, 1 },
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
	{ "pwm_ao_b_6",		AOREG0,	18,	{ GPIOAO_6 }, 1 },
	{ "pwm_ao_a_8",		AOREG0,	17,	{ GPIOAO_8 }, 1 },
	{ "pwm_ao_b",		AOREG0,	3,	{ GPIOAO_9 }, 1 },
	{ "i2s_out_ch23_ao",	AOREG1,	0,	{ GPIOAO_8 }, 1 },
	{ "i2s_out_ch45_ao",	AOREG1,	1,	{ GPIOAO_9 }, 1 },
	{ "spdif_out_ao_6",	AOREG0,	16,	{ GPIOAO_6 }, 1 },
	{ "spdif_out_ao_9",	AOREG0,	4,	{ GPIOAO_9 }, 1 },
	{ "ao_cec",		AOREG0,	15,	{ GPIOAO_8 }, 1 },
	{ "ee_cec",		AOREG0,	14,	{ GPIOAO_8 }, 1 },

	/* TEST_N */
	{ "i2s_out_ch67_ao",	AOREG1,	2,	{ GPIO_TEST_N }, 1 },

};

const struct meson_pinctrl_config mesongxl_periphs_pinctrl_config = {
	.name = "Meson GXL periphs GPIO",
	.groups = mesongxl_periphs_groups,
	.ngroups = __arraycount(mesongxl_periphs_groups),
	.gpios = mesongxl_periphs_gpios,
	.ngpios = __arraycount(mesongxl_periphs_gpios),
};

const struct meson_pinctrl_config mesongxl_aobus_pinctrl_config = {
	.name = "Meson GXL AO GPIO",
	.groups = mesongxl_aobus_groups,
	.ngroups = __arraycount(mesongxl_aobus_groups),
	.gpios = mesongxl_aobus_gpios,
	.ngpios = __arraycount(mesongxl_aobus_gpios),
};
