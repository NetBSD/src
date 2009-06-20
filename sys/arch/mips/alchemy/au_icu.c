/*	$NetBSD: au_icu.c,v 1.22.16.2 2009/06/20 07:20:06 yamt Exp $	*/

/*-
 * Copyright (c) 2006 Itronix Inc.
 * All rights reserved.
 *
 * Written by Garrett D'Amore for Itronix Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Itronix Inc. may not be used to endorse
 *    or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ITRONIX INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL ITRONIX INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

/*
 * Interrupt support for the Alchemy Semiconductor Au1x00 CPUs.
 *
 * The Alchemy Semiconductor Au1x00's interrupts are wired to two internal
 * interrupt controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: au_icu.c,v 1.22.16.2 2009/06/20 07:20:06 yamt Exp $");

#include "opt_ddb.h"

#include <sys/param.h>
#include <sys/queue.h>
#include <sys/malloc.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/kernel.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <mips/locore.h>
#include <mips/alchemy/include/aureg.h>
#include <mips/alchemy/include/auvar.h>

#define	REGVAL(x)	*((volatile uint32_t *)(MIPS_PHYS_TO_KSEG1((x))))

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */

const uint32_t ipl_sr_bits[_IPL_N] = {
	0,					/*  0: IPL_NONE */
	MIPS_SOFT_INT_MASK_0,			/*  1: IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_0,			/*  2: IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_3,		/*  3: IPL_VM */
	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_3|
		MIPS_INT_MASK_4|
		MIPS_INT_MASK_5,		/*  4: IPL_{SCHED,HIGH} */
};

#define	NIRQS		64

struct au_icu_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct au_icu_intrhead au_icu_intrtab[NIRQS];

#define	NINTRS			4	/* MIPS INT0 - INT3 */

struct au_intrhand {
	LIST_ENTRY(au_intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
	int ih_mask;
};

struct au_cpuintr {
	LIST_HEAD(, au_intrhand) cintr_list;
	struct evcnt cintr_count;
};

struct au_cpuintr au_cpuintrs[NINTRS];
const char *au_cpuintrnames[NINTRS] = {
	"icu 0, req 0",
	"icu 0, req 1",
	"icu 1, req 0",
	"icu 1, req 1",
};

static bus_addr_t ic0_base, ic1_base;

void
au_intr_init(void)
{
	int			i;
	struct au_chipdep	*chip;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&au_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&au_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", au_cpuintrnames[i]);
	}

	chip = au_chipdep();
	KASSERT(chip != NULL);

	ic0_base = chip->icus[0];
	ic1_base = chip->icus[1];

	for (i = 0; i < NIRQS; i++) {
		au_icu_intrtab[i].intr_refcnt = 0;
		evcnt_attach_dynamic(&au_icu_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, chip->name, chip->irqnames[i]);
	}

	/* start with all interrupts masked */
	REGVAL(ic0_base + IC_MASK_CLEAR) = 0xffffffff;
	REGVAL(ic0_base + IC_WAKEUP_CLEAR) = 0xffffffff;
	REGVAL(ic0_base + IC_SOURCE_SET) = 0xffffffff;
	REGVAL(ic0_base + IC_RISING_EDGE) = 0xffffffff;
	REGVAL(ic0_base + IC_FALLING_EDGE) = 0xffffffff;
	REGVAL(ic0_base + IC_TEST_BIT) = 0;

	REGVAL(ic1_base + IC_MASK_CLEAR) = 0xffffffff;
	REGVAL(ic1_base + IC_WAKEUP_CLEAR) = 0xffffffff;
	REGVAL(ic1_base + IC_SOURCE_SET) = 0xffffffff;
	REGVAL(ic1_base + IC_RISING_EDGE) = 0xffffffff;
	REGVAL(ic1_base + IC_FALLING_EDGE) = 0xffffffff;
	REGVAL(ic1_base + IC_TEST_BIT) = 0;
}

