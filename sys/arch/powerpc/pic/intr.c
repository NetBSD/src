/*	$NetBSD: intr.c,v 1.1.2.13 2007/05/07 18:11:41 garbled Exp $ */

/*-
 * Copyright (c) 2007 Michael Lorenz
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of The NetBSD Foundation nor the names of its
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.1.2.13 2007/05/07 18:11:41 garbled Exp $");

#include "opt_multiprocessor.h"

#include <sys/param.h>
#include <sys/malloc.h>
#include <sys/kernel.h>

#include <uvm/uvm_extern.h>

#include <arch/powerpc/pic/picvar.h>
#include "opt_pic.h"
#include "opt_interrupt.h"
#if defined(PIC_I8259) || defined (PIC_PREPIVR)
#include <machine/isa_machdep.h>
#endif

#define MAX_PICS	8	/* 8 PICs ought to be enough for everyone */

#define NVIRQ		32	/* 32 virtual IRQs */
#define NIRQ		128	/* up to 128 HW IRQs */

#define HWIRQ_MAX	(NVIRQ - 4 - 1)
#define HWIRQ_MASK	0x0fffffff
#define	LEGAL_VIRQ(x)	((x) >= 0 && (x) < NVIRQ)

struct pic_ops *pics[MAX_PICS];
int num_pics = 0;
int max_base = 0;
uint8_t	virq[NIRQ];
int	virq_max = 0;
int	imask[NIPL];
int	primary_pic = 0;

static int	fakeintr(void *);
static int	mapirq(uint32_t);
static void	intr_calculatemasks(void);
static struct pic_ops *find_pic_by_irq(int);

static struct intr_source intrsources[NVIRQ];

void
pic_init(void)
{
	int i;

	for (i = 0; i < NIRQ; i++)
		virq[i] = 0;
	memset(intrsources, 0, sizeof(intrsources));
}

int
pic_add(struct pic_ops *pic)
{

	if (num_pics >= MAX_PICS)
		return -1;

	pics[num_pics] = pic;
	pic->pic_intrbase = max_base;
	max_base += pic->pic_numintrs;
	num_pics++;
	
	return pic->pic_intrbase;
}

static struct pic_ops *
find_pic_by_irq(int irq)
{
	struct pic_ops *current;
	int base = 0;

	while (base < num_pics) {

		current = pics[base];
		if ((irq >= current->pic_intrbase) &&
		    (irq < (current->pic_intrbase + current->pic_numintrs))) {

			return current;
		}
		base++;
	}
	return NULL;
}

static int
fakeintr(void *arg)
{

	return 0;
}

/*
 * Register an interrupt handler.
 */
void *
intr_establish(int hwirq, int type, int level, int (*ih_fun)(void *),
    void *ih_arg)
{
	struct intrhand **p, *q, *ih;
	struct intr_source *is;
	struct pic_ops *pic;
	static struct intrhand fakehand;
	int irq, maxlevel = level;

	if (maxlevel == IPL_NONE)
		maxlevel = IPL_HIGH;

	if (hwirq >= max_base) {

		panic("%s: bogus IRQ %d, max is %d", __func__, hwirq,
		    max_base - 1);
	}

	pic = find_pic_by_irq(hwirq);
	if (pic == NULL) {

		panic("%s: cannot find a pic for IRQ %d", __func__, hwirq);
	}

	irq = mapirq(hwirq);

	/* no point in sleeping unless someone can free memory. */
	ih = malloc(sizeof *ih, M_DEVBUF, cold ? M_NOWAIT : M_WAITOK);
	if (ih == NULL)
		panic("intr_establish: can't malloc handler info");

	if (!LEGAL_VIRQ(irq) || type == IST_NONE)
		panic("intr_establish: bogus irq (%d) or type (%d)", irq, type);

	is = &intrsources[irq];

	switch (is->is_type) {
	case IST_NONE:
		is->is_type = type;
		break;
	case IST_EDGE:
	case IST_LEVEL:
		if (type == is->is_type)
			break;
	case IST_PULSE:
		if (type != IST_NONE)
			panic("intr_establish: can't share %s with %s",
			    intr_typename(is->is_type),
			    intr_typename(type));
		break;
	}
	if (is->is_hand == NULL) {
		snprintf(is->is_source, sizeof(is->is_source), "irq %d",
		    is->is_hwirq);
		evcnt_attach_dynamic(&is->is_ev, EVCNT_TYPE_INTR, NULL,
		    pic->pic_name, is->is_source);
	}

	/*
	 * Figure out where to put the handler.
	 * This is O(N^2), but we want to preserve the order, and N is
	 * generally small.
	 */
	for (p = &is->is_hand; (q = *p) != NULL; p = &q->ih_next) {

		maxlevel = max(maxlevel, q->ih_level);
	}

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_level = level;
	fakehand.ih_fun = fakeintr;
	*p = &fakehand;

	intr_calculatemasks();

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_next = NULL;
	ih->ih_level = level;
	ih->ih_irq = irq;
	*p = ih;

	if (pic->pic_establish_irq != NULL)
		pic->pic_establish_irq(pic, hwirq - pic->pic_intrbase,
		    is->is_type, maxlevel);

	return ih;
}

