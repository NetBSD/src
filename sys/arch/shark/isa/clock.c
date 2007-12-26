/*	$NetBSD: clock.c,v 1.14.16.1 2007/12/26 22:24:54 rjs Exp $	*/

/*
 * Copyright 1997
 * Digital Equipment Corporation. All rights reserved.
 *
 * This software is furnished under license and may be used and
 * copied only in accordance with the following terms and conditions.
 * Subject to these conditions, you may download, copy, install,
 * use, modify and distribute this software in source and/or binary
 * form. No title or ownership is transferred hereby.
 *
 * 1) Any source code used, modified or distributed must reproduce
 *    and retain this copyright notice and list of conditions as
 *    they appear in the source file.
 *
 * 2) No right is granted to use any trade name, trademark, or logo of
 *    Digital Equipment Corporation. Neither the "Digital Equipment
 *    Corporation" name nor any trademark or logo of Digital Equipment
 *    Corporation may be used to endorse or promote products derived
 *    from this software without the prior written permission of
 *    Digital Equipment Corporation.
 *
 * 3) This software is provided "AS-IS" and any express or implied
 *    warranties, including but not limited to, any implied warranties
 *    of merchantability, fitness for a particular purpose, or
 *    non-infringement are disclaimed. In no event shall DIGITAL be
 *    liable for any damages whatsoever, and in particular, DIGITAL
 *    shall not be liable for special, indirect, consequential, or
 *    incidental damages or damages for lost profits, loss of
 *    revenue or loss of use, whether such damages arise in contract,
 *    negligence, tort, under statute, in equity, at law or otherwise,
 *    even if advised of the possibility of such damage.
 */

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
 * 3. Neither the name of the University nor the names of its contributors
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
/*-
 * Copyright (c) 1993, 1994 Charles M. Hannum.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: clock.c,v 1.14.16.1 2007/12/26 22:24:54 rjs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/time.h>
#include <sys/timetc.h>
#include <sys/kernel.h>
#include <sys/device.h>

#include <machine/cpu.h>
#include <machine/intr.h>
#include <machine/irqhandler.h>
#include <machine/pio.h>
#include <arm/cpufunc.h>

#include <dev/isa/isareg.h>
#include <dev/isa/isavar.h>
#include <dev/ic/mc146818reg.h>
#include <dev/ic/i8253reg.h>
#include <shark/isa/nvram.h>
#include <shark/isa/spkrreg.h>
#include <shark/shark/hat.h>

void	sysbeepstop(void *);
void	sysbeep(int, int);
void	rtcinit(void);
int     timer_hz_to_count(int);

static void findcpuspeed(void);
static void delayloop(int);
static int  clockintr(void *);
static int  gettick(void);
static void tc_init_i8253(void);

void startrtclock(void);

inline unsigned mc146818_read(void *, unsigned);
inline void mc146818_write(void *, unsigned, unsigned);

inline unsigned
mc146818_read(void *sc, unsigned reg)
{

	outb(IO_RTC, reg);
	return (inb(IO_RTC+1));
}

inline void
mc146818_write(void *sc, unsigned reg, unsigned datum)
{

	outb(IO_RTC, reg);
	outb(IO_RTC+1, datum);
}

unsigned int count1024usec; /* calibrated loop variable (1024 microseconds) */

/* number of timer ticks in a Musec = 2^20 usecs */
#define TIMER_MUSECFREQ\
    (((((((TIMER_FREQ) * 1024) + 999) / 1000) * 1024) + 999) / 1000)
#define TIMER_MUSECDIV(x) ((TIMER_MUSECFREQ+(x)/2)/(x))

/*
 * microtime() makes use of the following globals.
 * timer_msb_table[] and timer_lsb_table[] are used to compute the 
 * microsecond increment.
 *
 * time.tv_usec += isa_timer_msb_table[cnt_msb] + isa_timer_lsb_table[cnt_lsb];
 */

u_short	isa_timer_lsb_table[256];	/* timer->usec conversion for LSB */

/* 64 bit counts from timer 0 */
struct count64 {
	unsigned lo;   /* low 32 bits */
	unsigned hi;   /* high 32 bits */
};

#define TIMER0_ROLLOVER  0xFFFF  /* maximum rollover for 8254 counter */

struct count64 timer0count;
struct count64 timer0_at_last_clockintr;
unsigned       timer0last;

/*#define TESTHAT*/
#ifdef TESTHAT
#define HATSTACKSIZE 1024
#define HATHZ  50000
#define HATHZ2 10000
unsigned char hatStack[HATSTACKSIZE];

unsigned testHatOn = 0;
unsigned nHats = 0;
unsigned nHatWedges = 0;
unsigned fiqReason = 0;
unsigned hatCount = 0;
unsigned hatCount2 = 0;

void hatTest(int testReason)
{

	fiqReason |= testReason;
	nHats++;
}

