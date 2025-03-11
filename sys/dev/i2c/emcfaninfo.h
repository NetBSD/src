/*	$NetBSD: emcfaninfo.h,v 1.1 2025/03/11 13:56:46 brad Exp $	*/

/*
 * Copyright (c) 2025 Brad Spencer <brad@anduin.eldar.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _DEV_I2C_EMCFANINFO_H_
#define _DEV_I2C_EMCFANINFO_H_

#include <sys/gpio.h>

struct emcfan_chip_info {
	const int	family; /* The EMC chipset family, 210X or 230X */
	const uint8_t	product_id; /* The product ID as read from the chip */
	const char	*name; /* What we are calling this chip */
	const int	num_tachs; /* The number of tachometers */
	const int	num_fans; /* The number of fans.  This may be different than the number of tachometers. */
	const uint8_t	fan_drive_registers[5]; /* The registers used to drive the fans, one for each */
	const uint8_t	fan_divider_registers[5]; /* The divider registers, one for each possible fan */
	const bool	internal_temp_zone; /* Does the chip have an internal temperature zone */
	const int	num_external_temp_zones; /* The number of external temperature zones except for ones that are VIN4 */
	const bool	vin4_temp_zone; /* Does the chip have a VIN4 temperature zone */
	const int	num_gpio_pins; /* The number of gpio pins that this chip has */
	const int	gpio_pin_ability[6]; /* The abilities for each gpio pin */
	const char	*gpio_names[6]; /* The default names of the gpio pins */
	const uint64_t	register_void[4]; /* 4 64 bit values that specify if a particular register is valid */
};