void
dummy_pic_establish_intr(struct pic_ops *pic, int irq, int type, int pri)
{
}

/*
 * Deregister an interrupt handler.
 */
void
intr_disestablish(void *arg)
{
	struct intrhand *ih = arg;
	int irq = ih->ih_irq;
	struct intr_source *is = &intrsources[irq];
	struct intrhand **p, *q;

	if (!LEGAL_VIRQ(irq))
		panic("intr_disestablish: bogus irq %d", irq);

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &is->is_hand; (q = *p) != NULL && q != ih; p = &q->ih_next)
		;
	if (q)
		*p = q->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	free((void *)ih, M_DEVBUF);

	intr_calculatemasks();

	if (is->is_hand == NULL) {
		is->is_type = IST_NONE;
		evcnt_detach(&is->is_ev);
	}
}

/*
 * Map max_base irqs into 32 (bits).
 */
static int
mapirq(uint32_t irq)
{
	struct pic_ops *pic;
	int v;

	if (irq >= max_base)
		panic("invalid irq %d", irq);

	if ((pic = find_pic_by_irq(irq)) == NULL)
		panic("%s: cannot find PIC for IRQ %d", __func__, irq);

	if (virq[irq])
		return virq[irq];

	virq_max++;
	v = virq_max;
	if (v > HWIRQ_MAX)
		panic("virq overflow");

	intrsources[v].is_hwirq = irq;
	intrsources[v].is_pic = pic;
	virq[irq] = v;
#ifdef PIC_DEBUG
	printf("mapping irq %d to virq %d\n", irq, v);
#endif
	return v;
}

const char *
intr_typename(int type)
{

	switch (type) {
        case IST_NONE :
		return "none";
        case IST_PULSE:
		return "pulsed";
        case IST_EDGE:
		return "edge-triggered";
        case IST_LEVEL:
		return "level-triggered";
	default:
		panic("intr_typename: invalid type %d", type);
	}
}

/*
 * Recalculate the interrupt masks from scratch.
 * We could code special registry and deregistry versions of this function that
 * would be faster, but the code would be nastier, and we don't expect this to
 * happen very much anyway.
 */
static void
intr_calculatemasks(void)
{
	struct intr_source *is;
	struct intrhand *q;
	struct pic_ops *current;
	int irq, level, i, base;

	/* First, figure out which levels each IRQ uses. */
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		register int levels = 0;
		for (q = is->is_hand; q; q = q->ih_next)
			levels |= 1 << q->ih_level;
		is->is_level = levels;
	}

	/* Then figure out which IRQs use each level. */
	for (level = 0; level < NIPL; level++) {
		register int irqs = 0;
		for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++)
			if (is->is_level & (1 << level))
				irqs |= 1 << irq;
		imask[level] = irqs;
	}

	/*
	 * IPL_CLOCK should mask clock interrupt even if interrupt handler
	 * is not registered.
	 */
	imask[IPL_CLOCK] |= 1 << SPL_CLOCK;

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

