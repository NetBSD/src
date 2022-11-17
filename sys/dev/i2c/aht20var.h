/*	$NetBSD: aht20var.h,v 1.1 2022/11/17 19:20:06 brad Exp $	*/

/*
 * Copyright (c) 2022 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_I2C_AHT20VAR_H_
#define _DEV_I2C_AHT20VAR_H_

#define AHT20_NUM_SENSORS	2
#define AHT20_HUMIDITY_SENSOR 0
#define AHT20_TEMP_SENSOR 1

struct aht20_sc {
	int 		sc_aht20debug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t 	sc_mutex;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	struct sysctllog *sc_aht20log;
	envsys_data_t 	sc_sensors[AHT20_NUM_SENSORS];
	bool 		sc_ignorecrc;
	int 		sc_readattempts;
};

struct aht20_sensor {
	const char     *desc;
	enum envsys_units type;
};

struct aht20_timing {
	uint8_t cmd;
	int     typicaldelay;
};

#endif
