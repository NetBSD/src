/*	$NetBSD: arm_irqhandler.c,v 1.1.2.5 2008/01/20 16:03:54 chris Exp $	*/

/*
 * Copyright (c) 2007 Christopher Gilbert
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 2001, 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ARM_SPL_NOINLINE
#define	ARM_SPL_NOINLINE
#endif

#include <sys/cdefs.h>
__KERNEL_RCSID(0,"$NetBSD: arm_irqhandler.c,v 1.1.2.5 2008/01/20 16:03:54 chris Exp $");

#include "opt_irqstats.h"

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <uvm/uvm_extern.h>

#include <arm/arm_intr.h>
#include <machine/intr.h>
#include <machine/cpu.h>

/*
 * This generic code provides for multiple interrupt sources all merging into one place
 * eg pci, isa and soft interrupts.  They're all treated as an irq source,
 * each registers with a few simple functions
 */


/* ipl queues */
static struct iplq arm_iplq[NIPL];

TAILQ_HEAD(, irq_group) irq_groups_list;	/* groups list */

static void arm_intr_run_handlers_for_ipl(int ipl_level, struct clockframe *frame);
static int arm_intr_fls(uint32_t n);

const struct evcnt *
arm_intr_evcnt(irqgroup_t cookie, int irq)
{
	struct irq_group *irqs = cookie;
	if (irq < 0 || irq >= irqs->nirqs)
		return NULL;
	else
		return &(irqs->irqs[irq].iq_ev);
}

/* XXX should this be a define so it's always inlined? */
static inline void
arm_intr_set_hardware_mask(struct irq_group *irqs)
{
	irqs->set_irq_hardware_mask(irqs->irq_hardware_cookie, irqs->intr_enabled & (irqs->intr_soft_enabled));
}

static inline void
arm_intr_enable_irq(struct irq_group *irqs, int irq)
{
	irqs->intr_enabled |= (1U << irq);
	arm_intr_set_hardware_mask(irqs);
}

static inline void
arm_intr_disable_irq(struct irq_group *irqs, int irq)
{
	irqs->intr_enabled &= ~(1U << irq);
	arm_intr_set_hardware_mask(irqs);
}

void
arm_intr_soft_enable_irq(irqgroup_t cookie, int irq)
{
	struct irq_group *irqs = cookie;	
	uint32_t oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	irqs->intr_soft_enabled |= (1U << irq);
	arm_intr_set_hardware_mask(irqs);
	
	restore_interrupts(oldirqstate);
}

void
arm_intr_soft_disable_irq(irqgroup_t cookie, int irq)
{
	struct irq_group *irqs = cookie;
	uint32_t oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	irqs->intr_soft_enabled &= ~(1U << irq);
	arm_intr_set_hardware_mask(irqs);
	
	restore_interrupts(oldirqstate);
}

/* 
 * fake up a frame, and handle pending interrupts
 * 
 * This should be quicker than turning on interrupts, and going the normal route
 */
void arm_intr_splx_lifter(int newspl)
{
	int oldirqstate;
	int oldintrdepth;
	struct clockframe frame;

	/* fake up the clockframe parts we take interest in */
	frame.cf_if.if_spsr = PSR_MODE & PSR_SVC32_MODE;
	frame.cf_if.if_pc = (unsigned int)&arm_intr_splx_lifter;
	
	oldirqstate = disable_interrupts(I32_bit);
	oldintrdepth = curcpu()->ci_idepth++;

	arm_intr_process_pending_ipls(&frame, newspl);

	curcpu()->ci_idepth = oldintrdepth;
	restore_interrupts(oldirqstate);
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
arm_intr_calculate_masks(struct irq_group *irqs)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < irqs->nirqs; irq++) {
		iq = &(irqs->irqs[irq]);
		arm_intr_disable_irq(irqs, irq);
		
		TAILQ_REMOVE(&(arm_iplq[iq->iq_ipl].ipl_list), iq, ipl_list);

		iq->iq_ipl = IPL_NONE;
		TAILQ_FOREACH(ih, &(iq->iq_list), ih_list)
		{
			/* check if this is the highest ipl for this irq */
			if (iq->iq_ipl < ih->ih_ipl)
				iq->iq_ipl = ih->ih_ipl;
		}
	}

	/*
	 * flag enable all active lines in the hardware
	 */
	for (irq = 0; irq < irqs->nirqs; irq++) {
		iq = &(irqs->irqs[irq]);
		TAILQ_INSERT_TAIL(&(arm_iplq[iq->iq_ipl].ipl_list), iq, ipl_list);
		if (!(TAILQ_EMPTY(&iq->iq_list)))
		{
			arm_intr_enable_irq(irqs, irq);
		}
	}
}

