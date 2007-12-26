/*	$NetBSD: sa11x0_intr.c,v 1.1.2.3 2007/12/26 22:24:41 rjs Exp $	*/

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
 * IRQ handler for the Intel SA11X0 processor.
 * It has integrated interrupt controller.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: sa11x0_intr.c,v 1.1.2.3 2007/12/26 22:24:41 rjs Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/lock.h>

#include <arm/sa11x0/sa11x0_reg.h>
#include <arm/sa11x0/sa11x0_var.h>
#include <arm/sa11x0/sa11x0_intr.h>

/*
 * INTC autoconf glue
 */
static int	saintc_match(struct device *, struct cfdata *, void *);
static void	saintc_attach(struct device *, struct device *, void *);

CFATTACH_DECL(saintc, sizeof(struct device),
    saintc_match, saintc_attach, NULL, NULL);

static int saintc_attached;

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
	sa11x0_irq_handler_t func;
#endif
	void *cookie;		/* NULL for stackframe */
	/* struct evbnt ev; */
} handler[ICU_LEN];

volatile int softint_pending;
volatile int current_spl_level;
volatile imask_t intr_mask;
/* interrupt masks for each level */
imask_t sa11x0_imask[NIPL];
static int extirq_level[ICU_LEN];

static int
saintc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	struct sa11x0_attach_args *sa = aux;

	if (saintc_attached || sa->sa_addr != SAIPIC_BASE)
		return (0);

	return (1);
}

void
saintc_attach(struct device *parent, struct device *self, void *aux)
{
	int i;
	struct sa11x0_attach_args *sa = aux;

	saintc_attached = 1;

	aprint_normal("\n");

	if (bus_space_map(sa->sa_iot, sa->sa_addr, sa->sa_size, 0, 
	    &saipic_base))
		panic("%s: Cannot map registers", self->dv_xname);

	write_icu(SAIPIC_CR, 1);
	write_icu(SAIPIC_MR, 0);

	for(i = 0; i < sizeof handler / sizeof handler[0]; ++i){
		handler[i].func = stray_interrupt;
		handler[i].cookie = (void *)(intptr_t) i;
		extirq_level[i] = IPL_SERIAL;
	}

	init_interrupt_masks();

	_splraise(IPL_SERIAL);
	enable_interrupts(I32_bit);

	aprint_normal("%s: Interrupt Controller\n", self->dv_xname);
}

/*
 * Invoked very early on from the board-specific initarm(), in order to
 * inform us the virtual address of the interrupt controller's registers.
 */
void
sa11x0_intr_bootstrap(vaddr_t addr)
{

	saipic_base = addr;
}

static inline void
__raise(int ipl)
{

	if (current_spl_level < ipl)
		sa11x0_setipl(ipl);
}


/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[SI_NQUEUES] = {
	IPL_SOFTCLOCK,		/* SI_SOFTCLOCK */
	IPL_SOFTBIO,		/* SI_SOFTBIO */
	IPL_SOFTNET,		/* SI_SOFTNET */
	IPL_SOFTSERIAL,		/* SI_SOFTSERIAL */
};

/*
 * called from irq_entry.
 */
void
sa11x0_irq_handler(void *arg)
{
	struct clockframe *frame = arg;
	uint32_t irqbits;
	int irqno;
	int saved_spl_level;
	struct cpu_info *ci;

	ci = curcpu();
	ci->ci_idepth++;
	saved_spl_level = current_spl_level;

	/* get pending IRQs */
	irqbits = read_icu(SAIPIC_IP);

	for (irqno = 0 ; irqno < 32 ; irqno++) {
		/* XXX: Should we handle IRQs in priority order? */
		if (irqbits & (1 << irqno)) {

			/* raise spl to stop interrupts of lower priorities */
			if (saved_spl_level < extirq_level[irqno])
				sa11x0_setipl(extirq_level[irqno]);

#ifdef notyet
			/* Enable interrupt */
#endif
#ifndef MULTIPLE_HANDLERS_ON_ONE_IRQ
			(* handler[irqno].func)(handler[irqno].cookie == 0
						? frame : handler[irqno].cookie );
#else
			/* process all handlers for this interrupt.
			   XXX not yet */
#endif
		
#ifdef notyet
			/* Disable interrupt */
#endif

		}
	}

	/* restore spl to that was when this interrupt happen */
	sa11x0_setipl(saved_spl_level);
			
	ci->ci_idepth--;

	if(softint_pending & intr_mask.bits[IMASK_SOFTINT])
		sa11x0_do_pending();
}

