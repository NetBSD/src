/*	$Id: at91aic.c,v 1.1.2.1 2007/11/10 02:56:27 matt Exp $	*/
/*	$NetBSD: at91aic.c,v 1.1.2.1 2007/11/10 02:56:27 matt Exp $	*/

/*
 * Copyright (c) 2007 Embedtronics Oy.
 * All rights reserved.
 *
 * Based on ep93xx_intr.c
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 * 
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jesse Off
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Ichiro FUKUHARA and Naoto Shimazaki.
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


/*
 * Interrupt support for the Atmel's AT91xx9xxx family controllers
 */

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>
#include <sys/termios.h>

#include <uvm/uvm_extern.h>

#include <machine/bus.h>
#include <machine/intr.h>

#include <arm/cpufunc.h>

#include <arm/at91/at91reg.h>
#include <arm/at91/at91var.h>
#include <arm/at91/at91aicreg.h>
#include <arm/at91/at91aicvar.h>

#define	NIRQ	32

/* Interrupt handler queues. */
struct intrq intrq[NIRQ];

/* Interrupts to mask at each level. */
static u_int32_t aic_imask[NIPL];
static u_int32_t swi_imask[NIPL];	// software interrupt mask

/* Current interrupt priority level. */
volatile int current_spl_level = 0;

/* Software copy of the IRQs we have enabled. */
volatile u_int32_t aic_intr_enabled;

/* Interrupts pending. */
static volatile int swipending;

#define	SI_TO_IRQBIT(si)	(1U << (si))

/*
 * Map a software interrupt queue to an interrupt priority level.
 */
static const int si_to_ipl[SI_NQUEUES] = {
	IPL_SOFT,		/* SI_SOFT */
	IPL_SOFTCLOCK,		/* SI_SOFTCLOCK */
	IPL_SOFTNET,		/* SI_SOFTNET */
	IPL_SOFTSERIAL,		/* SI_SOFTSERIAL */
};

#define	AICREG(reg)	*((volatile u_int32_t*) (AT91AIC_BASE + (reg)))

static int	at91aic_match(struct device *, struct cfdata *, void *);
static void	at91aic_attach(struct device *, struct device *, void *);

CFATTACH_DECL(at91aic, sizeof(struct device),
	      at91aic_match, at91aic_attach, NULL, NULL);

static int
at91aic_match(struct device *parent, struct cfdata *match, void *aux)
{
	if (strcmp(match->cf_name, "at91aic") == 0)
		return 2;
	return 0;
}

static void
at91aic_attach(struct device *parent, struct device *self, void *aux)
{
	(void)parent; (void)self; (void)aux;
	printf("\n");
}

static inline void
at91_set_intrmask(u_int32_t aic_irqs)
{
	AICREG(AIC_IDCR)	= aic_irqs;
	AICREG(AIC_IECR)	= aic_intr_enabled & ~aic_irqs;
}

static inline void
at91_enable_irq(int irq)
{
	aic_intr_enabled       |= (1U << irq);
	AICREG(AIC_IECR)	= (1U << irq);
}

static inline void
at91_disable_irq(int irq)
{
	aic_intr_enabled       &= ~(1U << irq);
	AICREG(AIC_IDCR)	=  (1U << irq);
}

/*
 * NOTE: This routine must be called with interrupts disabled in the CPSR.
 */