#ifdef SLOPPY_IPLS
	/*
	 * Enforce a sloppy hierarchy as in spl(9)
	 */
	/* everything above softclock must block softclock */
	for (i = IPL_SOFTCLOCK; i < NIPL; i++)
		imask[i] |= imask[IPL_SOFTCLOCK];

	/* everything above softnet must block softnet */
	for (i = IPL_SOFTNET; i < NIPL; i++)
		imask[i] |= imask[IPL_SOFTNET];

	/* IPL_TTY must block softserial */
	imask[IPL_TTY] |= imask[IPL_SOFTSERIAL];

	/* IPL_VM must block net, block IO and tty */
	imask[IPL_VM] |= (imask[IPL_NET] | imask[IPL_BIO] | imask[IPL_TTY]);

	/* IPL_SERIAL must block IPL_TTY */
	imask[IPL_SERIAL] |= imask[IPL_TTY];

	/* IPL_HIGH must block all other priority levels */	
	for (i = IPL_NONE; i < IPL_HIGH; i++)
		imask[IPL_HIGH] |= imask[i];
#else	/* !SLOPPY_IPLS */
	/*
	 * strict hierarchy - all IPLs block everything blocked by any lower
	 * IPL
	 */
	for (i = 1; i < NIPL; i++)
		imask[i] |= imask[i - 1];
#endif	/* !SLOPPY_IPLS */

#ifdef DEBUG_IPL
	for (i = 0; i < NIPL; i++) {
		printf("%2d: %08x\n", i, imask[i]);
	}
#endif

	/* And eventually calculate the complete masks. */
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		register int irqs = 1 << irq;
		for (q = is->is_hand; q; q = q->ih_next)
			irqs |= imask[q->ih_level];
		is->is_mask = irqs;
	}

	/* Lastly, enable IRQs actually in use. */
	for (base = 0; base < num_pics; base++) {
		current = pics[base];
		for (i = 0; i < current->pic_numintrs; i++)
			current->pic_disable_irq(current, i);
	}
	
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		if (is->is_hand)
			pic_enable_irq(is->is_hwirq);
	}
}

void
pic_enable_irq(int num)
{
	struct pic_ops *current;
	int type;

	current = find_pic_by_irq(num);
	if (current == NULL)
		panic("%s: bogus IRQ %d", __func__, num);
	type = intrsources[virq[num]].is_type;
	current->pic_enable_irq(current, num - current->pic_intrbase, type);
}

void
pic_mark_pending(int irq)
{
	struct cpu_info * const ci = curcpu();
	int v, msr;

	v = virq[irq];
	if (v == 0)
		printf("IRQ %d maps to 0\n", irq);	

	msr = mfmsr();
	mtmsr(msr & ~PSL_EE);
	ci->ci_ipending |= 1 << v;
	mtmsr(msr);
}	

