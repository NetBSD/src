/*	$NetBSD: ds1307.c,v 1.13.4.1 2012/04/17 00:07:30 yamt Exp $	*/

/*
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ds1307.c,v 1.13.4.1 2012/04/17 00:07:30 yamt Exp $");

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
#include <dev/i2c/ds1307reg.h>

struct dsrtc_model {
	uint16_t dm_model;
	uint8_t dm_ch_reg;
	uint8_t dm_ch_value;
	uint8_t dm_rtc_start;
	uint8_t dm_rtc_size;
	uint8_t dm_nvram_start;
	uint8_t dm_nvram_size;
	uint8_t dm_flags;
#define	DSRTC_FLAG_CLOCK_HOLD	1
#define	DSRTC_FLAG_BCD		2	
};

static const struct dsrtc_model dsrtc_models[] = {
	{
		.dm_model = 1307,
		.dm_ch_reg = DSXXXX_SECONDS,
		.dm_ch_value = DS1307_SECONDS_CH,
		.dm_rtc_start = DS1307_RTC_START,
		.dm_rtc_size = DS1307_RTC_SIZE,
		.dm_nvram_start = DS1307_NVRAM_START,
		.dm_nvram_size = DS1307_NVRAM_SIZE,
		.dm_flags = DSRTC_FLAG_BCD | DSRTC_FLAG_CLOCK_HOLD,
	}, {
		.dm_model = 1339,
		.dm_rtc_start = DS1339_RTC_START,
		.dm_rtc_size = DS1339_RTC_SIZE,
		.dm_flags = DSRTC_FLAG_BCD,
	}, {
		.dm_model = 1672,
		.dm_rtc_start = DS1672_RTC_START,
		.dm_rtc_size = DS1672_RTC_SIZE,
		.dm_flags = 0,
	}, {
		.dm_model = 3232,
		.dm_rtc_start = DS3232_RTC_START,
		.dm_rtc_size = DS3232_RTC_SIZE,
		.dm_nvram_start = DS3232_NVRAM_START,
		.dm_nvram_size = DS3232_NVRAM_SIZE,
		.dm_flags = DSRTC_FLAG_BCD,
	},
};

struct dsrtc_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	uint8_t sc_address;
	bool sc_open;
	struct dsrtc_model sc_model;
	struct todr_chip_handle sc_todr;
};

static void	dsrtc_attach(device_t, device_t, void *);
static int	dsrtc_match(device_t, cfdata_t, void *);

CFATTACH_DECL_NEW(dsrtc, sizeof(struct dsrtc_softc),
    dsrtc_match, dsrtc_attach, NULL, NULL);
extern struct cfdriver dsrtc_cd;

dev_type_open(dsrtc_open);
dev_type_close(dsrtc_close);
dev_type_read(dsrtc_read);
dev_type_write(dsrtc_write);

const struct cdevsw dsrtc_cdevsw = {
	dsrtc_open, dsrtc_close, dsrtc_read, dsrtc_write, noioctl,
	nostop, notty, nopoll, nommap, nokqfilter, D_OTHER
};

static int dsrtc_gettime_ymdhms(struct todr_chip_handle *, struct clock_ymdhms *);
static int dsrtc_settime_ymdhms(struct todr_chip_handle *, struct clock_ymdhms *);
static int dsrtc_clock_read_ymdhms(struct dsrtc_softc *, struct clock_ymdhms *);
static int dsrtc_clock_write_ymdhms(struct dsrtc_softc *, struct clock_ymdhms *);

static int dsrtc_gettime_timeval(struct todr_chip_handle *, struct timeval *);
static int dsrtc_settime_timeval(struct todr_chip_handle *, struct timeval *);
static int dsrtc_clock_read_timeval(struct dsrtc_softc *, time_t *);
static int dsrtc_clock_write_timeval(struct dsrtc_softc *, time_t);

static const struct dsrtc_model *
dsrtc_model(u_int model)
{
	/* no model given, assume it's a DS1307 (the first one) */
	if (model == 0)
		return &dsrtc_models[0];

	for (const struct dsrtc_model *dm = dsrtc_models;
	     dm < dsrtc_models + __arraycount(dsrtc_models); dm++) {
		if (dm->dm_model == model)
			return dm;
	}
	return NULL;
}

