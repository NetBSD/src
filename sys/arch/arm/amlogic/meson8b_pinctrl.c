/* $NetBSD: meson8b_pinctrl.c,v 1.3 2022/06/23 03:36:00 brook Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: meson8b_pinctrl.c,v 1.3 2022/06/23 03:36:00 brook Exp $");

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
#define	REG		0x00

/*
 * GPIO banks. The values must match those in dt-bindings/gpio/meson8b-gpio.h
 */
enum {
	GPIOX_0 = 0,
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
	GPIOX_16,
	GPIOX_17,
	GPIOX_18,
	GPIOX_19,
	GPIOX_20,
	GPIOX_21,

	GPIOY_0 = 18,
	GPIOY_1,
	GPIOY_3,
	GPIOY_6,
	GPIOY_7,
	GPIOY_8,
	GPIOY_9,
	GPIOY_10,
	GPIOY_11,
	GPIOY_12,
	GPIOY_13,
	GPIOY_14,

	GPIODV_9 = 30,
	GPIODV_24,
	GPIODV_25,
	GPIODV_26,
	GPIODV_27,
	GPIODV_28,
	GPIODV_29,

	GPIOH_0 = 37,
	GPIOH_1,
	GPIOH_2,
	GPIOH_3,
	GPIOH_4,
	GPIOH_5,
	GPIOH_6,
	GPIOH_7,
	GPIOH_8,
	GPIOH_9,

	CARD_0 = 47,
	CARD_1,
	CARD_2,
	CARD_3,
	CARD_4,
	CARD_5,
	CARD_6,

	BOOT_0 = 54,
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
	BOOT_18,

	DIF_0_P = 73,
	DIF_0_N,
	DIF_1_P,
	DIF_1_N,
	DIF_2_P,
	DIF_2_N,
	DIF_3_P,
	DIF_3_N,
	DIF_4_P,
	DIF_4_N,

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
	GPIO_BSD_EN,
	GPIO_TEST_N,
};

#define	CBUS_GPIO(_id, _gpiobase, _gpiobit, _pullbase, _pullbit)	\
	[_id] = {							\
		.id = (_id),						\
		.name = __STRING(_id),					\
		.oen = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_gpiobase) + 0),		\
			.mask = __BIT(_gpiobit)				\
		},							\
		.out = {						\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_gpiobase) + 1),		\
			.mask = __BIT(_gpiobit)				\
		},							\
		.in = {							\
			.type = MESON_PINCTRL_REGTYPE_GPIO,		\
			.reg = CBUS_REG((_gpiobase) + 2),		\
			.mask = __BIT(_gpiobit)				\
		},							\
		.pupden = {						\
			.type = MESON_PINCTRL_REGTYPE_PULL_ENABLE,	\
			.reg = CBUS_REG(_pullbase),			\
			.mask = __BIT(_pullbit)				\
		},							\
		.pupd = {						\
			.type = MESON_PINCTRL_REGTYPE_PULL,		\
			.reg = CBUS_REG(_pullbase),			\
			.mask = __BIT(_pullbit)				\
		},							\
	}

