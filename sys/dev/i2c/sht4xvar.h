/*	$NetBSD: sht4xvar.h,v 1.1 2021/10/03 17:27:02 brad Exp $	*/

/*
 * Copyright (c) 2021 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_I2C_SHT4XVAR_H_
#define _DEV_I2C_SHT4XVAR_H_

#define SHT4X_NUM_SENSORS	2
#define SHT4X_HUMIDITY_SENSOR 0
#define SHT4X_TEMP_SENSOR 1

#define SHT4X_RES_NAME 7
#define SHT4X_PULSE_NAME 6

struct sht4x_sc {
	int 		sc_sht4xdebug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t 	sc_mutex;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	struct sysctllog *sc_sht4xlog;
	envsys_data_t 	sc_sensors[SHT4X_NUM_SENSORS];
	bool 		sc_ignorecrc;
	char 		sc_resolution[SHT4X_RES_NAME];
	int 		sc_readattempts;
	bool 		sc_heateron;
	int 		sc_heaterval;
	char            sc_heaterpulse[SHT4X_PULSE_NAME];
};

struct sht4x_sensor {
	const char     *desc;
	enum envsys_units type;
};

struct sht4x_timing {
	uint8_t cmd;
	int     typicaldelay;
};

struct sht4x_resolution {
	const char     *text;
	uint8_t        cmd;
};

struct sht4x_heaterpulse {
	const char     *length;
};

struct sht4x_heateron_command {
	int            heatervalue;
	const char     *pulselength;
	uint8_t        cmd;
};

#endif
