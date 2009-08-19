/*	$NetBSD: todclock.c,v 1.11.10.2 2009/08/19 18:45:59 yamt Exp $	*/

/*
 * Copyright (c) 1994-1997 Mark Brinicombe.
 * Copyright (c) 1994 Brini.
 * All rights reserved.
 *
 * This code is derived from software written for Brini by Mark Brinicombe
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
 *	This product includes software developed by Mark Brinicombe.
 * 4. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * RiscBSD kernel project
 *
 * clock.c
 *
 * Timer related machine specific code
 *
 * Created      : 29/09/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: todclock.c,v 1.11.10.2 2009/08/19 18:45:59 yamt Exp $");

/* Include header files */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/kernel.h>
#include <sys/time.h>
#include <sys/device.h>

#include <machine/rtc.h>
#include <arm/footbridge/todclockvar.h>
#include <dev/clock_subr.h>

#include "todclock.h"

#if NTODCLOCK > 1
#error "Can only had 1 todclock device"
#endif

static int tod_get_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
static int tod_set_ymdhms(todr_chip_handle_t, struct clock_ymdhms *);
 
/*
 * softc structure for the todclock device
 */

struct todclock_softc {
	struct device	sc_dev;			/* device node */
	void	*sc_rtc_arg;			/* arg to read/write */
	int	(*sc_rtc_write)(void *, rtc_t *);	/* rtc write function */
	int	(*sc_rtc_read)(void *, rtc_t *);	/* rtc read function */
};

/* prototypes for functions */

static void todclockattach(device_t parent, device_t self, void *aux);
static int todclockmatch(device_t parent, cfdata_t cf, void *aux);

/*
 * We need to remember our softc for functions like inittodr()
 * and resettodr()
 * since we only ever have one time-of-day device we can just store
 * the direct pointer to softc.
 */

static struct todclock_softc *todclock_sc = NULL;

/* driver and attach structures */

CFATTACH_DECL_NEW(todclock, sizeof(struct todclock_softc),
    todclockmatch, todclockattach, NULL, NULL);

/*
 * int todclockmatch(device_t parent, cfdata_t cf, void *aux)
 *
 * todclock device probe function.
 * just validate the attach args
 */

int
todclockmatch(device_t parent, cfdata_t cf, void *aux)
{
	struct todclock_attach_args *ta = aux;

	if (todclock_sc != NULL)
		return(0);
	if (strcmp(ta->ta_name, "todclock") != 0)
		return(0);

	if (ta->ta_flags & TODCLOCK_FLAG_FAKE)
		return(1);
	return(2);
}

/*
 * void todclockattach(device_t parent, device_t self, void *aux)
 *
 * todclock device attach function.
 * Initialise the softc structure and do a search for children
 */

void
todclockattach(device_t parent, device_t self, void *aux)
{
	static struct todr_chip_handle	tch;

	struct todclock_softc *sc = (void *)self;
	struct todclock_attach_args *ta = aux;

	/* set up our softc */
	todclock_sc = sc;
	todclock_sc->sc_dev = self;
	todclock_sc->sc_rtc_arg = ta->ta_rtc_arg;
	todclock_sc->sc_rtc_write = ta->ta_rtc_write;
	todclock_sc->sc_rtc_read = ta->ta_rtc_read;

	tch.todr_gettime_ymdhms = tod_get_ymdhms;
	tch.todr_settime_ymdhms = tod_set_ymdhms;
	tch.todr_setwen = NULL;

	todr_attach(&tch);

	aprint_normal("\n");
}

static int
tod_set_ymdhms(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	rtc_t rtc;
	int s;

	rtc.rtc_cen = dt->dt_year / 100;
	rtc.rtc_year = dt->dt_year % 100;
	rtc.rtc_mon = dt->dt_mon + 1;
	rtc.rtc_day = dt->dt_day + 1;
	rtc.rtc_hour = dt->dt_hour;
	rtc.rtc_min = dt->dt_min;
	rtc.rtc_sec = dt->dt_sec;
	rtc.rtc_centi = 0;
	rtc.rtc_micro = 0;

	aprint_normal("resettod: %02d/%02d/%02d%02d %02d:%02d:%02d\n", rtc.rtc_day,
	    rtc.rtc_mon, rtc.rtc_cen, rtc.rtc_year, rtc.rtc_hour,
	    rtc.rtc_min, rtc.rtc_sec);

	s = splclock();
	todclock_sc->sc_rtc_write(todclock_sc->sc_rtc_arg, &rtc);
	splx(s);
}

static int
tod_get_ymdhms(todr_chip_handle_t tch, struct clock_ymdhms *dt)
{
	rtc_t rtc;
	int s, err;

	s = splclock();
	if (todclock_sc->sc_rtc_read(todclock_sc->sc_rtc_arg, &rtc) == 0) {
		splx(s);
		return -1;
	}
	splx(s);

	dt->dt_year = rtc.rtc_cen * 100 + rtc.rtc_year;
	dt->dt_mon = rtc.rtc_mon - 1;
	dt->dt_day = rtc.rtc_day - 1;
	dt->dt_hour = rtc.rtc_hour;
	dt->dt_min = rtc.rtc_min;
	dt->dt_sec = rtc.rtc_sec;

	return 0;
}

/* End of todclock.c */