int
_splraise(int ipl)
{
    return (arm_intr_splraise(ipl));
}

void
splx(int new)
{
    arm_intr_splx(new);
}

int
_spllower(int ipl)
{
    return (arm_intr_spllower(ipl));
}

void
arm_intr_init(void)
{
	struct iplq *ipl;
	int i;

	/*
	 * clear down the global items, once irqs are registered we enable irqs
	 * doing so now would cause issues as this code couldn't disable any of them
	 */
	current_ipl_level = NIPL-1;
	ipls_pending = 0;
	
	for (i = 0; i < NIPL; i++) {
		ipl = &arm_iplq[i];
		TAILQ_INIT(&ipl->ipl_list);

		/* XXX: this should be a hard coded array */
		sprintf(ipl->ipl_name, "ipl %d", i);
		evcnt_attach_dynamic(&ipl->ipl_ev, EVCNT_TYPE_MISC,
		    NULL, "arm_intr", ipl->ipl_name);
	}
	TAILQ_INIT(&irq_groups_list);	/* groups list */
	disable_interrupts(I32_bit|F32_bit);

}

void
arm_intr_enable_irqs(void)
{
	enable_interrupts(I32_bit|F32_bit);
	spl0();
}


irqgroup_t
arm_intr_register_irq_provider(const char *name, int nirqs, 
		void (*set_irq_hardware_mask)(irq_hardware_cookie_t, uint32_t intr_enabled),
		void (*set_irq_hardware_type)(irq_hardware_cookie_t, int irq_line, int type),
		irq_hardware_cookie_t cookie)
{
	struct irq_group *irqg;
	int i;
	
	/* grab all the memory we need */
	irqg = malloc(sizeof(*irqg), M_DEVBUF, M_NOWAIT);
	if (irqg == NULL)
	{
		panic("No memory for irqgroup %s", name);
		return NULL;
	}

	irqg->irqs = malloc(sizeof(struct intrq) * nirqs, M_DEVBUF, M_NOWAIT);
	if (irqg->irqs == NULL)
	{
		panic("No memory for irq handlers for group %s", name);
		free(irqg, M_DEVBUF);
		return NULL;
	}

	/* initialise the structure */
	irqg->nirqs = nirqs;
	irqg->intr_enabled = 0;
	irqg->intr_soft_enabled = 0xffffffff;
	irqg->set_irq_hardware_mask = set_irq_hardware_mask;
	irqg->set_irq_hardware_type = set_irq_hardware_type;
	irqg->irq_hardware_cookie = cookie;
	irqg->group_name = name;

	for (i = 0; i < nirqs; i++) {
	   	struct intrq *iq = &(irqg->irqs[i]);
	       	TAILQ_INIT(&iq->iq_list);
		
		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
	       			NULL, irqg->group_name, iq->iq_name);
		iq->iq_ipl = IPL_NONE;
		iq->iq_ist = IST_NONE;
		iq->iq_irq = i;
		iq->iq_mask = 0;
		iq->iq_group = irqg;
		iq->iq_pending = false;
		TAILQ_INSERT_TAIL(&(arm_iplq[IPL_NONE].ipl_list), iq, ipl_list);
    	}
	
	TAILQ_INSERT_TAIL(&irq_groups_list, irqg, irq_groups_list);

	/* clear the masks etc */
	arm_intr_calculate_masks(irqg);
	return irqg;
}

