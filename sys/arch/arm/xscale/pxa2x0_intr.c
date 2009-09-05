/*	$NetBSD: pxa2x0_intr.c,v 1.16 2009/09/05 17:40:35 bsh Exp $	*/

/*
 * Copyright (c) 2002  Genetec Corporation.  All rights reserved.
 * Written by Hiroyuki Bessho for Genetec Corporation.
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
 *	Genetec Corporation.
 * 4. The name of Genetec Corporation may not be used to endorse or 
 *    promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY GENETEC CORPORATION ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GENETEC CORPORATION
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * IRQ handler for the Intel PXA2X0 processor.
 * It has integrated interrupt controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pxa2x0_intr.c,v 1.16 2009/09/05 17:40:35 bsh Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/lock.h>

#include <arm/xscale/pxa2x0cpu.h>
#include <arm/xscale/pxa2x0reg.h>
#include <arm/xscale/pxa2x0var.h>
#include <arm/xscale/pxa2x0_intr.h>
#include <arm/sa11x0/sa11x0_var.h>

/*
 * INTC autoconf glue
 */
static int	pxaintc_match(struct device *, struct cfdata *, void *);
static void	pxaintc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(pxaintc, sizeof(struct device),
    pxaintc_match, pxaintc_attach, NULL, NULL);

static int pxaintc_attached;

static int stray_interrupt(void *);
static void init_interrupt_masks(void);

/*
 * interrupt dispatch table. 
 */
#ifdef MULTIPLE_HANDLERS_ON_ONE_IRQ
struct intrhand {
	TAILQ_ENTRY(intrhand) ih_list;	/* link on intrq list */
	int (*ih_func)(void *);		/* handler */
	void *ih_arg;			/* arg for handler */
};
#endif

static struct intrhandler {
#ifdef MULTIPLE_HANDLERS_ON_ONE_IRQ
	TAILQ_HEAD(,intrhand) list;
#else
	pxa2x0_irq_handler_t func;
#endif
	void *cookie;		/* NULL for stackframe */
	/* struct evbnt ev; */
} handler[ICU_LEN];

volatile int softint_pending;
volatile int intr_mask;
/* interrupt masks for each level */
int pxa2x0_imask[NIPL];
static int extirq_level[ICU_LEN];


static int
pxaintc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct pxaip_attach_args *pxa = aux;

	if (pxaintc_attached || pxa->pxa_addr != PXA2X0_INTCTL_BASE)
		return (0);

	return (1);
}

void
pxaintc_attach(struct device *parent, struct device *self, void *args)
{
	int i;

	pxaintc_attached = 1;

	aprint_normal(": Interrupt Controller\n");

#define	SAIPIC_ICCR	0x14

	write_icu(SAIPIC_ICCR, 1);
	write_icu(SAIPIC_MR, 0);

	for(i = 0; i < sizeof handler / sizeof handler[0]; ++i){
		handler[i].func = stray_interrupt;
		handler[i].cookie = (void *)(intptr_t) i;
		extirq_level[i] = IPL_SERIAL;
	}

	init_interrupt_masks();

	_splraise(IPL_SERIAL);
	enable_interrupts(I32_bit);
}

/*
 * Invoked very early on from the board-specific initarm(), in order to
 * inform us the virtual address of the interrupt controller's registers.
 */
void
pxa2x0_intr_bootstrap(vaddr_t addr)
{

	pxaic_base = addr;
}

static inline void
__raise(int ipl)
{

	if (curcpu()->ci_cpl < ipl)
		pxa2x0_setipl(ipl);
}

/*
 * called from irq_entry.
 */