static const struct meson_pinctrl_gpio meson8b_cbus_gpios[] = {
	/* GPIOX */
	CBUS_GPIO(GPIOX_0, 0, 0, 4, 0),
	CBUS_GPIO(GPIOX_1, 0, 1, 4, 1),
	CBUS_GPIO(GPIOX_2, 0, 2, 4, 2),
	CBUS_GPIO(GPIOX_3, 0, 3, 4, 3),
	CBUS_GPIO(GPIOX_4, 0, 4, 4, 4),
	CBUS_GPIO(GPIOX_5, 0, 5, 4, 5),
	CBUS_GPIO(GPIOX_6, 0, 6, 4, 6),
	CBUS_GPIO(GPIOX_7, 0, 7, 4, 7),
	CBUS_GPIO(GPIOX_8, 0, 8, 4, 8),
	CBUS_GPIO(GPIOX_9, 0, 9, 4, 9),
	CBUS_GPIO(GPIOX_10, 0, 10, 4, 10),
	CBUS_GPIO(GPIOX_11, 0, 11, 4, 11),
	CBUS_GPIO(GPIOX_16, 0, 16, 4, 16),
	CBUS_GPIO(GPIOX_17, 0, 17, 4, 17),
	CBUS_GPIO(GPIOX_18, 0, 18, 4, 18),
	CBUS_GPIO(GPIOX_19, 0, 19, 4, 19),
	CBUS_GPIO(GPIOX_20, 0, 20, 4, 20),
	CBUS_GPIO(GPIOX_21, 0, 21, 4, 21),

	/* GPIOY */
	CBUS_GPIO(GPIOY_0, 3, 0, 3, 0),
	CBUS_GPIO(GPIOY_1, 3, 1, 3, 1),
	CBUS_GPIO(GPIOY_3, 3, 3, 3, 3),
	CBUS_GPIO(GPIOY_6, 3, 6, 3, 6),
	CBUS_GPIO(GPIOY_7, 3, 7, 3, 7),
	CBUS_GPIO(GPIOY_8, 3, 8, 3, 8),
	CBUS_GPIO(GPIOY_9, 3, 9, 3, 9),
	CBUS_GPIO(GPIOY_10, 3, 10, 3, 10),
	CBUS_GPIO(GPIOY_11, 3, 11, 3, 11),
	CBUS_GPIO(GPIOY_12, 3, 12, 3, 12),
	CBUS_GPIO(GPIOY_13, 3, 13, 3, 13),
	CBUS_GPIO(GPIOY_14, 3, 14, 3, 14),

	/* GPIODV */
	CBUS_GPIO(GPIODV_9, 6, 9, 0, 9),
	CBUS_GPIO(GPIODV_24, 6, 24, 0, 24),
	CBUS_GPIO(GPIODV_25, 6, 25, 0, 25),
	CBUS_GPIO(GPIODV_26, 6, 26, 0, 26),
	CBUS_GPIO(GPIODV_27, 6, 27, 0, 27),
	CBUS_GPIO(GPIODV_28, 6, 28, 0, 28),
	CBUS_GPIO(GPIODV_29, 6, 29, 0, 29),

	/* GPIOH */
	CBUS_GPIO(GPIOH_0, 9, 19, 1, 16),
	CBUS_GPIO(GPIOH_1, 9, 20, 1, 17),
	CBUS_GPIO(GPIOH_2, 9, 21, 1, 18),
	CBUS_GPIO(GPIOH_3, 9, 22, 1, 19),
	CBUS_GPIO(GPIOH_4, 9, 23, 1, 20),
	CBUS_GPIO(GPIOH_5, 9, 24, 1, 21),
	CBUS_GPIO(GPIOH_6, 9, 25, 1, 22),
	CBUS_GPIO(GPIOH_7, 9, 26, 1, 23),
	CBUS_GPIO(GPIOH_8, 9, 27, 1, 24),
	CBUS_GPIO(GPIOH_9, 9, 28, 1, 25),

	/* BOOT */
	CBUS_GPIO(BOOT_0, 9, 0, 2, 0),
	CBUS_GPIO(BOOT_1, 9, 1, 2, 1),
	CBUS_GPIO(BOOT_2, 9, 2, 2, 2),
	CBUS_GPIO(BOOT_3, 9, 3, 2, 3),
	CBUS_GPIO(BOOT_4, 9, 4, 2, 4),
	CBUS_GPIO(BOOT_5, 9, 5, 2, 5),
	CBUS_GPIO(BOOT_6, 9, 6, 2, 6),
	CBUS_GPIO(BOOT_7, 9, 7, 2, 7),
	CBUS_GPIO(BOOT_8, 9, 8, 2, 8),
	CBUS_GPIO(BOOT_9, 9, 9, 2, 9),
	CBUS_GPIO(BOOT_10, 9, 10, 2, 10),
	CBUS_GPIO(BOOT_11, 9, 11, 2, 11),
	CBUS_GPIO(BOOT_12, 9, 12, 2, 12),
	CBUS_GPIO(BOOT_13, 9, 13, 2, 13),
	CBUS_GPIO(BOOT_14, 9, 14, 2, 14),
	CBUS_GPIO(BOOT_15, 9, 15, 2, 15),
	CBUS_GPIO(BOOT_18, 9, 18, 2, 18),

	/* CARD */
	CBUS_GPIO(CARD_0, 0, 22, 2, 20),
	CBUS_GPIO(CARD_1, 0, 23, 2, 21),
	CBUS_GPIO(CARD_2, 0, 24, 2, 22),
	CBUS_GPIO(CARD_3, 0, 25, 2, 23),
	CBUS_GPIO(CARD_4, 0, 26, 2, 24),
	CBUS_GPIO(CARD_5, 0, 27, 2, 25),
	CBUS_GPIO(CARD_6, 0, 28, 2, 26),
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

static const struct meson_pinctrl_gpio meson8b_aobus_gpios[] = {
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

static const struct meson_pinctrl_group meson8b_cbus_groups[] = {
	/* GPIOX */
	{ "sd_d0_a",		REG8,	5,	{ GPIOX_0 }, 1 },
	{ "sd_d1_a",		REG8,	4,	{ GPIOX_1 }, 1 },
	{ "sd_d2_a",		REG8,	3,	{ GPIOX_2 }, 1 },
	{ "sd_d3_a",		REG8,	2,	{ GPIOX_3 }, 1 },
	{ "sdxc_d0_0_a",	REG5,	29,	{ GPIOX_4 }, 1 },
	{ "sdxc_d47_a",		REG5,	12,	{ GPIOX_4, GPIOX_5, GPIOX_6, GPIOX_7 }, 4 },
	{ "sdxc_d13_0_a",	REG5,	28,	{ GPIOX_5, GPIOX_6, GPIOX_7 }, 3 },
	{ "sd_clk_a",		REG8,	1,	{ GPIOX_8 }, 1 },
	{ "sd_cmd_a",		REG8,	0,	{ GPIOX_9 }, 1 },
	{ "xtal_32k_out",	REG3,	22,	{ GPIOX_10 }, 1 },
	{ "xtal_24m_out",	REG3,	20,	{ GPIOX_11 }, 1 },
	{ "uart_tx_b0",		REG4,	9,	{ GPIOX_16 }, 1 },
	{ "uart_rx_b0",		REG4,	8,	{ GPIOX_17 }, 1 },
	{ "uart_cts_b0",	REG4,	7,	{ GPIOX_18 }, 1 },
	{ "uart_rts_b0",	REG4,	6,	{ GPIOX_19 }, 1 },
	{ "sdxc_d0_1_a",	REG5,	14,	{ GPIOX_0 }, 1 },
	{ "sdxc_d13_1_a",	REG5,	13,	{ GPIOX_1, GPIOX_2, GPIOX_3 }, 3 },
	{ "pcm_out_a",		REG3,	30,	{ GPIOX_4 }, 1 },
	{ "pcm_in_a",		REG3,	29,	{ GPIOX_5 }, 1 },
	{ "pcm_fs_a",		REG3,	28,	{ GPIOX_6 }, 1 },
	{ "pcm_clk_a",		REG3,	27,	{ GPIOX_7 }, 1 },
	{ "sdxc_clk_a",		REG5,	11,	{ GPIOX_8 }, 1 },
	{ "sdxc_cmd_a",		REG5,	10,	{ GPIOX_9 }, 1 },
	{ "pwm_vs_0",		REG7,	31,	{ GPIOX_10 }, 1 },
	{ "pwm_e",		REG9,	19,	{ GPIOX_10 }, 1 },
	{ "pwm_vs_1",		REG7,	30,	{ GPIOX_11 }, 1 },
	{ "uart_tx_a",		REG4,	17,	{ GPIOX_4 }, 1 },
	{ "uart_rx_a",		REG4,	16,	{ GPIOX_5 }, 1 },
	{ "uart_cts_a",		REG4,	15,	{ GPIOX_6 }, 1 },
	{ "uart_rts_a",		REG4,	14,	{ GPIOX_7 }, 1 },
	{ "uart_tx_b1",		REG6,	19,	{ GPIOX_8 }, 1 },
	{ "uart_rx_b1",		REG6,	18,	{ GPIOX_9 }, 1 },
	{ "uart_cts_b1",	REG6,	17,	{ GPIOX_10 }, 1 },
	{ "uart_rts_b1",	REG6,	16,	{ GPIOX_20 }, 1 },
	{ "iso7816_0_clk",	REG5,	9,	{ GPIOX_6 }, 1 },
	{ "iso7816_0_data",	REG5,	8,	{ GPIOX_7 }, 1 },
	{ "spi_sclk_0",		REG4,	22,	{ GPIOX_8 }, 1 },
	{ "spi_miso_0",		REG4,	24,	{ GPIOX_9 }, 1 },
	{ "spi_mosi_0",		REG4,	23,	{ GPIOX_10 }, 1 },
	{ "iso7816_det",	REG4,	21,	{ GPIOX_16 }, 1 },
	{ "iso7816_reset",	REG4,	20,	{ GPIOX_17 }, 1 },
	{ "iso7816_1_clk",	REG4,	19,	{ GPIOX_18 }, 1 },
	{ "iso7816_1_data",	REG4,	18,	{ GPIOX_19 }, 1 },
	{ "spi_ss0_0",		REG4,	25,	{ GPIOX_20 }, 1 },
	{ "tsin_clk_b",		REG3,	6,	{ GPIOX_8 }, 1 },
	{ "tsin_sop_b",		REG3,	7,	{ GPIOX_9 }, 1 },
	{ "tsin_d0_b",		REG3,	8,	{ GPIOX_10 }, 1 },
	{ "pwm_b",		REG2,	3,	{ GPIOX_11 }, 1 },
	{ "i2c_sda_d0",		REG4,	5,	{ GPIOX_16 }, 1 },
	{ "i2c_sck_d0",		REG4,	4,	{ GPIOX_17 }, 1 },
	{ "tsin_d_valid_b",	REG3,	9,	{ GPIOX_20 }, 1 },

	/* GPIOY */
	{ "tsin_d_valid_a",	REG3,	2,	{ GPIOY_0 }, 1},
	{ "tsin_sop_a",		REG3,	1,	{ GPIOY_1 }, 1 },
	{ "tsin_d17_a",		REG3,	5,	{ GPIOY_6, GPIOY_7, GPIOY_10, GPIOY_11, GPIOY_12, GPIOY_13, GPIOY_14 }, 8 },
	{ "tsin_clk_a",		REG3,	0,	{ GPIOY_8 }, 1 },
	{ "tsin_d0_a",		REG3,	4,	{ GPIOY_9 }, 1 },
	{ "spdif_out_0",	REG1,	7,	{ GPIOY_3 }, 1 },
	{ "xtal_24m",		REG3,	18,	{ GPIOY_3 }, 1 },
	{ "iso7816_2_clk",	REG5,	7,	{ GPIOY_13 }, 1 },
	{ "iso7816_2_data",	REG5,	6,	{ GPIOY_14 }, 1 },

	/* GPIODV */
	{ "pwm_d",		REG3,	26,	{ GPIODV_28 }, 1 },
	{ "pwm_c0",		REG3,	25,	{ GPIODV_29 }, 1 },
	{ "pwm_vs_2",		REG7,	28,	{ GPIODV_9 }, 1 },
	{ "pwm_vs_3",		REG7,	27,	{ GPIODV_28 }, 1 },
	{ "pwm_vs_4",		REG7,	26,	{ GPIODV_29 }, 1 },
	{ "xtal24_out",		REG7,	25,	{ GPIODV_29 }, 1 },
	{ "uart_tx_c",		REG6,	23,	{ GPIODV_24 }, 1 },
	{ "uart_rx_c",		REG6,	22,	{ GPIODV_25 }, 1 },
	{ "uart_cts_c",		REG6,	21,	{ GPIODV_26 }, 1 },
	{ "uart_rts_c",		REG6,	20,	{ GPIODV_27 }, 1 },
	{ "pwm_c1",		REG3,	24,	{ GPIODV_9 }, 1 },
	{ "i2c_sda_a",		REG9,	31,	{ GPIODV_24 }, 1 },
	{ "i2c_sck_a",		REG9,	30,	{ GPIODV_25 }, 1 },
	{ "i2c_sda_b0",		REG9,	29,	{ GPIODV_26 }, 1 },
	{ "i2c_sck_b0",		REG9,	28,	{ GPIODV_27 }, 1 },
	{ "i2c_sda_c0",		REG9,	27,	{ GPIODV_28 }, 1 },
	{ "i2c_sck_c0",		REG9,	26,	{ GPIODV_29 }, 1 },

	/* GPIOH */
	{ "hdmi_hpd",		REG1,	26,	{ GPIOH_0 }, 1 },
	{ "hdmi_sda",		REG1,	25,	{ GPIOH_1 }, 1 },
	{ "hdmi_scl",		REG1,	24,	{ GPIOH_2 }, 1 },
	{ "hdmi_cec_0",		REG1,	23,	{ GPIOH_3 }, 1 },
	{ "eth_txd1_0",		REG7,	21,	{ GPIOH_5 }, 1 },
	{ "eth_txd0_0",		REG7,	20,	{ GPIOH_6 }, 1 },
	{ "clk_24m_out",	REG4,	1,	{ GPIOH_9 }, 1 },
	{ "spi_ss1",		REG8,	11,	{ GPIOH_0 }, 1 },
	{ "spi_ss2",		REG8,	12,	{ GPIOH_1 }, 1 },
	{ "spi_ss0_1",		REG9,	13,	{ GPIOH_3 }, 1 },
	{ "spi_miso_1",		REG9,	12,	{ GPIOH_4 }, 1 },
	{ "spi_mosi_1",		REG9,	11,	{ GPIOH_5 }, 1 },
	{ "spi_sclk_1",		REG9,	10,	{ GPIOH_6 }, 1 },
	{ "eth_txd3",		REG6,	13,	{ GPIOH_7 }, 1 },
	{ "eth_txd2",		REG6,	12,	{ GPIOH_8 }, 1 },
	{ "eth_tx_clk",		REG6,	11,	{ GPIOH_9 }, 1 },
	{ "i2c_sda_b1",		REG5,	27,	{ GPIOH_3 }, 1 },
	{ "i2c_sck_b1",		REG5,	26,	{ GPIOH_4 }, 1 },
	{ "i2c_sda_c1",		REG5,	25,	{ GPIOH_5 }, 1 },
	{ "i2c_sck_c1",		REG5,	24,	{ GPIOH_6 }, 1 },
	{ "i2c_sda_d1",		REG4,	3,	{ GPIOH_7 }, 1 },
	{ "i2c_sck_d1",		REG4,	2,	{ GPIOH_8 }, 1 },

	/* BOOT */
	{ "nand_io",		REG2,	26,	{ BOOT_0, BOOT_1, BOOT_2, BOOT_3, BOOT_4, BOOT_5, BOOT_6, BOOT_7 }, 8 },
	{ "nand_io_ce0",	REG2,	25,	{ BOOT_8 }, 1 },
	{ "nand_io_ce1",	REG2,	24,	{ BOOT_9 }, 1 },
	{ "nand_io_rb0",	REG2,	17,	{ BOOT_10 }, 1 },
	{ "nand_ale",		REG2,	21,	{ BOOT_11 }, 1 },
	{ "nand_cle",		REG2,	20,	{ BOOT_12 }, 1 },
	{ "nand_wen_clk",	REG2,	19,	{ BOOT_13 }, 1 },
	{ "nand_ren_clk",	REG2,	18,	{ BOOT_14 }, 1 },
	{ "nand_dqs_15",	REG2,	27,	{ BOOT_15 }, 1 },
	{ "nand_dqs_18",	REG2,	28,	{ BOOT_18 }, 1 },
	{ "sdxc_d0_c",		REG4,	30,	{ BOOT_0 }, 1 },
	{ "sdxc_d13_c",		REG4,	29,	{ BOOT_1, BOOT_2, BOOT_3 }, 3 },
	{ "sdxc_d47_c",		REG4,	28,	{ BOOT_4, BOOT_5, BOOT_6, BOOT_7 }, 4 },
	{ "sdxc_clk_c",		REG7,	19,	{ BOOT_8 }, 1 },
	{ "sdxc_cmd_c",		REG7,	18,	{ BOOT_10 }, 1 },
	{ "nor_d",		REG5,	1,	{ BOOT_11 }, 1 },
	{ "nor_q",		REG5,	3,	{ BOOT_12 }, 1 },
	{ "nor_c",		REG5,	2,	{ BOOT_13 }, 1 },
	{ "nor_cs",		REG5,	0,	{ BOOT_18 }, 1 },
	{ "sd_d0_c",		REG6,	29,	{ BOOT_0 }, 1 },
	{ "sd_d1_c",		REG6,	28,	{ BOOT_1 }, 1 },
	{ "sd_d2_c",		REG6,	27,	{ BOOT_2 }, 1 },
	{ "sd_d3_c",		REG6,	26,	{ BOOT_3 }, 1 },
	{ "sd_cmd_c",		REG6,	30,	{ BOOT_8 }, 1 },
	{ "sd_clk_c",		REG6,	31,	{ BOOT_10 }, 1 },

	/* CARD */
	{ "sd_d1_b",		REG2,	14,	{ CARD_0 }, 1 },
	{ "sd_d0_b",		REG2,	15,	{ CARD_1 }, 1 },
	{ "sd_clk_b",		REG2,	11,	{ CARD_2 }, 1 },
	{ "sd_cmd_b",		REG2,	10,	{ CARD_3 }, 1 },
	{ "sd_d3_b",		REG2,	12,	{ CARD_4 }, 1 },
	{ "sd_d2_b",		REG2,	13,	{ CARD_5 }, 1 },
	{ "sdxc_d13_b",		REG2,	6,	{ CARD_0, CARD_4, CARD_5 }, 3 },
	{ "sdxc_d0_b",		REG2,	7,	{ CARD_1 }, 1 },
	{ "sdxc_clk_b",		REG2,	5,	{ CARD_2 }, 1 },
	{ "sdxc_cmd_b",		REG2,	4,	{ CARD_3 }, 1 },

	/* DIF */
	{ "eth_rxd1",		REG6,	0,	{ DIF_0_P }, 1 },
	{ "eth_rxd0",		REG6,	1,	{ DIF_0_N }, 1 },
	{ "eth_rx_dv",		REG6,	2,	{ DIF_1_P }, 1 },
	{ "eth_rx_clk",		REG6,	3,	{ DIF_1_N }, 1 },
	{ "eth_txd0_1",		REG6,	4,	{ DIF_2_P }, 1 },
	{ "eth_txd1_1",		REG6,	5,	{ DIF_2_N }, 1 },
	{ "eth_tx_en",		REG6,	6,	{ DIF_3_P }, 1 },
	{ "eth_ref_clk",	REG6,	8,	{ DIF_3_N }, 1 },
	{ "eth_mdc",		REG6,	9,	{ DIF_4_P }, 1 },
	{ "eth_mdio_en",	REG6,	10,	{ DIF_4_N }, 1 },
	{ "eth_rxd3",		REG7,	22,	{ DIF_2_P }, 1 },
	{ "eth_rxd2",		REG7,	23,	{ DIF_2_N }, 1 },
};

static const struct meson_pinctrl_group meson8b_aobus_groups[] = {
	/* GPIOAO */
	{ "uart_tx_ao_a",	REG,	12,	{ GPIOAO_0 }, 1 },
	{ "uart_rx_ao_a",	REG,	11,	{ GPIOAO_1 }, 1 },
	{ "uart_cts_ao_a",	REG,	10,	{ GPIOAO_2 }, 1 },
	{ "uart_rts_ao_a",	REG,	9,	{ GPIOAO_3 }, 1 },
	{ "i2c_mst_sck_ao",	REG,	6,	{ GPIOAO_4 }, 1 },
	{ "i2c_mst_sda_ao",	REG,	5,	{ GPIOAO_5 }, 1 },
	{ "clk_32k_in_out",	REG,	18,	{ GPIOAO_6 }, 1 },
	{ "remote_input",	REG,	0,	{ GPIOAO_7 }, 1 },
	{ "hdmi_cec_1",		REG,	17,	{ GPIOAO_12 }, 1 },
	{ "ir_blaster",		REG,	31,	{ GPIOAO_13 }, 1 },
	{ "pwm_c2",		REG,	22,	{ GPIOAO_3 }, 1 },
	{ "i2c_sck_ao",		REG,	2,	{ GPIOAO_4 }, 1 },
	{ "i2c_sda_ao",		REG,	1,	{ GPIOAO_5 }, 1 },
	{ "ir_remote_out",	REG,	21,	{ GPIOAO_7 }, 1 },
	{ "i2s_am_clk_out",	REG,	30,	{ GPIOAO_8 }, 1 },
	{ "i2s_ao_clk_out",	REG,	29,	{ GPIOAO_9 }, 1 },
	{ "i2s_lr_clk_out",	REG,	28,	{ GPIOAO_10 }, 1 },
	{ "i2s_out_01",		REG,	27,	{ GPIOAO_11 }, 1 },
	{ "uart_tx_ao_b0",	REG,	26,	{ GPIOAO_0 }, 1 },
	{ "uart_rx_ao_b0",	REG,	25,	{ GPIOAO_1 }, 1 },
	{ "uart_cts_ao_b",	REG,	8,	{ GPIOAO_2 }, 1 },
	{ "uart_rts_ao_b",	REG,	7,	{ GPIOAO_3 }, 1 },
	{ "uart_tx_ao_b1",	REG,	24,	{ GPIOAO_4 }, 1 },
	{ "uart_rx_ao_b1",	REG,	23,	{ GPIOAO_5 }, 1 },
	{ "spdif_out_1",	REG,	16,	{ GPIOAO_6 }, 1 },
	{ "i2s_in_ch01",	REG,	13,	{ GPIOAO_6 }, 1 },
	{ "i2s_ao_clk_in",	REG,	15,	{ GPIOAO_9 }, 1 },
	{ "i2s_lr_clk_in",	REG,	14,	{ GPIOAO_10 }, 1 },
};

const struct meson_pinctrl_config meson8b_cbus_pinctrl_config = {
	.name = "Meson8b CBUS GPIO",
	.groups = meson8b_cbus_groups,
	.ngroups = __arraycount(meson8b_cbus_groups),
	.gpios = meson8b_cbus_gpios,
	.ngpios = __arraycount(meson8b_cbus_gpios),
};

const struct meson_pinctrl_config meson8b_aobus_pinctrl_config = {
	.name = "Meson8b AO GPIO",
	.groups = meson8b_aobus_groups,
	.ngroups = __arraycount(meson8b_aobus_groups),
	.gpios = meson8b_aobus_gpios,
	.ngpios = __arraycount(meson8b_aobus_gpios),
};
