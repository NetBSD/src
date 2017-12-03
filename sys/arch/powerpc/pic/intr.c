/*	$NetBSD: intr.c,v 1.23.2.1 2017/12/03 11:36:37 jdolecek Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: intr.c,v 1.23.2.1 2017/12/03 11:36:37 jdolecek Exp $");

#include "opt_interrupt.h"
#include "opt_multiprocessor.h"
#include "opt_pic.h"

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/kmem.h>
#include <sys/interrupt.h>

#include <powerpc/psl.h>
#include <powerpc/pic/picvar.h>

#if defined(PIC_I8259) || defined (PIC_PREPIVR)
#include <machine/isa_machdep.h>
#endif

#ifdef MULTIPROCESSOR
#include <powerpc/pic/ipivar.h>
#endif

#ifdef __HAVE_FAST_SOFTINTS
#include <powerpc/softint.h>
#endif

#define MAX_PICS	8	/* 8 PICs ought to be enough for everyone */

#define	PIC_VIRQ_LEGAL_P(x)	((u_int)(x) < NVIRQ)

struct pic_ops *pics[MAX_PICS];
int num_pics = 0;
int max_base = 0;
uint8_t	virq_map[NIRQ];
imask_t virq_mask = HWIRQ_MASK;
imask_t	imask[NIPL];
int	primary_pic = 0;

static int	fakeintr(void *);
static int	mapirq(int);
static void	intr_calculatemasks(void);
static struct pic_ops *find_pic_by_hwirq(int);

static struct intr_source intrsources[NVIRQ];

