/*	$NetBSD: umcpmio_info.h,v 1.1 2025/11/29 18:39:14 brad Exp $	*/

/*
 * Copyright (c) 2024, 2025 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _UMCPMIO_INFO_H_
#define _UMCPMIO_INFO_H_

#include <sys/gpio.h>
#include <dev/usb/umcpmio_io.h>

#define UMCPMIO_MAX_GPIO_PINS 9

struct umcpmio_chip_info {
	const uint16_t	usb_id;
	const int	num_gpio_pins;
	const uint32_t	gpio_pin_ability[UMCPMIO_MAX_GPIO_PINS];
	const char	*gpio_names[UMCPMIO_MAX_GPIO_PINS];
	const int	num_iic_ports;
	const int	num_spi_slaves;
};

static const struct umcpmio_chip_info umcpmio_chip_infos[] __unused = {
	{
		.usb_id = USB_PRODUCT_MICROCHIP_MCP2210,
		.num_gpio_pins = 9,
		.gpio_pin_ability = {
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3 | GPIO_PIN_ALT4 | GPIO_PIN_ALT5 | GPIO_PIN_ALT6,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_ALT3,
		},
		.gpio_names = { "GP0", "GP1", "GP2", "GP3", "GP4", "GP5", "GP6", "GP7", "GP8" },
		.num_iic_ports = 0,
		.num_spi_slaves = 8,
	},
	{
		.usb_id = USB_PRODUCT_MICROCHIP_MCP2221,
		.num_gpio_pins = 4,
		.gpio_pin_ability = {
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT1 | GPIO_PIN_ALT2 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT1 | GPIO_PIN_ALT3,
			GPIO_PIN_INPUT | GPIO_PIN_OUTPUT | GPIO_PIN_ALT0 | GPIO_PIN_ALT1 | GPIO_PIN_ALT3,
		},
		.gpio_names = { "GP0", "GP1", "GP2", "GP3" },
		.num_iic_ports = 1,
		.num_spi_slaves = 0,
	}
};

#endif
