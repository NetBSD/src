/*	$NetBSD: rtc.c,v 1.2.2.3 2002/04/01 07:39:54 nathanw Exp $	*/

/*
 * Copyright (c) 1988 University of Utah.
 * Copyright (c) 1982, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * from: Utah $Hdr: clock.c 1.18 91/01/21$
 *
 *	@(#)clock.c	8.2 (Berkeley) 1/12/94
 */

/*
 * attachment for HP300 real-time clock (RTC)
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rtc.c,v 1.2.2.3 2002/04/01 07:39:54 nathanw Exp $");                                                  

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <hp300/dev/intiovar.h>
#include <hp300/dev/rtcreg.h>

#include <dev/clock_subr.h>
#include <hp300/hp300/clockvar.h>

struct rtc_softc {
	struct device sc_dev;
	bus_space_tag_t sc_bst;
	bus_space_handle_t sc_bsh;
};

static int rtcmatch(struct device *, struct cfdata *, void *);
static void rtcattach(struct device *, struct device *, void *aux);

struct cfattach rtc_ca = {
	sizeof (struct rtc_softc), rtcmatch, rtcattach, 
};

static int rtc_gettime(todr_chip_handle_t, struct timeval *);
static int rtc_settime(todr_chip_handle_t, struct timeval *);
static int rtc_getcal(todr_chip_handle_t, int *);
static int rtc_setcal(todr_chip_handle_t, int);
static u_int8_t	rtc_readreg(struct rtc_softc *, int);
static u_int8_t rtc_writereg(struct rtc_softc *, int, u_int8_t);


static int
rtcmatch(parent, match, aux)
	struct device *parent;
	struct cfdata *match;
	void *aux;
{
	struct intio_attach_args *ia = aux;

	if (strcmp("rtc", ia->ia_modname) != 0)
		return (0);

	return (1);
}

static void
rtcattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct intio_attach_args *ia = aux;
	struct rtc_softc *sc = (struct rtc_softc *)self;
	struct todr_chip_handle *todr_handle;

	printf("\n");

	if (bus_space_map(ia->ia_bst, ia->ia_iobase, INTIO_DEVSIZE, 0,
	    &sc->sc_bsh)) {
		printf("%s: can't map registers\n",
		    sc->sc_dev.dv_xname);
		return;
	}

	MALLOC(todr_handle, struct todr_chip_handle *,
	    sizeof(struct todr_chip_handle), M_DEVBUF, M_NOWAIT);

	todr_handle->cookie = sc;
	todr_handle->todr_gettime = rtc_gettime;
	todr_handle->todr_settime = rtc_settime;
	todr_handle->todr_getcal = rtc_getcal;
	todr_handle->todr_setcal = rtc_setcal;
	todr_handle->todr_setwen = NULL;

	clockattach(todr_handle);
}

static int
rtc_gettime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct rtc_softc *sc = (struct rtc_softc *)handle->cookie;
	int i, read_okay, year;
	struct clock_ymdhms dt;
	u_int8_t rtc_registers[NUM_RTC_REGS];

	/* read rtc registers */
	read_okay = 0;
	while (!read_okay) {
		read_okay = 1;
		for (i = 0; i < NUM_RTC_REGS; i++)
			rtc_registers[i] = rtc_readreg(sc, i);
		for (i = 0; i < NUM_RTC_REGS; i++)
			if (rtc_registers[i] != rtc_readreg(sc, i))
				read_okay = 0;
	}

#define	rtc_to_decimal(a,b) 	(rtc_registers[a] * 10 + rtc_registers[b])

	dt.dt_sec  = rtc_to_decimal(1, 0);
	dt.dt_min  = rtc_to_decimal(3, 2);
	dt.dt_hour = (rtc_registers[5] & RTC_REG5_HOUR) * 10 + rtc_registers[4];
	dt.dt_day  = rtc_to_decimal(8, 7);
	dt.dt_mon  = rtc_to_decimal(10, 9);

	year = rtc_to_decimal(12, 11) + RTC_BASE_YEAR;
	if (year < POSIX_BASE_YEAR)
		year += 100;
	dt.dt_year = year;

#undef	rtc_to_decimal

	/* simple sanity checks */
	if (dt.dt_mon > 12 || dt.dt_day > 31 ||
	    dt.dt_hour >= 24 || dt.dt_min >= 60 || dt.dt_sec >= 60)
		return (1);

	tv->tv_sec = clock_ymdhms_to_secs(&dt);
	tv->tv_usec = 0;
	return (0);
}

static int
rtc_settime(todr_chip_handle_t handle, struct timeval *tv)
{
	struct rtc_softc *sc = (struct rtc_softc *)handle->cookie;
	int i, year;
	struct clock_ymdhms dt;
	u_int8_t rtc_registers[NUM_RTC_REGS];

	/* Note: we ignore `tv_usec' */
	clock_secs_to_ymdhms(tv->tv_sec, &dt);

	year = dt.dt_year - RTC_BASE_YEAR;
	if (year > 99)
		year -= 100;

#define	decimal_to_rtc(a,b,n)		\
	rtc_registers[a] = (n) % 10;	\
	rtc_registers[b] = (n) / 10;

	decimal_to_rtc(0, 1, dt.dt_sec);
	decimal_to_rtc(2, 3, dt.dt_min);
	decimal_to_rtc(7, 8, dt.dt_day);
	decimal_to_rtc(9, 10, dt.dt_mon);
	decimal_to_rtc(11, 12, year);

	rtc_registers[4] = dt.dt_hour % 10;
	rtc_registers[5] = ((dt.dt_hour / 10) & RTC_REG5_HOUR) | RTC_REG5_24HR;

	rtc_registers[6] = 0;

#undef	decimal_to_rtc

	/* write rtc registers */
	rtc_writereg(sc, 15, 13);	/* reset prescalar */
	for (i = 0; i < NUM_RTC_REGS; i++)
		if (rtc_registers[i] !=
		    rtc_writereg(sc, i, rtc_registers[i]))
			return (1);
	return (0);
}

static int
rtc_getcal(todr_chip_handle_t handle, int *vp)
{
	return (EOPNOTSUPP);
}

static int
rtc_setcal(todr_chip_handle_t handle, int v)
{
	return (EOPNOTSUPP);
}

static u_int8_t
rtc_readreg(struct rtc_softc *sc, int reg)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	u_int8_t data;
	int s = splvm();

	data = reg;
	intio_device_writecmd(bst, bsh, RTC_SET_REG, &data, 1);
	intio_device_readcmd(bst, bsh, RTC_READ_REG, &data);

	splx(s);
	return (data);
}

static u_int8_t
rtc_writereg(struct rtc_softc *sc, int reg, u_int8_t data)
{
	bus_space_tag_t bst = sc->sc_bst;
	bus_space_handle_t bsh = sc->sc_bsh;
	u_int8_t tmp;
	int s = splvm();

	tmp = (data << 4) | reg;
	intio_device_writecmd(bst, bsh, RTC_SET_REG, &tmp, 1);
	intio_device_writecmd(bst, bsh, RTC_WRITE_REG, NULL, 0);
	intio_device_readcmd(bst, bsh, RTC_READ_REG, &tmp);

	splx(s);
	return (tmp);
}