void *
au_intr_establish(int irq, int req, int level, int type,
    int (*func)(void *), void *arg)
{
	struct au_intrhand	*ih;
	uint32_t		icu_base;
	int			cpu_int, s;
	struct au_chipdep	*chip;

	chip = au_chipdep();
	KASSERT(chip != NULL);

	if (irq >= NIRQS)
		panic("au_intr_establish: bogus IRQ %d", irq);
	if (req > 1)
		panic("au_intr_establish: bogus request %d", req);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_mask = (1 << (irq & 31));

	s = splhigh();

	/*
	 * First, link it into the tables.
	 * XXX do we want a separate list (really, should only be one item, not
	 *     a list anyway) per irq, not per CPU interrupt?
	 */
	cpu_int = (irq < 32 ? 0 : 2) + req;
	LIST_INSERT_HEAD(&au_cpuintrs[cpu_int].cintr_list, ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (au_icu_intrtab[irq].intr_refcnt++ == 0) {
		icu_base = (irq < 32) ? ic0_base : ic1_base;

		irq &= 31;	/* throw away high bit if set */
		irq = 1 << irq;	/* only used as a mask from here on */

		/* XXX Only level interrupts for now */
		switch (type) {
		case IST_NONE:
		case IST_PULSE:
		case IST_EDGE:
			panic("unsupported irq type %d", type);
			/* NOTREACHED */
		case IST_LEVEL:
		case IST_LEVEL_HIGH:
			REGVAL(icu_base + IC_CONFIG2_SET) = irq;
			REGVAL(icu_base + IC_CONFIG1_CLEAR) = irq;
			REGVAL(icu_base + IC_CONFIG0_SET) = irq;
			break;
		case IST_LEVEL_LOW:
			REGVAL(icu_base + IC_CONFIG2_SET) = irq;
			REGVAL(icu_base + IC_CONFIG1_SET) = irq;
			REGVAL(icu_base + IC_CONFIG0_CLEAR) = irq;
			break;
		}
		wbflush();

		/* XXX handle GPIO interrupts - not done at all yet */
		if (cpu_int & 0x1)
			REGVAL(icu_base + IC_ASSIGN_REQUEST_CLEAR) = irq;
		else
			REGVAL(icu_base + IC_ASSIGN_REQUEST_SET) = irq;

		/* Associate interrupt with peripheral */
		REGVAL(icu_base + IC_SOURCE_SET) = irq;

		/* Actually enable the interrupt */
		REGVAL(icu_base + IC_MASK_SET) = irq;

		/* And allow the interrupt to interrupt idle */
		REGVAL(icu_base + IC_WAKEUP_SET) = irq;

		wbflush();
	}
	splx(s);

	return (ih);
}

void
au_intr_disestablish(void *cookie)
{
	struct au_intrhand *ih = cookie;
	uint32_t icu_base;
	int irq, s;

	irq = ih->ih_irq;

	s = splhigh();

	/*
	 * First, remove it from the table.
	 */
	LIST_REMOVE(ih, ih_q);

	/*
	 * Now, disable it, if there is nothing remaining on the
	 * list.
	 */
	if (au_icu_intrtab[irq].intr_refcnt-- == 1) {
		icu_base = (irq < 32) ? ic0_base : ic1_base;

		irq &= 31;	/* throw away high bit if set */
		irq = 1 << irq;	/* only used as a mask from here on */

		REGVAL(icu_base + IC_CONFIG2_CLEAR) = irq;
		REGVAL(icu_base + IC_CONFIG1_CLEAR) = irq;
		REGVAL(icu_base + IC_CONFIG0_CLEAR) = irq;

		/* disable with MASK_CLEAR and WAKEUP_CLEAR */
		REGVAL(icu_base + IC_MASK_CLEAR) = irq;
		REGVAL(icu_base + IC_WAKEUP_CLEAR) = irq;
		wbflush();
	}

	splx(s);

	free(ih, M_DEVBUF);
}

void
au_iointr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
	struct au_intrhand *ih;
	int level;
	uint32_t icu_base, irqstat, irqmask;

	icu_base = irqstat = 0;

	for (level = 3; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;

		/*
		 * XXX	the following may well be slow to execute.
		 *	investigate and possibly speed up.
		 *
		 * is something like:
		 *
		 *    irqstat = REGVAL(
		 *	 (level & 4 == 0) ? IC0_BASE ? IC1_BASE +
		 *	 (level & 2 == 0) ? IC_REQUEST0_INT : IC_REQUEST1_INT);
		 *
		 * be any better?
		 *
		 */
		switch (level) {
		case 0:
			icu_base = ic0_base;
			irqstat = REGVAL(icu_base + IC_REQUEST0_INT);
			break;
		case 1:
			icu_base = ic0_base;
			irqstat = REGVAL(icu_base + IC_REQUEST1_INT);
			break;
		case 2:
			icu_base = ic1_base;
			irqstat = REGVAL(icu_base + IC_REQUEST0_INT);
			break;
		case 3:
			icu_base = ic1_base;
			irqstat = REGVAL(icu_base + IC_REQUEST1_INT);
			break;
		}
		irqmask = REGVAL(icu_base + IC_MASK_READ);
		au_cpuintrs[level].cintr_count.ev_count++;
		LIST_FOREACH(ih, &au_cpuintrs[level].cintr_list, ih_q) {
			int mask = ih->ih_mask;

			if (mask & irqmask & irqstat) {
				au_icu_intrtab[ih->ih_irq].intr_count.ev_count++;
				(*ih->ih_func)(ih->ih_arg);

				if (REGVAL(icu_base + IC_MASK_READ) & mask) {
					REGVAL(icu_base + IC_MASK_CLEAR) = mask;
					REGVAL(icu_base + IC_MASK_SET) = mask;
					wbflush();
				}
			}
		}
		cause &= ~(MIPS_INT_MASK_0 << level);
	}

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
}

/*
 * Some devices (e.g. PCMCIA) want to be able to mask interrupts at
 * the ICU, and leave them masked off until some later time
 * (e.g. reenabled by a soft interrupt).
 */

void
au_intr_enable(int irq)
{
	int		s;
	uint32_t	icu_base, mask;

	if (irq >= NIRQS)
		panic("au_intr_enable: bogus IRQ %d", irq);

	icu_base = (irq < 32) ? ic0_base : ic1_base;
	mask = irq & 31;
	mask = 1 << mask;

	s = splhigh();
	/* only enable the interrupt if we have a handler */
	if (au_icu_intrtab[irq].intr_refcnt) {
		REGVAL(icu_base + IC_MASK_SET) = mask;
		REGVAL(icu_base + IC_WAKEUP_SET) = mask;
		wbflush();
	}
	splx(s);
}

void
au_intr_disable(int irq)
{
	int		s;
	uint32_t	icu_base, mask;

	if (irq >= NIRQS)
		panic("au_intr_disable: bogus IRQ %d", irq);

	icu_base = (irq < 32) ? ic0_base : ic1_base;
	mask = irq & 31;
	mask = 1 << mask;

	s = splhigh();
	REGVAL(icu_base + IC_MASK_CLEAR) = mask;
	REGVAL(icu_base + IC_WAKEUP_CLEAR) = mask;
	wbflush();
	splx(s);
}
