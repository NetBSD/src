/*	$NetBSD: au_icu.c,v 1.7 2003/07/15 02:43:34 lukem Exp $	*/

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
 * Interrupt support for the Alchemy Semiconductor Au1x00 CPUs.
 *
 * The Alchemy Semiconductor Au1x00's interrupts are wired to two internal
 * interrupt controllers.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: au_icu.c,v 1.7 2003/07/15 02:43:34 lukem Exp $");

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

#define	REGVAL(x)	*((__volatile u_int32_t *)(MIPS_PHYS_TO_KSEG1((x))))

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */

const u_int32_t ipl_sr_bits[_IPL_N] = {
	0,					/*  0: IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/*  1: IPL_SOFT */

	MIPS_SOFT_INT_MASK_0,			/*  2: IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0,			/*  3: IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0,			/*  4: IPL_SOFTSERIAL */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/*  5: IPL_BIO */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/*  6: IPL_NET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/*  7: IPL_{SERIAL,TTY} */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_3|
		MIPS_INT_MASK_4|
		MIPS_INT_MASK_5,		/*  8: IPL_{CLOCK,HIGH} */
};

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given software interrupt priority level.
 * Hardware ipls are port/board specific.
 */
const u_int32_t mips_ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTSERIAL */
};

#define	NIRQS		64

const char *au1000_intrnames[NIRQS] = {
	"uart0",
	"uart1",
	"uart2",
	"uart3",
	"ssi0",
	"ssi1",
	"dma0",
	"dma1",
	"dma2",
	"dma3",
	"dma4",
	"dma5",
	"dma6",
	"dma7",
	"pc0",
	"pc0 match1",
	"pc0 match2",
	"pc0 match3",
	"pc1",
	"pc1 match1",
	"pc1 match2",
	"pc1 match3",
	"irda tx",
	"irda rx",
	"usb intr",
	"usb suspend",
	"usb host",
	"ac97",
	"mac0",
	"mac1",
	"i2s",
	"ac97 cmd",

	"gpio 0",
	"gpio 1",
	"gpio 2",
	"gpio 3",
	"gpio 4",
	"gpio 5",
	"gpio 6",
	"gpio 7",
	"gpio 8",
	"gpio 9",
	"gpio 10",
	"gpio 11",
	"gpio 12",
	"gpio 13",
	"gpio 14",
	"gpio 15",
	"gpio 16",
	"gpio 17",
	"gpio 18",
	"gpio 19",
	"gpio 20",
	"gpio 21",
	"gpio 22",
	"gpio 23",
	"gpio 24",
	"gpio 25",
	"gpio 26",
	"gpio 27",
	"gpio 28",
	"gpio 29",
	"gpio 30",
	"gpio 31",
};

struct au1000_intrhead {
	struct evcnt intr_count;
	int intr_refcnt;
};
struct au1000_intrhead au1000_intrtab[NIRQS];

#define	NINTRS			4	/* MIPS INT0 - INT3 */

struct au1000_cpuintr {
	LIST_HEAD(, evbmips_intrhand) cintr_list;
	struct evcnt cintr_count;
};

struct au1000_cpuintr au1000_cpuintrs[NINTRS];
const char *au1000_cpuintrnames[NINTRS] = {
	"icu 0, req 0",
	"icu 0, req 1",
	"icu 1, req 0",
	"icu 1, req 1",
};

void
au_intr_init(void)
{
	int i;

	for (i = 0; i < NINTRS; i++) {
		LIST_INIT(&au1000_cpuintrs[i].cintr_list);
		evcnt_attach_dynamic(&au1000_cpuintrs[i].cintr_count,
		    EVCNT_TYPE_INTR, NULL, "mips", au1000_cpuintrnames[i]);
	}

	for (i = 0; i < NIRQS; i++) {
		/* XXX steering - use an irqmap array? */

		au1000_intrtab[i].intr_refcnt = 0;
		evcnt_attach_dynamic(&au1000_intrtab[i].intr_count,
		    EVCNT_TYPE_INTR, NULL, "au1000", au1000_intrnames[i]);
	}
}

