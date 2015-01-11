/* $NetBSD: hym8563.c,v 1.1 2015/01/11 15:22:26 jmcneill Exp $ */

/*-
 * Copyright (c) 2015 Jared D. McNeill <jmcneill@invisible.ca>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: hym8563.c,v 1.1 2015/01/11 15:22:26 jmcneill Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/device.h>
#include <sys/conf.h>
#include <sys/bus.h>
#include <sys/kmem.h>

#include <dev/clock_subr.h>

#include <dev/i2c/i2cvar.h>

#define HYM_CSR1_REG		0x00
#define HYM_CSR2_REG		0x01
#define HYM_SECONDS_REG		0x02
#define HYM_MINUTES_REG		0x03
#define HYM_HOURS_REG		0x04
#define HYM_DAYS_REG		0x05
#define HYM_WEEKDAYS_REG	0x06
#define HYM_MONTHS_REG		0x07
#define HYM_YEARS_REG		0x08
#define HYM_MINUTE_ALARM_REG	0x09
#define HYM_HOUR_ALARM_REG	0x0a
#define HYM_DATE_ALARM_REG	0x0b
#define HYM_WEEKDAY_ALARM_REG	0x0c
#define HYM_CLKOUT_CTRL_REG	0x0d
#define HYM_TIMER_CTRL_REG	0x0e
#define HYM_TIMER_COUNTDOWN_REG	0x0f

#define HYM_CSR1_TEST		__BIT(7)
#define HYM_CSR1_STOP		__BIT(5)
#define HYM_CSR1_TESTC		__BIT(3)

#define HYM_CSR2_TITP		__BIT(4)
#define HYM_CSR2_AF		__BIT(3)
#define HYM_CSR2_TF		__BIT(2)
#define HYM_CSR2_AIE		__BIT(1)
#define HYM_CSR2_TIE		__BIT(0)

#define HYM_SECONDS_VL		__BIT(7)
#define HYM_SECONDS_MASK	__BITS(6,0)

#define HYM_MINUTES_MASK	__BITS(6,0)

#define HYM_HOURS_MASK		__BITS(5,0)

#define HYM_DAYS_MASK		__BITS(5,0)

#define HYM_WEEKDAYS_MASK	__BITS(2,0)

#define HYM_MONTHS_C		__BIT(7)
#define HYM_MONTHS_MASK		__BITS(4,0)

#define HYM_YEARS_MASK		__BITS(7,0)

#define HYM_TODR_OFF		HYM_SECONDS_REG
#define HYM_TODR_LEN		(HYM_YEARS_REG - HYM_SECONDS_REG + 1)
#define HYM_TODR_SECONDS	(HYM_SECONDS_REG - HYM_TODR_OFF)
#define HYM_TODR_MINUTES	(HYM_MINUTES_REG - HYM_TODR_OFF)
#define HYM_TODR_HOURS		(HYM_HOURS_REG - HYM_TODR_OFF)
#define HYM_TODR_DAYS		(HYM_DAYS_REG - HYM_TODR_OFF)
#define HYM_TODR_WEEKDAYS	(HYM_WEEKDAYS_REG - HYM_TODR_OFF)
#define HYM_TODR_MONTHS		(HYM_MONTHS_REG - HYM_TODR_OFF)
#define HYM_TODR_YEARS		(HYM_YEARS_REG - HYM_TODR_OFF)

struct hymrtc_softc {
	device_t	sc_dev;
	i2c_tag_t	sc_i2c;
	i2c_addr_t	sc_addr;

	struct todr_chip_handle sc_todr;
};

static int	hymrtc_match(device_t, cfdata_t, void *);
static void	hymrtc_attach(device_t, device_t, void *);

static int	hymrtc_rtc_gettime(todr_chip_handle_t, struct clock_ymdhms *);
static int	hymrtc_rtc_settime(todr_chip_handle_t, struct clock_ymdhms *);

CFATTACH_DECL_NEW(hymrtc, sizeof(struct hymrtc_softc),
    hymrtc_match, hymrtc_attach, NULL, NULL);

static int
hymrtc_match(device_t parent, cfdata_t match, void *aux)
{
	struct i2c_attach_args *ia = aux;
	uint8_t val;
	int error;

	iic_acquire_bus(ia->ia_tag, I2C_F_POLL);
	error = iic_smbus_read_byte(ia->ia_tag, ia->ia_addr, HYM_CSR1_REG,
	    &val, I2C_F_POLL);
	iic_release_bus(ia->ia_tag, I2C_F_POLL);

	return error == 0;
}

static void
hymrtc_attach(device_t parent, device_t self, void *aux)
{
	struct hymrtc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_dev = self;
	sc->sc_i2c = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;

	aprint_naive("\n");
	aprint_normal(": RTC\n");

	iic_acquire_bus(sc->sc_i2c, I2C_F_POLL);
	iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr, HYM_CSR1_REG, 0, 0);
	iic_smbus_write_byte(sc->sc_i2c, sc->sc_addr, HYM_CSR2_REG, 0, 0);
	iic_release_bus(sc->sc_i2c, I2C_F_POLL);

	sc->sc_todr.todr_gettime_ymdhms = hymrtc_rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = hymrtc_rtc_settime;
	sc->sc_todr.cookie = sc;
	todr_attach(&sc->sc_todr);
}

static int
hymrtc_rtc_gettime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct hymrtc_softc *sc = tch->cookie;
	uint8_t data[HYM_TODR_LEN];
	int error;

	memset(data, 0, HYM_TODR_LEN);

	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_smbus_block_read(sc->sc_i2c, sc->sc_addr,
	    HYM_TODR_OFF, data, HYM_TODR_LEN, cold ? I2C_F_POLL : 0);
	iic_release_bus(sc->sc_i2c, 0);

	if (error)
		return error;

	/* Voltage low flag, RTC data may be invalid */
	if (data[HYM_TODR_SECONDS] & HYM_SECONDS_VL)
		return ENODEV;

	dt->dt_year = 1900 + bcdtobin(data[HYM_TODR_YEARS] & HYM_YEARS_MASK);
	if (data[HYM_TODR_MONTHS] & HYM_MONTHS_C)
		dt->dt_year += 100;
	dt->dt_mon = bcdtobin(data[HYM_TODR_MONTHS] & HYM_MONTHS_MASK);
	dt->dt_day = bcdtobin(data[HYM_TODR_DAYS] & HYM_DAYS_MASK);
	dt->dt_wday = bcdtobin(data[HYM_TODR_WEEKDAYS] & HYM_WEEKDAYS_MASK);
	dt->dt_hour = bcdtobin(data[HYM_TODR_HOURS] & HYM_HOURS_MASK);
	dt->dt_min = bcdtobin(data[HYM_TODR_MINUTES] & HYM_MINUTES_MASK);
	dt->dt_sec = bcdtobin(data[HYM_TODR_SECONDS] & HYM_SECONDS_MASK);

	return 0;
}

