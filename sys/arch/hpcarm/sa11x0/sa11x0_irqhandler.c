/*	$NetBSD: sa11x0_irqhandler.c,v 1.3 2001/03/31 12:48:10 toshii Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to the NetBSD Foundation
 * by IWAMOTO Toshihiro.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Charles M. Hannum and by Jason R. Thorpe of the Numerical Aerospace
 * Simulation Facility, NASA Ames Research Center.
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

/*-
 * Copyright (c) 1991 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * William Jolitz.
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
 *	@(#)isa.c	7.2 (Berkeley) 5/13/91
 */


#include "opt_cputypes.h"
#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/syslog.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>

#include <hpcarm/sa11x0/sa11x0_reg.h>
#include <hpcarm/sa11x0/sa11x0_var.h>

#include <machine/intr.h>
#include <machine/cpu.h>

irqhandler_t *irqhandlers[NIRQS];

int current_intr_depth;
u_int current_mask;
u_int actual_mask;
u_int disabled_mask;
u_int spl_mask;
u_int imask[NIPL];
u_int irqblock[NIRQS];


extern void set_spl_masks();
static int fakeintr(void *);
#ifdef DEBUG
static int dumpirqhandlers();
#endif
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
	struct irqhandler *q;
	int intrlevel[ICU_LEN];

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0; irq < ICU_LEN; irq++) {
		int levels = 0;
		for (q = irqhandlers[irq]; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		intrlevel[irq] = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		int irqs = 0;
		for (irq = 0; irq < ICU_LEN; irq++)
			if (intrlevel[irq] & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs;
	}

#if 0
	/*
	 * Initialize soft interrupt masks to block themselves.
	 */
	imask[IPL_SOFTCLOCK] = 1 << SIR_CLOCK;
	imask[IPL_SOFTNET] = 1 << SIR_NET;
	imask[IPL_SOFTSERIAL] = 1 << SIR_SERIAL;

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	imask[IPL_NONE] = 0;
#endif

	/*
	 * Enforce a hierarchy that gives slow devices a better chance at not
	 * dropping data.
	 */
	for(level = NIPL - 1; level > 0; level--)
		imask[level - 1] |= imask[level];

	/*
	 * Calculate irqblock[], which emulates hardware interrupt levels.
	 */
	for(irq = 0; irq < ICU_LEN; irq++) {
		int irqs = 1 << irq;
		for (q = irqhandlers[irq]; q; q = q->ih_next)
			irqs |= ~imask[q->ih_level];
		irqblock[irq] = irqs;
	}
}


const struct evcnt *
sa11x0_intr_evcnt(sa11x0_chipset_tag_t ic, int irq)
{

	/* XXX for now, no evcnt parent reported */
	return NULL;
}

void *
sa11x0_intr_establish(ic, irq, type, level, ih_fun, ih_arg)
	sa11x0_chipset_tag_t ic;
	int irq;
	int type;
	int level;
	int (*ih_fun)(void *);
	void *ih_arg;
{
	int saved_cpsr;
	struct irqhandler **p, *q, *ih;
	static struct irqhandler fakehand = {fakeintr};

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("sa11x0_intr_establish: can't malloc handler info");

	if (irq < 0 || irq >= ICU_LEN || type == IST_NONE)
		panic("intr_establish: bogus irq or type");

	/* All interrupts are level intrs. */

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &irqhandlers[irq]; (q = *p) != NULL; p = &q->ih_next)
		;

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = level;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_func = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_count = 0;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	ih->ih_name = NULL; /* XXX */
	*p = ih;

	saved_cpsr = SetCPSR(I32_bit, I32_bit);
	set_spl_masks();
	spl_mask = spl_masks[current_spl_level];
	current_mask |= (1 << irq);

	irq_setmasks();

	SetCPSR(I32_bit, saved_cpsr & I32_bit);
#ifdef DEBUG
	dumpirqhandlers();
#endif
	return (ih);
}

/*
 * Deregister an interrupt handler.
 */
void
sa11x0_intr_disestablish(ic, arg)
	sa11x0_chipset_tag_t ic;
	void *arg;
{
	struct irqhandler *ih = arg;
	int irq = ih->ih_irq;
	int saved_cpsr;
	struct irqhandler **p, *q;

#if DIAGNOSTIC
	if (irq < 0 || irq >= ICU_LEN)
		panic("intr_disestablish: bogus irq");
#endif

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &irqhandlers[irq]; (q = *p) != NULL && q != ih; p = &q->ih_next)
		;
	if (q)
		*p = q->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	free(ih, M_DEVBUF);

	intr_calculatemasks();
	saved_cpsr = SetCPSR(I32_bit, I32_bit);
	set_spl_masks();
	spl_mask = spl_masks[current_spl_level];

	current_mask &= ~(1 << irq);
	irq_setmasks();
	SetCPSR(I32_bit, saved_cpsr & I32_bit);

}

void
stray_irqhandler(void *p)
{
	printf("stray interrupt\n");
}

int
fakeintr(void *p)
{
	return 0;
}

#ifdef DEBUG
int
dumpirqhandlers()
{
	int irq;
	struct irqhandler *p;

	for(irq = 0; irq < ICU_LEN; irq++) {
		printf("irq %d:", irq);
		p = irqhandlers[irq];
		for(; p; p = p->ih_next)
			printf("ih_func: 0x%lx, ", (unsigned long)p->ih_func);
		printf("\n");
	}
	return 0;
}
#endif
/* End of irqhandler.c */
