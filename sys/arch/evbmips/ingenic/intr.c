/*	$NetBSD: intr.c,v 1.9.2.4 2016/10/05 20:55:27 skrll Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.9.2.4 2016/10/05 20:55:27 skrll Exp $");

#define __INTR_PRIVATE

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/device.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/timetc.h>
#include <sys/bitops.h>

#include <mips/locore.h>
#include <machine/intr.h>

#include <mips/ingenic/ingenic_regs.h>

#include "opt_ingenic.h"

#ifdef INGENIC_INTR_DEBUG
#define DPRINTF printf
#else
#define DPRINTF while (0) printf
#endif

extern void ingenic_clockintr(struct clockframe *);
extern void ingenic_puts(const char *);
/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
static const struct ipl_sr_map ingenic_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1,
	[IPL_VM] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_SCHED] =
	    MIPS_SOFT_INT_MASK_0 | MIPS_SOFT_INT_MASK_1 |
	    MIPS_INT_MASK_0 |
	    MIPS_INT_MASK_1 |
	    MIPS_INT_MASK_2 |
	    MIPS_INT_MASK_3 |
	    MIPS_INT_MASK_4 |
	    MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =            MIPS_INT_MASK,
    },
};

#define NINTR 64

/* some timer channels share interrupts, couldn't find any others */
struct intrhand {
	struct evcnt ih_count;
	char ih_name[16];
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_ipl;
};

struct intrhand intrs[NINTR];
struct evcnt clockintrs;

void ingenic_irq(int);

void
evbmips_intr_init(void)
{
	uint32_t reg;
	int i;

	ipl_sr_map = ingenic_ipl_sr_map;

	evcnt_attach_dynamic(&clockintrs,
	    EVCNT_TYPE_INTR, NULL, "timer", "intr");

	/* zero all handlers */
	for (i = 0; i < NINTR; i++) {
		intrs[i].ih_func = NULL;
		intrs[i].ih_arg = NULL;
		snprintf(intrs[i].ih_name, sizeof(intrs[i].ih_name),
		    "irq %d", i);
		evcnt_attach_dynamic(&intrs[i].ih_count, EVCNT_TYPE_INTR,
		    NULL, "INTC", intrs[i].ih_name);
	}

	/* mask all peripheral IRQs */
	writereg(JZ_ICMR0, 0xffffffff);
	writereg(JZ_ICMR1, 0xffffffff);

	/* allow peripheral interrupts to core 0 only */
	reg = MFC0(12, 4);	/* reset entry and interrupts */
	reg &= 0xffff0000;
	reg |= REIM_IRQ0_M | REIM_MIRQ0_M;
#ifdef MULTIPROCESSOR
	reg |= REIM_MIRQ1_M;
#endif
	MTC0(reg, 12, 4);
	MTC0(0, 20, 1);	/* ping the 2nd core */
	DPRINTF("%s %08x\n", __func__, reg);
}

void
evbmips_iointr(int ipl, uint32_t ipending, struct clockframe *cf)
{
	uint32_t id;
#ifdef INGENIC_INTR_DEBUG
	char buffer[256];

#if 0
	snprintf(buffer, 256, "pending: %08x CR %08x\n", ipending,
	    MFC0(MIPS_COP_0_CAUSE, 0));
	ingenic_puts(buffer);
#endif
#endif
	/* see which core we're on */
	id = MFC0(15, 1) & 7;

	/*
	 * XXX
	 * the manual counts the softint bits as INT0 and INT1, our headers
	 * don't so everything here looks off by two
	 */
	if (ipending & MIPS_INT_MASK_1) {
		/*
		 * this is a mailbox interrupt / IPI
		 */
		uint32_t reg;
		int s = splsched();

		/* read pending IPIs */
		reg = MFC0(12, 3);
		if (id == 0) {
			if (reg & CS_MIRQ0_P) {
#ifdef MULTIPROCESSOR
				uint32_t tag;
				tag = MFC0(CP0_CORE_MBOX, 0);

				ipi_process(curcpu(), tag);
#ifdef INGENIC_INTR_DEBUG
				snprintf(buffer, 256,
				    "IPI for core 0, msg %08x\n", tag);
				ingenic_puts(buffer);
#endif
#endif
				reg &= (~CS_MIRQ0_P);
				/* clear it */
				MTC0(reg, 12, 3);
			}
		} else if (id == 1) {
			if (reg & CS_MIRQ1_P) {
#ifdef MULTIPROCESSOR
				uint32_t tag;
				tag = MFC0(CP0_CORE_MBOX, 1);
				ingenic_puts("1");
				if (tag & 0x400)
					hardclock(cf);
				//ipi_process(curcpu(), tag);
#ifdef INGENIC_INTR_DEBUG
				snprintf(buffer, 256,
				    "IPI for core 1, msg %08x\n", tag);
				ingenic_puts(buffer);
#endif
#endif
				reg &= (~CS_MIRQ1_P);
				/* clear it */
				MTC0(reg, 12, 3);
			}
		}
		splx(s);
	}
	if (ipending & MIPS_INT_MASK_2) {
		/* this is a timer interrupt */
		ingenic_clockintr(cf);
		clockintrs.ev_count++;
		ingenic_puts("INT2\n");
	}
	if (ipending & MIPS_INT_MASK_0) {
		uint32_t mask;
		/* peripheral interrupt */

		/*
		 * XXX
		 * OS timer interrupts are supposed to show up as INT2 as well
		 * but I haven't seen them there so for now we just weed them
		 * out right here.
		 * The idea is to allow peripheral interrupts on both cores but
		 * block INT0 on core1 so it would see only timer interrupts
		 * and IPIs. If that doesn't work we'll have to send an IPI to
		 * core1 for each timer tick.
		 */
		mask = readreg(JZ_ICPR0);
		if (mask & 0x0c000000) {
			writereg(JZ_ICMSR0, 0x0c000000);
			ingenic_clockintr(cf);
			writereg(JZ_ICMCR0, 0x0c000000);
			clockintrs.ev_count++;
		}
		ingenic_irq(ipl);
		KASSERT(id == 0);
	}
}

