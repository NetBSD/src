/*	$NetBSD: extintr.c,v 1.4 2002/07/05 18:45:20 matt Exp $	*/

/*
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Lennart Augustsson (lennart@augustsson.net) at Sandburst Corp.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*-
 * Copyright (c) 1995 Per Fogelstrom
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
 *	@(#)isa.c	7.2 (Berkeley) 5/12/91
 */

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <machine/intr.h>
#include <machine/intrpriv.h>
#include <machine/pio.h>
#include <machine/psl.h>
#include <machine/bus.h>

#include <dev/ic/cpc700reg.h>
#include <dev/ic/cpc700uic.h>

#include "com.h"
#if NCOM > 0
extern void comsoft(void);
#endif

const char intrnames[] =
	"clock\0wrr\0wrc\0com0\0" /* clock == ecc */
	"com1\0iic0\0iic1\0gpt0cmp\0"
	"gpt1cmp\0gpt2cmp\0gpt3cmp\0gpt4cmp\0"
	"gpt0cap\0gpt1cap\0gpt2cap\0gpt3cap\0"
	"gpt4cap\0unk0\0unk1\0unk2\0"
	"pciA\0pciB\0pciC\0pciD\0"
	"cs\0rtc\0unk3\0unk4\0"
	"unk5\0snet\0sclk\0sser"; /* unk6-unk8 */
long intrcnt[32];

const char eintrnames[] = "";
long eintrcnt[32];


u_int32_t imen = 0xffffffff;
volatile int cpl, ipending, tickspending;
int imask[NIPL];		/* XXX type */
u_int32_t intrtype[ICU_LEN], intrmask[ICU_LEN], intrlevel[ICU_LEN];
struct intrhand *intrhand[ICU_LEN];

void intr_calculatemasks(void);
int fakeintr(void *);
void ext_intr(void);
char *intr_typename(int);

int
fakeintr(void *arg)
{
	return 0;
}

void setleds(int);

/*
 * external interrupt handler
 */
void
ext_intr()
{
	int irq, intbit, pcpl;
	struct intrhand *ih;
#if RECINTR
	int msr;
#endif

	pcpl = cpl;		/* save mask */
#if RECINTR
	__asm volatile ("mfmsr %0" : "=r"(msr)); /* and msr */
#endif

	for (;;) {
		irq = cpc700_read_irq(); /* get high pri intr */
		if (irq < 0)
			break;

		intbit = CPC_INTR_MASK(irq);
		if (pcpl & intbit) {
			/* Interrupt is masked -- mark it as pending */
			ipending |= intbit;
			cpc700_disable_irq(irq);
		} else {
#if RECINTR
			/* XXX disbale irq? */
			splraise(intrmask[irq]);
			__asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
#endif
			ih = intrhand[irq];
			while (ih) {
				(*ih->ih_fun)(ih->ih_arg);
				ih = ih->ih_next;
			}
#if RECINTR
			asm volatile ("mtmsr %0" :: "r"(msr));
			cpl = pcpl; /* restore splraise() */
#endif

			uvmexp.intrs++;
			intrcnt[irq]++;

		}
		cpc700_eoi(irq);
	}

#if RECINTR
	asm volatile ("mtmsr %0" :: "r"(msr | PSL_EE));
	splx(pcpl); /* also processes pendings */
	asm volatile ("mtmsr %0" :: "r"(msr));
#else
	splx(pcpl); /* also processes pendings */
#endif
}

char *
intr_typename(type)
{
	switch (type) {
	case IST_NONE:
		return "none";
	case IST_PULSE:
		return "pulsed";
	case IST_EDGE:
		return "edge-triggered";
	case IST_LEVEL:
		return "level-triggered";
	default:
		panic("intr_typename: invalid type %d", type);
		return "unknown";
	}
}

/*
 * Register an interrupt handler.
 */
