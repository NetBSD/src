/*	$NetBSD: omap_intr.c,v 1.2.24.2 2008/01/20 16:04:03 chris Exp $	*/

/*
 * Based on arch/arm/xscale/pxa2x0_intr.c
 *
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
 * IRQ handler for the Texas Instruments OMAP processors.  The OMAPs
 * have two cascaded interrupt controllers.  They have similarities, but
 * are not identical.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: omap_intr.c,v 1.2.24.2 2008/01/20 16:04:03 chris Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/malloc.h>

#include <machine/bus.h>
#include <machine/intr.h>
#include <machine/lock.h>

#include <arm/omap/omap_reg.h>
#include <arm/omap/omap_tipb.h>

/*
 * Array of arrays to give us the per bank masks for each level.  The OMAP
 * interrupt controller blocks an interrupt when the corresponding bit is set
 * in the mask register.  Initialize our masks to have all interrupts blocked
 * for all levels.
 */
uint32_t omap_spl_masks[NIPL][OMAP_NBANKS] =
	{ [ 0 ... NIPL-1 ] = { [ 0 ... OMAP_NBANKS-1 ] = ~0 } };

/*
 * And OR in the following global interrupt masks at all levels.  This is
 * used to hold off interrupts that omap_irq_handler will soon service,
 * since servicing is performed in random order wrt spl levels.
 */

uint32_t omap_global_masks[OMAP_NBANKS];

#ifdef __HAVE_FAST_SOFTINTS
#define	SI_SOFTCLOCK	0
#define	SI_SOFTBIO	1
#define	SI_SOFTNET	2
#define	SI_SOFTSERIAL	3
/* Array to translate from software interrupt number to priority level. */
static const int si_to_ipl[] = {
	[SI_SOFTCLOCK] = IPL_SOFTCLOCK,
	[SI_SOFTBIO] = IPL_SOFTBIO,
	[SI_SOFTNET] = IPL_SOFTNET,
	[SI_SOFTSERIAL] = IPL_SOFTSERIAL,
};

static int soft_interrupt(void *);
#endif

static int stray_interrupt(void *);
static void init_interrupt_masks(void);
static void omap_update_intr_masks(int, int);
static void omapintc_set_name(int, const char *, int);

typedef int (* omap_irq_handler_t)(void *);

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

static struct irq_handler {
	struct evcnt ev;
	char irq_num_str[8];
	const char *name;
#ifdef MULTIPLE_HANDLERS_ON_ONE_IRQ
	TAILQ_HEAD(,intrhand) list;
#else
	omap_irq_handler_t func;
#endif
	void *cookie;		/* NULL for stack frame */
	/* struct evbnt ev; */
} handler[OMAP_NIRQ];

volatile int current_spl_level;
static int extirq_level[OMAP_NIRQ];

int
omapintc_match(struct device *parent, struct cfdata *cf, void *aux)
{
	return (1);
}

