/*-
 * Copyright (c) 1993 Charles Hannum.
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz and Don Ahn.
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
 *	from: @(#)clock.c	7.2 (Berkeley) 5/12/91
 *	$Id: clock.c,v 1.13.2.11 1993/10/11 01:51:15 mycroft Exp $
 */
/* 
 * Mach Operating System
 * Copyright (c) 1991,1990,1989 Carnegie Mellon University
 * All Rights Reserved.
 * 
 * Permission to use, copy, modify and distribute this software and its
 * documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 * 
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND FOR
 * ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 * 
 * Carnegie Mellon requests users of this software to return to
 * 
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 * 
 * any improvements or extensions that they make and grant Carnegie Mellon
 * the rights to redistribute these changes.
 */
/*
  Copyright 1988, 1989 by Intel Corporation, Santa Clara, California.

		All Rights Reserved

Permission to use, copy, modify, and distribute this software and
its documentation for any purpose and without fee is hereby
granted, provided that the above copyright notice appears in all
copies and that both the copyright notice and this permission notice
appear in supporting documentation, and that the name of Intel
not be used in advertising or publicity pertaining to distribution
of the software without specific, written prior permission.

INTEL DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS,
IN NO EVENT SHALL INTEL BE LIABLE FOR ANY SPECIAL, INDIRECT, OR
CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
LOSS OF USE, DATA OR PROFITS, WHETHER IN ACTION OF CONTRACT,
NEGLIGENCE, OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION
WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/*
 * Primitive clock interrupt routines.
 */
#include "param.h"
#include "systm.h"
#include "time.h"
#include "kernel.h"
#include "sys/device.h"
#include "machine/cpu.h"
#include "machine/pio.h"
#include "i386/isa/icu.h"
#include "i386/isa/isavar.h"
#include "i386/isa/nvram.h"
#include "i386/isa/clockreg.h"
#include "i386/isa/timerreg.h"

void spinwait __P((int));

struct clock_softc {
	struct	device sc_dev;
	struct	isadev sc_id;

	u_short	sc_iobase;
};

static int clockprobe __P((struct device *, struct cfdata *, void *));
static void clockattach __P((struct device *, struct device *, void *));

struct cfdriver clockcd =
{ NULL, "clock", clockprobe, clockattach, DV_DULL, sizeof(struct device) };

static int
clockprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_iobase == IOBASEUNK)
		return 0;

	/* XXX need real probe */

	ia->ia_iosize = CLOCK_NPORTS;
	ia->ia_irq = IRQNONE;
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	return 1;
}

static void
clockattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct clock_softc *sc = (struct clock_softc *)self;
	u_short iobase = ia->ia_iobase;
	u_char d;

	printf(": mc16458\n");
	sc->sc_iobase = iobase;

	/* check and clear diagnostic flags */
	if (d = nvram(NVRAM_DIAG))
		printf("clock%d: diagnostic error %b\n", sc->sc_dev.dv_unit,
			d, NVRAM_DIAG_BITS);
	outb(iobase, NVRAM_DIAG);
	outb(iobase + 1, 0);

	/* set clock rate */
	outb(iobase, CLOCK_RATE);
	outb(iobase + 1, CLOCK_RATE_DIV2 | CLOCK_RATE_6);
	outb(iobase, CLOCK_MODE);
	outb(iobase + 1, CLOCK_MODE_HM);

	isa_establish(&sc->sc_id, &sc->sc_dev);
}

u_char
nvram(pos)
	u_char pos;
{
	struct clock_softc *sc;
	u_short iobase;

	if (clockcd.cd_ndevs < 1 ||
	    !(sc = clockcd.cd_devs[0]))
		panic("nvram: no clock");
	iobase = sc->sc_iobase;

	outb(iobase, pos);
	return inb(iobase + 1);
}

struct timer_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	u_short	sc_iobase;
	u_short	sc_irq;
	u_short sc_limit;
};

static int timerprobe __P((struct device *, struct cfdata *, void *));
static void timerforceintr __P((void *));
static void timerattach __P((struct device *, struct device *, void *));
static int timerintr __P((void *));

struct cfdriver timercd =
{ NULL, "timer", timerprobe, timerattach, DV_DULL, sizeof(struct device) };

static int
timerprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;

#ifdef DIAGNOSTIC
	if (cf->cf_unit != 0)
		panic("timerprobe: unit != 0");
#endif

	if (ia->ia_iobase == IOBASEUNK)
		return 0;

	/* XXX need a real probe */

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(timerforceintr, aux);
		if (ia->ia_irq == IRQNONE)
			return 0;
	}

	ia->ia_iosize = TIMER_NPORTS;
	ia->ia_drq = DRQUNK;
	ia->ia_msize = 0;
	return 1;
}