static int
dsrtc_match(device_t parent, cfdata_t cf, void *arg)
{
	struct i2c_attach_args *ia = arg;

	if (ia->ia_name) {
		/* direct config - check name */
		if (strcmp(ia->ia_name, "dsrtc") == 0)
			return 1;
	} else {
		/* indirect config - check typical address */
		if (ia->ia_addr == DS1307_ADDR)
			return dsrtc_model(cf->cf_flags & 0xffff) != NULL;
	}
	return 0;
}

static void
dsrtc_attach(device_t parent, device_t self, void *arg)
{
	struct dsrtc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = arg;
	const struct dsrtc_model * const dm =
	    dsrtc_model(device_cfdata(self)->cf_flags);

	aprint_naive(": Real-time Clock%s\n",
	    dm->dm_nvram_size > 0 ? "/NVRAM" : "");
	aprint_normal(": DS%u Real-time Clock%s\n", dm->dm_model,
	    dm->dm_nvram_size > 0 ? "/NVRAM" : "");

	sc->sc_tag = ia->ia_tag;
	sc->sc_address = ia->ia_addr;
	sc->sc_model = *dm;
	sc->sc_dev = self;
	sc->sc_open = 0;
	sc->sc_todr.cookie = sc;
	if (dm->dm_flags & DSRTC_FLAG_BCD) {
		sc->sc_todr.todr_gettime_ymdhms = dsrtc_gettime_ymdhms;
		sc->sc_todr.todr_settime_ymdhms = dsrtc_settime_ymdhms;
	} else {
		sc->sc_todr.todr_gettime = dsrtc_gettime_timeval;
		sc->sc_todr.todr_settime = dsrtc_settime_timeval;
	}
	sc->sc_todr.todr_setwen = NULL;

	todr_attach(&sc->sc_todr);
}

/*ARGSUSED*/
int
dsrtc_open(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct dsrtc_softc *sc;

	if ((sc = device_lookup_private(&dsrtc_cd, minor(dev))) == NULL)
		return ENXIO;

	/* XXX: Locking */
	if (sc->sc_open)
		return EBUSY;

	sc->sc_open = true;
	return 0;
}

/*ARGSUSED*/
int
dsrtc_close(dev_t dev, int flag, int fmt, struct lwp *l)
{
	struct dsrtc_softc *sc;

	if ((sc = device_lookup_private(&dsrtc_cd, minor(dev))) == NULL)
		return ENXIO;

	sc->sc_open = false;
	return 0;
}

