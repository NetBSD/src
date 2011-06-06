/*	$NetBSD: pic.c,v 1.6.2.1 2011/06/06 09:05:06 jruoho Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas.
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
__KERNEL_RCSID(0, "$NetBSD: pic.c,v 1.6.2.1 2011/06/06 09:05:06 jruoho Exp $");

#define _INTR_PRIVATE
#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/atomic.h>
#include <sys/malloc.h>
#include <sys/mallocvar.h>
#include <sys/atomic.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

#include <arm/pic/picvar.h>

MALLOC_DEFINE(M_INTRSOURCE, "intrsource", "interrupt source");

static uint32_t
	pic_find_pending_irqs_by_ipl(struct pic_softc *, size_t, uint32_t, int);
static struct pic_softc *
	pic_list_find_pic_by_pending_ipl(uint32_t);
static void
	pic_deliver_irqs(struct pic_softc *, int, void *);
static void
	pic_list_deliver_irqs(register_t, int, void *);

struct pic_softc *pic_list[PIC_MAXPICS];
#if PIC_MAXPICS > 32
#error PIC_MAXPICS > 32 not supported
#endif
volatile uint32_t pic_blocked_pics;
volatile uint32_t pic_pending_pics;
volatile uint32_t pic_pending_ipls;
struct intrsource *pic_sources[PIC_MAXMAXSOURCES];
struct intrsource *pic__iplsources[PIC_MAXMAXSOURCES];
struct intrsource **pic_iplsource[NIPL] = {
	[0 ... NIPL-1] = pic__iplsources,
};
size_t pic_ipl_offset[NIPL+1];
size_t pic_sourcebase;
static struct evcnt pic_deferral_ev = 
    EVCNT_INITIALIZER(EVCNT_TYPE_MISC, NULL, "deferred", "intr");
EVCNT_ATTACH_STATIC(pic_deferral_ev);



int
pic_handle_intr(void *arg)
{
	struct pic_softc * const pic = arg;
	int rv;

	rv = (*pic->pic_ops->pic_find_pending_irqs)(pic);

	return rv > 0;
}

void
pic_mark_pending_source(struct pic_softc *pic, struct intrsource *is)
{
	const uint32_t ipl_mask = __BIT(is->is_ipl);

	atomic_or_32(&pic->pic_pending_irqs[is->is_irq >> 5],
	    __BIT(is->is_irq & 0x1f));

	atomic_or_32(&pic->pic_pending_ipls, ipl_mask);
	atomic_or_32(&pic_pending_ipls, ipl_mask);
	atomic_or_32(&pic_pending_pics, __BIT(pic->pic_id));
}

void
pic_mark_pending(struct pic_softc *pic, int irq)
{
	struct intrsource * const is = pic->pic_sources[irq];

	KASSERT(irq < pic->pic_maxsources);
	KASSERT(is != NULL);

	pic_mark_pending_source(pic, is);
}

uint32_t
pic_mark_pending_sources(struct pic_softc *pic, size_t irq_base,
	uint32_t pending)
{
	struct intrsource ** const isbase = &pic->pic_sources[irq_base];
	struct intrsource *is;
	volatile uint32_t *ipending = &pic->pic_pending_irqs[irq_base >> 5];
	uint32_t ipl_mask = 0;

	if (pending == 0)
		return ipl_mask;

	KASSERT((irq_base & 31) == 0);
	
	(*pic->pic_ops->pic_block_irqs)(pic, irq_base, pending);

	atomic_or_32(ipending, pending);
        while (pending != 0) {
		int n = ffs(pending);
		if (n-- == 0)
			break;
		is = isbase[n];
		KASSERT(is != NULL);
		KASSERT(irq_base <= is->is_irq && is->is_irq < irq_base + 32);
		pending &= ~__BIT(n);
		ipl_mask |= __BIT(is->is_ipl);
	}

	atomic_or_32(&pic->pic_pending_ipls, ipl_mask);
	atomic_or_32(&pic_pending_ipls, ipl_mask);
	atomic_or_32(&pic_pending_pics, __BIT(pic->pic_id));

	return ipl_mask;
}

uint32_t
pic_find_pending_irqs_by_ipl(struct pic_softc *pic, size_t irq_base,
	uint32_t pending, int ipl)
{
	uint32_t ipl_irq_mask = 0;
	uint32_t irq_mask;

	for (;;) {
		int irq = ffs(pending);
		if (irq-- == 0)
			return ipl_irq_mask;

		irq_mask = __BIT(irq);
#if 1
    		KASSERT(pic->pic_sources[irq_base + irq] != NULL);
#else
		if (pic->pic_sources[irq_base + irq] == NULL) {
			aprint_error("stray interrupt? irq_base=%zu irq=%d\n",
			    irq_base, irq);
		} else
#endif
		if (pic->pic_sources[irq_base + irq]->is_ipl == ipl)
			ipl_irq_mask |= irq_mask;

		pending &= ~irq_mask;
	}
}

void
pic_dispatch(struct intrsource *is, void *frame)
{
	int rv;

	if (__predict_false(is->is_arg == NULL)
	    && __predict_true(frame != NULL)) {
		rv = (*is->is_func)(frame);
	} else if (__predict_true(is->is_arg != NULL)) {
		rv = (*is->is_func)(is->is_arg);
	} else {
		pic_deferral_ev.ev_count++;
		return;
	}
	is->is_ev.ev_count++;
}

void
pic_deliver_irqs(struct pic_softc *pic, int ipl, void *frame)
{
	const uint32_t ipl_mask = __BIT(ipl);
	struct intrsource *is;
	volatile uint32_t *ipending = pic->pic_pending_irqs;
	volatile uint32_t *iblocked = pic->pic_blocked_irqs;
	size_t irq_base;
#if PIC_MAXSOURCES > 32
	size_t irq_count;
	int poi = 0;		/* Possibility of interrupting */