void
omapintc_attach(struct device *parent, struct device *self, void *args)
{
	int i;
	aprint_normal(": Interrupt Controller\n");
	aprint_naive("\n");

	/* Make sure we have interrupts globally off. */
	disable_interrupts(I32_bit);

	/*
	 * Initialize the controller hardware.
	 */
	/* Turn off the first level's global mask. */
	write_icu(OMAP_INT_L1_BASE, OMAP_INTL1_GMR, 0);
	/* Turn off the second level's global mask. */
	write_icu(OMAP_INT_L2_BASE, OMAP_INTL2_CONTROL, 0);
	/* Allow the second level to automatically idle. */
	write_icu(OMAP_INT_L2_BASE, OMAP_INTL2_OCP_CFG,
	    OMAP_INTL2_OCP_CFG_SMART_IDLE | OMAP_INTL2_OCP_CFG_AUTOIDLE);
	/*
	 * Now set all of the Interrupt Level Registers.  Set the triggering
	 * as appropriate for that interrupt, but otherwise, set them all to
	 * low priority and to be an IRQ (not a FIQ).
	 */
	static const uint32_t ilr_def
	    = ((OMAP_INTB_ILR_PRIO_LOWEST << OMAP_INTB_ILR_PRIO_SHIFT)
	       | OMAP_INTB_ILR_IRQ);
	for (i=0; i<OMAP_NIRQ; i++) {
		const omap_intr_info_t *inf = &omap_intr_info[i];
		volatile uint32_t *ILR = (volatile uint32_t *)inf->ILR;

		switch (inf->trig) {
		case INVALID:
			break;
		case TRIG_LEVEL:
			*ILR = ilr_def | OMAP_INTB_ILR_LEVEL;
			break;
		case TRIG_EDGE:
		case TRIG_LEVEL_OR_EDGE:
			*ILR = ilr_def | OMAP_INTB_ILR_EDGE;
			break;
		default:
			panic("Bad trigger (%d) on irq %d\n", inf->trig, i);
			break;
		}
	}

	/* Set all interrupts to be stray and to have event counters. */
	omapintc_set_name(OMAP_INT_L2_IRQ, "IRQ from L2", false);
	omapintc_set_name(OMAP_INT_L2_FIQ, "FIQ from L2", false);
#ifdef __HAVE_FAST_SOFTINTS
	omapintc_set_name(omap_si_to_irq[SI_SOFTCLOCK], "SOFTCLOCK", false);
	omapintc_set_name(omap_si_to_irq[SI_SOFTBIO], "SOFTBIO", false);
	omapintc_set_name(omap_si_to_irq[SI_SOFTNET], "SOFTNET", false);
	omapintc_set_name(omap_si_to_irq[SI_SOFTSERIAL], "SOFTSERIAL", false);
#endif
	for(i = 0; i < __arraycount(handler); ++i) {
		handler[i].func = stray_interrupt;
		handler[i].cookie = (void *)(intptr_t) i;
		extirq_level[i] = IPL_SERIAL;
		sprintf(handler[i].irq_num_str, "#%d", i);
		if (handler[i].name == NULL)
			omapintc_set_name(i, handler[i].irq_num_str, false);
	}
#ifdef __HAVE_FAST_SOFTINTS
	/* and then set up the software interrupts. */
	for(i = 0; i < __arraycount(omap_si_to_irq); ++i) {
		int irq = omap_si_to_irq[i];
		handler[irq].func = soft_interrupt;
		/* Cookie value zero means pass interrupt frame instead */
		handler[irq].cookie = (void *)(intptr_t) (i | 0x80000000);
		KASSERT(i < __arraycount(si_to_ipl));
		extirq_level[irq] = si_to_ipl[i];
	}
#endif

	/* Initialize our table of masks. */
	init_interrupt_masks();

	/*
	 * Now that we have the masks for all the levels set up, write the
	 * masks to the hardware.
	 */
	omap_splx(IPL_SERIAL);

	/* Everything is all set.  Enable the interrupts. */
	enable_interrupts(I32_bit);
}

static void
dispatch_irq(int irqno, struct clockframe *frame)
{
	if (extirq_level[irqno] != current_spl_level)
		splx(extirq_level[irqno]);

#ifndef MULTIPLE_HANDLERS_ON_ONE_IRQ
	(* handler[irqno].func)(handler[irqno].cookie == 0
				? frame : handler[irqno].cookie );
#else
		/* process all handlers for this interrupt. */
#error Having multiple handlers per IRQ is not yet supported.
#endif
}

/*
 * called from irq_entry.
 */
