/*	$NetBSD: emcfanvar.h,v 1.1 2025/03/11 13:56:46 brad Exp $	*/

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

#ifndef _DEV_I2C_EMCFANVAR_H_
#define _DEV_I2C_EMCFANVAR_H_

#include <sys/gpio.h>
#include <dev/gpio/gpiovar.h>

#define EMCFAN_NUM_FANS 5
#define EMCFAN_NUM_SENSORS 8 /* Up to 5 tachs, or 8 temperature zones (1 internal, 4 external,
			      * 1 VIN4) and 2 tachs and some combos in between.
			      */

struct emcfan_sc;

#define EMCFAN_INTERNAL_TEMP 0x01
#define EMCFAN_VIN4_TEMP 0x02
#define EMCFAN_MAX_PINS 6

/* This supplements the envsys_data_t structure stored in the sc. */

struct emcfan_sensor_instance {
	uint8_t sc_i_flags;
	int	sc_i_member;
	int	sc_i_envnum;
};

struct emcfan_sc {
	int 		sc_emcfandebug;
	struct sysctllog *sc_emcfanlog;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	int		sc_info_index;
	bool		sc_dying;
	bool		sc_opened;
	kmutex_t 	sc_mutex;
	struct sysmon_envsys *sc_sme;
	int		sc_ftach;
	bool		sc_vin4_temp;
	int		sc_num_poles[EMCFAN_NUM_FANS];
	struct emcfan_sensor_instance sc_sensor_instances[EMCFAN_NUM_SENSORS];
	envsys_data_t		sc_sensors[EMCFAN_NUM_SENSORS];
	device_t		sc_gpio_dev;
	struct gpio_chipset_tag	sc_gpio_gc;
	gpio_pin_t		sc_gpio_pins[EMCFAN_MAX_PINS];
};

#endif