static struct emcfan_chip_info emcfan_chip_infos[] = {
	{
		.family = EMCFAN_FAMILY_210X,
		.product_id = EMCFAN_PRODUCT_2101,
		.name = "EMC2101",
		.num_tachs = 1,
		.num_fans = 1,
		.fan_drive_registers = { EMCFAN_2101_FAN_DRIVE },
		.fan_divider_registers = { EMCFAN_2101_FAN_DIVIDE },
		.internal_temp_zone = true,
		.num_external_temp_zones = 1,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b0000000000000000000000000000001000000011110111111111111110111111,
		.register_void[1] = 0b0000000000000000000000000000000011111111111111111111111111000000,
		.register_void[2] = 0b1000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1110000000000000000000000000000000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_210X,
		.product_id = EMCFAN_PRODUCT_2101R,
		.name = "EMC2101-R",
		.num_tachs = 1,
		.num_fans = 1,
		.fan_drive_registers = { EMCFAN_2101_FAN_DRIVE },
		.fan_divider_registers = { EMCFAN_2101_FAN_DIVIDE },
		.internal_temp_zone = true,
		.num_external_temp_zones = 1,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b0000000000000000000000000000001000000011110111111111111110111111,
		.register_void[1] = 0b0000000000000000000000000000000011111111111111111111111111000000,
		.register_void[2] = 0b1000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1110000000000000000000000000000000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_210X,
		.product_id = EMCFAN_PRODUCT_2103_1,
		.name = "EMC2103-1",
		.num_tachs = 1,
		.num_fans = 1,
		.fan_drive_registers = { EMCFAN_210_346_FAN_1_DRIVE },
		.fan_divider_registers = { EMCFAN_210_346_FAN_1_DIVIDE },
		.internal_temp_zone = true,
		.num_external_temp_zones = 1,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b0001000100010001000011111111101110100010100100110011010000001111,
		.register_void[1] = 0b0000001111111111111111111111111111111111111111111111111111101111,
		.register_void[2] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1111000000000000100000000000000000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_210X,
		.product_id = EMCFAN_PRODUCT_2103_24,
		.name = "EMC2103-2/4",
		.num_tachs = 1,
		.num_fans = 1,
		.fan_drive_registers = { EMCFAN_210_346_FAN_1_DRIVE },
		.fan_divider_registers = { EMCFAN_210_346_FAN_1_DIVIDE },
		.internal_temp_zone = true,
		.num_external_temp_zones = 3,
		.vin4_temp_zone = false,
		.num_gpio_pins = 2,
		.gpio_pin_ability = {
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL
		},
		.gpio_names = { "GPIO1", "GPIO2" },
		.register_void[0] = 0b0001011100010111000011111111101110101110101101110011010011111111,
		.register_void[1] = 0b0000001111111111111111111111111111111111111111111111111111101111,
		.register_void[2] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1111000000000000100000000111111000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_210X,
		.product_id = EMCFAN_PRODUCT_2104,
		.name = "EMC2104",
		.num_tachs = 2,
		.num_fans = 2,
		.fan_drive_registers = { EMCFAN_210_346_FAN_1_DRIVE, EMCFAN_210_346_FAN_2_DRIVE },
		.fan_divider_registers = { EMCFAN_210_346_FAN_1_DIVIDE, EMCFAN_210_346_FAN_1_DIVIDE },
		.internal_temp_zone = true,
		.num_external_temp_zones = 4,
		.vin4_temp_zone = true,
		.num_gpio_pins = 3,
		.gpio_pin_ability = {
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0
		},
		.gpio_names = { "CLK_IN / GPIO1", "TACH2 / GPIO2", "PWM2 / GPIO3" },
		.register_void[0] = 0b0011111100111111000011111111111110111111111100011111011111111111,
		.register_void[1] = 0b0000001111111111111111111111111111111111111111111111111111101111,
		.register_void[2] = 0b0000001111111111111111111111111111111111111111111111111111101111,
		.register_void[3] = 0b1111000000000000100000000111111100000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_210X,
		.product_id = EMCFAN_PRODUCT_2106,
		.name = "EMC2106",
		.num_tachs = 2,
		.num_fans = 4,
		.fan_drive_registers = { EMCFAN_210_346_FAN_1_DRIVE, EMCFAN_210_346_FAN_2_DRIVE, EMCFAN_2106_FAN_3_DRIVE, EMCFAN_2106_FAN_4_DRIVE },
		.fan_divider_registers = { EMCFAN_210_346_FAN_1_DIVIDE, EMCFAN_210_346_FAN_1_DIVIDE, EMCFAN_2106_FAN_3_DIVIDE, EMCFAN_2106_FAN_4_DIVIDE },
		.internal_temp_zone = true,
		.num_external_temp_zones = 4,
		.vin4_temp_zone = true,
		.num_gpio_pins = 6,
		.gpio_pin_ability = {
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0 | GPIO_PIN_ALT1,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL | GPIO_PIN_ALT0 | GPIO_PIN_ALT1,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_OPENDRAIN | GPIO_PIN_PUSHPULL
		},
		.gpio_names = { "CLK_IN / GPIO1", "TACH2 / GPIO2", "PWM2 / GPIO3", "OVERT2 / GPIO4 / PWM3", "OVERT3 / GPIO5 / PWM4", "GPIO6" },
		.register_void[0] = 0b0011111100111111111111111111111110111111111100011111011111111111,
		.register_void[1] = 0b0000001111111111111111111111111111111111111111111111111111101111,
		.register_void[2] = 0b0000001111111111111111111111111111111111111111111111111111101111,
		.register_void[3] = 0b1111000000000000100000000111111100000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_230X,
		.product_id = EMCFAN_PRODUCT_2301,
		.name = "EMC2301",
		.num_tachs = 1,
		.num_fans = 1,
		.fan_drive_registers = { EMCFAN_230X_FAN_1_DRIVE },
		.fan_divider_registers = { EMCFAN_230X_FAN_1_DIVIDE },
		.internal_temp_zone = false,
		.num_external_temp_zones = 0,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b1111111111101111001111101111000100000000000000000000000000000000,
		.register_void[1] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[2] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1110000000000000100000000000000000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_230X,
		.product_id = EMCFAN_PRODUCT_2302,
		.name = "EMC2302",
		.num_tachs = 2,
		.num_fans = 2,
		.fan_drive_registers = { EMCFAN_230X_FAN_1_DRIVE, EMCFAN_230X_FAN_2_DRIVE },
		.fan_divider_registers = { EMCFAN_230X_FAN_1_DIVIDE, EMCFAN_230X_FAN_2_DIVIDE },
		.internal_temp_zone = false,
		.num_external_temp_zones = 0,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b1111111111101111001111101111000100000000000000000000000000000000,
		.register_void[1] = 0b0000000000000000000000000000000000000000000000001111111111101111,
		.register_void[2] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1110000000000000100000000000000000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_230X,
		.product_id = EMCFAN_PRODUCT_2303,
		.name = "EMC2303",
		.num_tachs = 3,
		.num_fans = 3,
		.fan_drive_registers = { EMCFAN_230X_FAN_1_DRIVE, EMCFAN_230X_FAN_2_DRIVE, EMCFAN_230X_FAN_3_DRIVE },
		.fan_divider_registers = { EMCFAN_230X_FAN_1_DIVIDE, EMCFAN_230X_FAN_2_DIVIDE, EMCFAN_230X_FAN_3_DIVIDE },
		.internal_temp_zone = false,
		.num_external_temp_zones = 0,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b1111111111101111001111101111000100000000000000000000000000000000,
		.register_void[1] = 0b0000000000000000000000000000000011111111111011111111111111101111,
		.register_void[2] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1111000000000000100000000000000000000000000000000000000000000000,
	},
	{
		.family = EMCFAN_FAMILY_230X,
		.product_id = EMCFAN_PRODUCT_2305,
		.name = "EMC2305",
		.num_tachs = 5,
		.num_fans = 5,
		.fan_drive_registers = { EMCFAN_230X_FAN_1_DRIVE, EMCFAN_230X_FAN_2_DRIVE, EMCFAN_230X_FAN_3_DRIVE, EMCFAN_230X_FAN_4_DRIVE, EMCFAN_230X_FAN_5_DRIVE },
		.fan_divider_registers = { EMCFAN_230X_FAN_1_DIVIDE, EMCFAN_230X_FAN_2_DIVIDE, EMCFAN_230X_FAN_3_DIVIDE, EMCFAN_230X_FAN_4_DIVIDE, EMCFAN_230X_FAN_5_DIVIDE },
		.internal_temp_zone = false,
		.num_external_temp_zones = 0,
		.vin4_temp_zone = false,
		.num_gpio_pins = 0,
		.register_void[0] = 0b1111111111101111001111101111000100000000000000000000000000000000,
		.register_void[1] = 0b1111111111101111111111111110111111111111111011111111111111101111,
		.register_void[2] = 0b0000000000000000000000000000000000000000000000000000000000000000,
		.register_void[3] = 0b1111000000000000100000000000000000000000000000000000000000000000,
	}
};

#endif
