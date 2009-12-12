/*	$NetBSD: dsclock.c,v 1.2 2009/12/12 14:44:09 tsutsui Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: dsclock.c,v 1.2 2009/12/12 14:44:09 tsutsui Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/machtype.h>

#include <dev/clock_subr.h>
#include <dev/ic/ds1286reg.h>

#include <sgimips/sgimips/clockvar.h>

struct dsclock_softc {
	struct device sc_dev;

	struct todr_chip_handle sc_todrch;

	/* RTC registers */
	bus_space_tag_t sc_rtct;
	bus_space_handle_t sc_rtch;
};

static int	dsclock_match(struct device *, struct cfdata *, void *);
static void	dsclock_attach(struct device *, struct device *, void *);
static int	dsclock_gettime(struct todr_chip_handle *, struct timeval *);
static int	dsclock_settime(struct todr_chip_handle *, struct timeval *);

CFATTACH_DECL(dsclock, sizeof(struct dsclock_softc),
    dsclock_match, dsclock_attach, NULL, NULL);

#define	ds1286_write(dev, reg, datum)					\
    bus_space_write_1(dev->sc_rtct, dev->sc_rtch, reg, datum)
#define	ds1286_read(dev, reg)						\
    (bus_space_read_1(dev->sc_rtct, dev->sc_rtch, reg))

static int
dsclock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	if (mach_type == MACH_SGI_IP22 && ma->ma_addr == 0x1fbe0000)
		return (1);

	return (0);
}

static void
dsclock_attach(struct device *parent, struct device *self, void *aux)
{
	struct dsclock_softc *sc = (void *)self;
	struct mainbus_attach_args *ma = aux;
	int err;

	printf("\n");

	sc->sc_rtct = SGIMIPS_BUS_SPACE_HPC;
	if ((err = bus_space_map(sc->sc_rtct, ma->ma_addr, 0x1ffff,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_rtch)) != 0) {
		printf(": unable to map RTC registers, error = %d\n", err);
		return;
	}

	sc->sc_todrch.cookie = sc;
	sc->sc_todrch.todr_gettime = dsclock_gettime;
	sc->sc_todrch.todr_settime = dsclock_settime;
	sc->sc_todrch.todr_setwen = NULL;

	todr_attach(&sc->sc_todrch);
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static int
dsclock_gettime(struct todr_chip_handle *todrch, struct timeval *tv)
{
	struct dsclock_softc *sc = (struct dsclock_softc *)todrch->cookie;
	struct clock_ymdhms dt;
	ds1286_todregs regs;
	int s;

	s = splhigh();
	DS1286_GETTOD(sc, &regs)
	splx(s);

	dt.dt_sec = FROMBCD(regs[DS1286_SEC]);
	dt.dt_min = FROMBCD(regs[DS1286_MIN]);

	if (regs[DS1286_HOUR] & DS1286_HOUR_12MODE) {
		dt.dt_hour = FROMBCD(regs[DS1286_HOUR] & DS1286_HOUR_12HR_MASK) 
			+ ((regs[DS1286_HOUR] & DS1286_HOUR_12HR_PM) ? 12 : 0);

		/*
		 * In AM/PM mode, hour range is 01-12, so adding in 12 hours
		 * for PM gives us 01-24, whereas we want 00-23, so map hour
		 * 24 to hour 0.
		 */
		if (dt.dt_hour == 24)
			dt.dt_hour = 0;
	} else {
		 dt.dt_hour= FROMBCD(regs[DS1286_HOUR] & DS1286_HOUR_24HR_MASK);
	}

	dt.dt_wday = FROMBCD(regs[DS1286_DOW]);
	dt.dt_day = FROMBCD(regs[DS1286_DOM]);
	dt.dt_mon = FROMBCD(regs[DS1286_MONTH] & DS1286_MONTH_MASK);
	dt.dt_year = FROM_IRIX_YEAR(FROMBCD(regs[DS1286_YEAR]));

	/* simple sanity checks */
	if (dt.dt_mon > 12 || dt.dt_day > 31 ||
	    dt.dt_hour >= 24 || dt.dt_min >= 60 || dt.dt_sec >= 60)
		return (EIO);

	tv->tv_sec = (long)clock_ymdhms_to_secs(&dt);
	if (tv->tv_sec == -1)
		return (ERANGE);
	tv->tv_usec = 0;

	return (0);
}

/*
 * Reset the TODR based on the time value.
 */
static int
dsclock_settime(struct todr_chip_handle *todrch, struct timeval *tv)
{
	struct dsclock_softc *sc = (struct dsclock_softc *)todrch->cookie;
	struct clock_ymdhms dt;
	ds1286_todregs regs;
	int s;

	clock_secs_to_ymdhms((time_t)(tv->tv_sec + (tv->tv_usec > 500000)),&dt);

	s = splhigh();
	DS1286_GETTOD(sc, &regs);
	splx(s);

	regs[DS1286_SUBSEC] = 0;
	regs[DS1286_SEC] = TOBCD(dt.dt_sec);
	regs[DS1286_MIN] = TOBCD(dt.dt_min);
	regs[DS1286_HOUR] = TOBCD(dt.dt_hour) & DS1286_HOUR_24HR_MASK;
	regs[DS1286_DOW] = TOBCD(dt.dt_wday);
	regs[DS1286_DOM] = TOBCD(dt.dt_day);

	/* Leave wave-generator bits as set originally */
	regs[DS1286_MONTH] &=  ~DS1286_MONTH_MASK;
	regs[DS1286_MONTH] |=  TOBCD(dt.dt_mon) & DS1286_MONTH_MASK;

	regs[DS1286_YEAR] = TOBCD(TO_IRIX_YEAR(dt.dt_year));

	s = splhigh();
	DS1286_PUTTOD(sc, &regs);
	splx(s);

	return (0);
}