void
pxa2x0_irq_handler(void *arg)
{
	struct clockframe *frame = arg;
	uint32_t irqbits;
	int irqno;
	int saved_spl_level;

	saved_spl_level = curcpu()->ci_cpl;

	/* get pending IRQs */
	irqbits = read_icu(SAIPIC_IP);

	while ((irqno = find_first_bit(irqbits)) >= 0) {
		/* XXX: Shuould we handle IRQs in priority order? */

		/* raise spl to stop interrupts of lower priorities */
		if (saved_spl_level < extirq_level[irqno])
			pxa2x0_setipl(extirq_level[irqno]);

#ifdef notyet
		/* Enable interrupt */
#endif
#ifndef MULTIPLE_HANDLERS_ON_ONE_IRQ
		(* handler[irqno].func)( 
			handler[irqno].cookie == 0
			? frame : handler[irqno].cookie );
#else
		/* process all handlers for this interrupt.
		   XXX not yet */
#endif
		
#ifdef notyet
		/* Disable interrupt */
#endif

		irqbits &= ~(1<<irqno);
	}

	/* restore spl to that was when this interrupt happen */
	pxa2x0_setipl(saved_spl_level);

#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

static int
stray_interrupt(void *cookie)
{
	int irqno = (int)cookie;
	int irqmin = CPU_IS_PXA250 ? PXA250_IRQ_MIN : PXA270_IRQ_MIN;

	printf("stray interrupt %d\n", irqno);

	if (irqmin <= irqno && irqno < ICU_LEN){
		int save = disable_interrupts(I32_bit);
		write_icu(SAIPIC_MR,
		    read_icu(SAIPIC_MR) & ~(1U<<irqno));
		restore_interrupts(save);
	}

	return 0;
}



/*
 * Interrupt Mask Handling
 */

void
pxa2x0_update_intr_masks(int irqno, int level)
{
	int mask = 1U<<irqno;
	int psw = disable_interrupts(I32_bit);
	int i;

	for(i = 0; i < level; ++i)
		pxa2x0_imask[i] |= mask; /* Enable interrupt at lower level */

	for( ; i < NIPL-1; ++i)
		pxa2x0_imask[i] &= ~mask; /* Disable itnerrupt at upper level */

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	pxa2x0_imask[IPL_SOFTBIO] &= pxa2x0_imask[IPL_SOFTCLOCK];
	pxa2x0_imask[IPL_SOFTNET] &= pxa2x0_imask[IPL_SOFTBIO];
	pxa2x0_imask[IPL_SOFTSERIAL] &= pxa2x0_imask[IPL_SOFTNET];
	pxa2x0_imask[IPL_VM] &= pxa2x0_imask[IPL_SOFTSERIAL];
	pxa2x0_imask[IPL_SCHED] &= pxa2x0_imask[IPL_VM];
	pxa2x0_imask[IPL_HIGH] &= pxa2x0_imask[IPL_SCHED];

	write_icu(SAIPIC_MR, pxa2x0_imask[curcpu()->ci_cpl]);

	restore_interrupts(psw);
}


static void
init_interrupt_masks(void)
{

	/*
	 * disable all interrups until handlers are installed.
	 */
	memset(pxa2x0_imask, 0, sizeof(pxa2x0_imask));

}

#undef splx
void
splx(int ipl)
{
	pxa2x0_splx(ipl);
}

#undef _splraise
int
_splraise(int ipl)
{
	return pxa2x0_splraise(ipl);
}

#undef _spllower
int
_spllower(int ipl)
{
	return pxa2x0_spllower(ipl);
}

void *
pxa2x0_intr_establish(int irqno, int level,
    int (*func)(void *), void *cookie)
{
	int psw;
	int irqmin = CPU_IS_PXA250 ? PXA250_IRQ_MIN : PXA270_IRQ_MIN;

	if (irqno < irqmin || irqno >= ICU_LEN)
		panic("intr_establish: bogus irq number %d", irqno);

	psw = disable_interrupts(I32_bit);

	handler[irqno].cookie = cookie;
	handler[irqno].func = func;
	extirq_level[irqno] = level;
	pxa2x0_update_intr_masks(irqno, level);

	intr_mask = pxa2x0_imask[curcpu()->ci_cpl];

	restore_interrupts(psw);

	return (&handler[irqno]);
}

void
pxa2x0_intr_disestablish(void *cookie)
{
	struct intrhandler *lhandler = cookie, *ih;
	int irqmin = CPU_IS_PXA250 ? PXA250_IRQ_MIN : PXA270_IRQ_MIN;
	int irqno = lhandler - handler;
	int psw;

	if (irqno < irqmin || irqno >= ICU_LEN)
		panic("intr_disestablish: bogus irq number %d", irqno);

	psw = disable_interrupts(I32_bit);

	ih = &handler[irqno];
	ih->func = stray_interrupt;
	ih->cookie = (void *)(intptr_t)irqno;
	extirq_level[irqno] = IPL_SERIAL;
	pxa2x0_update_intr_masks(irqno, IPL_SERIAL);

	restore_interrupts(psw);
}

/*
 * Glue for drivers of sa11x0 compatible integrated logics.
 */
void *
sa11x0_intr_establish(sa11x0_chipset_tag_t ic, int irq, int type, int level,
    int (*ih_fun)(void *), void *ih_arg)
{

	return pxa2x0_intr_establish(irq, level, ih_fun, ih_arg);
}
