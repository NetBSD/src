/*	$NetBSD: soft_spl.c,v 1.1 1997/10/14 06:47:51 sakamoto Exp $	*/

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
#include <machine/intr.h>

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
static void do_sir __P((int, int, int));

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

/*
 * Current processor level.
 */
int cpl;
int softclockpending, softnetpending;

static int
splraise(new)
	int new;
{
	int old = cpl;
	int emsr, dmsr;

	asm volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	asm volatile ("mtmsr %0" :: "r"(dmsr));
	cpl = new;
	ext_imask_set(cpl);
	if (cpl > IPL_CLOCK)
		asm volatile ("mtmsr %0" :: "r"(emsr));

	return (old);
}

static int
soft_splx(new)
	int new;
{
	int old = cpl;
	int dmsr, emsr;

	asm volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	asm volatile ("mtmsr %0" :: "r"(dmsr));

	ext_imask_set(new);
	do_sir(new, emsr, dmsr);

	if (cpl > IPL_CLOCK)
		asm volatile ("mtmsr %0" :: "r"(emsr));
	return (old);
}

static int
soft_splhigh()
{
	return (splraise(IPL_HIGH));
}

static int
soft_spl0()
{
	return (soft_splx(IPL_NONE));
}

static int
soft_splbio()
{
	return (splraise(IPL_BIO));
}

static int
soft_splnet()
{
	return (splraise(IPL_NET));
}

static int
soft_spltty()
{
	return (splraise(IPL_TTY));
}

static int
soft_splimp()
{
	return (splraise(IPL_IMP));
}

static int
soft_splclock()
{
	return (splraise(IPL_CLOCK));
}

static int
soft_splsoftclock()
{
	return (splraise(IPL_SOFTCLOCK));
}

static int
soft_splsoftnet()
{
	return (splraise(IPL_SOFTNET));
}

static void
soft_setsoftclock()
{
	softclockpending = 1;
}

static void
soft_setsoftnet()
{
	softnetpending = 1;
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
void
intr_return()
{
	int pending, old = cpl;
	int emsr, dmsr;

	asm volatile ("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;

	do_sir(old, emsr, dmsr);

	asm volatile ("mtmsr %0" :: "r"(dmsr));
}

static void
soft_clock_return(frame, nticks)
	struct clockframe *frame;
	int nticks;
{

	while (--nticks > 0)
		hardclock(frame);
	hardclock(frame);
}

void
do_sir(pl, emsr, dmsr)
	int pl;
	int emsr;
	int dmsr;
{
	extern long intrcnt[];

	for (;;) {
		asm volatile ("mtmsr %0" :: "r"(dmsr));
		cpl = pl;

		if (softclockpending && (cpl > IPL_SOFTCLOCK)) {
			intrcnt[30]++;		/* XXX */
			softclockpending = 0;
			cpl = IPL_SOFTCLOCK;
			asm volatile ("mtmsr %0" :: "r"(emsr));
			softclock();
			continue;
		}
		if (softnetpending && (cpl > IPL_SOFTNET)) {
			intrcnt[29]++;		/* XXX */
			softnetpending = 0;
			cpl = IPL_SOFTNET;
			asm volatile ("mtmsr %0" :: "r"(emsr));
			softnet();
			continue;
		}

		break;
	}
}
