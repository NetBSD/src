/*	$NetBSD: soft_spl.c,v 1.1 1997/04/16 21:20:35 thorpej Exp $	*/

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
#include <sys/param.h>


static int soft_splhigh __P((void));
static int soft_spl0 __P((void));
static int soft_splbio __P((void));
static int soft_splnet __P((void));
static int soft_spltty __P((void));
static int soft_splimp __P((void));
static int soft_splclock __P((void));
static int soft_splsoftclock __P((void));
static int soft_splsoftnet __P((void));
static int soft_splx __P((int));
static void soft_setsoftclock __P((void));
static void soft_setsoftnet __P((void));
static void soft_clock_return __P((struct clockframe *, int));
static void soft_irq_establish __P((int, int, void (*)(void *), void *));

struct machvec soft_machvec = {
	soft_splhigh,
	soft_spl0,
	soft_splbio,
	soft_splnet,
	soft_spltty,
	soft_splimp,
	soft_splclock,
	soft_splsoftclock,
	soft_splsoftnet,
	soft_splx,
	soft_setsoftclock,
	soft_setsoftnet,
	soft_clock_return,
	soft_irq_establish,
};

static int cpl;
/*
 * Current processor level.
 */
#define	SPLBIO		0x01
#define	SPLNET		0x02
#define	SPLTTY		0x04
#define	SPLIMP		0x08
#define	SPLSOFTCLOCK	0x10
#define	SPLSOFTNET	0x20
#define	SPLCLOCK	0x80

static int clockpending, softclockpending, softnetpending;

static int
splraise(bits)
	int bits;
{
	int old;
	
	old = cpl;
	cpl |= bits;

	return old;
}

static int
soft_splx(new)
	int new;
{
	int pending, old = cpl;
	int emsr, dmsr;

	asm volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	
	cpl = new;
	
	while (1) {
		cpl = new;

		asm volatile ("mtmsr %0" :: "r"(dmsr));
		if (clockpending && !(cpl & SPLCLOCK)) {
			struct clockframe frame;
			extern int intr_depth;

			cpl |= SPLCLOCK;
			clockpending--;
			asm volatile ("mtmsr %0" :: "r"(emsr));

			/*
			 * Fake a clock interrupt frame
			 */
			frame.pri = new;
			frame.depth = intr_depth + 1;
			frame.srr1 = 0;
			frame.srr0 = (int)soft_splx;
			/*
			 * Do standard timer interrupt stuff
			 */
			hardclock(&frame);
			continue;
		}
		if (softclockpending && !(cpl & SPLSOFTCLOCK)) {
			cpl |= SPLSOFTCLOCK;
			softclockpending = 0;
			asm volatile ("mtmsr %0" :: "r"(emsr));
			softclock();
			continue;
		}
		if (softnetpending && !(cpl & SPLSOFTNET)) {
			cpl |= SPLSOFTNET;
			softnetpending = 0;
			asm volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}
		
		asm volatile ("mtmsr %0" :: "r"(emsr));
		return old;
	}
}

static int
soft_splhigh()
{
	return splraise(-1);
}

static int
soft_spl0()
{
	return soft_splx(0);
}

static int
soft_splbio()
{
	return splraise(SPLBIO | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
soft_splnet()
{
	return splraise(SPLNET | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
soft_spltty()
{
	return splraise(SPLTTY | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
soft_splimp()
{
	return splraise(SPLIMP | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
soft_splclock()
{
	return splraise(SPLCLOCK | SPLSOFTCLOCK | SPLSOFTNET);
}

static int
soft_splsoftclock()
{
	return splraise(SPLSOFTCLOCK);
}

static int
soft_splsoftnet()
{
	return splraise(SPLSOFTNET);
}

static void
soft_setsoftclock()
{
	softclockpending = 1;
	if (!(cpl & SPLSOFTCLOCK))
		soft_splx(cpl);
}

static void
soft_setsoftnet()
{
	softnetpending = 1;
	if (!(cpl & SPLSOFTNET))
		soft_splx(cpl);
}

static void
soft_irq_establish(irq, level, handler, arg)
	int irq, level;
	void (*handler) __P((void *));
	void *arg;
{
	panic("soft_irq_establish");
}

/*
 * This one is similar to soft_splx, but returns with interrupts disabled.
 * It is intended for use during interrupt exit (as the name implies :-)).
 */
static void
intr_return(frame, level)
	struct clockframe *frame;
	int level;
{
	int pending, old = cpl;
	int emsr, dmsr;

	asm volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;

	cpl = level;

	while (1) {
		cpl = level;

		asm volatile ("mtmsr %0" :: "r"(dmsr));
		if (clockpending && !(cpl & SPLCLOCK)) {
			extern int intr_depth;

			cpl |= SPLCLOCK;
			clockpending--;
			asm volatile ("mtmsr %0" :: "r"(emsr));

			/*
			 * Do standard timer interrupt stuff
			 */
			hardclock(frame);
			continue;
		}
		if (softclockpending && !(cpl & SPLSOFTCLOCK)) {
			
			cpl |= SPLSOFTCLOCK;
			softclockpending = 0;
			asm volatile ("mtmsr %0" :: "r"(emsr));
			
			softclock();
			continue;
		}
		if (softnetpending && !(cpl & SPLSOFTNET)) {
			cpl |= SPLSOFTNET;
			softnetpending = 0;
			asm volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}
		break;
	}
}

static void
soft_clock_return(frame, nticks)
	struct clockframe *frame;
	int nticks;
{
	int pri;
	int msr;

	pri = cpl;
	
	if (pri & SPLCLOCK)
		clockpending += nticks;
	else {
		cpl = pri | SPLCLOCK | SPLSOFTCLOCK | SPLSOFTNET;

		/*
		 * Reenable interrupts
		 */
		asm volatile ("mfmsr %0; ori %0,%0,%1; mtmsr %0"
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