void
ingenic_irq(int ipl)
{
	uint32_t irql, irqh, mask, ll, hh;
	int bit, idx, bail;
#ifdef INGENIC_INTR_DEBUG
	char buffer[16];
#endif

	irql = readreg(JZ_ICPR0);
	irqh = readreg(JZ_ICPR1);
#ifdef INGENIC_INTR_DEBUG
	if (irql != 0) {
		snprintf(buffer, 16, " il%08x", irql);
		ingenic_puts(buffer);
	}
#endif
	bail = 32;
	ll = irql;
	hh = irqh;
	writereg(JZ_ICMSR0, ll);
	writereg(JZ_ICMSR1, hh);
	bit = ffs32(irql);
	while (bit != 0) {
		idx = bit - 1;
		mask = 1 << idx;
		intrs[idx].ih_count.ev_count++;
		if (intrs[idx].ih_func != NULL) {
			if (intrs[idx].ih_ipl == IPL_VM)
				KERNEL_LOCK(1, NULL);
			intrs[idx].ih_func(intrs[idx].ih_arg);
			if (intrs[idx].ih_ipl == IPL_VM)
				KERNEL_UNLOCK_ONE(NULL);
		} else {
			/* spurious interrupt, mask it */
			writereg(JZ_ICMSR0, mask);
		}
		irql &= ~mask;
		bit = ffs32(irql);
		bail--;
		KASSERT(bail > 0);
	}

#ifdef INGENIC_INTR_DEBUG
	if (irqh != 0) {
		snprintf(buffer, 16, " ih%08x", irqh);
		ingenic_puts(buffer);
	}
#endif
	bit = ffs32(irqh);
	while (bit != 0) {
		idx = bit - 1;
		mask = 1 << idx;
		idx += 32;
		intrs[idx].ih_count.ev_count++;
		if (intrs[idx].ih_func != NULL) {
			if (intrs[idx].ih_ipl == IPL_VM)
				KERNEL_LOCK(1, NULL);
			intrs[idx].ih_func(intrs[idx].ih_arg);
			if (intrs[idx].ih_ipl == IPL_VM)
				KERNEL_UNLOCK_ONE(NULL);
		} else {
			/* spurious interrupt, mask it */
			writereg(JZ_ICMSR1, mask);
		}
		irqh &= ~mask;
		bit = ffs32(irqh);
	}
	writereg(JZ_ICMCR0, ll);
	writereg(JZ_ICMCR1, hh);
}

void *
evbmips_intr_establish(int irq, int (*func)(void *), void *arg)
{
	int s;

	if ((irq < 0) || (irq >= NINTR)) {
		aprint_error("%s: invalid irq %d\n", __func__, irq);
		return NULL;
	}

	s = splhigh();	/* XXX probably needs a mutex */
	intrs[irq].ih_func = func;
	intrs[irq].ih_arg = arg;
	intrs[irq].ih_ipl = IPL_VM;

	/* now enable the IRQ */
	if (irq >= 32) {
		writereg(JZ_ICMCR1, 1 << (irq - 32));
	} else
		writereg(JZ_ICMCR0, 1 << irq);

	splx(s);

	return ((void *)(irq + 1));
}

void
evbmips_intr_disestablish(void *cookie)
{
	int irq = ((int)cookie) - 1;
	int s;

	if ((irq < 0) || (irq >= NINTR)) {
		aprint_error("%s: invalid irq %d\n", __func__, irq);
		return;
	}

	s = splhigh();

	/* disable the IRQ */
	if (irq >= 32) {
		writereg(JZ_ICMSR1, 1 << (irq - 32));
	} else
		writereg(JZ_ICMSR0, 1 << irq);

	intrs[irq].ih_func = NULL;
	intrs[irq].ih_arg = NULL;
	intrs[irq].ih_ipl = 0;

	splx(s);
}
