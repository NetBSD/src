/*	$NetBSD: ofwgen_intr.c,v 1.2.8.1 2002/03/17 23:43:54 thorpej Exp $	*/

/*
 * Copyright (C) 1997 Wolfgang Solfrank.
 * Copyright (C) 1997 TooLs GmbH.
 * All rights reserved.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Software-simulated spl/interrupt routines.  Used in generic
 * OpenFirmware driver configurations.
 */

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/autoconf.h>

#include <ofppc/ofwgen/ofwgenvar.h>

int	ofwgen_splraise(int);
int	ofwgen_spllower(int);
void	ofwgen_splx(int);
void	ofwgen_setsoft(int);
void	ofwgen_clock_return(struct clockframe *, int);
void	*ofwgen_intr_establish(int, int, int, int (*)(void *), void *);
void	ofwgen_intr_disestablish(void *);

struct machvec ofwgen_machvec = {
	ofwgen_splraise,
	ofwgen_spllower,
	ofwgen_splx,
	ofwgen_setsoft,
	ofwgen_clock_return,
	ofwgen_intr_establish,
	ofwgen_intr_disestablish,
};

void	ofwgen_intr_calculate_masks(void);

#define	B(x)	(1U << (x))

/* Interrupts to mask at each level. */
static int imask[NIPL];

/* Current interrupt priority level. */
static __volatile int cpl;

/* Number of clock interrupts pending. */
static __volatile int clockpending;

/* Other interrupts pending. */
static __volatile int ipending;

void
ofwgen_intr_init(void)
{
	int ipl;

	/*
	 * Compute the initial interrupt masks.  Note that since we
	 * don't actually use interrupts, this will never change.
	 */

	for (ipl = 0; ipl < NIPL; ipl++)
		imask[ipl] = B(ipl);

	ofwgen_intr_calculate_masks();

	machine_interface = ofwgen_machvec;
}

void
ofwgen_intr_calculate_masks(void)
{

	imask[IPL_NONE] = 0;

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_SOFT];

	/*
	 * splsoftnet() must also block IPL_SOFTCLOCK, since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];

	/*
	 * Enfore a heirarchy that gives "slow" devices (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	imask[IPL_VM] |= imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	imask[IPL_AUDIO] |= imask[IPL_VM];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];

	/*
	 * splhigh() must block "everything".
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * XXX We need serial drivers to run at the absolute highest priority
	 * in order to avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];
}

static void
do_pending_int(void)
{
	int emsr, dmsr, new;

	__asm __volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;

	new = cpl;

	for (;;) {
		cpl = new;

		__asm __volatile ("mtmsr %0" :: "r"(dmsr));
		if (clockpending && (cpl & B(IPL_CLOCK)) == 0) {
			struct clockframe frame;

			cpl |= imask[IPL_CLOCK];
			clockpending--;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));

			/*
			 * Fake a clock interrupt frame
			 */
			frame.pri = new;
			frame.depth = intr_depth + 1;
			frame.srr1 = 0;
			frame.srr0 = (int)ofwgen_splx;
			/*
			 * Do standard timer interrupt stuff
			 */
			hardclock(&frame);
			continue;
		}
		/*
		 * Check for software interrupts.
		 */
		if ((ipending & B(IPL_SOFTCLOCK)) != 0 &&
		    (cpl & B(IPL_SOFTCLOCK)) == 0) {
			cpl |= imask[IPL_SOFTCLOCK];
			ipending &= ~B(IPL_SOFTCLOCK);
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softclock(NULL);
			continue;
		}
		if ((ipending & B(IPL_SOFTNET)) != 0 &&
		    (cpl & B(IPL_SOFTNET)) == 0) {
			cpl |= imask[IPL_SOFTNET];
			ipending &= ~B(IPL_SOFTNET);
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}
		if ((ipending & B(IPL_SOFT)) != 0 &&
		    (cpl & B(IPL_SOFT)) == 0) {
			cpl |= imask[IPL_SOFT];
			ipending &= ~B(IPL_SOFT);
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
#if 0 /* XXX notyet */
			softintr_dispatch(&softintrs[SI_SOFT]);
#endif
			continue;
		}

		__asm __volatile ("mtmsr %0" :: "r"(emsr));
		return;
	}
}

int
ofwgen_splraise(int ipl)
{
	int old;

	old = cpl;
	cpl |= imask[ipl];

	return (old);
}

int
ofwgen_spllower(int ipl)
{
	int old = cpl;

	splx(imask[ipl]);
	return (old);
}

void
ofwgen_splx(int new)
{

	cpl = new;
	do_pending_int();
}

void
ofwgen_setsoft(int ipl)
{
	int msr;

	__asm __volatile ("mfmsr %0" : "=r"(msr));
	__asm __volatile ("mtmsr %0" :: "r"(msr & ~PSL_EE));
	ipending |= B(ipl);
	__asm __volatile ("mtmsr %0" :: "r"(msr));

	if ((cpl & B(ipl)) == 0)
		splx(cpl);
}

void *
ofwgen_intr_establish(int irq, int ipl, int ist,
    int (*func)(void *), void *arg)
{

	panic("ofwgen_intr_establish");
}

void
ofwgen_intr_disestablish(void *cookie)
{

	panic("ofwgen_intr_disestablish");
}

/*
 * This one is similar to ofwgen_splx, but returns with interrupts disabled.
 * It is intended for use during interrupt exit (as the name implies :-)).
 */
static void
intr_return(struct clockframe *frame, int level)
{
	int emsr, dmsr;

	__asm __volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;

	for (;;) {
		cpl = level;

		__asm __volatile ("mtmsr %0" :: "r"(dmsr));
		if (clockpending && (cpl & B(IPL_CLOCK)) == 0) {
			cpl |= imask[IPL_CLOCK];
			clockpending--;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));

			/*
			 * Do standard timer interrupt stuff
			 */
			hardclock(frame);
			continue;
		}
		/*
		 * Check for software interrupts.
		 */
		if ((ipending & B(IPL_SOFTCLOCK)) != 0 &&
		    (cpl & B(IPL_SOFTCLOCK)) == 0) {
			cpl |= imask[IPL_SOFTCLOCK];
			ipending &= ~B(IPL_SOFTCLOCK);
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softclock(NULL);
			continue;
		}
		if ((ipending & B(IPL_SOFTNET)) != 0 &&
		    (cpl & B(IPL_SOFTNET)) == 0) {
			cpl |= imask[IPL_SOFTNET];
			ipending &= ~B(IPL_SOFTNET);
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}
		if ((ipending & B(IPL_SOFT)) != 0 &&
		    (cpl & B(IPL_SOFT)) == 0) {
			cpl |= imask[IPL_SOFT];
			ipending &= ~B(IPL_SOFT);
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
#if 0 /* XXX notyet */
			softintr_dispatch(&softintrs[SI_SOFT]);
#endif
			continue;
		}
		break;
	}
}

void
ofwgen_clock_return(struct clockframe *frame, int nticks)
{
	int pri, msr;

	pri = cpl;
	
	if (pri & B(IPL_CLOCK))
		clockpending += nticks;
	else {
		cpl = pri | imask[IPL_CLOCK];

		/* Reenable interrupts. */
		__asm __volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
		    : "=r"(msr)
		    : "K"((u_short)PSL_EE));

		/*
		 * Do standard timer interrupt stuff.  Do softclock stuff
		 * only on the last iteration.
		 */
		frame->pri = pri | B(IPL_SOFTCLOCK);
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);
	}
	intr_return(frame, pri);
}
