/*-
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
 *	$Id: clock.c,v 1.13.2.1 1993/09/14 17:32:21 mycroft Exp $
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
#include "machine/segments.h"
#include "sys/device.h"
#include "i386/isa/icu.h"
#include "i386/isa/isa.h"
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
	ia->ia_irq = IRQUNK;
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

	printf(": Intel XXXX\n");
	sc->sc_iobase = iobase;

	/* check and clear diagnostic flags */
	if (d = nvram(NVRAM_DIAG))
		printf("clock%d: diagnostic error %b\n", self->sc_dev.dv_unit,
			d, NVRAM_DIAG_BITS);
	outb(iobase, NVRAM_DIAG);
	outb(iobase + 1, 0);

	/* set clock rate */
	outb(iobase, CLOCK_RATE);
	outb(iobase + 1, CLOCK_RATE_DIV2 | CLOCK_RATE_6);
	outb(iobase, CLOCK_MODE);
	outb(iobase + 1, CLOCK_MODE_HM);
}

u_char
nvram(pos)
	u_char pos;
{
	struct clock_softc *sc;

	if (clockcd.cd_ndevs < 1 ||
	    !(sc = (struct clock_softc *)clockcd.cd_devs[0]))
		panic("nvram: no clock");
	outb(sc->sc_iobase, pos);
	return inb(sc->sc_iobase + 1);
}

struct timer_softc {
	struct	device sc_dev;
	struct	isadev sc_id;
	struct	intrhand sc_ih;

	u_short	sc_iobase;
	u_short sc_limit;
};

static int timerprobe __P((struct device *, struct cfdata *, void *));
static void timerforceintr __P((void *));
static void timerattach __P((struct device *, struct device *, void *));

struct cfdriver timercd =
{ NULL, "timer", timerprobe, timerattach, DV_DULL, sizeof(struct device) };

static int
timerprobe(parent, cf, aux)
	struct device *parent;
	struct cfdata *cf;
	void *aux;
{
	struct isa_attach_args *ia = aux;

	if (ia->ia_iobase == IOBASEUNK)
		return 0;

	/* XXX need a real probe */

	if (ia->ia_irq == IRQUNK) {
		ia->ia_irq = isa_discoverintr(timerforceintr, aux);
		if (ia->ia_irq == IRQUNK)
			return 0:
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

	printf(": Intel 8253\n");
	sc->sc_iobase = iobase;
	sc->sc_limit = limit;

	if (sc->sc_dev.dv_unit == 0) {
		/* need to do this here so the rest of autoconfig can
		   use delay() */

		findcpuspeed(iobase);	/* use the clock (while it's free)
					   to find the cpu speed XXXX */

		outb(iobase + TIMER_MODE, TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);
		outb(iobase + TIMER_CNTR0, limit%256);
		outb(iobase + TIMER_CNTR0, limit/256);
	}
}

void
cpu_initclocks(void)
{
	struct timer_softc *sc;

	if (timercd.cd_ndevs < 1 ||
	    !(sc = (struct timer_softc *)timercd.cd_devs[0]))
		panic("cpu_initclocks: no timer");

	sc->sc_ih.ih_fun = timerintr;
	sc->sc_ih.ih_arg = XXXX;
	intr_establish(ia->ia_irq, &sc->sc_ih, DV_DULL);
}

u_int delaycount;	/* calibrated loop variable (1 millisecond) */

#define FIRST_GUESS	0x2000

findcpuspeed(iobase)
	u_short iobase;
{
	u_char lo, hi;
	u_int remainder;

	/* put counter in count down mode */
	outb(iobase + TIMER_MODE, TIMER_SEL0|TIMER_INTTC|TIMER_16BIT);
	outb(iobase + TIMER_CNTR0, 0xff);
	outb(iobase + TIMER_CNTR0, 0xff);
	delaycount = FIRST_GUESS;
	spinwait(1);
	/* read the value left in the counter */
	lo = inb(iobase + TIMER_CNTR0);
	hi = inb(iobase + TIMER_CNTR0);
	remainder = (hi << 8) | lo;
	/*
	 * Formula for delaycount is:
	 * (loopcount * timer clock speed) / (counter ticks * 1000)
	 */
	delaycount = (FIRST_GUESS * TIMER_DIV(1000)) / (0xffff-remainder);
}

/*
 * Delay for some number of milliseconds.
 */
void
spinwait(int millisecs)
{
	/* XXXX */
	DELAY(1000 * millisecs);
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
	    !(sc = (struct timer_softc *)timercd.cd_devs[0]))
		panic("cpu_initclocks: no timer");
	
	iobase = sc->sc_iobase;
	limit = sc->sc_limit;

	otick = gettick(iobase);

	n -= 25;

	/*
	 * Calculate (n * (TIMER_FREQ / 1e6)) without using floating point and
	 * without any avoidable overflows.
	 */
	{
		int sec = n / 1000000,
		    usec = n % 1000000;
		n = sec * TIMER_FREQ +
		    usec * (TIMER_FREQ / 1000000) +
		    usec * ((TIMER_FREQ % 1000000) / 1000) / 1000 +
		    usec * (TIMER_FREQ % 1000) / 1000000;
	}

	while (n > 0) {
		tick = gettick(iobase);
		if (tick > otick)
			n -= otick - (tick - limit);
		else
			n -= otick - tick;
		otick = tick;
	}
}

/* XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX */

int
rtcget(struct rtc_st *rtc_regs)
{
	int	i;
        u_char *regs = (u_char *)rtc_regs;
        
	outb(IO_RTC, RTC_D); 
	if (inb(IO_RTC+1) & RTC_VRT == 0) return(-1);
	outb(IO_RTC, RTC_STATUSA);	
	while (inb(IO_RTC+1) & RTC_UIP)		/* busy wait */
		outb(IO_RTC, RTC_STATUSA);	
	for (i = 0; i < RTC_NREG; i++) {
		outb(IO_RTC, i);
		regs[i] = inb(IO_RTC+1);
	}
	return(0);
}	

void
rtcput(struct rtc_st *rtc_regs)
{
	u_char	x;
	int	i;
        u_char *regs = (u_char *)rtc_regs;

	outb(IO_RTC, RTC_STATUSB);
	x = inb(IO_RTC+1);
	outb(IO_RTC, RTC_STATUSB);
	outb(IO_RTC+1, x | RTC_SET); 	
	for (i = 0; i < RTC_NREGP; i++) {
		outb(IO_RTC, i);
		outb(IO_RTC+1, regs[i]);
	}
	outb(IO_RTC, RTC_STATUSB);
	outb(IO_RTC+1, x & ~RTC_SET); 
}

static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int
yeartoday(int year)
{
	return((year%4) ? 365 : 366);
}

static int
hexdectodec(char n)
{
	return(((n>>4)&0x0F)*10 + (n&0x0F));
}

static char
dectohexdec(int n)
{
	return((char)(((n/10)<<4)&0xF0) | ((n%10)&0x0F));
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
	int i, days = 0;
	int ospl;

	ospl = splclock();
	if (rtcget(&rtclk)) {
		splx(ospl);
		return;
	}
	splx (ospl);

	sec = hexdectodec(rtclk.rtc_sec);
	min = hexdectodec(rtclk.rtc_min);
	hr = hexdectodec(rtclk.rtc_hr);
	dom = hexdectodec(rtclk.rtc_dom);
	mon = hexdectodec(rtclk.rtc_mon);
	yr = hexdectodec(rtclk.rtc_yr);
	yr = (yr < 70) ? yr+100 : yr;

	n = sec + 60 * min + 3600 * hr;
	n += (dom - 1) * 3600 * 24;

	if (yeartoday(yr) == 366)
		month[1] = 29;
	for (i = mon - 2; i >= 0; i--)
		days += month[i];
	month[1] = 28;
	for (i = 70; i < yr; i++)
		days += yeartoday(i);
	n += days * 3600 * 24;

	n += tz.tz_minuteswest * 60;
	if (tz.tz_dsttime)
		n -= 3600;
	time.tv_sec = n;
        time.tv_usec = 0;
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
	int ospl;

	ospl = splclock();
	if (rtcget(&rtclk)) {
		splx(ospl);
		return;
	}
	splx(ospl);

	diff = tz.tz_minuteswest * 60;
	if (tz.tz_dsttime)
		diff -= 3600;
	n = (time.tv_sec - diff) % (3600 * 24);   /* hrs+mins+secs */
	rtclk.rtc_sec = dectohexdec(n%60);
	n /= 60;
	rtclk.rtc_min = dectohexdec(n%60);
	rtclk.rtc_hr = dectohexdec(n/60);

	n = (time.tv_sec - diff) / (3600 * 24);	/* days */
	rtclk.rtc_dow = (n + 4) % 7;  /* 1/1/70 is Thursday */

	for (j = 1970, i = yeartoday(j); n >= i; j++, i = yeartoday(j))
		n -= i;

	rtclk.rtc_yr = dectohexdec(j - 1900);

	if (i == 366)
		month[1] = 29;
	for (i = 0; n >= month[i]; i++)
		n -= month[i];
	month[1] = 28;
	rtclk.rtc_mon = dectohexdec(++i);

	rtclk.rtc_dom = dectohexdec(++n);

	ospl = splclock();
	rtcput(&rtclk);
	splx(ospl);
}