irqhandler_t
arm_intr_claim(irqgroup_t cookie, int irq, int type, int ipl, const char *name,
		int (*func)(void *), void *arg)
{
	struct irq_group *irqg = cookie;
	struct intrq *iq;
	struct intrhand *ih;
	u_int oldirqstate;

	if (irq < 0 || irq >= irqg->nirqs)
		panic("arm_intr_establish: IRQ %d out of range", irq);

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
	{
		printf("No memory for irq %d, group %s", irq, irqg->group_name);
		return (NULL);
	}

	iq = &(irqg->irqs[irq]);
	switch (iq->iq_ist)
	{
		case IST_NONE:
			iq->iq_ist = type;
			if (irqg->set_irq_hardware_type)
			{
				irqg->set_irq_hardware_type(irqg->irq_hardware_cookie, irq, type);
			}
			break;
		case IST_EDGE:
		case IST_LEVEL:
			if (iq->iq_ist == type)
				break;
		case IST_PULSE:
			if (type != IST_NONE)
				panic("Can't share irq type %d with %d",
						iq->iq_ist, type);
			break;
	}

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_ipl = ipl;
	ih->ih_irq = irq;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);

	arm_intr_calculate_masks(irqg);

	if (name != NULL)
	{
		/* detach the existing event counter and add the new name */
		evcnt_detach(&iq->iq_ev);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				NULL, irqg->group_name, name);
	}
	else
	{
		iq->iq_ev.ev_count = 0;
	}
	
	restore_interrupts(oldirqstate);
	
	return(ih);
}

void
arm_intr_disestablish(irqgroup_t group_cookie, irqhandler_t cookie)
{
	struct irq_group *irqg = group_cookie;
	struct intrhand *ih = cookie;
	struct intrq *iq = &(irqg->irqs[ih->ih_irq]);
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);
	
	/* reset the IST type */
	if (TAILQ_EMPTY(&iq->iq_list))
	{
		iq->iq_ist = IST_NONE;
		if (irqg->set_irq_hardware_type)
		{
			irqg->set_irq_hardware_type(irqg->irq_hardware_cookie,
				       	ih->ih_irq, IST_NONE);
		}
	}

	arm_intr_calculate_masks(irqg);

	restore_interrupts(oldirqstate);
	free(ih, M_DEVBUF);
}

void arm_intr_schedule(irqgroup_t group_cookie, irqhandler_t cookie)
{
	struct irq_group *irqg = group_cookie;
	struct intrhand *ih = cookie;
	struct intrq *iq = &(irqg->irqs[ih->ih_irq]);
	int oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);

	/* flag irq line as pending */
	iq->iq_pending = true;
	/* flag ipl level as pending */
	ipls_pending |= 1 << iq->iq_ipl;

	restore_interrupts(oldirqstate);

	if (current_ipl_level < iq->iq_ipl)
		arm_intr_splx_lifter(iq->iq_ipl);
}

void
arm_intr_print_all_masks()
{
	int ipl;

	for (ipl = NIPL-1; ipl > 0; ipl--)
	{
		struct intrq *iplq;
		printf("%02d: ", ipl);

		/* process the interrupt queues */
		for (iplq = TAILQ_FIRST(&(arm_iplq[ipl].ipl_list));
				(iplq != NULL);
				iplq = TAILQ_NEXT(iplq, ipl_list))
		{
		 	struct irq_group *irqg = iplq->iq_group;
			printf("%s (%s irq %d) ->", iplq->iq_name, irqg->group_name, iplq->iq_irq);
		}
		printf("\n");
	}
}

void
arm_intr_print_pending_irq_details(void);


void
arm_intr_print_pending_irq_details()
{
	int ipl;
	struct irq_group *irqg;

	printf("ipls_pending: 0x%0x\n", ipls_pending);

	for (ipl = NIPL-1; ipl > 0; ipl--)
	{
		struct intrq *iplq;
		printf("%02d: ", ipl);

		/* process the interrupt queues */
		for (iplq = TAILQ_FIRST(&(arm_iplq[ipl].ipl_list));
				(iplq != NULL);
				iplq = TAILQ_NEXT(iplq, ipl_list))
		{
			if (iplq->iq_pending)
			{
				irqg = iplq->iq_group;
				printf("%s (%s irq %d) ->", iplq->iq_name, irqg->group_name, iplq->iq_irq);
			}
		}
		printf("\n");
	}

	TAILQ_FOREACH(irqg, &irq_groups_list, irq_groups_list)
	{
		printf("%s: enabled: 0x%0x, soft_enabled = 0x%08x\n", irqg->group_name,
				irqg->intr_enabled, 
				irqg->intr_soft_enabled);
	}
	
}