/*ARGSUSED*/
int
dsrtc_read(dev_t dev, struct uio *uio, int flags)
{
	struct dsrtc_softc *sc;
	int error;

	if ((sc = device_lookup_private(&dsrtc_cd, minor(dev))) == NULL)
		return ENXIO;

	const struct dsrtc_model * const dm = &sc->sc_model;
	if (uio->uio_offset >= dm->dm_nvram_size)
		return EINVAL;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	KASSERT(uio->uio_offset >= 0);
	while (uio->uio_resid && uio->uio_offset < dm->dm_nvram_size) {
		uint8_t ch, cmd;
		const u_int a = uio->uio_offset;
		cmd = a + dm->dm_nvram_start;
		if ((error = iic_exec(sc->sc_tag,
		    uio->uio_resid > 1 ? I2C_OP_READ : I2C_OP_READ_WITH_STOP,
		    sc->sc_address, &cmd, 1, &ch, 1, 0)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			aprint_error_dev(sc->sc_dev,
			    "dsrtc_read: read failed at 0x%x\n", a);
			return error;
		}
		if ((error = uiomove(&ch, 1, uio)) != 0) {
			iic_release_bus(sc->sc_tag, 0);
			return error;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	return 0;
}

/*ARGSUSED*/
int
dsrtc_write(dev_t dev, struct uio *uio, int flags)
{
	struct dsrtc_softc *sc;
	int error;

	if ((sc = device_lookup_private(&dsrtc_cd, minor(dev))) == NULL)
		return ENXIO;

	const struct dsrtc_model * const dm = &sc->sc_model;
	if (uio->uio_offset >= dm->dm_nvram_size)
		return EINVAL;

	if ((error = iic_acquire_bus(sc->sc_tag, 0)) != 0)
		return error;

	while (uio->uio_resid && uio->uio_offset < dm->dm_nvram_size) {
		uint8_t cmdbuf[2];
		const u_int a = (int)uio->uio_offset;
		cmdbuf[0] = a + dm->dm_nvram_start;
		if ((error = uiomove(&cmdbuf[1], 1, uio)) != 0)
			break;

		if ((error = iic_exec(sc->sc_tag,
		    uio->uio_resid ? I2C_OP_WRITE : I2C_OP_WRITE_WITH_STOP,
		    sc->sc_address, cmdbuf, 1, &cmdbuf[1], 1, 0)) != 0) {
			aprint_error_dev(sc->sc_dev,
			    "dsrtc_write: write failed at 0x%x\n", a);
			break;
		}
	}

	iic_release_bus(sc->sc_tag, 0);

	return error;
}

static int
dsrtc_gettime_ymdhms(struct todr_chip_handle *ch, struct clock_ymdhms *dt)
{
	struct dsrtc_softc *sc = ch->cookie;
	struct clock_ymdhms check;
	int retries;

	memset(dt, 0, sizeof(*dt));
	memset(&check, 0, sizeof(check));

	/*
	 * Since we don't support Burst Read, we have to read the clock twice
	 * until we get two consecutive identical results.
	 */
	retries = 5;
	do {
		dsrtc_clock_read_ymdhms(sc, dt);
		dsrtc_clock_read_ymdhms(sc, &check);
	} while (memcmp(dt, &check, sizeof(check)) != 0 && --retries);

	return 0;
}

static int
dsrtc_settime_ymdhms(struct todr_chip_handle *ch, struct clock_ymdhms *dt)
{
	struct dsrtc_softc *sc = ch->cookie;

	if (dsrtc_clock_write_ymdhms(sc, dt) == 0)
		return -1;

	return 0;
}

static int
dsrtc_clock_read_ymdhms(struct dsrtc_softc *sc, struct clock_ymdhms *dt)
{
	struct dsrtc_model * const dm = &sc->sc_model;
	uint8_t bcd[DSXXXX_RTC_SIZE], cmdbuf[1];

	KASSERT(DSXXXX_RTC_SIZE >= dm->dm_rtc_size);

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev,
		    "dsrtc_clock_read: failed to acquire I2C bus\n");
		return 0;
	}

	/* Read each RTC register in order. */
	for (u_int i = 0; i < dm->dm_rtc_size; i++) {
		cmdbuf[0] = dm->dm_rtc_start + i;

		if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP,
			     sc->sc_address, cmdbuf, 1,
			     &bcd[i], 1, I2C_F_POLL)) {
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			aprint_error_dev(sc->sc_dev,
			    "dsrtc_clock_read: failed to read rtc "
			    "at 0x%x\n", i);
			return 0;
		}
	}

	/* Done with I2C */
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	/*
	 * Convert the RTC's register values into something useable
	 */
	dt->dt_sec = FROMBCD(bcd[DSXXXX_SECONDS] & DSXXXX_SECONDS_MASK);
	dt->dt_min = FROMBCD(bcd[DSXXXX_MINUTES] & DSXXXX_MINUTES_MASK);

	if ((bcd[DSXXXX_HOURS] & DSXXXX_HOURS_12HRS_MODE) != 0) {
		dt->dt_hour = FROMBCD(bcd[DSXXXX_HOURS] &
		    DSXXXX_HOURS_12MASK) % 12; /* 12AM -> 0, 12PM -> 12 */
		if (bcd[DSXXXX_HOURS] & DSXXXX_HOURS_12HRS_PM)
			dt->dt_hour += 12;
	} else
		dt->dt_hour = FROMBCD(bcd[DSXXXX_HOURS] &
		    DSXXXX_HOURS_24MASK);

	dt->dt_day = FROMBCD(bcd[DSXXXX_DATE] & DSXXXX_DATE_MASK);
	dt->dt_mon = FROMBCD(bcd[DSXXXX_MONTH] & DSXXXX_MONTH_MASK);

	/* XXX: Should be an MD way to specify EPOCH used by BIOS/Firmware */
	dt->dt_year = FROMBCD(bcd[DSXXXX_YEAR]) + POSIX_BASE_YEAR;
	if (bcd[DSXXXX_MONTH] & DSXXXX_MONTH_CENTURY)
		dt->dt_year += 100;

	return 1;
}