static void
timerforceintr(aux)
	void *aux;
{
	struct isa_attach_args *ia = aux;
	u_short iobase = ia->ia_iobase;

	/* this generates a single interrupt */
	outb(iobase + TIMER_MODE, TIMER_SEL0|TIMER_INTTC|TIMER_16BIT);
	outb(iobase + TIMER_CNTR0, 100);
	outb(iobase + TIMER_CNTR0, 0);
}

static void
timerattach(parent, self, aux)
	struct device *parent, *self;
	void *aux;
{
	struct isa_attach_args *ia = aux;
	struct timer_softc *sc = (struct timer_softc *)self;
	u_short iobase = ia->ia_iobase;
	u_short limit = TIMER_DIV(hz);

	printf(": i8253\n");
	sc->sc_iobase = iobase;
	sc->sc_irq = ia->ia_irq;
	sc->sc_limit = limit;

	if (sc->sc_dev.dv_unit == 0) {
		/* need to do this here so the rest of autoconfig can
		   use delay(); interrupt line is still masked */

		findcpuspeed(iobase);	/* use the clock (while it's free)
					   to find the cpu speed XXXX */

		outb(iobase + TIMER_MODE, TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);
		outb(iobase + TIMER_CNTR0, limit%256);
		outb(iobase + TIMER_CNTR0, limit/256);
	}

	isa_establish(&sc->sc_id, &sc->sc_dev);

	sc->sc_ih.ih_fun = timerintr;
	sc->sc_ih.ih_arg = NULL;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_DULL);
}

static int
timerintr(aux)
	void *aux;
{
	hardclock((struct clockframe *)aux);
	return 1;
}

void
cpu_initclocks(void)
{
	struct timer_softc *sc;

	if (timercd.cd_ndevs < 1 ||
	    !(sc = timercd.cd_devs[0]))
		panic("cpu_initclocks: no timer");

	sc->sc_ih.ih_fun = timerintr;
	sc->sc_ih.ih_arg = (caddr_t)sc;
	intr_establish(sc->sc_irq, &sc->sc_ih, DV_DULL);
}

static __inline u_short
gettick(iobase)
	u_short iobase;
{
	u_char lo, hi;

	disable_intr();
	outb(iobase + TIMER_MODE, TIMER_SEL0|TIMER_LATCH);
	lo = inb(iobase + TIMER_CNTR0);
	hi = inb(iobase + TIMER_CNTR0);
	enable_intr();
	return (hi << 8) | lo;
}

u_int delaycount;	/* calibrated loop variable (1 millisecond) */

#define FIRST_GUESS	0x2000

findcpuspeed(iobase)
	u_short iobase;
{
	u_short remainder;

	/* put counter in count down mode */
	outb(iobase + TIMER_MODE, TIMER_SEL0|TIMER_INTTC|TIMER_16BIT);
	outb(iobase + TIMER_CNTR0, 0xff);
	outb(iobase + TIMER_CNTR0, 0xff);
	delaycount = FIRST_GUESS;
	spinwait(1);
	/* read the value left in the counter */
	remainder = gettick(iobase);
	/*
	 * Formula for delaycount is:
	 * (loopcount * timer clock speed) / (counter ticks * 1000)
	 */
	delaycount = (FIRST_GUESS * TIMER_DIV(1000)) / (0xffff-remainder);
}

/*
 * Wait `n' microseconds.
 */
void
delay(n)
	int n;
{
	struct timer_softc *sc;
	u_short iobase;
	u_short limit, tick, otick;

	if (timercd.cd_ndevs < 1 ||
	    !(sc = timercd.cd_devs[0]))
		panic("cpu_initclocks: no timer");
	
	iobase = sc->sc_iobase;
	limit = sc->sc_limit;

	otick = gettick(iobase);

#ifdef __GNUC__
	/*
	 * Calculate ((n * TIMER_FREQ) / 1e6) using explicit assembler code so
	 * we can take advantage of the intermediate 64-bit quantity to prevent
	 * loss of significance.
	 */
	n -= 10;
	{register int m;
	__asm __volatile("mul %3"
			 : "=a" (n), "=d" (m)
			 : "0" (n), "r" (TIMER_FREQ));
	__asm __volatile("div %3"
			 : "=a" (n)
			 : "0" (n), "d" (m), "r" (1000000)
			 : "2");}
#else
	/*
	 * Calculate (n * (TIMER_FREQ / 1e6)) without using floating point and
	 * without any avoidable overflows.
	 */
	n -= 20;
	{
		int sec = n / 1000000,
		    usec = n % 1000000;
		n = sec * TIMER_FREQ +
		    usec * (TIMER_FREQ / 1000000) +
		    usec * ((TIMER_FREQ % 1000000) / 1000) / 1000 +
		    usec * (TIMER_FREQ % 1000) / 1000000;
	}
#endif

	while (n > 0) {
		tick = gettick(iobase);
		if (tick > otick)
			n -= otick - (tick - limit);
		else
			n -= otick - tick;
		otick = tick;
	}
}

/*
 * Delay for some number of milliseconds.
 */
