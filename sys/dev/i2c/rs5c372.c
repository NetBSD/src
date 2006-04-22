/*	$NetBSD: rs5c372.c,v 1.4.6.1 2006/04/22 11:38:52 simonb Exp $	*/

/*
 * Copyright (c) 2005 Kimihiro Nonaka
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
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/uio.h>
#include <sys/conf.h>
#include <sys/event.h>

#include <dev/clock_subr.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/rs5c372reg.h>

struct rs5c372rtc_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;
	struct todr_chip_handle sc_todr;
};

static int rs5c372rtc_match(struct device *, struct cfdata *, void *);
static void rs5c372rtc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(rs5c372rtc, sizeof(struct rs5c372rtc_softc),
    rs5c372rtc_match, rs5c372rtc_attach, NULL, NULL);

static void rs5c372rtc_reg_write(struct rs5c372rtc_softc *, int, uint8_t);
static int rs5c372rtc_clock_read(struct rs5c372rtc_softc *, struct clock_ymdhms *);
static int rs5c372rtc_clock_write(struct rs5c372rtc_softc *, struct clock_ymdhms *);
static int rs5c372rtc_gettime(struct todr_chip_handle *, volatile struct timeval *);
static int rs5c372rtc_settime(struct todr_chip_handle *, volatile struct timeval *);
static int rs5c372rtc_getcal(struct todr_chip_handle *, int *);
static int rs5c372rtc_setcal(struct todr_chip_handle *, int);

static int
rs5c372rtc_match(struct device *parent, struct cfdata *cf, void *arg)
{
	struct i2c_attach_args *ia = arg;

	if (ia->ia_addr == RS5C372_ADDR)
		return (1);
	return (0);
}

static void
rs5c372rtc_attach(struct device *parent, struct device *self, void *arg)
{
	struct rs5c372rtc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = arg;

	aprint_naive(": Real-time Clock\n");
	aprint_normal(": RICOH RS5C372[AB] Real-time Clock\n");

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = rs5c372rtc_gettime;
	sc->sc_todr.todr_settime = rs5c372rtc_settime;
	sc->sc_todr.todr_getcal = rs5c372rtc_getcal;
	sc->sc_todr.todr_setcal = rs5c372rtc_setcal;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);

	/* Initialize RTC */
	rs5c372rtc_reg_write(sc, RS5C372_CONTROL2, RS5C372_CONTROL2_24HRS);
	rs5c372rtc_reg_write(sc, RS5C372_CONTROL1, 0);
}

static int
rs5c372rtc_gettime(struct todr_chip_handle *ch, volatile struct timeval *tv)
{
	struct rs5c372rtc_softc *sc = ch->cookie;
	struct clock_ymdhms dt;

	memset(&dt, 0, sizeof(dt));

	if (rs5c372rtc_clock_read(sc, &dt) == 0)
		return (-1);

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;

	return (0);
}

static int
rs5c372rtc_settime(struct todr_chip_handle *ch, volatile struct timeval *tv)
{
	struct rs5c372rtc_softc *sc = ch->cookie;
	struct clock_ymdhms dt;

	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	if (rs5c372rtc_clock_write(sc, &dt) == 0)
		return (-1);

	return (0);
}

static int
rs5c372rtc_setcal(struct todr_chip_handle *ch, int cal)
{

	return (EOPNOTSUPP);
}

static int
rs5c372rtc_getcal(struct todr_chip_handle *ch, int *cal)
{

	return (EOPNOTSUPP);
}

static void
rs5c372rtc_reg_write(struct rs5c372rtc_softc *sc, int reg, uint8_t val)
{
	uint8_t cmdbuf[2];

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: rs5c372rtc_reg_write: failed to acquire I2C bus\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	reg &= 0xf;
	cmdbuf[0] = (reg << 4);
	cmdbuf[1] = val;
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	             cmdbuf, 1, &cmdbuf[1], 1, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: rs5c372rtc_reg_write: failed to write reg%d\n",
		    sc->sc_dev.dv_xname, reg);
		return;
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);
}

static int
rs5c372rtc_clock_read(struct rs5c372rtc_softc *sc, struct clock_ymdhms *dt)
{
	uint8_t bcd[RS5C372_NRTC_REGS];
	uint8_t cmdbuf[1];

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: rs5c372rtc_clock_read: failed to acquire I2C bus\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	cmdbuf[0] = (RS5C372_SECONDS << 4);
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_address,
	             cmdbuf, 1, bcd, RS5C372_NRTC_REGS, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: rs5c372rtc_clock_read: failed to read rtc\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	/*
	 * Convert the RS5C372's register values into something useable
	 */
	dt->dt_sec = FROMBCD(bcd[RS5C372_SECONDS] & RS5C372_SECONDS_MASK);
	dt->dt_min = FROMBCD(bcd[RS5C372_MINUTES] & RS5C372_MINUTES_MASK);
	dt->dt_hour = FROMBCD(bcd[RS5C372_HOURS] & RS5C372_HOURS_24MASK);
	dt->dt_day = FROMBCD(bcd[RS5C372_DATE] & RS5C372_DATE_MASK);
	dt->dt_mon = FROMBCD(bcd[RS5C372_MONTH] & RS5C372_MONTH_MASK);
	dt->dt_year = FROMBCD(bcd[RS5C372_YEAR]) + 2000;

	return (1);
}

static int
rs5c372rtc_clock_write(struct rs5c372rtc_softc *sc, struct clock_ymdhms *dt)
{
	uint8_t bcd[RS5C372_NRTC_REGS];
	uint8_t cmdbuf[1];

	/*
	 * Convert our time representation into something the RS5C372
	 * can understand.
	 */
	bcd[RS5C372_SECONDS] = TOBCD(dt->dt_sec);
	bcd[RS5C372_MINUTES] = TOBCD(dt->dt_min);
	bcd[RS5C372_HOURS] = TOBCD(dt->dt_hour);
	bcd[RS5C372_DATE] = TOBCD(dt->dt_day);
	bcd[RS5C372_DAY] = TOBCD(dt->dt_wday);
	bcd[RS5C372_MONTH] = TOBCD(dt->dt_mon);
	bcd[RS5C372_YEAR] = TOBCD(dt->dt_year % 100);

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: rs5c372rtc_clock_write: failed to "
		    "acquire I2C bus\n", sc->sc_dev.dv_xname);
		return (0);
	}

	cmdbuf[0] = (RS5C372_SECONDS << 4);
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	             cmdbuf, 1, bcd, RS5C372_NRTC_REGS, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: rs5c372rtc_clock_write: failed to write rtc\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return (1);
}