static void
arm_intr_run_handlers_for_ipl(int ipl_level, struct clockframe *frame)
{
	struct intrq *iplq;
	int oldirqstate;

	arm_iplq[ipl_level].ipl_ev.ev_count++;
	
	/* process the interrupt queues */
	TAILQ_FOREACH(iplq, &(arm_iplq[ipl_level].ipl_list), ipl_list)
	{
		struct intrhand *ih;
		int intr_rc = 0;
		/* process the handler list queue */
		/* skip the intrq if it's not pending */
		if (__predict_false(!iplq->iq_pending))
			continue;

		/* 
		 * we're handling this item, so clear the pending flag
		 * Note it may get set again while we're running the irq handlers
		 */
		iplq->iq_pending = false;
		
		/* bump stats */
		iplq->iq_ev.ev_count++;
		uvmexp.intrs++;
		
		oldirqstate = enable_interrupts(I32_bit);
		for (ih = TAILQ_FIRST(&iplq->iq_list);
			((ih != NULL) && (intr_rc != 1));
		     ih = TAILQ_NEXT(ih, ih_list)) {
			intr_rc = (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
		}
		restore_interrupts(oldirqstate);

		/* Re-enable this interrupt now that it has been handled */
		arm_intr_enable_irq(iplq->iq_group, iplq->iq_irq);
	}
}

#if 0
int panic_ipl_level;
int panic_target_ipl_level;
int panic_ipls_pending;
#endif

void
arm_intr_process_pending_ipls(struct clockframe *frame, int target_ipl_level)
{
	int ipl_level;

	/* check that there are ipls we can process */
	while (ipls_pending >= (2 << target_ipl_level))
	{
		ipl_level = arm_intr_fls(ipls_pending);
		--ipl_level;
		current_ipl_level = ipl_level;
#if 0
		if (ipl_level <= target_ipl_level)
		{
			panic_ipl_level = ipl_level;
			panic_target_ipl_level = target_ipl_level;
			panic_ipls_pending = ipls_pending;
			Debugger();
		}
#endif
		ipls_pending &= ~(1<<ipl_level);
		arm_intr_run_handlers_for_ipl(ipl_level, frame);
	}
	current_ipl_level = target_ipl_level;
}

void
arm_intr_queue_irqs(irqgroup_t group_cookie, uint32_t hwpend)
{
	struct irq_group *irqg = group_cookie;
	uint32_t oldirqstate;

	/*
	 * Disable all the interrupts that are pending.  We will
	 * reenable them once they are processed and not masked.
	 */
	irqg->intr_enabled &= ~hwpend;
	arm_intr_set_hardware_mask(irqg);
	
	/* protect ipls_pending by disabling interrupts */
	oldirqstate = disable_interrupts(I32_bit);

	/* run through the pending bits */
	while (hwpend != 0) {
		int irq;
		struct intrq *iq;
		
		irq = ffs(hwpend) - 1;
		iq = &(irqg->irqs[irq]);
		hwpend &= ~(1 << irq);
		
		/* flag irq line as pending */
		iq->iq_pending = true;
		/* flag ipl level as pending */
		ipls_pending |= 1 << iq->iq_ipl;
	}
	restore_interrupts(oldirqstate);
}


/* 
 * an optimised find last set bit routine (basically which bit is the top most
 * set bit)  Like ffs except we're after the top bit not the bottom bit.
 *
 * Note this returns 0 if passed 0.  bits are numbered 1-32.
 *
 * a more optimal routine may find a way to normalize the top bit and call the
 * ffs routines lookup
 * 
 * on Arch v5, this could be a clz call
 *
 * Note that the compiler inlines this in an optimised build.  The if's actually end up as
 * a couple of instructions, inlined this totals 10 instructions.
 */
static uint8_t fls_table[16];
static int
arm_intr_fls(uint32_t n)
{
	int offset = 0;

	if (n & 0x00f0)
		offset = 4;
#if NIPL > 8
	if (n & 0x0f00)
		offset = 8;
#endif
#if NIPL > 12
	if (n & 0xf000)
		offset = 12;
#endif
#if NIPL > 16
#error fls code needs to be examined for larger IPL counts
#endif

	/* note that the table lookup can be done by the following instructions
	 * it's not clear which is quicker
	 */
	return fls_table[n>>offset]+offset;

#if 0
	int result = 0;
	int shifted;
	
	shifted = n >> offset;
	if (n & 0x1)
		result = 1;
	if (n & 0x2)
		result = 2;
	if (n & 0x4)
		result = 3;
	if (n & 0x8)
		result = 4;
	return result + offset;
#endif
}

static uint8_t fls_table[16] = {
	0,
/* 1 */
	1,
/* 2 */
	2, 2,
/* 4 */
	3, 3, 3, 3,
/* 8 */
	4, 4, 4, 4, 4, 4, 4, 4
};

