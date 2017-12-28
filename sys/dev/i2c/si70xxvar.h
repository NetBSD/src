/*
 * Copyright (c) 2017 Brad Spencer <brad@anduin.eldar.org>
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

#ifndef _DEV_I2C_SI70XXVAR_H_
#define _DEV_I2C_SI70XXVAR_H_

#define SI70XX_NUM_SENSORS	2
#define SI70XX_HUMIDITY_SENSOR 0
#define SI70XX_TEMP_SENSOR 1

#define SI70XX_RES_NAME 12

struct si70xx_sc {
	int 		sc_si70xxdebug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t 	sc_mutex;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	envsys_data_t 	sc_sensors[SI70XX_NUM_SENSORS];
	struct sysctllog *sc_si70xxlog;
	char 		sc_resolution[SI70XX_RES_NAME];
	int 		sc_resnumber;
	bool 		sc_ignorecrc;
	bool 		sc_vddok;
	bool 		sc_heateron;
	int 		sc_heaterval;
#ifdef HAVE_I2C_EXECV
	uint32_t 	sc_clockstretch;
#endif
	int 		sc_readattempts;
};

struct si70xx_sensor {
	const char     *desc;
	enum envsys_units type;
};

struct si70xx_resolution {
	const char     *text;
	int 		num;
};

#endif