#endif
	uint32_t pending_irqs;
	uint32_t blocked_irqs;
	int irq;
	bool progress = false;
	
	KASSERT(pic->pic_pending_ipls & ipl_mask);

	irq_base = 0;
#if PIC_MAXSOURCES > 32
	irq_count = 0;
#endif

	for (;;) {
		pending_irqs = pic_find_pending_irqs_by_ipl(pic, irq_base,
		    *ipending, ipl);
		KASSERT((pending_irqs & *ipending) == pending_irqs);
		KASSERT((pending_irqs & ~(*ipending)) == 0);
		if (pending_irqs == 0) {
#if PIC_MAXSOURCES > 32
			irq_count += 32;
			if (__predict_true(irq_count >= pic->pic_maxsources)) {
				if (!poi)
					/*Interrupt at this level was handled.*/
					break;
				irq_base = 0;
				irq_count = 0;
				poi = 0;
				ipending = pic->pic_pending_irqs;
				iblocked = pic->pic_blocked_irqs;
			} else {
				irq_base += 32;
				ipending++;
				iblocked++;
				KASSERT(irq_base <= pic->pic_maxsources);
			}
			continue;
#else
			break;
#endif
		}
		progress = true;
		blocked_irqs = 0;
		do {
			irq = ffs(pending_irqs) - 1;
			KASSERT(irq >= 0);

			atomic_and_32(ipending, ~__BIT(irq));
			is = pic->pic_sources[irq_base + irq];
			if (is != NULL) {
				cpsie(I32_bit);
				pic_dispatch(is, frame);
				cpsid(I32_bit);
#if PIC_MAXSOURCES > 32
				/*
				 * There is a possibility of interrupting
				 * from cpsie() to cpsid().
				 */
				poi = 1;
#endif
				blocked_irqs |= __BIT(irq);
			} else {
				KASSERT(0);
			}
			pending_irqs = pic_find_pending_irqs_by_ipl(pic,
			    irq_base, *ipending, ipl);
		} while (pending_irqs);
		if (blocked_irqs) {
			atomic_or_32(iblocked, blocked_irqs);
			atomic_or_32(&pic_blocked_pics, __BIT(pic->pic_id));
		}
	}

	KASSERT(progress);
	/*
	 * Since interrupts are disabled, we don't have to be too careful
	 * about these.
	 */
	if (atomic_and_32_nv(&pic->pic_pending_ipls, ~ipl_mask) == 0)
		atomic_and_32(&pic_pending_pics, ~__BIT(pic->pic_id));
}

