/*	$NetBSD: ofwgen_intr.c,v 1.1 2001/10/22 23:01:19 thorpej Exp $	*/

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

static int ofwgen_splhigh(void);
static int ofwgen_spl0(void);
static int ofwgen_splbio(void);
static int ofwgen_splnet(void);
static int ofwgen_spltty(void);
static int ofwgen_splvm(void);
static int ofwgen_splclock(void);
static int ofwgen_spllowersoftclock(void);
static int ofwgen_splsoftclock(void);
static int ofwgen_splsoftnet(void);
static int ofwgen_splx(int);
static void ofwgen_setsoftclock(void);
static void ofwgen_setsoftnet(void);
static void ofwgen_clock_return(struct clockframe *, int);
static void ofwgen_irq_establish(int, int, void (*)(void *), void *);

struct machvec ofwgen_machvec = {
	ofwgen_splhigh,
	ofwgen_spl0,
	ofwgen_splbio,
	ofwgen_splnet,
	ofwgen_spltty,
	ofwgen_splvm,
	ofwgen_splclock,
	ofwgen_spllowersoftclock,
	ofwgen_splsoftclock,
	ofwgen_splsoftnet,
	ofwgen_splx,
	ofwgen_setsoftclock,
	ofwgen_setsoftnet,
	ofwgen_clock_return,
	ofwgen_irq_establish,
};

/*
 * Current interrupt priority level.
 */
static int cpl;

#define	SPLBIO		0x01
#define	SPLNET		0x02
#define	SPLTTY		0x04
#define	SPLIMP		0x08
#define	SPLSOFTCLOCK	0x10
#define	SPLSOFTNET	0x20
#define	SPLCLOCK	0x80

static int clockpending, softclockpending, softnetpending;

static int
splraise(int bits)
{
	int old;
	
	old = cpl;
	cpl |= bits;

	return old;
}

static int
ofwgen_splx(int new)
{
	int old = cpl;
	int emsr, dmsr;

	__asm __volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	
	cpl = new;
	
	while (1) {
		cpl = new;

		__asm __volatile ("mtmsr %0" :: "r"(dmsr));
		if (clockpending && !(cpl & SPLCLOCK)) {
			struct clockframe frame;

			cpl |= SPLCLOCK;
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
		if (softclockpending && !(cpl & SPLSOFTCLOCK)) {
			cpl |= SPLSOFTCLOCK;
			softclockpending = 0;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softclock(NULL);
			continue;
		}
		if (softnetpending && !(cpl & SPLSOFTNET)) {
			cpl |= SPLSOFTNET;
			softnetpending = 0;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}
		
		__asm __volatile ("mtmsr %0" :: "r"(emsr));
		return old;
	}
}

static int
ofwgen_splhigh(void)
{

	return splraise(-1);
}

static int
ofwgen_spl0(void)
{

	return ofwgen_splx(0);
}

static int
ofwgen_splbio(void)
{

	return splraise(SPLBIO | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
ofwgen_splnet(void)
{

	return splraise(SPLNET | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
ofwgen_spltty(void)
{

	return splraise(SPLTTY | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
ofwgen_splvm(void)
{

	return splraise(SPLIMP | SPLBIO | SPLNET | SPLTTY | SPLSOFTCLOCK |
	    SPLSOFTNET);
}

static int
ofwgen_splclock(void)
{

	return splraise(SPLCLOCK | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
ofwgen_spllowersoftclock(void)
{

	return ofwgen_splx(SPLSOFTCLOCK);
}

static int
ofwgen_splsoftclock(void)
{

	return splraise(SPLSOFTCLOCK);
}

static int
ofwgen_splsoftnet(void)
{

	/* splsoftnet() needs to block softclock */
	return splraise(SPLSOFTNET|SPLSOFTCLOCK);
}

static void
ofwgen_setsoftclock(void)
{

	softclockpending = 1;
	if (!(cpl & SPLSOFTCLOCK))
		ofwgen_splx(cpl);
}

static void
ofwgen_setsoftnet(void)
{

	softnetpending = 1;
	if (!(cpl & SPLSOFTNET))
		ofwgen_splx(cpl);
}

static void
ofwgen_irq_establish(int irq, int level, void (*handler)(void *), void *arg)
{

	panic("ofwgen_irq_establish");
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

	cpl = level;

	while (1) {
		cpl = level;

		__asm __volatile ("mtmsr %0" :: "r"(dmsr));
		if (clockpending && !(cpl & SPLCLOCK)) {
			cpl |= SPLCLOCK;
			clockpending--;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));

			/*
			 * Do standard timer interrupt stuff
			 */
			hardclock(frame);
			continue;
		}
		if (softclockpending && !(cpl & SPLSOFTCLOCK)) {
			
			cpl |= SPLSOFTCLOCK;
			softclockpending = 0;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			
			softclock(NULL);
			continue;
		}
		if (softnetpending && !(cpl & SPLSOFTNET)) {
			cpl |= SPLSOFTNET;
			softnetpending = 0;
			__asm __volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}
		break;
	}
}

static void
ofwgen_clock_return(struct clockframe *frame, int nticks)
{
	int pri, msr;

	pri = cpl;
	
	if (pri & SPLCLOCK)
		clockpending += nticks;
	else {
		cpl = pri | SPLCLOCK | SPLSOFTCLOCK | SPLSOFTNET;

		/*
		 * Reenable interrupts
		 */
		__asm __volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
			      : "=r"(msr) : "K"((u_short)PSL_EE));
		
		/*
		 * Do standard timer interrupt stuff.
		 * Do softclock stuff only on the last iteration.
		 */
		frame->pri = pri | SPLSOFTCLOCK;
		while (--nticks > 0)
			hardclock(frame);
		frame->pri = pri;
		hardclock(frame);
	}
	intr_return(frame, pri);
}