static int
dsrtc_clock_write_ymdhms(struct dsrtc_softc *sc, struct clock_ymdhms *dt)
{
	struct dsrtc_model * const dm = &sc->sc_model;
	uint8_t bcd[DSXXXX_RTC_SIZE], cmdbuf[2];

	KASSERT(DSXXXX_RTC_SIZE >= dm->dm_rtc_size);

	/*
	 * Convert our time representation into something the DSXXXX
	 * can understand.
	 */
	bcd[DSXXXX_SECONDS] = TOBCD(dt->dt_sec);
	bcd[DSXXXX_MINUTES] = TOBCD(dt->dt_min);
	bcd[DSXXXX_HOURS] = TOBCD(dt->dt_hour); /* DSXXXX_HOURS_12HRS_MODE=0 */
	bcd[DSXXXX_DATE] = TOBCD(dt->dt_day);
	bcd[DSXXXX_DAY] = TOBCD(dt->dt_wday);
	bcd[DSXXXX_MONTH] = TOBCD(dt->dt_mon);
	bcd[DSXXXX_YEAR] = TOBCD((dt->dt_year - POSIX_BASE_YEAR) % 100);
	if (dt->dt_year - POSIX_BASE_YEAR >= 100)
		bcd[DSXXXX_MONTH] |= DSXXXX_MONTH_CENTURY;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev,
		    "dsrtc_clock_write: failed to acquire I2C bus\n");
		return 0;
	}

	/* Stop the clock */
	cmdbuf[0] = dm->dm_ch_reg;

	if (iic_exec(sc->sc_tag, I2C_OP_READ, sc->sc_address,
	    cmdbuf, 1, &cmdbuf[1], 1, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		aprint_error_dev(sc->sc_dev,
		    "dsrtc_clock_write: failed to read Hold Clock\n");
		return 0;
	}

	cmdbuf[1] |= dm->dm_ch_value;

	if (iic_exec(sc->sc_tag, I2C_OP_WRITE, sc->sc_address,
	    cmdbuf, 1, &cmdbuf[1], 1, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		aprint_error_dev(sc->sc_dev,
		    "dsrtc_clock_write: failed to write Hold Clock\n");
		return 0;
	}

	/*
	 * Write registers in reverse order. The last write (to the Seconds
	 * register) will undo the Clock Hold, above.
	 */
	uint8_t op = I2C_OP_WRITE;
	for (signed int i = dm->dm_rtc_size - 1; i >= 0; i--) {
		cmdbuf[0] = dm->dm_rtc_start + i;
		if (dm->dm_rtc_start + i == dm->dm_ch_reg) {
			op = I2C_OP_WRITE_WITH_STOP;
		}
		if (iic_exec(sc->sc_tag, op, sc->sc_address,
		    cmdbuf, 1, &bcd[i], 1, I2C_F_POLL)) {
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			aprint_error_dev(sc->sc_dev,
			    "dsrtc_clock_write: failed to write rtc "
			    " at 0x%x\n", i);
			/* XXX: Clock Hold is likely still asserted! */
			return 0;
		}
	}
	/*
	 * If the clock hold register isn't the same register as seconds,
	 * we need to reeanble the clock.
	 */
	if (op != I2C_OP_WRITE_WITH_STOP) {
		cmdbuf[0] = dm->dm_ch_reg;
		cmdbuf[1] &= ~dm->dm_ch_value;

		if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
		    cmdbuf, 1, &cmdbuf[1], 1, I2C_F_POLL)) {
			iic_release_bus(sc->sc_tag, I2C_F_POLL);
			aprint_error_dev(sc->sc_dev,
			    "dsrtc_clock_write: failed to Hold Clock\n");
			return 0;
		}
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return 1;
}

