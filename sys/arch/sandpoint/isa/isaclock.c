/*	$NetBSD: isaclock.c,v 1.2 2001/07/22 15:04:00 wiz Exp $	*/

/*-
 * Copyright (c) 1993, 1994 Charles M. Hannum.
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
 *	@(#)clock.c	7.2 (Berkeley) 5/12/91
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
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/callout.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/pio.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/i8253reg.h>
#include <sandpoint/isa/nvram.h>
#include <sandpoint/isa/spkrreg.h>

extern void disable_intr(void);	/* In locore.S */
extern void enable_intr(void);	/* In locore.S */

void	sysbeepstop __P((void *));
void	sysbeep __P((int, int));
void	rtcinit __P((void));
int	rtcget __P((mc_todregs *));
void	rtcput __P((mc_todregs *));
static int yeartoday __P((int));
int 	hexdectodec __P((int));
int	dectohexdec __P((int));

__inline u_int mc146818_read __P((void *, u_int));
__inline void mc146818_write __P((void *, u_int, u_int));

#define	SECMIN	((unsigned)60)			/* seconds per minute */
#define	SECHOUR	((unsigned)(60*SECMIN))		/* seconds per hour */
#define	SECDAY	((unsigned)(24*SECHOUR))	/* seconds per day */
#define	SECYR	((unsigned)(365*SECDAY))	/* seconds per common year */

__inline u_int
mc146818_read(sc, reg)
	void *sc;					/* XXX use it? */
	u_int reg;
{

	isa_outb(IO_RTC, (u_char)reg);
	return (isa_inb(IO_RTC+1));
}

__inline void
mc146818_write(sc, reg, datum)
	void *sc;					/* XXX use it? */
	u_int reg, datum;
{

	isa_outb(IO_RTC, reg);
	isa_outb(IO_RTC+1, datum);
}

static int beeping;

void
sysbeepstop(arg)
	void *arg;
{
	/* disable counter 2 */
	disable_intr();
	isa_outb(PITAUX_PORT, isa_inb(PITAUX_PORT) & ~PIT_SPKR);
	enable_intr();
	beeping = 0;
}

void
sysbeep(pitch, period)
	int pitch, period;
{
	static struct callout sysbeep_ch = CALLOUT_INITIALIZER;
	static int last_pitch;

	if (beeping)
		callout_stop(&sysbeep_ch);
	if (pitch == 0 || period == 0) {
		sysbeepstop(0);
		last_pitch = 0;
		return;
	}
	if (!beeping || last_pitch != pitch) {
		disable_intr();
		isa_outb(IO_TIMER1 + TIMER_MODE,
			 TIMER_SEL2 | TIMER_16BIT | TIMER_SQWAVE);
		isa_outb(IO_TIMER1 + TIMER_CNTR2, TIMER_DIV(pitch) % 256);
		isa_outb(IO_TIMER1 + TIMER_CNTR2, TIMER_DIV(pitch) / 256);
		isa_outb(PITAUX_PORT, isa_inb(PITAUX_PORT) | PIT_SPKR);	/* enable counter 2 */
		enable_intr();
	}
	last_pitch = pitch;
	beeping = 1;
	callout_reset(&sysbeep_ch, period, sysbeepstop, NULL);
}

void
rtcinit()
{
	static int first_rtcopen_ever = 1;

	if (!first_rtcopen_ever)
		return;
	first_rtcopen_ever = 0;

	mc146818_write(NULL, MC_REGA,			/* XXX softc */
	    MC_BASE_32_KHz | MC_RATE_1024_Hz);
	mc146818_write(NULL, MC_REGB, MC_REGB_24HR);	/* XXX softc */
}

int
rtcget(regs)
	mc_todregs *regs;
{

	rtcinit();
	if ((mc146818_read(NULL, MC_REGD) & MC_REGD_VRT) == 0) /* XXX softc */
		return (-1);
	MC146818_GETTOD(NULL, regs);			/* XXX softc */
	return (0);
}	

