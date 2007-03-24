/*	$NetBSD: adm5120_intr.c,v 1.1.2.2 2007/03/24 14:54:49 yamt Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Platform-specific interrupt support for the Alchemy Semiconductor Pb1000.
 *
 * The Alchemy Semiconductor Pb1000's interrupts are wired to two internal
 * interrupt controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: adm5120_intr.c,v 1.1.2.2 2007/03/24 14:54:49 yamt Exp $");

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
#include <mips/adm5120/include/adm5120reg.h>
#include <mips/adm5120/include/adm5120var.h>

#include <dev/pci/pcireg.h>
#include <dev/pci/pcivar.h>

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
const uint32_t ipl_sr_bits[_IPL_N] = {
	0,					/*  0: IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/*  1: IPL_SOFT */

	MIPS_SOFT_INT_MASK_0,			/*  2: IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0,			/*  3: IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0,			/*  4: IPL_SOFTSERIAL */

	MIPS_SOFT_INT_MASK_0|
	MIPS_SOFT_INT_MASK_1|
	MIPS_INT_MASK_0,			/*  5: IPL_BIO */

	MIPS_SOFT_INT_MASK_0|
	MIPS_SOFT_INT_MASK_1|
	MIPS_INT_MASK_0,			/*  6: IPL_NET */

	MIPS_SOFT_INT_MASK_0|
	MIPS_SOFT_INT_MASK_1|
	MIPS_INT_MASK_0|
	MIPS_INT_MASK_1,			/*  7: IPL_{SERIAL,TTY} */

	MIPS_SOFT_INT_MASK_0|
	MIPS_SOFT_INT_MASK_1|
	MIPS_INT_MASK_0|
	MIPS_INT_MASK_1|
	MIPS_INT_MASK_2|
	MIPS_INT_MASK_3|
	MIPS_INT_MASK_4|
	MIPS_INT_MASK_5,			/*  8: IPL_{CLOCK,HIGH} */
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given software interrupt priority level.
 * Hardware ipls are port/board specific.
 */
const uint32_t mips_ipl_si_to_sr[SI_NQUEUES] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTSERIAL */
};

#define	NIRQS		32
const char *adm5120_intrnames[NIRQS] = {
	"timer", /*  0 */
	"uart0", /*  1 */
	"uart1", /*  2 */
	"usb",   /*  3 */
	"intx0/gpio2", /*  4 */
	"intx1/gpio4", /*  5 */
	"pci0",  /*  6 */
	"pci1",  /*  7 */
	"pci2",  /*  8 */
	"switch",/*  9 */
	"res10", /* 10 */
	"res11", /* 11 */
	"res12", /* 12 */
	"res13", /* 13 */
	"res14", /* 14 */
	"res15", /* 15 */
	"res16", /* 16 */
	"res17", /* 17 */
	"res18", /* 18 */
	"res19", /* 19 */
	"res20", /* 20 */
	"res21", /* 21 */
	"res22", /* 22 */
	"res23", /* 23 */
	"res24", /* 24 */
	"res25", /* 25 */
	"res26", /* 26 */
	"res27", /* 27 */
	"res28", /* 28 */
	"res29", /* 29 */
	"res30", /* 30 */
	"res31", /* 31 */
};

struct adm5120_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct adm5120_intrhead adm5120_intrtab[NIRQS];


#define	NINTRS			2	/* MIPS INT0 - INT1 */
struct adm5120_cpuintr {
	LIST_HEAD(, evbmips_intrhand) cintr_list;
	struct evcnt cintr_count;
};
struct adm5120_cpuintr adm5120_cpuintrs[NINTRS];

const char *adm5120_cpuintrnames[NINTRS] = {
	"int 0 (irq)",
	"int 1 (fiq)",
};

#define REG_READ(o) *((volatile uint32_t *)MIPS_PHYS_TO_KSEG1(ADM5120_BASE_ICU + (o)))
#define REG_WRITE(o,v) (REG_READ(o)) = (v)

void
evbmips_intr_init(void)
{
	int i;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&adm5120_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&adm5120_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", adm5120_cpuintrnames[i]);
	}

	for (i = 0; i < NIRQS; i++) {
		/* XXX steering - use an irqmap array? */

		adm5120_intrtab[i].intr_refcnt = 0;
		evcnt_attach_dynamic(&adm5120_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "adm5120", adm5120_intrnames[i]);
	}

	/* disable all interrupts */
	REG_WRITE(ICU_DISABLE_REG, ICU_INT_MASK);
}

void *
adm5120_intr_establish(int irq, int priority, int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	uint32_t irqmask;
	int	cpu_int, s;

	if (irq < 0 || irq >= NIRQS)
		panic("adm5120_intr_establish: bogus IRQ %d", irq);

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

	LIST_INSERT_HEAD(&adm5120_cpuintrs[cpu_int].cintr_list, ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (adm5120_intrtab[irq].intr_refcnt++ == 0) {
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
}

void
adm5120_intr_disestablish(void *cookie)
{
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
	if (adm5120_intrtab[irq].intr_refcnt-- == 1) {
		irqmask = 1 << irq;	/* only used as a mask from here on */

		/* disable this irq in HW */
		REG_WRITE(ICU_DISABLE_REG, irqmask);
	}

	splx(s);

	free(ih, M_DEVBUF);
}
void
evbmips_iointr(uint32_t status, uint32_t cause, uint32_t pc, uint32_t ipending)
{
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

		adm5120_cpuintrs[level].cintr_count.ev_count++;
		LIST_FOREACH(ih, &adm5120_cpuintrs[level].cintr_list, ih_q) {
			irqmask = 1 << ih->ih_irq;
			if (irqmask & irqstat) {
				adm5120_intrtab[ih->ih_irq].intr_count.ev_count++;
				(*ih->ih_func)(ih->ih_arg);
			}
		}
		cause &= ~(MIPS_INT_MASK_0 << level);
	}

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));

	return;
}