static void
at91aic_calculate_masks(void)
{
	struct intrq *iq;
	struct intrhand *ih;
	int irq, ipl;

	/* First, figure out which IPLs each IRQ has. */
	for (irq = 0; irq < NIRQ; irq++) {
		int levels = 0;
		iq = &intrq[irq];
		at91_disable_irq(irq);
		for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
		     ih = TAILQ_NEXT(ih, ih_list))
			levels |= (1U << ih->ih_ipl);
		iq->iq_levels = levels;
	}

	/* Next, figure out which IRQs are used by each IPL. */
	for (ipl = 0; ipl < NIPL; ipl++) {
		int aic_irqs = 0;
		for (irq = 0; irq < AIC_NIRQ; irq++) {
			if (intrq[irq].iq_levels & (1U << ipl))
				aic_irqs |= (1U << irq);
		}
		aic_imask[ipl] = aic_irqs;
	}

	aic_imask[IPL_NONE] = 0;
	swi_imask[IPL_NONE] = 0;

	/*
	 * Initialize the soft interrupt masks to block themselves.
	 */
	swi_imask[IPL_SOFT] = SI_TO_IRQBIT(SI_SOFT);
	swi_imask[IPL_SOFTCLOCK] = SI_TO_IRQBIT(SI_SOFTCLOCK);
	swi_imask[IPL_SOFTNET] = SI_TO_IRQBIT(SI_SOFTNET);
	swi_imask[IPL_SOFTSERIAL] = SI_TO_IRQBIT(SI_SOFTSERIAL);

	/*
	 * splsoftclock() is the only interface that users of the
	 * generic software interrupt facility have to block their
	 * soft intrs, so splsoftclock() must also block IPL_SOFT.
	 */
	aic_imask[IPL_SOFTCLOCK] |= aic_imask[IPL_SOFT];
	swi_imask[IPL_SOFTCLOCK] |= swi_imask[IPL_SOFT];

	/*
	 * splsoftnet() must also block splsoftclock(), since we don't
	 * want timer-driven network events to occur while we're
	 * processing incoming packets.
	 */
	aic_imask[IPL_SOFTNET] |= aic_imask[IPL_SOFTCLOCK];
	swi_imask[IPL_SOFTNET] |= swi_imask[IPL_SOFTCLOCK];

	/*
	 * Enforce a hierarchy that gives "slow" device (or devices with
	 * limited input buffer space/"real-time" requirements) a better
	 * chance at not dropping data.
	 */
	aic_imask[IPL_BIO] |= aic_imask[IPL_SOFTNET];
	swi_imask[IPL_BIO] |= swi_imask[IPL_SOFTNET];
	aic_imask[IPL_NET] |= aic_imask[IPL_BIO];
	swi_imask[IPL_NET] |= swi_imask[IPL_BIO];
	aic_imask[IPL_SOFTSERIAL] |= aic_imask[IPL_NET];
	swi_imask[IPL_SOFTSERIAL] |= swi_imask[IPL_NET];
	aic_imask[IPL_TTY] |= aic_imask[IPL_SOFTSERIAL];
	swi_imask[IPL_TTY] |= swi_imask[IPL_SOFTSERIAL];

	/*
	 * splvm() blocks all interrupts that use the kernel memory
	 * allocation facilities.
	 */
	aic_imask[IPL_VM] |= aic_imask[IPL_TTY];
	swi_imask[IPL_VM] |= swi_imask[IPL_TTY];

	/*
	 * Audio devices are not allowed to perform memory allocation
	 * in their interrupt routines, and they have fairly "real-time"
	 * requirements, so give them a high interrupt priority.
	 */
	aic_imask[IPL_AUDIO] |= aic_imask[IPL_VM];
	swi_imask[IPL_AUDIO] |= swi_imask[IPL_VM];

	/*
	 * splclock() must block anything that uses the scheduler.
	 */
	aic_imask[IPL_CLOCK] |= aic_imask[IPL_AUDIO];
	swi_imask[IPL_CLOCK] |= swi_imask[IPL_AUDIO];

	/*
	 * No separate statclock on the AT91?
	 */
	aic_imask[IPL_STATCLOCK] |= aic_imask[IPL_CLOCK];
	swi_imask[IPL_STATCLOCK] |= swi_imask[IPL_CLOCK];

	/*
	 * serial uarts have small buffers that need low-latency servicing 
	 */
	aic_imask[IPL_SERIAL] |= aic_imask[IPL_STATCLOCK];
	swi_imask[IPL_SERIAL] |= swi_imask[IPL_STATCLOCK];

	/*
	 * splhigh() must block "everything".
	 */
	aic_imask[IPL_HIGH] |= aic_imask[IPL_SERIAL];
	swi_imask[IPL_HIGH] |= swi_imask[IPL_SERIAL];

	/*
	 * Now compute which IRQs must be blocked when servicing any
	 * given IRQ.
	 */
	for (irq = 0; irq < MIN(NIRQ, AIC_NIRQ); irq++) {
		iq = &intrq[irq];
		if (TAILQ_FIRST(&iq->iq_list) != NULL)
			at91_enable_irq(irq);
	}
	/*
	 * update current mask
	 */
	at91_set_intrmask(aic_imask[current_spl_level]);
}

static void
at91_do_pending(void)
{
	static __cpu_simple_lock_t processing = __SIMPLELOCK_UNLOCKED;
	int	new, cur;
	u_int	oldirqstate, oldirqstate2;

	if (__cpu_simple_lock_try(&processing) == 0)
		return;

	new = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);

#define	DO_SOFTINT(si)							\
	if ((swipending & ~swi_imask[new]) & SI_TO_IRQBIT(si)) {	\
		swipending &= ~SI_TO_IRQBIT(si);			\
		current_spl_level = cur = si_to_ipl[(si)];		\
		at91_set_intrmask(aic_imask[cur]);			\
		oldirqstate2 = enable_interrupts(I32_bit);		\
		softintr_dispatch(si);					\
		restore_interrupts(oldirqstate2);			\
		current_spl_level = new;				\
		at91_set_intrmask(aic_imask[new]);			\
	}

	DO_SOFTINT(SI_SOFTSERIAL);
	DO_SOFTINT(SI_SOFTNET);
	DO_SOFTINT(SI_SOFTCLOCK);
	DO_SOFTINT(SI_SOFT);

	__cpu_simple_unlock(&processing);

	restore_interrupts(oldirqstate);
}

