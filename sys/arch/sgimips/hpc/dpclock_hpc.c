/*	$NetBSD: dpclock_hpc.c,v 1.1.4.4 2004/09/21 13:21:19 skrll Exp $	*/

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
#include <machine/sysconf.h>
#include <machine/machtype.h>

#include <dev/clock_subr.h>
#include <sgimips/hpc/dp8573areg.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/sgimips/clockvar.h>

struct dpclock_softc {
	struct device sc_dev;

	/* RTC registers */
	bus_space_tag_t		sc_rtct;
	bus_space_handle_t	sc_rtch;
};

static void	dpclock_init(struct device *);
static int	dpclock_match(struct device *, struct cfdata *, void *);
static void	dpclock_attach(struct device *, struct device *, void *);
static void	dpclock_get(struct device *, struct clock_ymdhms *);
static void	dpclock_set(struct device *, struct clock_ymdhms *);

const struct clockfns dpclock_clockfns = {
	dpclock_init, dpclock_get, dpclock_set,
};

CFATTACH_DECL(dpclock, sizeof(struct dpclock_softc),
    dpclock_match, dpclock_attach, NULL, NULL);

static int
dpclock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpc_attach_args *ha = aux;

	if (strcmp(ha->ha_name, cf->cf_name) == 0)
		return (1);

	return (0);
}

static void
dpclock_attach(struct device *parent, struct device *self, void *aux)
{
	struct dpclock_softc *sc = (void *)self;
	struct hpc_attach_args *haa = aux;
	int err;

	printf("\n");

	sc->sc_rtct = haa->ha_st;
	if ((err = bus_space_subregion(haa->ha_st, haa->ha_sh,
				       haa->ha_devoff, 0x1ffff,
				       &sc->sc_rtch)) != 0) {
		printf(": unable to map RTC registers, error = %d\n", err);
		return;
	}

	clockattach(&sc->sc_dev, &dpclock_clockfns);
}

static void
dpclock_init(struct device *dev)
{
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static void
dpclock_get(struct device *dev, struct clock_ymdhms * dt)
{
	struct dpclock_softc *sc = (struct dpclock_softc *)dev;
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

	dt->dt_sec = FROMBCD(regs[DP8573A_SAVE_SEC]);
	dt->dt_min = FROMBCD(regs[DP8573A_SAVE_MIN]);

	if (regs[DP8573A_RT_MODE] & DP8573A_RT_MODE_1224) {
		dt->dt_hour = FROMBCD(regs[DP8573A_SAVE_HOUR] &
						DP8573A_HOUR_12HR_MASK) +
		    ((regs[DP8573A_SAVE_HOUR] & DP8573A_RT_MODE_1224) ? 0 : 12);

		/*
		 * In AM/PM mode, hour range is 01-12, so adding in 12 hours
		 * for PM gives us 01-24, whereas we want 00-23, so map hour
		 * 24 to hour 0.
		 */

		if (dt->dt_hour == 24)
			dt->dt_hour = 0;
	} else {
		dt->dt_hour = FROMBCD(regs[DP8573A_SAVE_HOUR] &
							DP8573A_HOUR_24HR_MASK);
	}

	dt->dt_wday = FROMBCD(regs[DP8573A_DOW]);	/* Not from time saved */
	dt->dt_day = FROMBCD(regs[DP8573A_SAVE_DOM]);
	dt->dt_mon = FROMBCD(regs[DP8573A_SAVE_MONTH]);
	dt->dt_year = FROM_IRIX_YEAR(FROMBCD(regs[DP8573A_YEAR]));
}

/*
 * Reset the TODR based on the time value.
 */
static void
dpclock_set(struct device *dev, struct clock_ymdhms * dt)
{
	struct dpclock_softc *sc = (struct dpclock_softc *)dev;
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

	regs[DP8573A_SUBSECOND] = 0;
	regs[DP8573A_SECOND] = TOBCD(dt->dt_sec);
	regs[DP8573A_MINUTE] = TOBCD(dt->dt_min);
	regs[DP8573A_HOUR] = TOBCD(dt->dt_hour) & DP8573A_HOUR_24HR_MASK;
	regs[DP8573A_DOW] = TOBCD(dt->dt_wday);
	regs[DP8573A_DOM] = TOBCD(dt->dt_day);
	regs[DP8573A_MONTH] = TOBCD(dt->dt_mon);
	regs[DP8573A_YEAR] = TOBCD(TO_IRIX_YEAR(dt->dt_year));

	s = splhigh();
	i = bus_space_read_1(sc->sc_rtct, sc->sc_rtch, DP8573A_RT_MODE);
	j = i & ~DP8573A_RT_MODE_CLKSS;
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_RT_MODE, j);

	for (i = 0; i < 10; i++)
		bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_COUNTERS +i,
						    regs[DP8573A_COUNTERS + i]);
	
	bus_space_write_1(sc->sc_rtct, sc->sc_rtch, DP8573A_RT_MODE, i);
	splx(s);
}