void
rtcput(regs)
	mc_todregs *regs;
{

	rtcinit();
	MC146818_PUTTOD(NULL, regs);			/* XXX softc */
}

static int month[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

static int
yeartoday(year)
	int year;
{

	return ((year % 4) ? 365 : 366);
}

int
hexdectodec(n)
	int n;
{

	return (((n >> 4) & 0x0f) * 10 + (n & 0x0f));
}

int
dectohexdec(n)
	int n;
{

	return ((u_char)(((n / 10) << 4) & 0xf0) | ((n % 10) & 0x0f));
}

static int timeset;

/*
 * Initialize the time of day register, based on the time base which is, e.g.
 * from a filesystem.
 */
void
inittodr(base)
	time_t base;
{
	mc_todregs rtclk;
	time_t n;
	int sec, min, hr, dom, mon, yr;
	int i, days = 0;
	int s;

	/*
	 * We mostly ignore the suggested time and go for the RTC clock time
	 * stored in the CMOS RAM.  If the time can't be obtained from the
	 * CMOS, or if the time obtained from the CMOS is 5 or more years
	 * less than the suggested time, we used the suggested time.  (In
	 * the latter case, it's likely that the CMOS battery has died.)
	 */

	if (base < 15*SECYR) {	/* if before 1985, something's odd... */
		printf("WARNING: preposterous time in file system\n");
		/* read the system clock anyway */
		base = 17*SECYR + 186*SECDAY + SECDAY/2;
	}

	s = splclock();
	if (rtcget(&rtclk)) {
		splx(s);
		printf("WARNING: invalid time in clock chip\n");
		goto fstime;
	}
	splx(s);

	sec = hexdectodec(rtclk[MC_SEC]);
	min = hexdectodec(rtclk[MC_MIN]);
	hr = hexdectodec(rtclk[MC_HOUR]);
	dom = hexdectodec(rtclk[MC_DOM]);
	mon = hexdectodec(rtclk[MC_MONTH]);
	yr = hexdectodec(rtclk[MC_YEAR]);
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

	n += rtc_offset * 60;

	if (base < n - 5*SECYR)
		printf("WARNING: file system time much less than clock time\n");
	else if (base > n + 5*SECYR) {
		printf("WARNING: clock time much less than file system time\n");
		printf("WARNING: using file system time\n");
		goto fstime;
	}

	timeset = 1;
	time.tv_sec = n;
	time.tv_usec = 0;
	return;

fstime:
	timeset = 1;
	time.tv_sec = base;
	time.tv_usec = 0;
	printf("WARNING: CHECK AND RESET THE DATE!\n");
}

/*
 * Reset the clock.
 */
void
resettodr()
{
	mc_todregs rtclk;
	time_t n;
	int diff, i, j;
	int s;

	/*
	 * We might have been called by boot() due to a crash early
	 * on.  Don't reset the clock chip in this case.
	 */
	if (!timeset)
		return;

	s = splclock();
	if (rtcget(&rtclk))
		memset(&rtclk, 0, sizeof(rtclk));
	splx(s);

	diff = rtc_offset * 60;
	n = (time.tv_sec - diff) % (3600 * 24);   /* hrs+mins+secs */
	rtclk[MC_SEC] = dectohexdec(n % 60);
	n /= 60;
	rtclk[MC_MIN] = dectohexdec(n % 60);
	rtclk[MC_HOUR] = dectohexdec(n / 60);

	n = (time.tv_sec - diff) / (3600 * 24);	/* days */
	rtclk[MC_DOW] = (n + 4) % 7;  /* 1/1/70 is Thursday */

	for (j = 1970, i = yeartoday(j); n >= i; j++, i = yeartoday(j))
		n -= i;

	rtclk[MC_YEAR] = dectohexdec(j - 1900);

	if (i == 366)
		month[1] = 29;
	for (i = 0; n >= month[i]; i++)
		n -= month[i];
	month[1] = 28;
	rtclk[MC_MONTH] = dectohexdec(++i);

	rtclk[MC_DOM] = dectohexdec(++n);

	s = splclock();
	rtcput(&rtclk);
	splx(s);
}