inline void
splx(int new)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = current_spl_level;
	if (old != new) {
		current_spl_level = new;
		at91_set_intrmask(aic_imask[new]);
	}
	restore_interrupts(oldirqstate);

	/* If there are software interrupts to process, do it. */
	if (swipending & ~swi_imask[new])
		at91_do_pending();
}

int
_splraise(int ipl)
{
	int	old;
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	old = current_spl_level;
	if (old != ipl) {
		current_spl_level = ipl;
		at91_set_intrmask(aic_imask[ipl]);
	}
	restore_interrupts(oldirqstate);

	return (old);
}

int
_spllower(int ipl)
{
	int	old = current_spl_level;

	if (old <= ipl)
		return (old);
	splx(ipl);
	return (old);
}

void
_setsoftintr(int si)
{
	u_int	oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	swipending |= SI_TO_IRQBIT(si);
	restore_interrupts(oldirqstate);

	/* Process unmasked pending soft interrupts. */
	if (swipending & ~swi_imask[current_spl_level])
		at91_do_pending();
}

/*
 * at91aic_init:
 *
 *	Initialize the rest of the interrupt subsystem, making it
 *	ready to handle interrupts from devices.
 */
void
at91aic_init(void)
{
	struct intrq *iq;
	int i;

	aic_intr_enabled = 0;

	// disable intrrupts:
	AICREG(AIC_IDCR)	= -1;

	for (i = 0; i < NIRQ; i++) {
		iq = &intrq[i];
		TAILQ_INIT(&iq->iq_list);

		sprintf(iq->iq_name, "irq %d", i);
		evcnt_attach_dynamic(&iq->iq_ev, EVCNT_TYPE_INTR,
				     NULL, "aic", iq->iq_name);
	}
	current_intr_depth = 0;
	current_spl_level = 0;

	/* All interrupts should use IRQ not FIQ */

	AICREG(AIC_IDCR)	= -1;	/* disable interrupts	*/
	AICREG(AIC_ICCR)	= -1;	/* clear all interrupts	*/
	AICREG(AIC_DCR)		= 0;	/* not in debug mode, just to make sure */
	for (i = 0; i < NIRQ; i++) {
	  AICREG(AIC_SMR(i))	= 0;	/* disable interrupt */
	  AICREG(AIC_SVR(i))	= (u_int32_t)&intrq[i];	// address of interrupt queue
	}
	AICREG(AIC_FVR)		= 0;	// fast interrupt...
	AICREG(AIC_SPU)		= 0;	// spurious interrupt vector

	AICREG(AIC_EOICR)	= 0;	/* clear logic... */
	AICREG(AIC_EOICR)	= 0;	/* clear logic... */

	at91aic_calculate_masks();

	/* Enable IRQs (don't yet use FIQs). */
	enable_interrupts(I32_bit);
}

void *
at91aic_intr_establish(int irq, int ipl, int type, int (*ih_func)(void *), void *arg)
{
	struct intrq*		iq;
	struct intrhand*	ih;
	u_int			oldirqstate;
	unsigned		ok;
	uint32_t		smr;

	if (irq < 0 || irq >= NIRQ)
		panic("intr_establish: IRQ %d out of range", irq);
	if (ipl < 0 || ipl >= NIPL)
		panic("intr_establish: IPL %d out of range", ipl);

	smr = 1;		// all interrupts have priority one.. ok?
	switch (type) {
	case _INTR_LOW_LEVEL:
		smr |= AIC_SMR_SRCTYPE_LVL_LO;
		break;
	case INTR_HIGH_LEVEL:
		smr |= AIC_SMR_SRCTYPE_LVL_HI;
		break;
	case INTR_FALLING_EDGE:
		smr |= AIC_SMR_SRCTYPE_FALLING;
		break;
	case INTR_RISING_EDGE:
		smr |= AIC_SMR_SRCTYPE_RISING;
		break;
	default:
		panic("intr_establish: interrupt type %d is invalid", type);
	}

	ih = malloc(sizeof(*ih), M_DEVBUF, M_NOWAIT);
	if (ih == NULL)
		return (NULL);

	ih->ih_func = ih_func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	ih->ih_ipl = ipl;

	iq = &intrq[irq];

	oldirqstate = disable_interrupts(I32_bit);
	if (TAILQ_FIRST(&iq->iq_list) == NULL || (iq->iq_type & ~type) == 0) {
		AICREG(AIC_SMR(irq)) = smr;
		iq->iq_type = type;
		TAILQ_INSERT_TAIL(&iq->iq_list, ih, ih_list);
		at91aic_calculate_masks();
		ok = 1;
	} else
		ok = 0;
	restore_interrupts(oldirqstate);

	if (ok) {
#ifdef	AT91AIC_DEBUG
		int i;
		printf("\n");
		for (i = 0; i < NIPL; i++) {
			printf("IPL%d: aic_imask=0x%08X swi_imask=0x%08X\n",
			       i, aic_imask[i], swi_imask[i]);
		}
#endif
	} else {
		free(ih, M_DEVBUF);
		ih = NULL;
	}

	return (ih);
}

