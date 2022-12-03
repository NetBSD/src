/*	$NetBSD: bmx280var.h,v 1.1 2022/12/03 01:04:43 brad Exp $	*/

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

#ifndef _DEV_IC_BMX280VAR_H_
#define _DEV_IC_BMX280VAR_H_

#define BMX280_NUM_SENSORS	3
#define BMX280_TEMP_SENSOR 0
#define BMX280_PRESSURE_SENSOR 1
#define BMX280_HUMIDITY_SENSOR 2

struct bmx280_calibration_blob {
	uint16_t dig_T1;
	int16_t dig_T2;
	int16_t dig_T3;

	uint16_t dig_P1;
	int16_t dig_P2;
	int16_t dig_P3;
	int16_t dig_P4;
	int16_t dig_P5;
	int16_t dig_P6;
	int16_t dig_P7;
	int16_t dig_P8;
	int16_t dig_P9;
	uint8_t dig_H1;
	int16_t dig_H2;
	uint8_t dig_H3;
	int16_t dig_H4;
	int16_t dig_H5;
	int8_t dig_H6;
};

struct bmx280_sc {
	int 		sc_bmx280debug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	struct spi_handle *sc_sh;
	kmutex_t 	sc_mutex;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	struct sysctllog *sc_bmx280log;
	envsys_data_t 	sc_sensors[BMX280_NUM_SENSORS];
	struct bmx280_calibration_blob	sc_cal_blob;
	bool		sc_has_humidity;
	int		sc_readattempts;
	int		sc_osrs_t;
	int		sc_osrs_p;
	int		sc_osrs_h;
	int		sc_irr_samples;
	uint8_t		sc_previous_irr;
	bool		sc_bmx280dump;
	int		sc_waitfactor_t;
	int		sc_waitfactor_p;
	int		sc_waitfactor_h;
	void		(*sc_func_attach)(struct bmx280_sc *);
	int		(*sc_func_acquire_bus)(struct bmx280_sc *);
	void		(*sc_func_release_bus)(struct bmx280_sc *);
	int		(*sc_func_read_register)(struct bmx280_sc *, uint8_t, uint8_t *, size_t);
	int		(*sc_func_write_register)(struct bmx280_sc *, uint8_t *, size_t);
};

struct bmx280_sensor {
	const char     *desc;
	enum envsys_units type;
};

struct bmx280_osrs_list {
	const int	text;
	uint8_t		mask;
};

struct bmx280_irr_list {
	const int	text;
	uint8_t		mask;
};

#endif
