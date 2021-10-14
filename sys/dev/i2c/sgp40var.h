/*	$NetBSD: sgp40var.h,v 1.1 2021/10/14 13:54:46 brad Exp $	*/

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

#ifndef _DEV_I2C_SGP40VAR_H_
#define _DEV_I2C_SGP40VAR_H_

#define SGP40_NUM_SENSORS	1
#define SGP40_VOC_SENSOR 0

#define SGP40_DEFAULT_TEMP_COMP 25
#define SGP40_DEFAULT_RH_COMP 50


struct sgp40_sc {
	int 		sc_sgp40debug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t        sc_threadmutex; /* for the measurement kthread */
	kmutex_t 	sc_mutex; /* for reading the i2c bus */
	kcondvar_t	sc_condvar;
	struct lwp      *sc_thread;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	struct sysctllog *sc_sgp40log;
	envsys_data_t 	sc_sensors[SGP40_NUM_SENSORS];
	uint16_t        sc_voc;
	bool            sc_vocvalid;
	bool 		sc_ignorecrc;
	int 		sc_readattempts;
	bool            sc_stopping;
	int             sc_tempcomp;
	int             sc_rhcomp;
};

struct sgp40_sensor {
	const char     *desc;
	enum envsys_units type;
};

struct sgp40_timing {
	uint16_t cmd;
	int     typicaldelay;
};

#endif