void
pic_do_pending_int(void)
{
	struct cpu_info * const ci = curcpu();
	struct intr_source *is;
	struct intrhand *ih;
	struct pic_ops *pic;
	int irq;
	int pcpl;
	int hwpend;
	int emsr, dmsr;

	if (ci->ci_iactive)
		return;

	ci->ci_iactive = 1;
	emsr = mfmsr();
	KASSERT(emsr & PSL_EE);
	dmsr = emsr & ~PSL_EE;
	mtmsr(dmsr);

	pcpl = ci->ci_cpl;
again:

#ifdef MULTIPROCESSOR
	if (ci->ci_cpuid == 0) {
#endif
	/* Do now unmasked pendings */
	while ((hwpend = (ci->ci_ipending & ~pcpl & HWIRQ_MASK)) != 0) {
		irq = 31 - cntlzw(hwpend);
		KASSERT(irq <= virq_max);
		ci->ci_ipending &= ~(1 << irq);
		if (irq == 0) {
			printf("VIRQ0");
			continue;
		}
		is = &intrsources[irq];
		pic = is->is_pic;

		splraise(is->is_mask);
		mtmsr(emsr);
		KERNEL_LOCK(1, NULL);
		ih = is->is_hand;
		while (ih) {
#ifdef DIAGNOSTIC
			if (!ih->ih_fun) {
				printf("NULL interrupt handler!\n");
				panic("irq %02d, hwirq %02d, is %p\n",
					irq, is->is_hwirq, is);
			}
#endif
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
		}
		KERNEL_UNLOCK_ONE(NULL);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;

		is->is_ev.ev_count++;
		pic->pic_reenable_irq(pic, is->is_hwirq - pic->pic_intrbase,
		    is->is_type);
	}
#ifdef MULTIPROCESSOR
	}
#endif

	if ((ci->ci_ipending & ~pcpl) & (1 << SIR_SERIAL)) {
		ci->ci_ipending &= ~(1 << SIR_SERIAL);
		splsoftserial();
		mtmsr(emsr);
		KERNEL_LOCK(1, NULL);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr__run(IPL_SOFTSERIAL);
#else
		softserial();
#endif
		KERNEL_UNLOCK_ONE(NULL);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softserial.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & (1 << SIR_NET)) {
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		int pisr;
#endif
		ci->ci_ipending &= ~(1 << SIR_NET);
		splsoftnet();
#ifndef __HAVE_GENERIC_SOFT_INTERRUPTS
		pisr = netisr;
		netisr = 0;
#endif
		mtmsr(emsr);
		KERNEL_LOCK(1, NULL);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr__run(IPL_SOFTNET);
#else
		softnet(pisr);
#endif
		KERNEL_UNLOCK_ONE(NULL);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softnet.ev_count++;
		goto again;
	}
	if ((ci->ci_ipending & ~pcpl) & (1 << SIR_CLOCK)) {
		ci->ci_ipending &= ~(1 << SIR_CLOCK);
		splsoftclock();
		mtmsr(emsr);
		KERNEL_LOCK(1, NULL);
#ifdef __HAVE_GENERIC_SOFT_INTERRUPTS
		softintr__run(IPL_SOFTCLOCK);
#else
		softclock(NULL);
#endif
		KERNEL_UNLOCK_ONE(NULL);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		ci->ci_ev_softclock.ev_count++;
		goto again;
	}

	ci->ci_cpl = pcpl;	/* Don't use splx... we are here already! */
	ci->ci_iactive = 0;
	mtmsr(emsr);
}

int
pic_handle_intr(void *cookie)
{
	struct pic_ops *pic = cookie;
	struct cpu_info *ci = curcpu();
	struct intr_source *is;
	struct intrhand *ih;
	int irq, realirq;
	int pcpl, msr, r_imen, bail;

	realirq = pic->pic_get_irq(pic);
	if (realirq == 255)
		return 0;

	msr = mfmsr();
	pcpl = ci->ci_cpl;

#ifdef MULTIPROCESSOR
	/* Only cpu0 can handle interrupts. */
	if (cpu_number() != 0) {
		int realirq;

		realirq = pic[0]->pic_get_irq(pic);
		while (realirq == IPI_VECTOR) {
			pic->pic_ack_irq(pic, realirq);
			cpuintr(NULL);
			realirq = pic->pic_get_irq(pic);
		}
		if (realirq == 255) {
			return 0;
		}
		panic("non-IPI intr %d on cpu%d", realirq, cpu_number());
	}
#endif

start:
#ifdef MULTIPROCESSOR
	while (realirq == IPI_VECTOR) {
		pic->pic_ack_irq(pic, realirq);
		cpuintr(NULL);

		realirq = pic->pic_get_irq(pic);
		if (realirq == 255) {
			return 0;
		}
	}
#endif
	//aprint_error("%s: %s realirq %d", __func__, pic->pic_name, realirq);

	irq = virq[realirq + pic->pic_intrbase];
#ifdef PIC_DEBUG
	if (irq == 0) {
		printf("%s: %d virq 0\n", pic->pic_name, realirq);
		goto boo;
	}
#endif /* PIC_DEBUG */
	KASSERT(realirq < pic->pic_numintrs);
	r_imen = 1 << irq;
	is = &intrsources[irq];

	if ((pcpl & r_imen) != 0) {

		ci->ci_ipending |= r_imen; /* Masked! Mark this as pending */
		pic->pic_disable_irq(pic, realirq);
	} else {

		/* this interrupt is no longer pending */
		ci->ci_ipending &= ~r_imen;
		
		splraise(is->is_mask);
		mtmsr(msr | PSL_EE);
		KERNEL_LOCK(1, NULL);
		ih = is->is_hand;
		bail = 0;
		while ((ih != NULL) && (bail < 10)) {
			if (ih->ih_fun == NULL)
				panic("bogus handler for IRQ %s %d",
				    pic->pic_name, realirq);
			(*ih->ih_fun)(ih->ih_arg);
			ih = ih->ih_next;
			bail++;
		}
		KERNEL_UNLOCK_ONE(NULL);
		mtmsr(msr);
		ci->ci_cpl = pcpl;
		
		uvmexp.intrs++;
		is->is_ev.ev_count++;
	}
#ifdef PIC_DEBUG
boo:
#endif /* PIC_DEBUG */
	pic->pic_ack_irq(pic, realirq);
	realirq = pic->pic_get_irq(pic);
	if (realirq != 255)
		goto start;

	mtmsr(msr | PSL_EE);
	splx(pcpl);	/* Process pendings. */
	mtmsr(msr);

	return 0;
}