void
omap_irq_handler(void *arg)
{
	struct clockframe *frame = arg;
	int saved_spl_level;
	int unmaskedpend[OMAP_NBANKS];
	int bank;
	int level2 = 0;

	saved_spl_level = current_spl_level;

	for (bank = 0; bank < OMAP_NBANKS; bank++) {
		int masked = read_icu(omap_intr_bank_bases[bank],
				      OMAP_INTB_MIR);
		int pend = read_icu(omap_intr_bank_bases[bank],
				    OMAP_INTB_ITR);
		/*
		 * Save away pending unmasked interrupts for handling in
		 * a moment.  Mask those interrupts, will unmask after
		 * serviced.
		 */

		unmaskedpend[bank] = pend & ~masked;
		write_icu(omap_intr_bank_bases[bank], OMAP_INTB_ITR,
			  ~unmaskedpend[bank]);
		write_icu(omap_intr_bank_bases[bank], OMAP_INTB_MIR,
			  masked | unmaskedpend[bank]);
		omap_global_masks[bank] |= unmaskedpend[bank];

		/*
		 * If any interrupt is pending for the Level-2 controller
		 * then we need a Level-2 new IRQ agreement to receive
		 * another one.
		 */

		if (bank && pend)
			level2 = 1;
	}

	/* Let the interrupt controllers process the next interrupt.
	   Acknowledge Level-1 IRQs. */
	write_icu(OMAP_INT_L1_BASE, OMAP_INTL1_CONTROL,
		  OMAP_INTL1_CONTROL_NEW_IRQ_AGR);

	if (level2) {
		    /* Acknowledge Level-2 IRQs. */
		write_icu(OMAP_INT_L2_BASE, OMAP_INTL2_CONTROL,
			  OMAP_INTL2_CONTROL_NEW_IRQ_AGR);
	}

	/*
	 * Invoke handlers for interrupts found pending and unmasked
	 * above.  Unmask the interrupts for each bank after servicing.
	 */

	for (bank = 0; bank < OMAP_NBANKS; bank++) {
		int irqno;
		int oldirqstate;
		int pendtodo = unmaskedpend[bank];

		if (!unmaskedpend[bank])
		    continue;

		while ((irqno = ffs(pendtodo) - 1) != -1) {
			irqno += bank * OMAP_BANK_WIDTH;

			if (omap_intr_info[irqno].trig == INVALID)
				printf("Bad IRQ %d.\n", irqno);

			handler[irqno].ev.ev_count++;

			if (irqno != OMAP_INT_L2_IRQ) {
				oldirqstate = enable_interrupts(I32_bit);
				dispatch_irq(irqno, frame);
				restore_interrupts(oldirqstate);
			}

			pendtodo &= ~omap_intr_info[irqno].mask;
		}

		omap_global_masks[bank] &= ~unmaskedpend[bank];
		write_icu(omap_intr_bank_bases[bank], OMAP_INTB_MIR,
			  read_icu(omap_intr_bank_bases[bank],
				   OMAP_INTB_MIR) &
			  ~unmaskedpend[bank]);
	}

	/* Restore spl to what it was when this interrupt happened. */
	splx(saved_spl_level);
}

static int
stray_interrupt(void *cookie)
{
	int irqno = (int)cookie;
	printf("stray interrupt number %d: \"%s\".\n",
	       irqno, handler[irqno].name);

	if (OMAP_IRQ_MIN <= irqno && irqno < OMAP_NIRQ){
		/* Keep it from happening again. */
		omap_update_intr_masks(irqno, IPL_NONE);
	}

	return 0;
}

#ifdef __HAVE_FAST_SOFTINTS
static int
soft_interrupt(void *cookie)
{
	int si = (int)cookie & ~(0x80000000);

	softintr_dispatch(si);

	return 0;
}
#endif


static inline void
level_block_irq(int lvl, const omap_intr_info_t *inf)
{
	omap_spl_masks[lvl][inf->bank_num] |= inf->mask;
}
static inline void
level_allow_irq(int lvl, const omap_intr_info_t *inf)
{
	omap_spl_masks[lvl][inf->bank_num] &= ~inf->mask;
}
static inline void
level_mask_level(int dst_level, int src_level)
{
	int i;
	for (i = 0; i < OMAP_NBANKS; i++) {
		omap_spl_masks[dst_level][i]
		    |= omap_spl_masks[src_level][i];
	}
}

/*
 * Interrupt Mask Handling
 */

static void
omap_update_intr_masks(int irqno, int level)
{
	const omap_intr_info_t *irq_inf =&omap_intr_info[irqno];
	int i;
	int psw = disable_interrupts(I32_bit);

	for(i = 0; i < level; ++i)
		/* Enable interrupt at lower level */
		level_allow_irq(i, irq_inf);

	for( ; i < NIPL-1; ++i)
		/* Disable interrupt at upper level */
		level_block_irq(i, irq_inf);

	/*
	 * There is no way for interrupts to be enabled at upper levels, but
	 * not at lower levels and vice-versa.  This means that this function
	 * doesn't have to enforce it.
	 */

	/* Refresh the hardware's masks in case the current level's changed. */
	omap_splx(current_spl_level);

	restore_interrupts(psw);
}