static void
pic_list_unblock_irqs(void)
{
	uint32_t blocked_pics = pic_blocked_pics;

	pic_blocked_pics = 0;
	for (;;) {
		struct pic_softc *pic;
#if PIC_MAXSOURCES > 32
		volatile uint32_t *iblocked;
		uint32_t blocked;
		size_t irq_base;
#endif

		int pic_id = ffs(blocked_pics);
		if (pic_id-- == 0)
			return;

		pic = pic_list[pic_id];
		KASSERT(pic != NULL);
#if PIC_MAXSOURCES > 32
		for (irq_base = 0, iblocked = pic->pic_blocked_irqs;
		     irq_base < pic->pic_maxsources;
		     irq_base += 32, iblocked++) {
			if ((blocked = *iblocked) != 0) {
				(*pic->pic_ops->pic_unblock_irqs)(pic,
				    irq_base, blocked);
				atomic_and_32(iblocked, ~blocked);
			}
		}
#else
		KASSERT(pic->pic_blocked_irqs[0] != 0);
		(*pic->pic_ops->pic_unblock_irqs)(pic,
		    0, pic->pic_blocked_irqs[0]);
		pic->pic_blocked_irqs[0] = 0;
#endif
		blocked_pics &= ~__BIT(pic_id);
	}
}


struct pic_softc *
pic_list_find_pic_by_pending_ipl(uint32_t ipl_mask)
{
	uint32_t pending_pics = pic_pending_pics;
	struct pic_softc *pic;

	for (;;) {
		int pic_id = ffs(pending_pics);
		if (pic_id-- == 0)
			return NULL;

		pic = pic_list[pic_id];
		KASSERT(pic != NULL);
		if (pic->pic_pending_ipls & ipl_mask)
			return pic;
		pending_pics &= ~__BIT(pic_id);
	}
}

void
pic_list_deliver_irqs(register_t psw, int ipl, void *frame)
{
	const uint32_t ipl_mask = __BIT(ipl);
	struct pic_softc *pic;

	while ((pic = pic_list_find_pic_by_pending_ipl(ipl_mask)) != NULL) {
		pic_deliver_irqs(pic, ipl, frame);
		KASSERT((pic->pic_pending_ipls & ipl_mask) == 0);
	}
	atomic_and_32(&pic_pending_ipls, ~ipl_mask);
}

void
pic_do_pending_ints(register_t psw, int newipl, void *frame)
{
	struct cpu_info * const ci = curcpu();
	if (__predict_false(newipl == IPL_HIGH))
		return;
	while ((pic_pending_ipls & ~__BIT(newipl)) > __BIT(newipl)) {
		KASSERT(pic_pending_ipls < __BIT(NIPL));
		for (;;) {
			int ipl = 31 - __builtin_clz(pic_pending_ipls);
			KASSERT(ipl < NIPL);
			if (ipl <= newipl)
				break;

			ci->ci_cpl = ipl;
			pic_list_deliver_irqs(psw, ipl, frame);
			pic_list_unblock_irqs();
		}
	}
	if (ci->ci_cpl != newipl)
		ci->ci_cpl = newipl;
#ifdef __HAVE_FAST_SOFTINTS
	cpu_dosoftints();
#endif
}

void
pic_add(struct pic_softc *pic, int irqbase)
{
	int slot, maybe_slot = -1;

	for (slot = 0; slot < PIC_MAXPICS; slot++) {
		struct pic_softc * const xpic = pic_list[slot];
		if (xpic == NULL) {
			if (maybe_slot < 0)
				maybe_slot = slot;
			if (irqbase < 0)
				break;
			continue;
		}
		if (irqbase < 0 || xpic->pic_irqbase < 0)
			continue;
		if (irqbase >= xpic->pic_irqbase + xpic->pic_maxsources)
			continue;
		if (irqbase + pic->pic_maxsources <= xpic->pic_irqbase)
			continue;
		panic("pic_add: pic %s (%zu sources @ irq %u) conflicts"
		    " with pic %s (%zu sources @ irq %u)",
		    pic->pic_name, pic->pic_maxsources, irqbase,
		    xpic->pic_name, xpic->pic_maxsources, xpic->pic_irqbase);
	}
	slot = maybe_slot;
#if 0
	printf("%s: pic_sourcebase=%zu pic_maxsources=%zu\n",
	    pic->pic_name, pic_sourcebase, pic->pic_maxsources);
#endif
	KASSERT(pic_sourcebase + pic->pic_maxsources <= PIC_MAXMAXSOURCES);

	pic->pic_sources = &pic_sources[pic_sourcebase];
	pic->pic_irqbase = irqbase;
	pic_sourcebase += pic->pic_maxsources;
	pic->pic_id = slot;
	pic_list[slot] = pic;
}