void
pic_ext_intr(void)
{

	KASSERT(pics[primary_pic] != NULL);
	pic_handle_intr(pics[primary_pic]);

	return;

}

int
splraise(int ncpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio");	/* don't reorder.... */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ocpl | ncpl;
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

void
splx(int ncpl)
{

	struct cpu_info *ci = curcpu();
	__asm volatile("sync; eieio");	/* reorder protect */
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		pic_do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
}

int
spllower(int ncpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__asm volatile("sync; eieio");	/* reorder protect */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ncpl;
	if (ci->ci_ipending & ~ncpl)
		pic_do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

/* Following code should be implemented with lwarx/stwcx to avoid
 * the disable/enable. i need to read the manual once more.... */
void
softintr(int ipl)
{
	int msrsave;

	msrsave = mfmsr();
	mtmsr(msrsave & ~PSL_EE);
	curcpu()->ci_ipending |= 1 << ipl;
	mtmsr(msrsave);
}

#if defined(PIC_PREPIVR) || defined(PIC_I8259)
/*
 * isa_intr_alloc needs to be done here, because it needs direct access to
 * the various interrupt handler structures.
 */

int
genppc_isa_intr_alloc(isa_chipset_tag_t ic, struct pic_ops *pic,
    int mask, int type, int *irq_p)
{
	int irq, vi;
	int maybe_irq = -1;
	int shared_depth = 0;
	struct intr_source *is;

	if (pic == NULL)
		return 1;

	for (irq = 0; (mask != 0 && irq < pic->pic_numintrs);
	     mask >>= 1, irq++) {
		if ((mask & 1) == 0)
			continue;
		vi = virq[irq + pic->pic_intrbase];
		if (!vi) {
			*irq_p = irq;
			return 0;
		}
		is = &intrsources[vi];
		if (is->is_type == IST_NONE) {
			*irq_p = irq;
			return 0;
		}
		/* Level interrupts can be shared */
		if (type == IST_LEVEL && is->is_type == IST_LEVEL) {
			struct intrhand *ih = is->is_hand;
			int depth;

			if (maybe_irq == -1) {
				maybe_irq = irq;
				continue;
			}
			for (depth = 0; ih != NULL; ih = ih->ih_next)
				depth++;
			if (depth < shared_depth) {
				maybe_irq = irq;
				shared_depth = depth;
			}
		}
	}
	if (maybe_irq != -1) {
		*irq_p = maybe_irq;
		return 0;
	}
	return 1;
}
#endif
