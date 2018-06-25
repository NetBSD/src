/*	$NetBSD: pcf8563.c,v 1.8.2.1 2018/06/25 07:25:50 pgoyette Exp $	*/

/*
 * Copyright (c) 2011 Jonathan A. Kollasch
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/* XXX */
#if defined(__arm__) || defined(__aarch64__)
#include "opt_fdt.h"
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pcf8563.c,v 1.8.2.1 2018/06/25 07:25:50 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <dev/clock_subr.h>

#include <dev/i2c/i2cvar.h>
#include <dev/i2c/pcf8563reg.h>

#ifdef FDT
#include <dev/fdt/fdtvar.h>
#endif

static const char *compatible[] = {
	"nxp,pcf8563",
	"pcf8563rtc",
	NULL
};

static const struct device_compatible_entry pcf8563rtc_compat_data[] = {
	DEVICE_COMPAT_ENTRY(compatible),
	DEVICE_COMPAT_TERMINATOR
};

struct pcf8563rtc_softc {
	device_t sc_dev;
	i2c_tag_t sc_tag;
	int sc_addr;
	struct todr_chip_handle sc_todr;
};

static int pcf8563rtc_match(device_t, cfdata_t, void *);
static void pcf8563rtc_attach(device_t, device_t, void *);

CFATTACH_DECL_NEW(pcf8563rtc, sizeof(struct pcf8563rtc_softc),
    pcf8563rtc_match, pcf8563rtc_attach, NULL, NULL);

static int pcf8563rtc_clock_read(struct pcf8563rtc_softc *, struct clock_ymdhms *);
static int pcf8563rtc_clock_write(struct pcf8563rtc_softc *, struct clock_ymdhms *);
static int pcf8563rtc_gettime(struct todr_chip_handle *, struct clock_ymdhms *);
static int pcf8563rtc_settime(struct todr_chip_handle *, struct clock_ymdhms *);

static int
pcf8563rtc_match(device_t parent, cfdata_t cf, void *aux)
{
	struct i2c_attach_args *ia = aux;
	int match_result;

	if (iic_use_direct_match(ia, cf, pcf8563rtc_compat_data, &match_result))
		return match_result;

	/* indirect config - check typical address */
	if (ia->ia_addr == PCF8563_ADDR)
		return I2C_MATCH_ADDRESS_ONLY;

	return 0;
}

static void
pcf8563rtc_attach(device_t parent, device_t self, void *aux)
{
	struct pcf8563rtc_softc *sc = device_private(self);
	struct i2c_attach_args *ia = aux;

	aprint_naive(": Real-time Clock\n");
	aprint_normal(": NXP PCF8563 Real-time Clock\n");

	sc->sc_dev = self;
	sc->sc_tag = ia->ia_tag;
	sc->sc_addr = ia->ia_addr;
	sc->sc_todr.cookie = sc;
	sc->sc_todr.todr_gettime_ymdhms = pcf8563rtc_gettime;
	sc->sc_todr.todr_settime_ymdhms = pcf8563rtc_settime;
	sc->sc_todr.todr_setwen = NULL;

	iic_acquire_bus(sc->sc_tag, I2C_F_POLL);
	iic_smbus_write_byte(sc->sc_tag, sc->sc_addr, PCF8563_R_CS1, 0,
	    I2C_F_POLL);
	iic_smbus_write_byte(sc->sc_tag, sc->sc_addr, PCF8563_R_CS2, 0,
	    I2C_F_POLL);
	iic_release_bus(sc->sc_tag, I2C_F_POLL);

#ifdef FDT
	fdtbus_todr_attach(self, ia->ia_cookie, &sc->sc_todr);
#else
	todr_attach(&sc->sc_todr);
#endif
}

static int
pcf8563rtc_gettime(struct todr_chip_handle *ch, struct clock_ymdhms *dt)
{
	struct pcf8563rtc_softc *sc = ch->cookie;
	
	if (pcf8563rtc_clock_read(sc, dt) == 0)
		return -1;

	return 0;
}

