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

#ifndef _DEV_I2C_AM2315VAR_H_
#define _DEV_I2C_AM2315VAR_H_

#include <sys/time.h>

#define AM2315_NUM_SENSORS	2
#define AM2315_HUMIDITY_SENSOR 0
#define AM2315_TEMP_SENSOR 1

struct am2315_sc {
	int 		sc_am2315debug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t 	sc_mutex;
	kmutex_t 	sc_waitmutex;
	kcondvar_t 	sc_condwait;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	envsys_data_t 	sc_sensors[AM2315_NUM_SENSORS];
	struct sysctllog *sc_am2315log;
	int 		sc_readcount;
	int 		sc_readticks;
};

struct am2315_sensor {
	const char     *desc;
	enum envsys_units type;
};

#endif