void
spinwait(int millisecs)
{
	/* XXXX */
	delay(1000 * millisecs);
}

int
rtcget(struct rtc_st *rtc_regs)
{
	/* we would have panicked earlier if clock0 didn't exist */
	struct clock_softc *sc = clockcd.cd_devs[0];
	u_short iobase = sc->sc_iobase;
        u_char *regs = (u_char *)rtc_regs;
	int i;
        
	outb(iobase, NVRAM_VALID);
	if ((inb(iobase + 1) & NVRAM_VALID_VRT) == 0)
		return -1;
	outb(iobase, CLOCK_RATE);	
	while (inb(iobase + 1) & CLOCK_RATE_UIP)
		outb(iobase, CLOCK_RATE);
	for (i = 0; i < CLOCK_NREG; i++) {
		outb(iobase, i);
		regs[i] = inb(iobase + 1);
	}
	return 0;
}	

void
rtcput(struct rtc_st *rtc_regs)
{
	/* we would have panicked earlier if clock0 didn't exist */
	struct clock_softc *sc = clockcd.cd_devs[0];
	u_short iobase = sc->sc_iobase;
        u_char omode, *regs = (u_char *)rtc_regs;
	int i;

	outb(iobase, CLOCK_MODE);
	omode = inb(iobase + 1);
	outb(iobase, CLOCK_MODE);
	outb(iobase + 1, omode | CLOCK_MODE_SET);
	for (i = 0; i < CLOCK_NREG; i++) {
		outb(iobase, i);
		outb(iobase + 1, regs[i]);
	}
	outb(iobase, CLOCK_MODE);
	outb(iobase + 1, omode & ~CLOCK_MODE_SET);
}

static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int
yeartoday(int year)
{
	return((!(year % 4) - !(year % 100) + !(year % 400)) ? 366 : 365);
}

static int
bcdtodec(u_char n)
{
	return(((n >> 4) & 0x0f) * 10 + (n & 0x0f));
}

static u_char
dectobcd(int n)
{
	return((u_char)(((n / 10) << 4) | ((n % 10) & 0x0f)));
}

/*
 * Reset the clock.
 */
void
resettodr()
{
	struct rtc_st rtclk;
	time_t n;
	int diff, i, j;
	int s;

	/* attempt to read the clock to preserve the alarm time, but
	   don't care if it fails */
	s = splclock();
	if (rtcget(&rtclk))
		bzero(&rtclk, sizeof(rtclk));
	splx(s);

	/* clock is normally in local time, including DST */
	diff = tz.tz_minuteswest * 60;
	if (tz.tz_dsttime)
		diff -= 3600;
	n = time.tv_sec - diff;

	rtclk.rtc_sec = dectobcd(n%60);
	n /= 60;
	rtclk.rtc_min = dectobcd(n%60);
	n /= 60;
	rtclk.rtc_hr = dectobcd(n%24);
	n /= 24;

	/* 1/1/70 is Thursday */
	rtclk.rtc_dow = (n + 4) % 7;

	for (j = 1970, i = yeartoday(j); n >= i; j++, i = yeartoday(j))
		n -= i;
	rtclk.rtc_yr = dectobcd(j - 1900);

	if (i == 366)
		month[1] = 29;
	else
		month[1] = 28;
	for (i = 0; n >= month[i]; i++)
		n -= month[i];
	rtclk.rtc_mon = dectobcd(++i);

	rtclk.rtc_dom = dectobcd(++n);

	s = splclock();
	rtcput(&rtclk);
	splx(s);
}

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
	time_t base;
{
        /*
         * We ignore the suggested time for now and go for the RTC
         * clock time stored in the CMOS RAM.
         */
	struct rtc_st rtclk;
	time_t n;
	int sec, min, hr, dom, mon, yr;
	int i, days;
	int s;

	s = splclock();
	if (rtcget(&rtclk)) {
		splx(s);
		return;
	}
	splx(s);

	sec = bcdtodec(rtclk.rtc_sec);
	min = bcdtodec(rtclk.rtc_min);
	hr = bcdtodec(rtclk.rtc_hr);
	dom = bcdtodec(rtclk.rtc_dom);
	mon = bcdtodec(rtclk.rtc_mon);
	yr = bcdtodec(rtclk.rtc_yr);

	/* stupid clock has no century; fake it for another 70 years */
	yr = (yr < 70) ? yr+100 : yr;

	n = sec + 60 * min + 3600 * hr;

	days = dom - 1;
	if (yeartoday(yr) == 366)
		month[1] = 29;
	else
		month[1] = 28;
	for (i = mon - 2; i >= 0; i--)
		days += month[i];
	for (i = 70; i < yr; i++)
		days += yeartoday(i);
	n += days * (3600 * 24);

	n += tz.tz_minuteswest * 60;
	if (tz.tz_dsttime)
		n -= 3600;

	time.tv_sec = n;
        time.tv_usec = 0;
}