static int
stray_interrupt(void *cookie)
{
	int irqno = (int)cookie;
	printf("stray interrupt %d\n", irqno);

	if (irqno < ICU_LEN) {
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
sa11x0_update_intr_masks(int irqno, int level)
{
	int psw = disable_interrupts(I32_bit);
	int i;

	for(i = 0; i < level; ++i) {
		imask_orbit(&sa11x0_imask[i], irqno); /* Enable interrupt at lower level */
	}

	for( ; i < NIPL-1; ++i) {
		imask_clrbit(&sa11x0_imask[i], irqno); /* Disable interrupt at upper level */
	}

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	imask_and(&sa11x0_imask[IPL_SOFTBIO], &sa11x0_imask[IPL_SOFTCLOCK]);
	imask_and(&sa11x0_imask[IPL_SOFTNET], &sa11x0_imask[IPL_SOFTBIO]);
	imask_and(&sa11x0_imask[IPL_SOFTSERIAL], &sa11x0_imask[IPL_SOFTNET]);
	imask_and(&sa11x0_imask[IPL_VM], &sa11x0_imask[IPL_SOFTSERIAL]);
	imask_and(&sa11x0_imask[IPL_SCHED], &sa11x0_imask[IPL_VM]);
	imask_and(&sa11x0_imask[IPL_HIGH], &sa11x0_imask[IPL_SCHED]);

	write_icu(SAIPIC_MR, sa11x0_imask[current_spl_level].bits[IMASK_ICU]);

	restore_interrupts(psw);
}


static void
init_interrupt_masks(void)
{

	memset(sa11x0_imask, 0, sizeof(sa11x0_imask));

	/*
	 * IPL_NONE has soft interrupts enabled only, at least until
	 * hardware handlers are installed.
	 */
	imask_zero(&sa11x0_imask[IPL_NONE]);
	imask_orbit(&sa11x0_imask[IPL_NONE], SI_SOFTCLOCK);
	imask_orbit(&sa11x0_imask[IPL_NONE], SI_SOFTBIO);
	imask_orbit(&sa11x0_imask[IPL_NONE], SI_SOFTNET);
	imask_orbit(&sa11x0_imask[IPL_NONE], SI_SOFTSERIAL);

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	imask_clrbit(&sa11x0_imask[IPL_SOFTCLOCK], SI_SOFTCLOCK);
	imask_clrbit(&sa11x0_imask[IPL_SOFTBIO], SI_SOFTBIO);
	imask_clrbit(&sa11x0_imask[IPL_SOFTNET], SI_SOFTNET);
	imask_clrbit(&sa11x0_imask[IPL_SOFTSERIAL], SI_SOFTSERIAL);

	imask_and(&sa11x0_imask[IPL_SOFTCLOCK], &sa11x0_imask[IPL_NONE]);
	imask_and(&sa11x0_imask[IPL_SOFTBIO], &sa11x0_imask[IPL_SOFTCLOCK]);
	imask_and(&sa11x0_imask[IPL_SOFTNET], &sa11x0_imask[IPL_SOFTBIO]);
	imask_and(&sa11x0_imask[IPL_SOFTSERIAL], &sa11x0_imask[IPL_SOFTNET]);
}

void
sa11x0_do_pending(void)
{
#ifdef __HAVE_FAST_SOFTINTS
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int oldirqstate, spl_save;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	spl_save = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#if 1
#define	DO_SOFTINT(si,ipl)						\
	if ((softint_pending & intr_mask.bits[IMASK_SOFTINT]) & si) {	\
		softint_pending &= ~si;					\
		__raise(ipl);						\
		restore_interrupts(oldirqstate);			\
		softintr_dispatch(si);					\
		oldirqstate = disable_interrupts(I32_bit);		\
		sa11x0_setipl(spl_save);				\
	}

	do {
		DO_SOFTINT(SI_SOFTSERIAL,IPL_SOFTSERIAL);
		DO_SOFTINT(SI_SOFTNET, IPL_SOFTNET);
		DO_SOFTINT(SI_SOFTCLOCK, IPL_SOFTCLOCK);
		DO_SOFTINT(SI_SOFT, IPL_SOFT);
	} while (softint_pending & intr_mask.bits[IMASK_SOFTINT]);
#else
	while( (si = ffs(softint_pending & softint_mask)) >= 0 ){
		softint_pending &= ~SI_TO_IRQBIT(si);
		__raise(si_to_ipl(si));
		restore_interrupts(oldirqstate);
		softintr_dispatch(si);
		oldirqstate = disable_interrupts(I32_bit);
		sa11x0_setipl(spl_save);
	}
#endif

	__cpu_simple_unlock(&processing);

	restore_interrupts(oldirqstate);
#endif
}


#undef splx
void
splx(int ipl)
{

	sa11x0_splx(ipl);
}

#undef _splraise
int
_splraise(int ipl)
{

	return sa11x0_splraise(ipl);
}

#undef _spllower
int
_spllower(int ipl)
{

	return sa11x0_spllower(ipl);
}

#undef _setsoftintr
void
_setsoftintr(int si)
{

	return sa11x0_setsoftintr(si);
}

void *
sa11x0_intr_establish(int irqno, int level,
    int (*func)(void *), void *cookie)
{
	int psw;

	psw = disable_interrupts(I32_bit);

	handler[irqno].cookie = cookie;
	handler[irqno].func = func;
	extirq_level[irqno] = level;
	sa11x0_update_intr_masks(irqno, level);

	intr_mask = sa11x0_imask[current_spl_level];

	restore_interrupts(psw);

	return (&handler[irqno]);
}

void
sa11x0_intr_disestablish(void *cookie)
{
	struct intrhandler *lhandler = cookie, *ih;
	int irqno = lhandler - handler;
	int psw;

	psw = disable_interrupts(I32_bit);

	ih = &handler[irqno];
	ih->func = stray_interrupt;
	ih->cookie = (void *)(intptr_t)irqno;
	extirq_level[irqno] = IPL_SERIAL;
	sa11x0_update_intr_masks(irqno, IPL_SERIAL);

	restore_interrupts(psw);
}