void hatWedge(int nFIQs)
{

	printf("Unwedging the HAT.  fiqs_happened = %d\n", nFIQs);
	nHatWedges++;
}
#endif

void
startrtclock(void)
{

	findcpuspeed();		/* use the clock (while it's free) to
				   find the CPU speed */

	timer0count.lo = 0;
	timer0count.hi = 0;
	timer0_at_last_clockintr.lo = 0;
	timer0_at_last_clockintr.hi = 0;
	timer0last     = 0;

	/* initialize 8253 clock */
	outb(IO_TIMER1 + TIMER_MODE, TIMER_SEL0|TIMER_RATEGEN|TIMER_16BIT);
	outb(IO_TIMER1 + TIMER_CNTR0, TIMER0_ROLLOVER % 256);
	outb(IO_TIMER1 + TIMER_CNTR0, TIMER0_ROLLOVER / 256);

#ifdef TESTHAT
	hatCount = timer_hz_to_count(HATHZ);
	hatCount2 = timer_hz_to_count(HATHZ2);
	printf("HAT test on @ %d Hz = %d ticks\n", HATHZ, hatCount);
#endif
}

int
timer_hz_to_count(int timer_hz)
{
	u_long tval;

	tval = (TIMER_FREQ * 2) / (u_long) timer_hz;
	tval = (tval / 2) + (tval & 0x1);

	return (int)tval;
}

void gettimer0count(struct count64 *);

/* must be called at SPL_CLOCK or higher */
void gettimer0count(struct count64 *pcount)
{
	unsigned current, ticks, oldlo;

	/*
	 * Latch the current value of the timer and then read it.
	 * This guarantees an atomic reading of the time.
	 */

	current = gettick();

	if (timer0last >= current)
		ticks = timer0last - current;
	else
		ticks = timer0last + (TIMER0_ROLLOVER - current);

	timer0last = current;

	oldlo = timer0count.lo;

	if (oldlo > (timer0count.lo = oldlo + ticks)) /* carry? */
		timer0count.hi++;

	*pcount = timer0count;
}

static int
clockintr(void *arg)
{
	struct clockframe *frame = arg;		/* not strictly necessary */
	extern void isa_specific_eoi(int irq);
#ifdef TESTHAT
	static int ticks = 0;
#endif
	static int hatUnwedgeCtr = 0;

	gettimer0count(&timer0_at_last_clockintr);

	mc146818_read(NULL, MC_REGC); /* clear the clock interrupt */

	/* check to see if the high-availability timer needs to be unwedged */
	if (++hatUnwedgeCtr >= (hz / HAT_MIN_FREQ)) {
		hatUnwedgeCtr = 0;
		hatUnwedge(); 
	}

#ifdef TESTHAT
	++ticks;

	if (testHatOn && ((ticks & 0x3f) == 0)) {
		if (testHatOn == 1) {
			hatClkAdjust(hatCount2);
			testHatOn = 2;
		} else {
			testHatOn = 0;
			hatClkOff();
			printf("hat off status: %d %d %x\n", nHats,
			    nHatWedges, fiqReason);
		}
	} else if (!testHatOn && (ticks & 0x1ff) == 0) {
		printf("hat on status: %d %d %x\n",
		    nHats, nHatWedges, fiqReason);
		testHatOn = 1;
		nHats = 0;
		fiqReason = 0;
		hatClkOn(hatCount, hatTest, 0xfeedface,
		    hatStack + HATSTACKSIZE - sizeof(unsigned),
		    hatWedge);
	}
#endif
	hardclock(frame);
	return(1);
}

static int
gettick(void)
{
	u_char lo, hi;
	u_int savedints;

	/* Don't want someone screwing with the counter while we're here. */
	savedints = disable_interrupts(I32_bit);
	/* Select counter 0 and latch it. */
	outb(IO_TIMER1 + TIMER_MODE, TIMER_SEL0 | TIMER_LATCH);
	lo = inb(IO_TIMER1 + TIMER_CNTR0);
	hi = inb(IO_TIMER1 + TIMER_CNTR0);
	restore_interrupts(savedints);
	return ((hi << 8) | lo);
}

/* modifications from i386 to shark isa version:
   - removed hardcoded "n -=" values that approximated the time to
     calculate delay ticks
   - made the time to calculate delay ticks almost negligable. 4 multiplies
     = maximum of 12 cycles = 75ns on a slow SA-110, plus a bunch of shifts;
     as opposed to 4 multiplies plus a bunch of divides.
   - removed i386 assembly language hack
   - put code in findcpuspeed that works even if FIRST_GUESS is orders
     of magnitude low
   - put code in delay() to use delayloop() for short delays
   - microtime no longer in assembly language
*/

/*
 * Wait "n" microseconds.
 * Relies on timer 1 counting down from (TIMER_FREQ / hz) at TIMER_FREQ Hz.
 * Note: timer had better have been programmed before this is first used!
 * (Note that we use `rate generator' mode, which counts at 1:1; `square
 * wave' mode counts at 2:1).
 */
