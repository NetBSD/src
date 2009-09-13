/*	$NetBSD: rmixl_intr.c,v 1.1.2.1 2009/09/13 03:27:38 cliff Exp $	*/

/*-
 * Copyright (c) 2007 Ruslan Ermilov and Vsevolod Lobko.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or
 * without modification, are permitted provided that the following
 * conditions are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 * 3. The names of the authors may not be used to endorse or promote
 *    products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
 * OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
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
 * Platform-specific interrupt support for the RMI XLP, XLR, XLS
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: rmixl_intr.c,v 1.1.2.1 2009/09/13 03:27:38 cliff Exp $");

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
#include <mips/rmi/rmixlreg.h>
#include <mips/rmi/rmixlvar.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

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
	MIPS_INT_MASK_0,			/*  3: IPL_VM */

	MIPS_SOFT_INT_MASK_0|
	MIPS_SOFT_INT_MASK_1|
	MIPS_INT_MASK_0|
	MIPS_INT_MASK_1|
	MIPS_INT_MASK_2|
	MIPS_INT_MASK_3|
	MIPS_INT_MASK_4|
	MIPS_INT_MASK_5,			/*  4: IPL_{SCHED,HIGH} */
};

#define	NIRQS		32
const char *rmixl_intrnames[NIRQS] = {
	"irq0", /*  3 */
	"irq1", /*  1 */
	"irq2", /*  2 */
	"irq3", /*  3 */
	"irq4", /*  4 */
	"irq5", /*  5 */
	"irq6", /*  6 */
	"irq7", /*  7 */
	"irq8", /*  8 */
	"irq9", /*  9 */
	"irq10", /* 10 */
	"irq11", /* 11 */
	"irq12", /* 12 */
	"irq13", /* 13 */
	"irq14", /* 14 */
	"irq15", /* 15 */
	"irq16", /* 16 */
	"irq17", /* 17 */
	"irq18", /* 18 */
	"irq19", /* 19 */
	"irq20", /* 20 */
	"irq21", /* 21 */
	"irq22", /* 22 */
	"irq23", /* 23 */
	"irq24", /* 24 */
	"irq25", /* 25 */
	"irq26", /* 26 */
	"irq27", /* 27 */
	"irq28", /* 28 */
	"irq29", /* 29 */
	"irq30", /* 30 */
	"irq31", /* 31 */
};

struct rmixl_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct rmixl_intrhead rmixl_intrtab[NIRQS];


#define	NINTRS			2	/* MIPS INT0 - INT1 */
struct rmixl_cpuintr {
	LIST_HEAD(, evbmips_intrhand) cintr_list;
	struct evcnt cintr_count;
};
struct rmixl_cpuintr rmixl_cpuintrs[NINTRS];

const char *rmixl_cpuintrnames[NINTRS] = {
	"int 0 (irq)",
	"int 1 (fiq)",
};

#define REG_READ(o) *((volatile uint32_t *)MIPS_PHYS_TO_KSEG1(ADM5120_BASE_ICU + (o)))
#define REG_WRITE(o,v) (REG_READ(o)) = (v)

void
evbmips_intr_init(void)
{
#ifdef NOTYET
	int i;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&rmixl_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&rmixl_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", rmixl_cpuintrnames[i]);
	}

	for (i = 0; i < NIRQS; i++) {
		/* XXX steering - use an irqmap array? */

		rmixl_intrtab[i].intr_refcnt = 0;
		evcnt_attach_dynamic(&rmixl_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "rmixl", rmixl_intrnames[i]);
	}

	/* disable all interrupts */
	REG_WRITE(ICU_DISABLE_REG, ICU_INT_MASK);
#endif
}

void *
rmixl_intr_establish(int irq, int priority, int (*func)(void *), void *arg)
{
#ifdef NOTYET
	struct evbmips_intrhand *ih;
	uint32_t irqmask;
	int	cpu_int, s;

	if (irq < 0 || irq >= NIRQS)
		panic("rmixl_intr_establish: bogus IRQ %d", irq);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return NULL;

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;

	s = splhigh();

	/*
	 * First, link it into the tables.
	 * XXX do we want a separate list (really, should only be one item, not
	 *     a list anyway) per irq, not per CPU interrupt?
	 */

	cpu_int = (priority == INTR_FIQ) ? 1 : 0;

	LIST_INSERT_HEAD(&rmixl_cpuintrs[cpu_int].cintr_list, ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (rmixl_intrtab[irq].intr_refcnt++ == 0) {
		irqmask = 1 << irq;

		/* configure as IRQ or FIQ */
		if (priority == INTR_FIQ) {
			REG_WRITE(ICU_MODE_REG,
			    REG_READ(ICU_MODE_REG) | irqmask);
		} else {
			REG_WRITE(ICU_MODE_REG,
			    REG_READ(ICU_MODE_REG) & ~irqmask);
		}
		/* enable */
		REG_WRITE(ICU_ENABLE_REG, irqmask);
	}
	splx(s);

	return ih;
#else
	return NULL;
#endif
}

void
rmixl_intr_disestablish(void *cookie)
{
#ifdef NOTYET
	struct evbmips_intrhand *ih = cookie;
	int irq, s;
	uint32_t irqmask;

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
	if (rmixl_intrtab[irq].intr_refcnt-- == 1) {
		irqmask = 1 << irq;	/* only used as a mask from here on */

		/* disable this irq in HW */
		REG_WRITE(ICU_DISABLE_REG, irqmask);
	}

	splx(s);

	free(ih, M_DEVBUF);
#endif
}

void
evbmips_iointr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
#ifdef NOTYET
	struct evbmips_intrhand *ih;
	int level;
	uint32_t irqmask, irqstat;

	for (level = NINTRS - 1; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;

		if (level)
			irqstat = REG_READ(ICU_FIQ_STATUS_REG);
		else
			irqstat = REG_READ(ICU_STATUS_REG);

		rmixl_cpuintrs[level].cintr_count.ev_count++;
		LIST_FOREACH(ih, &rmixl_cpuintrs[level].cintr_list, ih_q) {
			irqmask = 1 << ih->ih_irq;
			if (irqmask & irqstat) {
				rmixl_intrtab[ih->ih_irq].intr_count.ev_count++;
				(*ih->ih_func)(ih->ih_arg);
			}
		}
		cause &= ~(MIPS_INT_MASK_0 << level);
	}

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
#endif
}