int
pic_alloc_irq(struct pic_softc *pic)
{
	int irq;

	for (irq = 0; irq < pic->pic_maxsources; irq++) {
		if (pic->pic_sources[irq] == NULL)
			return irq;
	}

	return -1;
}

void *
pic_establish_intr(struct pic_softc *pic, int irq, int ipl, int type,
	int (*func)(void *), void *arg)
{
	struct intrsource *is;
	int off, nipl;

	if (pic->pic_sources[irq]) {
		printf("pic_establish_intr: pic %s irq %d already present\n",
		    pic->pic_name, irq);
		return NULL;
	}

	is = malloc(sizeof(*is), M_INTRSOURCE, M_NOWAIT|M_ZERO);
	if (is == NULL)
		return NULL;

	is->is_pic = pic;
	is->is_irq = irq;
	is->is_ipl = ipl;
	is->is_type = type;
	is->is_func = func;
	is->is_arg = arg;
	
	if (pic->pic_ops->pic_source_name)
		(*pic->pic_ops->pic_source_name)(pic, irq, is->is_source,
		    sizeof(is->is_source));
	else
		snprintf(is->is_source, sizeof(is->is_source), "irq %d", irq);

	evcnt_attach_dynamic(&is->is_ev, EVCNT_TYPE_INTR, NULL,
	    pic->pic_name, is->is_source);

	pic->pic_sources[irq] = is;

	/*
	 * First try to use an existing slot which is empty.
	 */
	for (off = pic_ipl_offset[ipl]; off < pic_ipl_offset[ipl+1]; off++) {
		if (pic__iplsources[off] == NULL) {
			is->is_iplidx = off - pic_ipl_offset[ipl];
			pic__iplsources[off] = is;
			return is;
		}
	}

	/*
	 * Move up all the sources by one.
 	 */
	if (ipl < NIPL) {
		off = pic_ipl_offset[ipl+1];
		memmove(&pic__iplsources[off+1], &pic__iplsources[off],
		    sizeof(pic__iplsources[0]) * (pic_ipl_offset[NIPL] - off));
	}

	/*
	 * Advance the offset of all IPLs higher than this.  Include an
	 * extra one as well.  Thus the number of sources per ipl is
	 * pic_ipl_offset[ipl+1] - pic_ipl_offset[ipl].
	 */
	for (nipl = ipl + 1; nipl <= NIPL; nipl++)
		pic_ipl_offset[nipl]++;

	/*
	 * Insert into the previously made position at the end of this IPL's
	 * sources.
	 */
	off = pic_ipl_offset[ipl + 1] - 1;
	is->is_iplidx = off - pic_ipl_offset[ipl];
	pic__iplsources[off] = is;

	(*pic->pic_ops->pic_establish_irq)(pic, is);

	(*pic->pic_ops->pic_unblock_irqs)(pic, is->is_irq & ~0x1f,
	    __BIT(is->is_irq & 0x1f));
	
	/* We're done. */
	return is;
}

void
pic_disestablish_source(struct intrsource *is)
{
	struct pic_softc * const pic = is->is_pic;
	const int irq = is->is_irq;

	(*pic->pic_ops->pic_block_irqs)(pic, irq & ~31, __BIT(irq));
	pic->pic_sources[irq] = NULL;
	pic__iplsources[pic_ipl_offset[is->is_ipl] + is->is_iplidx] = NULL;
	evcnt_detach(&is->is_ev);

	free(is, M_INTRSOURCE);
}

void *
intr_establish(int irq, int ipl, int type, int (*func)(void *), void *arg)
{
	int slot;

	for (slot = 0; slot < PIC_MAXPICS; slot++) {
		struct pic_softc * const pic = pic_list[slot];
		if (pic == NULL || pic->pic_irqbase < 0)
			continue;
		if (pic->pic_irqbase <= irq
		    && irq < pic->pic_irqbase + pic->pic_maxsources) {
			return pic_establish_intr(pic, irq - pic->pic_irqbase,
			    ipl, type, func, arg);
		}
	}

	return NULL;
}

void
intr_disestablish(void *ih)
{
	struct intrsource * const is = ih;
	pic_disestablish_source(is);
}