void
delay(unsigned n)
{
	int ticks, otick;
	int nticks;

	if (n < 100) {
		/* it can take a long time (1 usec or longer) just for
		   1 ISA read, so it's best not to use the timer for
		   short delays */
		delayloop((n * count1024usec) >> 10);
		return;
	}

	/*
	 * Read the counter first, so that the rest of the setup overhead is
	 * counted.
	 */
	otick = gettick();

	/*
	 * Calculate ((n * TIMER_FREQ) / 1e6) without using floating point and
	 * without any avoidable overflows.
	 */
	{
	        /* a Musec = 2^20 usec */
		int Musec = n >> 20,
		    usec = n & ((1 << 20) - 1);
		nticks 
		  = (Musec * TIMER_MUSECFREQ) +
		    (usec * (TIMER_MUSECFREQ >> 20)) +
		    ((usec * ((TIMER_MUSECFREQ & ((1 <<20) - 1)) >>10)) >>10) +
		    ((usec * (TIMER_MUSECFREQ & ((1 << 10) - 1))) >> 20);
	}

	while (nticks > 0) {
		ticks = gettick();
		if (ticks > otick)
			nticks -= TIMER0_ROLLOVER - (ticks - otick);
		else
			nticks -= otick - ticks;
		otick = ticks;
	}

}

void
sysbeepstop(void *arg)
{
}

void
sysbeep(int pitch, int period)
{
}

#define FIRST_GUESS   0x2000

static void
findcpuspeed(void)
{
	int ticks;
	unsigned int guess = FIRST_GUESS;

	while (1) { /* loop until accurate enough */
		/* Put counter in count down mode */
		outb(IO_TIMER1 + TIMER_MODE,
		    TIMER_SEL0 | TIMER_16BIT | TIMER_RATEGEN);
		outb(IO_TIMER1 + TIMER_CNTR0, 0xff);
		outb(IO_TIMER1 + TIMER_CNTR0, 0xff);
		delayloop(guess);

		/* Read the value left in the counter */
		/*
		 * Formula for delaycount is:
		 *  (loopcount * timer clock speed) / (counter ticks * 1000)
		 */
		ticks = 0xFFFF - gettick();
		if (ticks == 0) ticks = 1; /* just in case */
		if (ticks < (TIMER_MUSECDIV(1024))) { /* not accurate enough */
			guess *= max(2, (TIMER_MUSECDIV(1024) / ticks));
			continue;
		}
		count1024usec = (guess * (TIMER_MUSECDIV(1024))) / ticks;
		return;
	}
}

static void
delayloop(int counts)
{
	while (counts--)
		__insn_barrier();
}

void
cpu_initclocks(void)
{
        unsigned hzval;

	printf("clock: hz=%d stathz = %d profhz = %d\n", hz, stathz, profhz);

	/* install RTC interrupt handler */
	(void)isa_intr_establish(NULL, IRQ_RTC, IST_LEVEL, IPL_CLOCK, 
				 clockintr, 0);

	/* set  up periodic interrupt @ hz
	   this is the subset of hz values in kern_clock.c that are
	   supported by the ISA RTC */
	switch (hz) {
	case 64:
		hzval = MC_RATE_64_Hz;
		break;
	case 128:
		hzval = MC_RATE_128_Hz;
		break;
	case 256:
		hzval = MC_RATE_256_Hz;
		break;
	case 1024:
		hzval = MC_RATE_1024_Hz;
		break;
	default:
		panic("cannot configure hz = %d", hz);
        }
	
	rtcinit(); /* make sure basics are done by now */

	/* blast values to set up clock interrupt */
	mc146818_write(NULL, MC_REGA, MC_BASE_32_KHz | hzval);
	/* enable periodic interrupt */
	mc146818_write(NULL, MC_REGB,
		       mc146818_read(NULL, MC_REGB) | MC_REGB_PIE);

	tc_init_i8253();
}

void
rtcinit(void)
{
	static int first_rtcopen_ever = 1;

	if (!first_rtcopen_ever)
		return;
	first_rtcopen_ever = 0;

	mc146818_write(NULL, MC_REGA,			/* XXX softc */
	    MC_BASE_32_KHz | MC_RATE_1024_Hz);
	mc146818_write(NULL, MC_REGB, MC_REGB_24HR);	/* XXX softc */
}

void
setstatclockrate(int arg)
{
}

static uint32_t
i8253_get_timecount(struct timecounter *tc)
{
	return (TIMER0_ROLLOVER - gettick());
}

void
tc_init_i8253(void)
{
	static struct timecounter i8253_tc = {
		.tc_get_timecount = i8253_get_timecount,
		.tc_counter_mask = TIMER0_ROLLOVER,
		.tc_frequency = TIMER_FREQ,
		.tc_name = "i8253",
		.tc_quality = 100
	};

	tc_init(&i8253_tc);
}

/* End of clock.c */
