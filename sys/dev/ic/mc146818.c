/*	$NetBSD: mc146818.c,v 1.5 2004/09/25 09:46:17 tsutsui Exp $	*/

/*
 * Copyright (c) 2003 Izumi Tsutsui.  All rights reserved.
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

/*
 * mc146818 and compatible time of day chip subroutines
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: mc146818.c,v 1.5 2004/09/25 09:46:17 tsutsui Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/errno.h>

#include <machine/bus.h>

#include <dev/clock_subr.h>

#include <dev/ic/mc146818reg.h>
#include <dev/ic/mc146818var.h>

int mc146818_gettime(todr_chip_handle_t, struct timeval *);
int mc146818_settime(todr_chip_handle_t, struct timeval *);
int mc146818_getcal(todr_chip_handle_t, int *);
int mc146818_setcal(todr_chip_handle_t, int);

void
mc146818_attach(struct mc146818_softc *sc)
{
	todr_chip_handle_t handle;

#ifdef DIAGNOSTIC
	if (sc->sc_mcread == NULL ||
	    sc->sc_mcwrite == NULL)
		panic("mc146818_attach: invalid read/write functions");
#endif

	printf(": mc146818 compatible time-of-day clock");

	handle = &sc->sc_handle;
	handle->cookie = sc;
	handle->todr_gettime = mc146818_gettime;
	handle->todr_settime = mc146818_settime;
	handle->todr_getcal  = mc146818_getcal;
	handle->todr_setcal  = mc146818_setcal;
	handle->todr_setwen  = NULL;
}

/*
 * todr_gettime function:
 *  Get time of day and convert it to a struct timeval.
 *  Return 0 on success, an error number othersize.
 */
int
mc146818_gettime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct mc146818_softc *sc;
	struct clock_ymdhms dt;
	int s, timeout, cent, year;

	sc = handle->cookie;

	s = splclock();		/* XXX really needed? */

	todr_wenable(handle, 1);

	timeout = 1000000;	/* XXX how long should we wait? */
	for (;;) {
		if ((*sc->sc_mcread)(sc, MC_REGA) & MC_REGA_UIP)
			break;
		if (--timeout < 0) {
			printf("mc146818_gettime: timeout\n");
			return EBUSY;
		}
	}

#define	FROMREG(x)	((sc->sc_flag & MC146818_BCD) ? FROMBCD(x) : (x))

	dt.dt_sec  = FROMREG((*sc->sc_mcread)(sc, MC_SEC));
	dt.dt_min  = FROMREG((*sc->sc_mcread)(sc, MC_MIN));
	dt.dt_hour = FROMREG((*sc->sc_mcread)(sc, MC_HOUR));
	dt.dt_wday = FROMREG((*sc->sc_mcread)(sc, MC_DOW));
	dt.dt_day  = FROMREG((*sc->sc_mcread)(sc, MC_DOM));
	dt.dt_mon  = FROMREG((*sc->sc_mcread)(sc, MC_MONTH));
	year       = FROMREG((*sc->sc_mcread)(sc, MC_YEAR));
	if (sc->sc_getcent) {
		cent = (*sc->sc_getcent)(sc);
		year += cent * 100;
	}

#undef FROMREG

	year += sc->sc_year0;
	if (year < POSIX_BASE_YEAR &&
	    (sc->sc_flag & MC146818_NO_CENT_ADJUST) == 0)
		year += 100;
	dt.dt_year = year;

	todr_wenable(handle, 0);

	splx(s);

	/* simple sanity checks */
	if (dt.dt_mon > 12 || dt.dt_day > 31 ||
	    dt.dt_hour >= 24 || dt.dt_min >= 60 || dt.dt_sec >= 60)
		return EIO;

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return 0;
}

/*
 * todr_settime function:
 *  Set the time of day clock based on the value of the struct timeval arg.
 *  Return 0 on success, an error number othersize.
 */
int
mc146818_settime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct mc146818_softc *sc;
	struct clock_ymdhms dt;
	int s, timeout, cent, year;

	sc = handle->cookie;

	/* Note: we ignore `tv_usec' */
	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	s = splclock();		/* XXX really needed? */

	todr_wenable(handle, 1);

	timeout = 1000000;	/* XXX how long should we wait? */
	for (;;) {
		if ((*sc->sc_mcread)(sc, MC_REGA) & MC_REGA_UIP)
			break;
		if (--timeout < 0) {
			printf("mc146818_settime: timeout\n");
			return EBUSY;
		}
	}

#define	TOREG(x)	((sc->sc_flag & MC146818_BCD) ? TOBCD(x) : (x))

	(*sc->sc_mcwrite)(sc, MC_SEC, TOREG(dt.dt_sec));
	(*sc->sc_mcwrite)(sc, MC_MIN, TOREG(dt.dt_min));
	(*sc->sc_mcwrite)(sc, MC_HOUR, TOREG(dt.dt_hour));
	(*sc->sc_mcwrite)(sc, MC_DOW, TOREG(dt.dt_wday));
	(*sc->sc_mcwrite)(sc, MC_DOM, TOREG(dt.dt_day));
	(*sc->sc_mcwrite)(sc, MC_MONTH, TOREG(dt.dt_mon));

	year = dt.dt_year - sc->sc_year0;
	if (sc->sc_setcent) {
		cent = year / 100;
		(*sc->sc_setcent)(sc, cent);
		year -= cent * 100;
	}
	if (year > 99 &&
	    (sc->sc_flag & MC146818_NO_CENT_ADJUST) == 0)
		year -= 100;
	(*sc->sc_mcwrite)(sc, MC_YEAR, TOREG(year));

#undef TOREG

	todr_wenable(handle, 0);

	splx(s);

	return 0;
}

int
mc146818_getcal(todr_chip_handle_t handle, int *vp)
{

	return EOPNOTSUPP;
}

int
mc146818_setcal(todr_chip_handle_t handle, int v)
{

	return EOPNOTSUPP;
}