void
at91aic_intr_disestablish(void *cookie)
{
	struct intrhand*	ih = cookie;
	struct intrq*		iq = &intrq[ih->ih_irq];
	u_int			oldirqstate;

	oldirqstate = disable_interrupts(I32_bit);
	TAILQ_REMOVE(&iq->iq_list, ih, ih_list);
	at91aic_calculate_masks();
	restore_interrupts(oldirqstate);
}

#include <arm/at91/at91reg.h>
#include <arm/at91/at91dbgureg.h>
#include <arm/at91/at91pdcreg.h>

static inline void intr_process(struct intrq *iq, int pcpl, struct irqframe *frame);

static inline void
intr_process(struct intrq *iq, int pcpl, struct irqframe *frame)
{
	struct intrhand*	ih;
	u_int			oldirqstate, intr;

	intr = iq - intrq;

	iq->iq_ev.ev_count++;
	uvmexp.intrs++;

	if ((1U << intr) & aic_imask[pcpl]) {
		panic("interrupt %d should be masked! (aic_imask=0x%X)", intr, aic_imask[pcpl]);
	}

	if (iq->iq_busy) {
		panic("interrupt %d busy!", intr);
	}

	iq->iq_busy = 1;

	for (ih = TAILQ_FIRST(&iq->iq_list); ih != NULL;
	     ih = TAILQ_NEXT(ih, ih_list)) {
		current_spl_level = ih->ih_ipl;
		at91_set_intrmask(aic_imask[ih->ih_ipl]);
		oldirqstate = enable_interrupts(I32_bit);
		(void) (*ih->ih_func)(ih->ih_arg ? ih->ih_arg : frame);
		restore_interrupts(oldirqstate);
	}

	if (!iq->iq_busy) {
		panic("interrupt %d not busy!", intr);
	}
	iq->iq_busy = 0;

	current_spl_level = pcpl;
	at91_set_intrmask(aic_imask[pcpl]);
}

void
at91aic_intr_dispatch(struct irqframe *frame)
{
	struct intrq*		iq;
	int			pcpl = current_spl_level;

	iq = (struct intrq *)AICREG(AIC_IVR);	// get current queue

	// OK, service interrupt
	if (iq)
		intr_process(iq, pcpl, frame);

	AICREG(AIC_EOICR) = 0;			// end of interrupt

	/* Check for pendings soft intrs. */
	if (swipending & ~swi_imask[pcpl]) {
		at91_do_pending();
	}
}

#if 0
void
at91aic_intr_poll(int irq)
{
	u_int		oldirqstate;
	uint32_t	ipr;
	int		pcpl = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);
	ipr = 	AICREG(AIC_IPR);
	if ((ipr & (1U << irq) & ~aic_imask[pcpl]))
		intr_process(&intrq[irq], pcpl, NULL);
	restore_interrupts(oldirqstate);

	/* Check for pendings soft intrs. */
	if (swipending & ~swi_imask[pcpl]) {
		at91_do_pending();
	}
}
#endif

void
at91aic_intr_poll(void *ihp, int flags)
{
	struct intrhand* ih = ihp;
	u_int		oldirqstate, irq = ih->ih_irq;
	uint32_t	ipr;
	int		pcpl = current_spl_level;

	oldirqstate = disable_interrupts(I32_bit);
	ipr = AICREG(AIC_IPR);
	if ((ipr & (1U << irq))
	    && (flags || !(aic_imask[pcpl] & (1U << irq)))) {
		current_spl_level = ih->ih_ipl;
		at91_set_intrmask(aic_imask[ih->ih_ipl]);
		(void)enable_interrupts(I32_bit);
		(void)(*ih->ih_func)(ih->ih_arg ? ih->ih_arg : NULL);
		(void)disable_interrupts(I32_bit);
		current_spl_level = pcpl;
		at91_set_intrmask(aic_imask[pcpl]);
	}
	restore_interrupts(oldirqstate);

	/* Check for pendings soft intrs. */
	if (swipending & ~swi_imask[pcpl]) {
		at91_do_pending();
	}
}