static void
init_interrupt_masks(void)
{
	const omap_intr_info_t
#ifdef __HAVE_FAST_SOFTINTS
	    *softclock_inf =&omap_intr_info[omap_si_to_irq[SI_SOFTCLOCK]],
	    *softbio_inf   =&omap_intr_info[omap_si_to_irq[SI_SOFTBIO]],
	    *softnet_inf   =&omap_intr_info[omap_si_to_irq[SI_SOFTNET]],
	    *softserial_inf=&omap_intr_info[omap_si_to_irq[SI_SOFTSERIAL]],
#endif
	    *l2_inf        =&omap_intr_info[OMAP_INT_L2_IRQ];
	int i;

#ifdef __HAVE_FAST_SOFTINTS
	/*
	 * We just blocked all the interrupts in all the masks.  Now we just
	 * go through and modify the masks to allow the software interrupts as
	 * documented in the spl(9) man page.
	 */
	for (i = IPL_NONE; i < IPL_SOFTCLOCK; ++i)
		level_allow_irq(i, softclock_inf);
	for (i = IPL_NONE; i < IPL_SOFTBIO; ++i)
		level_allow_irq(i, softbio_inf);
	for (i = IPL_NONE; i < IPL_SOFTNET; ++i)
		level_allow_irq(i, softnet_inf);
	for (i = IPL_NONE; i < IPL_SOFTSERIAL; ++i)
		level_allow_irq(i, softserial_inf);
#endif

	/*
	 * We block level 2 interrupts down in the level 2 controller, so we
	 * can allow the level 1 interrupt controller to service the level 2
	 * interrupts at all levels.
	 */
	for (i = IPL_NONE; i < IPL_SERIAL; ++i)
		level_allow_irq(i, l2_inf);
}

#undef splx
void
splx(int ipl)
{
	omap_splx(ipl);
}

#undef _splraise
int
_splraise(int ipl)
{
	return omap_splraise(ipl);
}

#undef _spllower
int
_spllower(int ipl)
{

	return omap_spllower(ipl);
}

#ifdef __HAVE_FAST_SOFTINTS
#undef _setsoftintr
void
_setsoftintr(int si)
{

	return omap_setsoftintr(si);
}
#endif

void *
omap_intr_establish(int irqno, int level, const char *name,
		    int (*func)(void *), void *cookie)
{
	const omap_intr_info_t *info;
	int psw;
	if (irqno < OMAP_IRQ_MIN || irqno >= OMAP_NIRQ
	    || irqno == OMAP_INT_L2_IRQ
	    || omap_intr_info[irqno].trig == INVALID)
		panic("%s(): bogus irq number %d", __func__, irqno);

#ifndef MULTIPLE_HANDLERS_ON_ONE_IRQ
	if (handler[irqno].func != stray_interrupt)
		panic("Shared interrupts are not supported (irqno=%d)\n",
		      irqno);
#endif

	psw = disable_interrupts(I32_bit);

	/*
	 * Name the evcnt what they passed us.  Note this will zero the
	 * count.
	 */
	omapintc_set_name(irqno, name, true);

	/* Clear any existing interrupt by writing a zero into its ITR bit. */
	info = &omap_intr_info[irqno];
	write_icu(info->bank_base, OMAP_INTB_ITR, ~info->mask);

	handler[irqno].cookie = cookie;
	handler[irqno].func = func;
	extirq_level[irqno] = level;
	omap_update_intr_masks(irqno, level);

	restore_interrupts(psw);

	return (&handler[irqno]);
}

void
omap_intr_disestablish(void *v)
{
	struct irq_handler *irq_handler = v;
	int irqno = ((unsigned)v - (unsigned)&handler[0])/sizeof(handler[0]);

	if (irqno < OMAP_IRQ_MIN || irqno >= OMAP_NIRQ
	    || irqno == OMAP_INT_L2_IRQ
	    || omap_intr_info[irqno].trig == INVALID)
		panic("%s(): bogus irq number %d", __func__, irqno);

	int psw = disable_interrupts(I32_bit);

	/*
	 * Put the evcnt name back to our number string.  Note this will zero
	 * the count.
	 */
	omapintc_set_name(irqno, handler[irqno].irq_num_str, true);

	irq_handler->cookie = (void *)(intptr_t) irqno;
	irq_handler->func = stray_interrupt;
	extirq_level[irqno] = IPL_SERIAL;
	omap_update_intr_masks(irqno, IPL_NONE);

	restore_interrupts(psw);
}

static void
omapintc_set_name(int irqno, const char *name, int detach_first)
{
	const char *group;

	if (irqno < OMAP_INT_L1_NIRQ)
		group = "omap_intr L1";
	else
		group = "omap_intr L2";

	if (detach_first)
		evcnt_detach(&handler[irqno].ev);

	handler[irqno].name = name;

	evcnt_attach_dynamic(&handler[irqno].ev, EVCNT_TYPE_INTR, NULL,
			     group, handler[irqno].name);
}
