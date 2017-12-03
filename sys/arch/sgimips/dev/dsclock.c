/*	$NetBSD: dsclock.c,v 1.5.12.1 2017/12/03 11:36:41 jdolecek Exp $	*/

/*
 * Copyright (c) 2001 Rafal K. Boni
 * Copyright (c) 2001 Christopher Sekiya
 * Copyright (c) 1998, 1999, 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Portions of this code are derived from software contributed to The
 * NetBSD Foundation by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: dsclock.c,v 1.5.12.1 2017/12/03 11:36:41 jdolecek Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sys/bus.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/machtype.h>

#include <dev/clock_subr.h>
#include <dev/ic/ds1286reg.h>

#include <sgimips/sgimips/clockvar.h>

struct dsclock_softc {
	device_t sc_dev;

	struct todr_chip_handle sc_todrch;

	/* RTC registers */
	bus_space_tag_t sc_rtct;
	bus_space_handle_t sc_rtch;
};

static int	dsclock_match(device_t, cfdata_t, void *);
static void	dsclock_attach(device_t, device_t, void *);
static int	dsclock_gettime_ymdhms(struct todr_chip_handle *,
		    struct clock_ymdhms *);
static int	dsclock_settime_ymdhms(struct todr_chip_handle *,
		    struct clock_ymdhms *);

CFATTACH_DECL_NEW(dsclock, sizeof(struct dsclock_softc),
    dsclock_match, dsclock_attach, NULL, NULL);

#define	ds1286_write(sc, reg, datum)					\
    bus_space_write_1((sc)->sc_rtct, (sc)->sc_rtch, (reg << 2) + 3, (datum))
#define	ds1286_read(sc, reg)						\
    bus_space_read_1((sc)->sc_rtct, (sc)->sc_rtch, (reg << 2) + 3)

static int
dsclock_match(device_t parent, cfdata_t cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (mach_type == MACH_SGI_IP22 && ma->ma_addr == 0x1fbe0000)
		return 1;

	return 0;
}

static void
dsclock_attach(device_t parent, device_t self, void *aux)
{
	struct dsclock_softc *sc = device_private(self);
	struct mainbus_attach_args *ma = aux;
	int err;

	aprint_normal("\n");

	sc->sc_rtct = normal_memt;
	if ((err = bus_space_map(sc->sc_rtct, ma->ma_addr, 0x1ffff,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_rtch)) != 0) {
		aprint_error_dev(self,
		    "unable to map RTC registers, error = %d\n", err);
		return;
	}

	sc->sc_todrch.cookie = sc;
	sc->sc_todrch.todr_gettime_ymdhms = dsclock_gettime_ymdhms;
	sc->sc_todrch.todr_settime_ymdhms = dsclock_settime_ymdhms;
	sc->sc_todrch.todr_setwen = NULL;

	todr_attach(&sc->sc_todrch);
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static int
dsclock_gettime_ymdhms(struct todr_chip_handle *todrch, struct clock_ymdhms *dt)
{
	struct dsclock_softc *sc = todrch->cookie;
	ds1286_todregs regs;
	int s;

	s = splhigh();
	DS1286_GETTOD(sc, &regs)
	splx(s);

	dt->dt_sec = bcdtobin(regs[DS1286_SEC]);
	dt->dt_min = bcdtobin(regs[DS1286_MIN]);

	if (regs[DS1286_HOUR] & DS1286_HOUR_12MODE) {
		dt->dt_hour =
		    bcdtobin(regs[DS1286_HOUR] & DS1286_HOUR_12HR_MASK) 
		    + ((regs[DS1286_HOUR] & DS1286_HOUR_12HR_PM) ? 12 : 0);

		/*
		 * In AM/PM mode, hour range is 01-12, so adding in 12 hours
		 * for PM gives us 01-24, whereas we want 00-23, so map hour
		 * 24 to hour 0.
		 */
		if (dt->dt_hour == 24)
			dt->dt_hour = 0;
	} else {
		 dt->dt_hour =
		    bcdtobin(regs[DS1286_HOUR] & DS1286_HOUR_24HR_MASK);
	}

	dt->dt_wday = bcdtobin(regs[DS1286_DOW]);
	dt->dt_day = bcdtobin(regs[DS1286_DOM]);
	dt->dt_mon = bcdtobin(regs[DS1286_MONTH] & DS1286_MONTH_MASK);
	dt->dt_year = FROM_IRIX_YEAR(bcdtobin(regs[DS1286_YEAR]));

	return 0;
}

/*
 * Reset the TODR based on the time value.
 */
static int
dsclock_settime_ymdhms(struct todr_chip_handle *todrch, struct clock_ymdhms *dt)
{
	struct dsclock_softc *sc = todrch->cookie;
	ds1286_todregs regs;
	int s;

	s = splhigh();
	DS1286_GETTOD(sc, &regs);
	splx(s);

	regs[DS1286_SUBSEC] = 0;
	regs[DS1286_SEC] = bintobcd(dt->dt_sec);
	regs[DS1286_MIN] = bintobcd(dt->dt_min);
	regs[DS1286_HOUR] = bintobcd(dt->dt_hour) & DS1286_HOUR_24HR_MASK;
	regs[DS1286_DOW] = bintobcd(dt->dt_wday);
	regs[DS1286_DOM] = bintobcd(dt->dt_day);

	/* Leave wave-generator bits as set originally */
	regs[DS1286_MONTH] &=  ~DS1286_MONTH_MASK;
	regs[DS1286_MONTH] |=  bintobcd(dt->dt_mon) & DS1286_MONTH_MASK;

	regs[DS1286_YEAR] = bintobcd(TO_IRIX_YEAR(dt->dt_year));

	s = splhigh();
	DS1286_PUTTOD(sc, &regs);
	splx(s);

	return 0;
}
