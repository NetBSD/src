/*	$NetBSD: dpclock.c,v 1.2 2009/12/12 14:44:09 tsutsui Exp $	*/

/*
 * Copyright (c) 2001 Erik Reid
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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/autoconf.h>
#include <machine/sysconf.h>
#include <machine/machtype.h>

#include <dev/clock_subr.h>
#include <sgimips/dev/dp8573areg.h>

#include <sgimips/sgimips/clockvar.h>

struct dpclock_softc {
	struct device sc_dev;

	struct todr_chip_handle sc_todrch;

	/* RTC registers */
	bus_space_tag_t		sc_rtct;
	bus_space_handle_t	sc_rtch;
};

static int	dpclock_match(struct device *, struct cfdata *, void *);
static void	dpclock_attach(struct device *, struct device *, void *);
static int	dpclock_gettime(struct todr_chip_handle *, struct timeval *);
static int	dpclock_settime(struct todr_chip_handle *, struct timeval *);

CFATTACH_DECL(dpclock, sizeof(struct dpclock_softc),
    dpclock_match, dpclock_attach, NULL, NULL);

static int
dpclock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct mainbus_attach_args *ma = aux;

	switch (mach_type) {
	case MACH_SGI_IP6 | MACH_SGI_IP10:
		if (ma->ma_addr == 0x1fbc0000)
			return (1);
		break;

	case MACH_SGI_IP12:
	case MACH_SGI_IP20:
		if (ma->ma_addr == 0x1fb80e00)
			return (1);
		break;
	}

	return (0);
}

static void
dpclock_attach(struct device *parent, struct device *self, void *aux)
{
	struct dpclock_softc *sc = (void *)self;
	struct mainbus_attach_args *ma = aux;
	int err;

	printf("\n");

	/*
	 * All machines have one byte register per word. IP6/IP10 use
	 * the MSB, others the LSB.
	 */
	if (mach_type == MACH_SGI_IP12 || mach_type == MACH_SGI_IP20)
		sc->sc_rtct = SGIMIPS_BUS_SPACE_HPC;
	else
		sc->sc_rtct = SGIMIPS_BUS_SPACE_IP6_DPCLOCK;

	if ((err = bus_space_map(sc->sc_rtct, ma->ma_addr, 0x1ffff,
	    BUS_SPACE_MAP_LINEAR, &sc->sc_rtch)) != 0) {
		printf(": unable to map RTC registers, error = %d\n", err);
		return;
	}

	sc->sc_todrch.cookie = sc;
	sc->sc_todrch.todr_gettime = dpclock_gettime;
	sc->sc_todrch.todr_settime = dpclock_settime;
	sc->sc_todrch.todr_setwen = NULL;

	todr_attach(&sc->sc_todrch);
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static int 
dpclock_gettime(struct todr_chip_handle *todrch, struct timeval *tv)
{
	struct dpclock_softc *sc = (struct dpclock_softc *)todrch->cookie;
	struct clock_ymdhms dt;
	int s;
	u_int8_t i, j;
	u_int8_t regs[32];

	s = splhigh();
	i = bus_space_read_1(sc->sc_rtct, sc->sc_rtch, DP8573A_TIMESAVE_CTL);
	j = i | DP8573A_TIMESAVE_CTL_EN; 
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_TIMESAVE_CTL, j);
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_TIMESAVE_CTL, i);
	splx(s);

	for (i = 0; i < 32; i++)
		regs[i] = bus_space_read_1(sc->sc_rtct, sc->sc_rtch, i);

	dt.dt_sec = FROMBCD(regs[DP8573A_SAVE_SEC]);
	dt.dt_min = FROMBCD(regs[DP8573A_SAVE_MIN]);

	if (regs[DP8573A_RT_MODE] & DP8573A_RT_MODE_1224) {
		dt.dt_hour = FROMBCD(regs[DP8573A_SAVE_HOUR] &
						DP8573A_HOUR_12HR_MASK) +
		    ((regs[DP8573A_SAVE_HOUR] & DP8573A_RT_MODE_1224) ? 0 : 12);

		/*
		 * In AM/PM mode, hour range is 01-12, so adding in 12 hours
		 * for PM gives us 01-24, whereas we want 00-23, so map hour
		 * 24 to hour 0.
		 */

		if (dt.dt_hour == 24)
			dt.dt_hour = 0;
	} else {
		dt.dt_hour = FROMBCD(regs[DP8573A_SAVE_HOUR] &
							DP8573A_HOUR_24HR_MASK);
	}

	dt.dt_wday = FROMBCD(regs[DP8573A_DOW]);    /* Not from time saved */
	dt.dt_day = FROMBCD(regs[DP8573A_SAVE_DOM]);
	dt.dt_mon = FROMBCD(regs[DP8573A_SAVE_MONTH]);
	dt.dt_year = FROM_IRIX_YEAR(FROMBCD(regs[DP8573A_YEAR]));

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
dpclock_settime(struct todr_chip_handle *todrch, struct timeval *tv)
{
	struct dpclock_softc *sc = (struct dpclock_softc *)todrch->cookie;
	struct clock_ymdhms dt;
	int s;
	u_int8_t i, j;
	u_int8_t regs[32];

	clock_secs_to_ymdhms((time_t)(tv->tv_sec + (tv->tv_usec > 500000)),&dt);

	s = splhigh();
	i = bus_space_read_1(sc->sc_rtct, sc->sc_rtch, DP8573A_TIMESAVE_CTL);
	j = i | DP8573A_TIMESAVE_CTL_EN; 
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_TIMESAVE_CTL, j);
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_TIMESAVE_CTL, i);
	splx(s);

	for (i = 0; i < 32; i++)
		regs[i] = bus_space_read_1(sc->sc_rtct, sc->sc_rtch, i);

	regs[DP8573A_SUBSECOND] = 0;
	regs[DP8573A_SECOND] = TOBCD(dt.dt_sec);
	regs[DP8573A_MINUTE] = TOBCD(dt.dt_min);
	regs[DP8573A_HOUR] = TOBCD(dt.dt_hour) & DP8573A_HOUR_24HR_MASK;
	regs[DP8573A_DOW] = TOBCD(dt.dt_wday);
	regs[DP8573A_DOM] = TOBCD(dt.dt_day);
	regs[DP8573A_MONTH] = TOBCD(dt.dt_mon);
	regs[DP8573A_YEAR] = TOBCD(TO_IRIX_YEAR(dt.dt_year));

	s = splhigh();
	i = bus_space_read_1(sc->sc_rtct, sc->sc_rtch, DP8573A_RT_MODE);
	j = i & ~DP8573A_RT_MODE_CLKSS;
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_RT_MODE, j);

	for (i = 0; i < 10; i++)
		bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_COUNTERS +i,
						    regs[DP8573A_COUNTERS + i]);
	
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_RT_MODE, i);
	splx(s);

	return (0);
}
