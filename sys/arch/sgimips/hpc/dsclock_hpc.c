/*	$NetBSD: dsclock_hpc.c,v 1.1.2.3 2002/02/28 04:11:35 nathanw Exp $	*/

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

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <machine/bus.h>
#include <machine/sysconf.h>
#include <machine/machtype.h>

#include <dev/clock_subr.h>
#include <dev/ic/ds1286reg.h>

#include <sgimips/hpc/hpcvar.h>
#include <sgimips/sgimips/clockvar.h>

struct dsclock_softc {
        struct device sc_dev;

	/* RTC registers */
        bus_space_tag_t         sc_rtct;
        bus_space_handle_t      sc_rtch;
};

static void	dsclock_init(struct device *);
static int	dsclock_match(struct device *, struct cfdata *, void *);
static void	dsclock_attach(struct device *, struct device *, void *);
static void	dsclock_get(struct device *, struct clock_ymdhms *);
static void	dsclock_set(struct device *, struct clock_ymdhms *);

const struct clockfns dsclock_clockfns = {
	dsclock_init, dsclock_get, dsclock_set,
};

struct cfattach dsclock_ca = {
        sizeof(struct dsclock_softc), dsclock_match, dsclock_attach
};

#define	ds1286_write(dev, reg, datum)					\
    bus_space_write_1(dev->sc_rtct, dev->sc_rtch, reg, datum)
#define	ds1286_read(dev, reg)						\
    (bus_space_read_1(dev->sc_rtct, dev->sc_rtch, reg))

static int 
dsclock_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct hpc_attach_args *ha = aux;

	if (strcmp(ha->ha_name, cf->cf_driver->cd_name) == 0)
		return (1);

	return (0);
}

static void
dsclock_attach(struct device *parent, struct device *self, void *aux)
{
	struct dsclock_softc *sc = (void *)self;
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

	clockattach(&sc->sc_dev, &dsclock_clockfns);
}

static void
dsclock_init(struct device *dev)
{
}

/*
 * Get the time of day, based on the clock's value and/or the base value.
 */
static void
dsclock_get(struct device *dev, struct clock_ymdhms * dt)
{
        struct dsclock_softc *sc = (struct dsclock_softc *)dev;
	ds_todregs regs;
	int s;

	s = splhigh();
	DS1286_GETTOD(sc, &regs)
	splx(s);

	dt->dt_sec = FROMBCD(regs[DS_SEC]);
	dt->dt_min = FROMBCD(regs[DS_MIN]);

	if (regs[DS_HOUR] & DS_HOUR_12MODE) {
	    dt->dt_hour = FROMBCD(regs[DS_HOUR] & DS_HOUR_12HR_MASK) +
			((regs[DS_HOUR] & DS_HOUR_12HR_PM) ? 12 : 0);

	    /* 
	     * In AM/PM mode, hour range is 01-12, so adding in 12 hours
	     * for PM gives us 01-24, whereas we want 00-23, so map hour
	     * 24 to hour 0.
	     */
	    if (dt->dt_hour == 24)
		dt->dt_hour = 0;
	} else {
	    dt->dt_hour = FROMBCD(regs[DS_HOUR] & DS_HOUR_24HR_MASK);
	}

	dt->dt_wday = FROMBCD(regs[DS_DOW]);
	dt->dt_day = FROMBCD(regs[DS_DOM]);
	dt->dt_mon = FROMBCD(regs[DS_MONTH] & DS_MONTH_MASK);
	dt->dt_year = FROM_IRIX_YEAR(FROMBCD(regs[DS_YEAR]));
}

/*
 * Reset the TODR based on the time value.
 */
static void
dsclock_set(struct device *dev, struct clock_ymdhms * dt)
{
        struct dsclock_softc *sc = (struct dsclock_softc *)dev;
	ds_todregs regs;
	int s;

	s = splhigh();
	DS1286_GETTOD(sc, &regs);
	splx(s);

	regs[DS_SUBSEC] = 0;
	regs[DS_SEC] = TOBCD(dt->dt_sec);
	regs[DS_MIN] = TOBCD(dt->dt_min);
	regs[DS_HOUR] = TOBCD(dt->dt_hour) & DS_HOUR_24HR_MASK;
	regs[DS_DOW] = TOBCD(dt->dt_wday);
	regs[DS_DOM] = TOBCD(dt->dt_day);

	/* Leave wave-generator bits as set originally */
	regs[DS_MONTH] &=  ~DS_MONTH_MASK;
	regs[DS_MONTH] |=  TOBCD(dt->dt_mon) & DS_MONTH_MASK;

	regs[DS_YEAR] = TOBCD(TO_IRIX_YEAR(dt->dt_year));

	s = splhigh();
	DS1286_PUTTOD(sc, &regs);
	splx(s);
}