static int
dsrtc_gettime_timeval(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct dsrtc_softc *sc = ch->cookie;
	struct timeval check;
	int retries;

	memset(tv, 0, sizeof(*tv));
	memset(&check, 0, sizeof(check));

	/*
	 * Since we don't support Burst Read, we have to read the clock twice
	 * until we get two consecutive identical results.
	 */
	retries = 5;
	do {
		dsrtc_clock_read_timeval(sc, &tv->tv_sec);
		dsrtc_clock_read_timeval(sc, &check.tv_sec);
	} while (memcmp(tv, &check, sizeof(check)) != 0 && --retries);

	return 0;
}

static int
dsrtc_settime_timeval(struct todr_chip_handle *ch, struct timeval *tv)
{
	struct dsrtc_softc *sc = ch->cookie;

	if (dsrtc_clock_write_timeval(sc, tv->tv_sec) == 0)
		return -1;

	return 0;
}

/*
 * The RTC probably has a nice Clock Burst Read/Write command, but we can't use
 * it, since some I2C controllers don't support anything other than single-byte
 * transfers.
 */
static int
dsrtc_clock_read_timeval(struct dsrtc_softc *sc, time_t *tp)
{
	const struct dsrtc_model * const dm = &sc->sc_model;
	uint8_t buf[4];

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev, "%s: failed to acquire I2C bus\n",
		    __func__);
		return (0);
	}

	/* read all registers: */
	uint8_t reg = dm->dm_rtc_start;
	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_address, &reg, 1,
		     buf, 4, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		aprint_error_dev(sc->sc_dev, "%s: failed to read rtc\n",
		    __func__);
		return (0);
	}

	/* Done with I2C */
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	uint32_t v = (buf[3] << 24) | (buf[2] << 16) | (buf[1] << 8) | buf[0];
	*tp = v;

	aprint_debug_dev(sc->sc_dev, "%s: cntr=0x%08"PRIx32"\n",
	    __func__, v);

	return (1);
}

static int
dsrtc_clock_write_timeval(struct dsrtc_softc *sc, time_t t)
{
	const struct dsrtc_model * const dm = &sc->sc_model;
	size_t buflen = dm->dm_rtc_size + 2; 
	uint8_t buf[buflen];

	KASSERT((dm->dm_flags & DSRTC_FLAG_CLOCK_HOLD) == 0);
	KASSERT(dm->dm_ch_reg == dm->dm_rtc_start + 4);

	buf[0] = dm->dm_rtc_start;
	buf[1] = (t >> 0) & 0xff;
	buf[2] = (t >> 8) & 0xff;
	buf[3] = (t >> 16) & 0xff;
	buf[4] = (t >> 24) & 0xff;
	buf[5] = 0;

	if (iic_acquire_bus(sc->sc_tag, I2C_F_POLL)) {
		aprint_error_dev(sc->sc_dev, "%s: failed to acquire I2C bus\n",
		    __func__);
		return (0);
	}

	/* send data */
	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_address,
	    &buf, buflen, NULL, 0, I2C_F_POLL)) {
		iic_release_bus(sc->sc_tag, I2C_F_POLL);
		aprint_error_dev(sc->sc_dev, "%s: failed to set time\n",
		    __func__);
		return (0);
	}

	iic_release_bus(sc->sc_tag, I2C_F_POLL);

	return (1);
}
