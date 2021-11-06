/*	$NetBSD: sht3xvar.h,v 1.1 2021/11/06 13:34:40 brad Exp $	*/

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

#ifndef _DEV_I2C_SHT3XVAR_H_
#define _DEV_I2C_SHT3XVAR_H_

#define SHT3X_NUM_SENSORS	2
#define SHT3X_HUMIDITY_SENSOR 0
#define SHT3X_TEMP_SENSOR 1

#define SHT3X_MODE_NAME 12
#define SHT3X_REP_NAME 7
#define SHT3X_RATE_NAME 8

struct sht3x_sc {
	int 		sc_sht3xdebug;
	device_t 	sc_dev;
	i2c_tag_t 	sc_tag;
	i2c_addr_t 	sc_addr;
	kmutex_t        sc_dying_mutex; /* for cleaning up */
	kmutex_t        sc_threadmutex; /* for the measurement kthread */
	kmutex_t 	sc_mutex; /* for reading the i2c bus */
	kmutex_t 	sc_read_mutex; /* for from the data queue */
	kcondvar_t	sc_condvar; /* for shutting down the thread */
	kcondvar_t	sc_condreadready; /* when there is data to be read */
	kcondvar_t	sc_cond_dying; /* interlock when cleaning up */
	struct lwp      *sc_thread;
	int 		sc_numsensors;
	struct sysmon_envsys *sc_sme;
	struct sysctllog *sc_sht3xlog;
	envsys_data_t 	sc_sensors[SHT3X_NUM_SENSORS];
	bool 		sc_ignorecrc;
	char		sc_mode[SHT3X_MODE_NAME];
	bool		sc_isperiodic;
	char 		sc_repeatability[SHT3X_REP_NAME];
	char		sc_periodic_rate[SHT3X_RATE_NAME];
	int 		sc_readattempts;
	bool 		sc_heateron;
	bool            sc_stopping;
	bool		sc_initperiodic;
	uint8_t		sc_pbuffer[6];
	bool		sc_dying;
	bool		sc_opened;
	bool		sc_wassingleshot;
	pool_cache_t	sc_readpool;
	char		*sc_readpoolname;
	SIMPLEQ_HEAD(,sht3x_read_q)  sc_read_queue;
};

struct sht3x_read_q {
	uint8_t		measurement[6];
	SIMPLEQ_ENTRY(sht3x_read_q) read_q;
};

struct sht3x_sensor {
	const char     *desc;
	enum envsys_units type;
};

struct sht3x_timing {
	uint16_t cmd;
	int     typicaldelay;
};

struct sht3x_periodic {
	const char	*repeatability;
	const char	*rate;
	int		sdelay;
	uint16_t        cmd;
};

struct sht3x_repeatability {
	const char     *text;
	uint16_t        cmd;
};

#endif
