/*	$NetBSD: mcclock_mace.c,v 1.8 2003/01/19 22:26:38 rafal Exp $	*/

/*
 * Copyright (c) 2001 Antti Kantee.  All Rights Reserved.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUASLITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1998 Mark Brinicombe.
 * Copyright (c) 1998 Causality Limited.
 * All rights reserved.
 *
 * Written by Mark Brinicombe, Causality Limited
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
 *	This product includes software developed by Mark Brinicombe
 *	for the NetBSD Project.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY CAUASLITY LIMITED ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CAUSALITY LIMITED OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/cpu.h>
#include <machine/autoconf.h>
#include <machine/bus.h>
#include <machine/machtype.h>

#include <dev/clock_subr.h>
#include <dev/ic/ds1687reg.h>

#include <sgimips/dev/macevar.h>

#include <sgimips/sgimips/clockvar.h>

struct mcclock_mace_softc {
	struct device		sc_dev;

	bus_space_tag_t		sc_st;
	bus_space_handle_t	sc_sh;
};

static int	mcclock_mace_match(struct device *, struct cfdata *, void *);
static void	mcclock_mace_attach(struct device*, struct device *, void *);

static void	mcclock_mace_init(struct device *);
static void	mcclock_mace_get(struct device *, struct clock_ymdhms *);
static void	mcclock_mace_set(struct device *, struct clock_ymdhms *);

unsigned int	ds1687_read(void *arg, unsigned int addr);
void		ds1687_write(void *arg, unsigned int addr, unsigned int data);

const struct clockfns mcclock_mace_clockfns = {
	mcclock_mace_init, mcclock_mace_get, mcclock_mace_set
};

CFATTACH_DECL(mcclock_mace, sizeof(struct mcclock_mace_softc),
    mcclock_mace_match, mcclock_mace_attach, NULL, NULL);

static int
mcclock_mace_match(struct device *parent, struct cfdata *match, void *aux)
{
		return 1;
}

void
mcclock_mace_attach(struct device *parent, struct device *self, void *aux)
{
	struct mcclock_mace_softc *sc = (void *)self;
	struct mace_attach_args *maa = aux;

	sc->sc_st = maa->maa_st;
	sc->sc_sh = maa->maa_sh;

	/* 
 	 * We want a fixed format: 24-hour, BCD data, so just force the
 	 * RTC to this mode; if there was junk there we'll be able to
 	 * fix things up later when we discover the time read back is
	 * no good.
	 */
	ds1687_write(sc, DS1687_CONTROLA, DS1687_DV1 | DS1687_BANK1);
	ds1687_write(sc, DS1687_CONTROLB, DS1687_24HRS);

	/* XXXrkb: init kickstart/wakeup stuff */

	if (!(ds1687_read(sc, DS1687_CONTROLD) & DS1687_VRT))
		printf(": lithium cell is dead, RTC unreliable");

	printf("\n");
	clockattach(&sc->sc_dev, &mcclock_mace_clockfns);
}

/*
 * We need to use the following two functions from
 * DS1687_PUT/GETTOD(). Hence the names.
 */
unsigned int
ds1687_read(void *arg, unsigned int addr)
{
	struct mcclock_mace_softc *sc = (struct mcclock_mace_softc *)arg;

	return (bus_space_read_1(sc->sc_st, sc->sc_sh, addr));
}

void
ds1687_write(void *arg, unsigned int addr, unsigned int data)
{
	struct mcclock_mace_softc *sc = (struct mcclock_mace_softc *)arg;

	bus_space_write_1(sc->sc_st, sc->sc_sh, addr, data);
}

static void
mcclock_mace_init(struct device *dev)
{
	return;
}

static void
mcclock_mace_get(struct device *dev, struct clock_ymdhms *dt)
{
	struct mcclock_mace_softc *sc = (struct mcclock_mace_softc *)dev;
	ds1687_todregs regs;
	int s;

	s = splhigh();
	DS1687_GETTOD(sc, &regs);
	splx(s);

	dt->dt_sec = FROMBCD(regs[DS1687_SOFT_SEC]);
	dt->dt_min = FROMBCD(regs[DS1687_SOFT_MIN]);
	dt->dt_hour = FROMBCD(regs[DS1687_SOFT_HOUR]);
	dt->dt_wday = FROMBCD(regs[DS1687_SOFT_DOW]);
	dt->dt_day = FROMBCD(regs[DS1687_SOFT_DOM]);
	dt->dt_mon = FROMBCD(regs[DS1687_SOFT_MONTH]);
	dt->dt_year = FROMBCD(regs[DS1687_SOFT_YEAR]) + 
			(100 * FROMBCD(regs[DS1687_SOFT_CENTURY]));
}

static void
mcclock_mace_set(struct device *dev, struct clock_ymdhms *dt)
{
	struct mcclock_mace_softc *sc = (struct mcclock_mace_softc *)dev;
	ds1687_todregs regs;
	int s;

	memset(&regs, 0, sizeof(regs));

	regs[DS1687_SOFT_SEC] = TOBCD(dt->dt_sec);
	regs[DS1687_SOFT_MIN] = TOBCD(dt->dt_min);
	regs[DS1687_SOFT_HOUR] = TOBCD(dt->dt_hour);
	regs[DS1687_SOFT_DOW] = TOBCD(dt->dt_wday);
	regs[DS1687_SOFT_DOM] = TOBCD(dt->dt_day);
	regs[DS1687_SOFT_MONTH] = TOBCD(dt->dt_mon);
	regs[DS1687_SOFT_YEAR] = TOBCD(dt->dt_year % 100);
	regs[DS1687_SOFT_CENTURY] = TOBCD(dt->dt_year / 100);
	s = splhigh();
	DS1687_PUTTOD(sc, &regs);
	splx(s);
}
