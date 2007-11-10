/*	$NetBSD: ds1672.c,v 1.1.2.1 2007/11/10 02:57:01 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy. All rights reserved.
 *
 * Based on dev/i2c/max6900.c,
 * Copyright (c) 2003 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Steve C. Woodford and Jason R. Thorpe for Wasabi Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
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
#include <dev/i2c/ds1672reg.h>

struct ds1672rtc_softc {
	struct device sc_dev;
	i2c_tag_t sc_tag;
	int sc_address;
	int sc_open;
	struct todr_chip_handle sc_todr;
};

static int  ds1672rtc_match(struct device *, struct cfdata *, void *);
static void ds1672rtc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(ds1672rtc, sizeof(struct ds1672rtc_softc),
	ds1672rtc_match, ds1672rtc_attach, NULL, NULL);

static int ds1672rtc_clock_read(struct ds1672rtc_softc *sc, time_t *t);
static int ds1672rtc_clock_write(struct ds1672rtc_softc *sc, time_t t);
static int ds1672rtc_gettime(struct todr_chip_handle *, volatile struct timeval *);
static int ds1672rtc_settime(struct todr_chip_handle *, volatile struct timeval *);

int
ds1672rtc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct i2c_attach_args *ia = aux;

	if ((ia->ia_addr & DS1672_ADDRMASK) == DS1672_ADDR)
		return (1);

	return (0);
}

void
ds1672rtc_attach(struct device *parent, struct device *self, void *aux)
{
	struct ds1672rtc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;

	aprint_naive(": Real-time Clock\n");
	aprint_normal(": DS1672 Real-time Clock\n");

	sc->sc_open = 0;

	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime = ds1672rtc_gettime;
	sc->sc_todr.todr_settime = ds1672rtc_settime;
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

static int
ds1672rtc_gettime(struct todr_chip_handle *ch, volatile struct timeval *tv)
{
	struct ds1672rtc_softc *sc = ch->cookie;
	time_t t;

	if (ds1672rtc_clock_read(sc, &t) == 0)
		return (-1);

	tv->tv_sec = t;
	tv->tv_usec = 0;

	return (0);
}

static int
ds1672rtc_settime(struct todr_chip_handle *ch, volatile struct timeval *tv)
{
	struct ds1672rtc_softc *sc = ch->cookie;

	if (ds1672rtc_clock_write(sc, tv->tv_sec) == 0)
		return (-1);

	return (0);
}

/*
 * While the DS1672 has a nice Clock Burst Read/Write command,
 * we can't use it, since some I2C controllers do not support
 * anything other than single-byte transfers.
 */

static int
ds1672rtc_clock_read(struct ds1672rtc_softc *sc, time_t *t)
{
	uint8_t buf[4];
	u_int8_t reg;
	time_t cntr;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: ds1672rtc_clock_read: failed to acquire I2C bus\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	/* read all registers: */
	reg = DS1672_REG_CNTR1;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_address, &reg, 1,
		     buf, 4, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: ds1672rtc_clock_read: failed to read rtc\n",
		       sc->sc_dev.dv_xname);
		return (0);
	}

	/* Done with I2C */
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	cntr = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	*t = cntr;

	printf("%s: cntr=0x%08"PRIX32"\n", sc->sc_dev.dv_xname, cntr);

	return (1);
}

static int
ds1672rtc_clock_write(struct ds1672rtc_softc *sc, time_t t)
{
	uint8_t buf[1 + DS1672_REG_CONTROL + 1];

	buf[0] = DS1672_REG_CNTR1;
	buf[1+DS1672_REG_CNTR1] = t & 0xff;
	buf[1+DS1672_REG_CNTR2] = (t >> 8) & 0xff;
	buf[1+DS1672_REG_CNTR3] = (t >> 16) & 0xff;
	buf[1+DS1672_REG_CNTR4] = (t >> 24) & 0xff;
	buf[1+DS1672_REG_CONTROL] = 0;	// make sure clock is running

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		printf("%s: ds1672rtc_clock_write: failed to acquire I2C bus\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	/* send data */
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE, sc->sc_address,
		     &buf, sizeof(buf), NULL, 0, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		printf("%s: ds1672rtc_clock_write: failed to set time\n",
		    sc->sc_dev.dv_xname);
		return (0);
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return (1);
}