static int
hymrtc_rtc_settime(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	struct hymrtc_softc *sc = tch->cookie;
	uint8_t data[HYM_TODR_LEN];
	int error;

	data[HYM_TODR_SECONDS] = bintobcd(dt->dt_sec) & HYM_SECONDS_MASK;
	data[HYM_TODR_MINUTES] = bintobcd(dt->dt_min) & HYM_MINUTES_MASK;
	data[HYM_TODR_HOURS] = bintobcd(dt->dt_hour) & HYM_HOURS_MASK;
	data[HYM_TODR_DAYS] = bintobcd(dt->dt_day) & HYM_DAYS_MASK;
	data[HYM_TODR_WEEKDAYS] = bintobcd(dt->dt_wday) & HYM_WEEKDAYS_MASK;
	data[HYM_TODR_MONTHS] = bintobcd(dt->dt_mon) & HYM_MONTHS_MASK;
	data[HYM_TODR_YEARS] = bintobcd(dt->dt_year % 100) & HYM_YEARS_MASK;
	if (dt->dt_year >= 2000)
		data[HYM_TODR_MONTHS] |= HYM_MONTHS_C;

	iic_acquire_bus(sc->sc_i2c, 0);
	error = iic_smbus_block_write(sc->sc_i2c, sc->sc_addr,
	    HYM_TODR_OFF, data, HYM_TODR_LEN, cold ? I2C_F_POLL : 0);
	iic_release_bus(sc->sc_i2c, 0);

	return error;
}