static int
pcf8563rtc_settime(struct todr_chip_handle *ch, struct clock_ymdhms *dt)
{
        struct pcf8563rtc_softc *sc = ch->cookie;

	if (pcf8563rtc_clock_write(sc, dt) == 0)
		return -1;

	return 0;
}

static int
pcf8563rtc_clock_read(struct pcf8563rtc_softc *sc, struct clock_ymdhms *dt)
{
	uint8_t bcd[PCF8563_NREGS];
	uint8_t reg = PCF8563_R_SECOND;
	const int flags = I2C_F_POLL;

	if (iic_acquire_bus(sc->sc_tag, flags)) {
		device_printf(sc->sc_dev, "acquire bus for read failed\n");
		return 0;
	}

	if (iic_exec(sc->sc_tag, I2C_OP_READ_WITH_STOP, sc->sc_addr, &reg, 1,
		     &bcd[reg], PCF8563_R_YEAR - reg + 1, flags)) {
		iic_release_bus(sc->sc_tag, flags);
		device_printf(sc->sc_dev, "read failed\n");
		return 0;
	}

	iic_release_bus(sc->sc_tag, flags);

	if (bcd[PCF8563_R_SECOND] & PCF8563_M_VL)
		return 0;

	dt->dt_sec = bcdtobin(bcd[PCF8563_R_SECOND] & PCF8563_M_SECOND);
	dt->dt_min = bcdtobin(bcd[PCF8563_R_MINUTE] & PCF8563_M_MINUTE);
	dt->dt_hour = bcdtobin(bcd[PCF8563_R_HOUR] & PCF8563_M_HOUR);
	dt->dt_day = bcdtobin(bcd[PCF8563_R_DAY] & PCF8563_M_DAY);
	dt->dt_wday = bcdtobin(bcd[PCF8563_R_WEEKDAY] & PCF8563_M_WEEKDAY);
	dt->dt_mon = bcdtobin(bcd[PCF8563_R_MONTH] & PCF8563_M_MONTH);
	dt->dt_year = 1900 +
	    (bcdtobin(bcd[PCF8563_R_YEAR] & PCF8563_M_YEAR) % 100);
	if ((bcd[PCF8563_R_MONTH] & PCF8563_M_CENTURY) == 0)
		dt->dt_year += 100;

	return 1;
}

static int
pcf8563rtc_clock_write(struct pcf8563rtc_softc *sc, struct clock_ymdhms *dt)
{
	uint8_t bcd[PCF8563_NREGS];
	uint8_t reg = PCF8563_R_SECOND;
	const int flags = I2C_F_POLL;

	bcd[PCF8563_R_SECOND] = bintobcd(dt->dt_sec);
	bcd[PCF8563_R_MINUTE] = bintobcd(dt->dt_min);
	bcd[PCF8563_R_HOUR] = bintobcd(dt->dt_hour);
	bcd[PCF8563_R_DAY] = bintobcd(dt->dt_day);
	bcd[PCF8563_R_WEEKDAY] = bintobcd(dt->dt_wday);
	bcd[PCF8563_R_MONTH] = bintobcd(dt->dt_mon);
	bcd[PCF8563_R_YEAR] = bintobcd(dt->dt_year % 100);
	if (dt->dt_year < 2000)
		bcd[PCF8563_R_MONTH] |= PCF8563_M_CENTURY;

	if (iic_acquire_bus(sc->sc_tag, flags)) {
		device_printf(sc->sc_dev, "acquire bus for write failed\n");
		return 0;
	}

	if (iic_exec(sc->sc_tag, I2C_OP_WRITE_WITH_STOP, sc->sc_addr, &reg, 1,
		     &bcd[reg], PCF8563_R_YEAR - reg + 1, flags)) {
		iic_release_bus(sc->sc_tag, flags);
		device_printf(sc->sc_dev, "write failed\n");
		return 0;
	}

	iic_release_bus(sc->sc_tag, flags);

	return 1;
}
