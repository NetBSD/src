/*	$NetBSD: clock_hb.c,v 1.4 2003/05/09 13:36:39 tsutsui Exp $	*/

/*-
 * Copyright (C) 1999 Tsubai Masanari.  All rights reserved.
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
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>

#include <dev/clock_subr.h>
#include <machine/adrsmap.h>
#include <newsmips/newsmips/clockvar.h>
#include <newsmips/newsmips/clockreg.h>

#include <newsmips/dev/hbvar.h>

struct clock_hb_softc {
	struct device sc_dev;
	u_int32_t *sc_addr;
};

int clock_hb_match __P((struct device *, struct cfdata *, void *));
void clock_hb_attach __P((struct device *, struct device *, void *));

CFATTACH_DECL(mkclock_hb, sizeof(struct clock_hb_softc),
    clock_hb_match, clock_hb_attach, NULL, NULL);

static void clockinit __P((struct device *));
static void clockget __P((struct device *, struct clock_ymdhms *));
static void clockset __P((struct device *, struct clock_ymdhms *));

struct clockfns clockfns_hb = {
	clockinit, clockget, clockset
};

int
clock_hb_match(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct hb_attach_args *ha = aux;

	if (strcmp(ha->ha_name, "mkclock") != 0)
		return 0;

	return 1;
}

void
clock_hb_attach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct clock_hb_softc *sc = (void *)self;

	printf("\n");
	clockattach(&sc->sc_dev, &clockfns_hb);
}

void
clockinit(dev)
	struct device *dev;
{
	/*
	 * Start the real-time clock.
	 */
	*(char *)ITIMER = IOCLOCK / 6144 / 100 - 1;

	/*
	 * Enable the real-time clock.
	 */
	*(char *)INTEN0 |= (char)INTEN0_TIMINT;
}

void
clockget(dev, dt)
	struct device *dev;
	struct clock_ymdhms *dt;
{
	volatile u_char *rtc_port = (u_char *)RTC_PORT;
	volatile u_char *rtc_data = (u_char *)DATA_PORT;
	u_int8_t x;
	int s;

	s = splclock();
	*rtc_port = READ_CLOCK;
	x = *rtc_data++; dt->dt_sec  = FROMBCD(x);
	x = *rtc_data++; dt->dt_min  = FROMBCD(x);
	x = *rtc_data++; dt->dt_hour = FROMBCD(x);
	x = *rtc_data++; dt->dt_wday = x - 1;
	x = *rtc_data++; dt->dt_day  = FROMBCD(x);
	x = *rtc_data++; dt->dt_mon  = FROMBCD(x);
	x = *rtc_data++; dt->dt_year = FROMBCD(x);
	*rtc_port = 0;
	splx(s);

	dt->dt_year = dt->dt_year + (dt->dt_year >= 70 ? 1900 : 2000);
}

void
clockset(dev, dt)
	struct device *dev;
	struct clock_ymdhms *dt;
{
	volatile u_char *rtc_port = (u_char *)RTC_PORT;
	volatile u_char *rtc_data = (u_char *)DATA_PORT;
	int year, s;

	year = dt->dt_year % 100;

	s = splclock();
	*rtc_port = SET_CLOCK;
	*rtc_data++ = TOBCD(dt->dt_sec);
	*rtc_data++ = TOBCD(dt->dt_min);
	*rtc_data++ = TOBCD(dt->dt_hour);
	*rtc_data++ = dt->dt_wday + 1;
	*rtc_data++ = TOBCD(dt->dt_day);
	*rtc_data++ = TOBCD(dt->dt_mon);
	*rtc_data++ = TOBCD(year);
	*rtc_port = 0;
	splx(s);
}