void
pic_init(void)
{
	/* everything is in bss, no reason to zero it. */
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

void
pic_finish_setup(void)
{
	for (size_t i = 0; i < num_pics; i++) {
		struct pic_ops * const pic = pics[i];
		if (pic->pic_finish_setup != NULL)
			pic->pic_finish_setup(pic);
	}
}

static struct pic_ops *
find_pic_by_hwirq(int hwirq)
{
	for (u_int base = 0; base < num_pics; base++) {
		struct pic_ops * const pic = pics[base];
		if (pic->pic_intrbase <= hwirq
		    && hwirq < pic->pic_intrbase + pic->pic_numintrs) {
			return pic;
		}
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
intr_establish(int hwirq, int type, int ipl, int (*ih_fun)(void *),
    void *ih_arg)
{
	return intr_establish_xname(hwirq, type, ipl, ih_fun, ih_arg, NULL);
}

void *
intr_establish_xname(int hwirq, int type, int ipl, int (*ih_fun)(void *),
    void *ih_arg, const char *xname)
{
	struct intrhand **p, *q, *ih;
	struct pic_ops *pic;
	static struct intrhand fakehand;
	int maxipl = ipl;

	if (maxipl == IPL_NONE)
		maxipl = IPL_HIGH;

	if (hwirq >= max_base) {
		panic("%s: bogus IRQ %d, max is %d", __func__, hwirq,
		    max_base - 1);
	}

	pic = find_pic_by_hwirq(hwirq);
	if (pic == NULL) {
		panic("%s: cannot find a pic for IRQ %d", __func__, hwirq);
	}

	const int virq = mapirq(hwirq);

	/* no point in sleeping unless someone can free memory. */
	ih = kmem_intr_alloc(sizeof(*ih), cold ? KM_NOSLEEP : KM_SLEEP);
	if (ih == NULL)
		panic("intr_establish: can't allocate handler info");

	if (!PIC_VIRQ_LEGAL_P(virq) || type == IST_NONE)
		panic("intr_establish: bogus irq (%d) or type (%d)",
		    hwirq, type);

	struct intr_source * const is = &intrsources[virq];

	switch (is->is_type) {
	case IST_NONE:
		is->is_type = type;
		break;
	case IST_EDGE_FALLING:
	case IST_EDGE_RISING:
	case IST_LEVEL_LOW:
	case IST_LEVEL_HIGH:
		if (type == is->is_type)
			break;
		/* FALLTHROUGH */
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
		maxipl = max(maxipl, q->ih_ipl);
	}

	/*
	 * Actually install a fake handler momentarily, since we might be doing
	 * this with interrupts enabled and don't want the real routine called
	 * until masking is set up.
	 */
	fakehand.ih_ipl = ipl;
	fakehand.ih_fun = fakeintr;
	*p = &fakehand;

	/*
	 * Poke the real handler in now.
	 */
	ih->ih_fun = ih_fun;
	ih->ih_arg = ih_arg;
	ih->ih_next = NULL;
	ih->ih_ipl = ipl;
	ih->ih_virq = virq;
	strlcpy(ih->ih_xname, xname != NULL ? xname : "unknown",
	    sizeof(ih->ih_xname));
	*p = ih;

	if (pic->pic_establish_irq != NULL)
		pic->pic_establish_irq(pic, hwirq - pic->pic_intrbase,
		    is->is_type, maxipl);

	/*
	 * Remember the highest IPL used by this handler.
	 */
	is->is_ipl = maxipl;

	/*
	 * now that the handler is established we're actually ready to
	 * calculate the masks
	 */
	intr_calculatemasks();

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
	struct intrhand * const ih = arg;
	const int virq = ih->ih_virq;
	struct intr_source * const is = &intrsources[virq];
	struct intrhand **p, **q;
	int maxipl = IPL_NONE;

	if (!PIC_VIRQ_LEGAL_P(virq))
		panic("intr_disestablish: bogus virq %d", virq);

	/*
	 * Remove the handler from the chain.
	 * This is O(n^2), too.
	 */
	for (p = &is->is_hand, q = NULL; (*p) != NULL; p = &(*p)->ih_next) {
		struct intrhand * const tmp_ih = *p;
		if (tmp_ih == ih) {
			q = p;
		} else {
			maxipl = max(maxipl, tmp_ih->ih_ipl);
		}
	}
	if (q)
		*q = ih->ih_next;
	else
		panic("intr_disestablish: handler not registered");
	kmem_intr_free((void *)ih, sizeof(*ih));

	/*
	 * Reset the IPL for this source now that we've removed a handler.
	 */
	is->is_ipl = maxipl;

	intr_calculatemasks();

	if (is->is_hand == NULL) {
		is->is_type = IST_NONE;
		evcnt_detach(&is->is_ev);
		/*
		 * Make the virutal IRQ available again.
		 */
		virq_map[virq] = 0;
		virq_mask |= PIC_VIRQ_TO_MASK(virq);
	}
}

/*
 * Map max_base irqs into 32 (bits).
 */
static int
mapirq(int hwirq)
{
	struct pic_ops *pic;

	if (hwirq >= max_base)
		panic("invalid irq %d", hwirq);

	if ((pic = find_pic_by_hwirq(hwirq)) == NULL)
		panic("%s: cannot find PIC for HWIRQ %d", __func__, hwirq);

	if (virq_map[hwirq])
		return virq_map[hwirq];

	if (virq_mask == 0)
		panic("virq overflow");

	const int virq = PIC_VIRQ_MS_PENDING(virq_mask);
	struct intr_source * const is = intrsources + virq;

	virq_mask &= ~PIC_VIRQ_TO_MASK(virq);

	is->is_hwirq = hwirq;
	is->is_pic = pic;
	virq_map[hwirq] = virq;
#ifdef PIC_DEBUG
	printf("mapping hwirq %d to virq %d\n", hwirq, virq);
#endif
	return virq;
}

static const char * const intr_typenames[] = {
   [IST_NONE]  = "none",
   [IST_PULSE] = "pulsed",
   [IST_EDGE_FALLING]  = "falling edge triggered",
   [IST_EDGE_RISING]  = "rising edge triggered",
   [IST_LEVEL_LOW] = "low level triggered",
   [IST_LEVEL_HIGH] = "high level triggered",
};

const char *
intr_typename(int type)
{
	KASSERT((unsigned int) type < __arraycount(intr_typenames));
	KASSERT(intr_typenames[type] != NULL);
	return intr_typenames[type];
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
	imask_t newmask[NIPL];
	struct intr_source *is;
	struct intrhand *ih;
	int irq;

	for (u_int ipl = IPL_NONE; ipl < NIPL; ipl++) {
		newmask[ipl] = 0;
	}

	/* First, figure out which ipl each IRQ uses. */
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		for (ih = is->is_hand; ih != NULL; ih = ih->ih_next) {
			newmask[ih->ih_ipl] |= PIC_VIRQ_TO_MASK(irq);
		}
	}

	/*
	 * IPL_NONE is used for hardware interrupts that are never blocked,
	 * and do not block anything else.
	 */
	newmask[IPL_NONE] = 0;

	/*
	 * strict hierarchy - all IPLs block everything blocked by any lower
	 * IPL
	 */
	for (u_int ipl = 1; ipl < NIPL; ipl++) {
		newmask[ipl] |= newmask[ipl - 1];
	}

#ifdef PIC_DEBUG
	for (u_int ipl = 0; ipl < NIPL; ipl++) {
		printf("%u: %08x -> %08x\n", ipl, imask[ipl], newmask[ipl]);
	}
#endif

	/*
	 * Disable all interrupts.
	 */
	for (u_int base = 0; base < num_pics; base++) {
		struct pic_ops * const pic = pics[base];
		for (u_int i = 0; i < pic->pic_numintrs; i++) {
			pic->pic_disable_irq(pic, i);
		}
	}

	/*
	 * Now that all interrupts are disabled, update the ipl masks.
	 */
	for (u_int ipl = 0; ipl < NIPL; ipl++) {
		imask[ipl] = newmask[ipl];
	}

	/*
	 * Lastly, enable IRQs actually in use.
	 */
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		if (is->is_hand)
			pic_enable_irq(is->is_hwirq);
	}
}

void
pic_enable_irq(int hwirq)
{
	struct pic_ops * const pic = find_pic_by_hwirq(hwirq);
	if (pic == NULL)
		panic("%s: bogus IRQ %d", __func__, hwirq);
	const int type = intrsources[virq_map[hwirq]].is_type;
	(*pic->pic_enable_irq)(pic, hwirq - pic->pic_intrbase, type);
}

void
pic_mark_pending(int hwirq)
{
	struct cpu_info * const ci = curcpu();

	const int virq = virq_map[hwirq];
	if (virq == 0)
		printf("IRQ %d maps to 0\n", hwirq);

	const register_t msr = mfmsr();
	mtmsr(msr & ~PSL_EE);
	ci->ci_ipending |= PIC_VIRQ_TO_MASK(virq);
	mtmsr(msr);
}

static void
intr_deliver(struct intr_source *is, int virq)
{
	bool locked = false;
	for (struct intrhand *ih = is->is_hand; ih != NULL; ih = ih->ih_next) {
		KASSERTMSG(ih->ih_fun != NULL,
		    "%s: irq %d, hwirq %d, is %p ih %p: "
		     "NULL interrupt handler!\n", __func__,
		     virq, is->is_hwirq, is, ih);
		if (ih->ih_ipl == IPL_VM) {
			if (!locked) {
				KERNEL_LOCK(1, NULL);
				locked = true;
			}
		} else if (locked) {
			KERNEL_UNLOCK_ONE(NULL);
			locked = false;
		}
		(*ih->ih_fun)(ih->ih_arg);
	}
	if (locked) {
		KERNEL_UNLOCK_ONE(NULL);
	}
	is->is_ev.ev_count++;
}

void
pic_do_pending_int(void)
{
	struct cpu_info * const ci = curcpu();
	imask_t vpend;

	if (ci->ci_iactive)
		return;

	ci->ci_iactive = 1;

	const register_t emsr = mfmsr();
	const register_t dmsr = emsr & ~PSL_EE;

	KASSERT(emsr & PSL_EE);
	mtmsr(dmsr);

	const int pcpl = ci->ci_cpl;
#ifdef __HAVE_FAST_SOFTINTS
again:
#endif

	/* Do now unmasked pendings */
	while ((vpend = (ci->ci_ipending & ~imask[pcpl])) != 0) {
		ci->ci_idepth++;
		KASSERT((PIC_VIRQ_TO_MASK(0) & ci->ci_ipending) == 0);

		/* Get most significant pending bit */
		const int virq = PIC_VIRQ_MS_PENDING(vpend);
		ci->ci_ipending &= ~PIC_VIRQ_TO_MASK(virq);

		struct intr_source * const is = &intrsources[virq];
		struct pic_ops * const pic = is->is_pic;

		splraise(is->is_ipl);
		mtmsr(emsr);
		intr_deliver(is, virq);
		mtmsr(dmsr);
		ci->ci_cpl = pcpl; /* Don't use splx... we are here already! */

		pic->pic_reenable_irq(pic, is->is_hwirq - pic->pic_intrbase,
		    is->is_type);
		ci->ci_idepth--;
	}

#ifdef __HAVE_FAST_SOFTINTS
	const u_int softints = ci->ci_data.cpu_softints &
				 (IPL_SOFTMASK << pcpl);

	/* make sure there are no bits to screw with the line above */
	KASSERT((ci->ci_data.cpu_softints & ~IPL_SOFTMASK) == 0);

	if (__predict_false(softints != 0)) {
		ci->ci_cpl = IPL_HIGH;
		mtmsr(emsr);
		powerpc_softint(ci, pcpl,
		    (vaddr_t)__builtin_return_address(0));
		mtmsr(dmsr);
		ci->ci_cpl = pcpl;
		if (__predict_false(ci->ci_ipending & ~imask[pcpl]))
			goto again;
	}
#endif

	ci->ci_iactive = 0;
	mtmsr(emsr);
}

int
pic_handle_intr(void *cookie)
{
	struct pic_ops *pic = cookie;
	struct cpu_info *ci = curcpu();
	int picirq;

	picirq = pic->pic_get_irq(pic, PIC_GET_IRQ);
	if (picirq == 255)
		return 0;

	const register_t msr = mfmsr();
	const int pcpl = ci->ci_cpl;

	do {
		const int virq = virq_map[picirq + pic->pic_intrbase];

		KASSERT(virq != 0);
		KASSERT(picirq < pic->pic_numintrs);
		imask_t v_imen = PIC_VIRQ_TO_MASK(virq);
		struct intr_source * const is = &intrsources[virq];

		if ((imask[pcpl] & v_imen) != 0) {
			ci->ci_ipending |= v_imen; /* Masked! Mark this as pending */
			pic->pic_disable_irq(pic, picirq);
		} else {
			/* this interrupt is no longer pending */
			ci->ci_ipending &= ~v_imen;
			ci->ci_idepth++;

			splraise(is->is_ipl);
			mtmsr(msr | PSL_EE);
			intr_deliver(is, virq);
			mtmsr(msr);
			ci->ci_cpl = pcpl;

			ci->ci_data.cpu_nintr++;
			ci->ci_idepth--;
		}
		pic->pic_ack_irq(pic, picirq);
	} while ((picirq = pic->pic_get_irq(pic, PIC_GET_RECHECK)) != 255);

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

	if (ncpl == ci->ci_cpl) return ncpl;
	__asm volatile("sync; eieio");	/* don't reorder.... */
	ocpl = ci->ci_cpl;
	KASSERT(ncpl < NIPL);
	ci->ci_cpl = max(ncpl, ocpl);
	__asm volatile("sync; eieio");	/* reorder protect */
	__insn_barrier();
	return ocpl;
}

static inline bool
have_pending_intr_p(struct cpu_info *ci, int ncpl)
{
	if (ci->ci_ipending & ~imask[ncpl])
		return true;
#ifdef __HAVE_FAST_SOFTINTS
	if (ci->ci_data.cpu_softints & (IPL_SOFTMASK << ncpl))
		return true;
#endif
	return false;
}

void
splx(int ncpl)
{
	struct cpu_info *ci = curcpu();

	__insn_barrier();
	__asm volatile("sync; eieio");	/* reorder protect */
	ci->ci_cpl = ncpl;
	if (have_pending_intr_p(ci, ncpl))
		pic_do_pending_int();

	__asm volatile("sync; eieio");	/* reorder protect */
}

int
spllower(int ncpl)
{
	struct cpu_info *ci = curcpu();
	int ocpl;

	__insn_barrier();
	__asm volatile("sync; eieio");	/* reorder protect */
	ocpl = ci->ci_cpl;
	ci->ci_cpl = ncpl;
	if (have_pending_intr_p(ci, ncpl))
		pic_do_pending_int();
	__asm volatile("sync; eieio");	/* reorder protect */
	return ocpl;
}

void
genppc_cpu_configure(void)
{
	aprint_normal("vmmask %x schedmask %x highmask %x\n",
	    (u_int)imask[IPL_VM] & 0x7fffffff,
	    (u_int)imask[IPL_SCHED] & 0x7fffffff,
	    (u_int)imask[IPL_HIGH] & 0x7fffffff);

	spl0();
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
		vi = virq_map[irq + pic->pic_intrbase];
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

static struct intr_source *
intr_get_source(const char *intrid)
{
	struct intr_source *is;
	int irq;

	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		if (strcmp(intrid, is->is_source) == 0)
			return is;
	}
	return NULL;
}

static struct intrhand *
intr_get_handler(const char *intrid)
{
	struct intr_source *is;

	is = intr_get_source(intrid);
	if (is != NULL)
		return is->is_hand;
	return NULL;
}

uint64_t
interrupt_get_count(const char *intrid, u_int cpu_idx)
{
	struct intr_source *is;

	/* XXX interrupt is always generated by CPU 0 */
	if (cpu_idx != 0)
		return 0;

	is = intr_get_source(intrid);
	if (is != NULL)
		return is->is_ev.ev_count;
	return 0;
}

void
interrupt_get_assigned(const char *intrid, kcpuset_t *cpuset)
{
	struct intr_source *is;

	kcpuset_zero(cpuset);

	is = intr_get_source(intrid);
	if (is != NULL)
		kcpuset_set(cpuset, 0);	/* XXX */
}

void
interrupt_get_available(kcpuset_t *cpuset)
{
	CPU_INFO_ITERATOR cii;
	struct cpu_info *ci;

	kcpuset_zero(cpuset);

	mutex_enter(&cpu_lock);
	for (CPU_INFO_FOREACH(cii, ci)) {
		if ((ci->ci_schedstate.spc_flags & SPCF_NOINTR) == 0)
			kcpuset_set(cpuset, cpu_index(ci));
	}
	mutex_exit(&cpu_lock);
}

void
interrupt_get_devname(const char *intrid, char *buf, size_t len)
{
	struct intrhand *ih;

	if (len == 0)
		return;

	buf[0] = '\0';

	for (ih = intr_get_handler(intrid); ih != NULL; ih = ih->ih_next) {
		if (buf[0] != '\0')
			strlcat(buf, ", ", len);
		strlcat(buf, ih->ih_xname, len);
	}
}

struct intrids_handler *
interrupt_construct_intrids(const kcpuset_t *cpuset)
{
	struct intr_source *is;
	struct intrids_handler *ii_handler;
	intrid_t *ids;
	int i, irq, count;

	if (kcpuset_iszero(cpuset))
		return NULL;
	if (!kcpuset_isset(cpuset, 0))	/* XXX */
		return NULL;

	count = 0;
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		if (is->is_hand != NULL)
			count++;
	}

	ii_handler = kmem_zalloc(sizeof(int) + sizeof(intrid_t) * count,
	    KM_SLEEP);
	if (ii_handler == NULL)
		return NULL;
	ii_handler->iih_nids = count;
	if (count == 0)
		return ii_handler;

	ids = ii_handler->iih_intrids;
	i = 0;
	for (irq = 0, is = intrsources; irq < NVIRQ; irq++, is++) {
		/* Ignore devices attached after counting "count". */
		if (i >= count)
			break;

		if (is->is_hand == NULL)
			continue;

		strncpy(ids[i], is->is_source, sizeof(intrid_t));
		i++;
	}

	return ii_handler;
}

void
interrupt_destruct_intrids(struct intrids_handler *ii_handler)
{
	size_t iih_size;

	if (ii_handler == NULL)
		return;

	iih_size = sizeof(int) + sizeof(intrid_t) * ii_handler->iih_nids;
	kmem_free(ii_handler, iih_size);
}

int
interrupt_distribute(void *ich, const kcpuset_t *newset, kcpuset_t *oldset)
{
	return EOPNOTSUPP;
}

int
interrupt_distribute_handler(const char *intrid, const kcpuset_t *newset,
    kcpuset_t *oldset)
{
	return EOPNOTSUPP;
}