void *
intr_establish(int irq, int type, int lvl, int (*ih_fun)(void *), void *ih_arg)
{
	struct intrhand **p, *q, *ih;
	static struct intrhand fakehand = { fakeintr };

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("isa_intr_establish: can't malloc handler info");

	if (!LEGAL_IRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq(%d) or type(%d)", irq, type);

	switch (intrtype[irq]) {
	case IST_NONE:
		intrtype[irq] = type;
		break;
	case IST_EDGE:
	case IST_LEVEL:
		if (type == intrtype[irq])
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    intr_typename(intrtype[irq]),
			    intr_typename(type));
		break;
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = lvl;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = lvl;
	ih->ih_irq = irq;
	*p = ih;

	return (ih);
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(void *arg)
{
	struct intrhand *ih = arg;
	int irq = ih->ih_irq;
	struct intrhand **p, *q;

	if (!LEGAL_IRQ(irq))
		panic("intr_disestablish: bogus irq");

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &intrhand[irq]; (q = *p) != NULL && q != ih; p = &q->ih_next)
		;
	if (q)
		*p = q->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	free((void *)ih, M_DEVBUF);

	intr_calculatemasks();

	if (intrhand[irq] == NULL)
		intrtype[irq] = IST_NONE;
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
void
intr_calculatemasks()
{
	int irq, level;
	struct intrhand *q;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int levels = 0;
		for (q = intrhand[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= CPC_INTR_MASK(irq);
		imask[level] = irqs | SINT_MASK;
	}

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] = SINT_SERIAL;
	imask[IPL_SOFTNET] = SINT_NET;
	imask[IPL_SOFTSERIAL] = SINT_SERIAL;

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	imask[IPL_SOFTCLOCK] |= imask[IPL_NONE];
	imask[IPL_SOFTNET] |= imask[IPL_SOFTCLOCK];
	imask[IPL_BIO] |= imask[IPL_SOFTNET];
	imask[IPL_NET] |= imask[IPL_BIO];
	imask[IPL_SOFTSERIAL] |= imask[IPL_NET];
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/*
	 * There are tty, network and disk drivers that use free() at interrupt
	 * time, so imp > (tty | net | bio).
	 */
	imask[IPL_IMP] |= imask[IPL_TTY];

#ifdef IPL_AUDIO
	imask[IPL_AUDIO] |= imask[IPL_IMP];
#endif

	/*
	 * Since run queues may be manipulated by both the statclock and tty,
	 * network, and disk drivers, clock > imp.
	 */
	imask[IPL_CLOCK] |= SPL_CLOCK;		/* block the clock */
#ifdef IPL_AUDIO
	imask[IPL_CLOCK] |= imask[IPL_AUDIO];
#else
	imask[IPL_CLOCK] |= imask[IPL_IMP];
#endif

	/*
	 * IPL_HIGH must block everything that can manipulate a run queue.
	 */
	imask[IPL_HIGH] |= imask[IPL_CLOCK];

	/*
	 * We need serial drivers to run at the absolute highest priority to
	 * avoid overruns, so serial > high.
	 */
	imask[IPL_SERIAL] |= imask[IPL_HIGH];

	/* And eventually calculate the complete masks. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int irqs = CPC_INTR_MASK(irq);
		for (q = intrhand[irq]; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		intrmask[irq] = irqs;
	}

	for (irq = 0; irq < ICU_LEN; irq++)
		if (intrhand[irq])
			cpc700_enable_irq(irq);
		else
			cpc700_disable_irq(irq);
		
#if 0
	{
	extern void print_intr_regs(void);
	print_intr_regs();
	}
#endif
}

void
do_pending_int(void)
{
	struct intrhand *ih;
	int irq;
	int pcpl;
	int hwpend;
	int emsr, dmsr;
	static int processing;
	extern long intrcnt[];

	if (processing)
		return;

	processing = 1;
	asm volatile("mfmsr %0" : "=r"(emsr));
	dmsr = emsr & ~PSL_EE;
	asm volatile("mtmsr %0" :: "r"(dmsr));

	pcpl = splhigh();		/* Turn off all */
	hwpend = ipending & ~pcpl;	/* Do now unmasked pendings */
	hwpend &= ICU_MASK;		/* Except soft ints */
	ipending &= ~hwpend;
	imen &= ~hwpend;
	while (hwpend) {
		irq = ICU_LEN - ffs(hwpend);
		hwpend &= ~CPC_INTR_MASK(irq);
		ih = intrhand[irq];
		while(ih) {
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}

		intrcnt[irq]++;
		cpc700_enable_irq(irq);
	}

	if ((ipending & ~pcpl) & SINT_CLOCK) {
		ipending &= ~SINT_CLOCK;
		softclock(NULL);
		intrcnt[CNT_SINT_CLOCK]++;
	}
	if ((ipending & ~pcpl) & SINT_NET) {
		int pisr = netisr;
		netisr = 0;
		ipending &= ~SINT_NET;
		softnet(pisr);
		intrcnt[CNT_SINT_NET]++;
	}
	if ((ipending & ~pcpl) & SINT_SERIAL) {
		ipending &= ~SINT_SERIAL;
#if NCOM > 0
		comsoft();
#endif
		intrcnt[CNT_SINT_SERIAL]++;
	}
	cpl = pcpl;	/* Don't use splx... we are here already! */
	processing = 0;
	asm volatile("mtmsr %0" :: "r"(emsr));
}
