/*	$NetBSD: ds2413gpiovar.h,v 1.1 2026/11/18 18:04:25 brad Exp $	*/

/*
 * Copyright (c) 2026 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_GPIO_DS2413GPIOVAR_H_
#define _DEV_GPIO_DS2413GPIOVAR_H_

#include <sys/gpio.h>
#include <sys/sysctl.h>
#include <sys/types.h>

#include <dev/gpio/gpiovar.h>

#define DS2413_MAX_PINS 2

struct ds2413gpio_softc {
	device_t			sc_dv;
	void				*sc_onewire;
	u_int64_t			sc_rom;
	int				sc_dying;
	int				sc_ds2413gpiodebug;

	struct sysctllog		*sc_ds2413gpiolog;

	device_t			sc_gpio_dev;
	struct gpio_chipset_tag		sc_gpio_gc;
	gpio_pin_t			sc_gpio_pins[DS2413_MAX_PINS];
};

#endif