void *
au_intr_establish(int irq, int req, int level, int type,
    int (*func)(void *), void *arg)
{
	struct evbmips_intrhand *ih;
	uint32_t icu_base;
	int cpu_intr, s;

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

	s = splhigh();

	/*
	 * First, link it into the tables.
	 * XXX do we want a separate list (really, should only be one item, not
	 *     a list anyway) per irq, not per cpu interrupt?
	 */
	cpu_intr = (irq < 32 ? 0 : 2);
	LIST_INSERT_HEAD(&au1000_cpuintrs[cpu_intr].cintr_list, ih, ih_q);

	/*
	 * Now enable it.
	 */
	if (au1000_intrtab[irq].intr_refcnt++ == 0) {
		icu_base = (irq < 32) ? IC0_BASE : IC1_BASE;

		irq &= 31;	/* throw away high bit if set */
		irq = 1 << irq;	/* only used as a mask from here on */

		/* XXX Only high-level interrupts for now */
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

		/* XXX handle GPIO interrupts - not done at all yet */
		if (cpu_intr & 0x1)
			REGVAL(icu_base + IC_ASSIGN_REQUEST_CLEAR) = irq;
		else
			REGVAL(icu_base + IC_ASSIGN_REQUEST_SET) = irq;

		/* Associate interrupt with peripheral */
		REGVAL(icu_base + IC_SOURCE_SET) = irq;

		/* Actually enable the interrupt */
		REGVAL(icu_base + IC_MASK_SET) = irq;

		/* And allow the interrupt to interrupt idle */
		REGVAL(icu_base + IC_WAKEUP_SET) = irq;
	}
	splx(s);

	return (ih);
}

void
au_intr_disestablish(void *cookie)
{
	struct evbmips_intrhand *ih = cookie;
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
	if (au1000_intrtab[irq].intr_refcnt-- == 1) {
		icu_base = (irq < 32) ? IC0_BASE : IC1_BASE;

		irq &= 31;	/* throw away high bit if set */
		irq = 1 << irq;	/* only used as a mask from here on */

		REGVAL(icu_base + IC_CONFIG2_CLEAR) = irq;
		REGVAL(icu_base + IC_CONFIG1_CLEAR) = irq;
		REGVAL(icu_base + IC_CONFIG0_CLEAR) = irq;

		/* XXX disable with MASK_CLEAR and WAKEUP_CLEAR */
	}

	splx(s);

	free(ih, M_DEVBUF);
}

void
au_iointr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	struct evbmips_intrhand *ih;
	int level;
	u_int32_t icu_base, irqmask;

	for (level = 3; level >= 0; level--) {
		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;

		/*
		 * XXX	the following may well be slow to execute.
		 *	investigate and possibly speed up.
		 *
		 * is something like:
		 *
		 *    irqmask = REGVAL(
		 *	 (level & 4 == 0) ? IC0_BASE ? IC1_BASE +
		 *	 (level & 2 == 0) ? IC_REQUEST0_INT : IC_REQUEST1_INT);
		 *
		 * be any better?
		 *
		 */
		switch (level) {
		case 0:
			icu_base = IC0_BASE;
			irqmask = REGVAL(icu_base + IC_REQUEST0_INT);
			break;
		case 1:
			icu_base = IC0_BASE;
			irqmask = REGVAL(icu_base + IC_REQUEST1_INT);
			break;
		case 2:
			icu_base = IC1_BASE;
			irqmask = REGVAL(icu_base + IC_REQUEST0_INT);
			break;
		case 3:
			icu_base = IC1_BASE;
			irqmask = REGVAL(icu_base + IC_REQUEST1_INT);
			break;
		}
		au1000_cpuintrs[level].cintr_count.ev_count++;
		LIST_FOREACH(ih, &au1000_cpuintrs[level].cintr_list, ih_q) {
			/* XXX should check is see if interrupt is masked? */
			if (1 << ih->ih_irq & irqmask) {
				au1000_intrtab[ih->ih_irq].intr_count.ev_count++;
				(*ih->ih_func)(ih->ih_arg);

				REGVAL(icu_base + IC_MASK_CLEAR) = 1 << ih->ih_irq;
				REGVAL(icu_base + IC_MASK_SET) = 1 << ih->ih_irq;
			}
		}
		cause &= ~(MIPS_INT_MASK_0 << level);
	}

	/* Re-enable anything that we have processed. */
	_splset(MIPS_SR_INT_IE | ((status & ~cause) & MIPS_HARD_INT_MASK));
}
