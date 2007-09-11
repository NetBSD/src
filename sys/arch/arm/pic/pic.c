/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pic.c,v 1.1.2.3 2007/09/11 02:31:12 matt Exp $");

#define _INTR_PRIVATE
#include <sys/param.h>
#include <sys/evcnt.h>
#include <sys/malloc.h>
#include <sys/mallocvar.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

#include <arm/pic/picvar.h>

/* should be in atomic.h */
uint32_t atomic_or_32(volatile uint32_t *, uint32_t);
uint32_t atomic_and_32(volatile uint32_t *, uint32_t);
uint32_t atomic_nand_32(volatile uint32_t *, uint32_t);
uint32_t atomic_or_32_nv(volatile uint32_t *, uint32_t);
uint32_t atomic_and_32_nv(volatile uint32_t *, uint32_t);
uint32_t atomic_nand_32_nv(volatile uint32_t *, uint32_t);
uint32_t atomic_inc_32_nv(volatile uint32_t *);
uint32_t atomic_dec_32_nv(volatile uint32_t *);

MALLOC_DEFINE(M_INTRSOURCE, "intrsource", "interrupt source");

static uint32_t
	pic_find_pending_irqs_by_ipl(struct pic_softc *, uint32_t, int);
static struct pic_softc *
	pic_list_find_pic_by_pending_ipl(size_t, uint32_t);
static void
	pic_deliver_irqs(struct pic_softc *, register_t, int, void *);
static void
	pic_list_deliver_irqs(register_t, int, void *);

struct pic_softc *pic_list[PIC_MAXPICS];
volatile uint32_t pic_pending_pics[(PIC_MAXPICS + 31) / 32];
volatile uint32_t pic_pending_ipls;
volatile uint32_t pic_pending_ipl_refcnt[NIPL];
struct intrsource *pic_sources[PIC_MAXMAXSOURCES];
struct intrsource *pic__iplsources[PIC_MAXMAXSOURCES];
struct intrsource **pic_iplsource[NIPL] = {
	[0 ... NIPL-1] = pic__iplsources,
};
size_t pic_ipl_offset[NIPL+1];
size_t pic_sourcebase;



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

	atomic_or_32(&pic->pic_pending_irqs[is->is_iplidx >> 5],
	    __BIT(is->is_iplidx & 0x1f));

	if ((atomic_or_32(&pic->pic_pending_ipls, ipl_mask) & ipl_mask) == 0)
		if ((atomic_or_32(&pic_pending_ipls, ipl_mask) & ipl_mask) == 0)
			atomic_inc_32_nv(&pic_pending_ipl_refcnt[is->is_ipl]);

	atomic_or_32(&pic_pending_pics[pic->pic_id >> 5], __BIT(pic->pic_id));
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
pic_find_pending_irqs_by_ipl(struct pic_softc *pic, uint32_t pending, int ipl)
{
	uint32_t ipl_irq_mask = 0;
	uint32_t irq_mask;

	for (;;) {
		int irq = 31 - __builtin_clz(pending);
		if (irq < 0)
			return ipl_irq_mask;

		irq_mask = __BIT(irq);
		KASSERT(pic->pic_sources[irq] != NULL);
		if (pic->pic_sources[irq]->is_ipl == ipl)
			ipl_irq_mask |= irq_mask;

		pending &= ~irq_mask;
	}
}

void
pic_dispatch(struct intrsource *is, void *frame)
{
	int rv;

	if (__predict_false(frame != NULL && is->is_arg == NULL)) {
		rv = (*is->is_func)(frame);
	} else {
		rv = (*is->is_func)(is->is_arg);
	}
	is->is_ev.ev_count++;
}

void
pic_deliver_irqs(struct pic_softc *pic, register_t psw, int ipl, void *frame)
{
	const uint32_t ipl_mask = __BIT(ipl);
	struct intrsource *is;
	uint32_t *ipending = pic->pic_pending_irqs;
	size_t irq_base;
	size_t irq_count;
	uint32_t pending_irqs;
	uint32_t blocked_irqs;
	int irq;
	
	KASSERT(pic->pic_pending_ipls & ipl_mask);

	irq_base = 0;
	irq_count = 0;

	for (;;) {
		pending_irqs = pic_find_pending_irqs_by_ipl(pic, ipl,
		    *ipending);
		if (pending_irqs == 0) {
#if PIC_MAXPICS > 32
			irq_count += 32;
			if (__predict_true(irq_count >= pic->pic_maxsources))
				break;
			irq_base += 32;
			ipending++;
			if (irq_base > pic->pic_maxsources)
				ipending = pic->pic_pending_irqs;
			continue;
#else
			break;
#endif
		}
		blocked_irqs = pending_irqs;
		do {
			irq = 31 - __builtin_clz(pending_irqs);
			KASSERT(irq >= 0);

			atomic_nand_32(ipending, __BIT(irq));
			is = pic->pic_sources[irq_base + irq];
			if (is != NULL) {
				if ((psw & I32_bit) == 0)
					cpsie(I32_bit);
				pic_dispatch(is, frame);
				cpsid(I32_bit);
			} else {
				blocked_irqs &= ~__BIT(irq);
			}
			pending_irqs = pic_find_pending_irqs_by_ipl(pic, ipl,
			    *ipending);
		} while (pending_irqs);
		if (blocked_irqs)
			(*pic->pic_ops->pic_unblock_irqs)(pic,
			    irq_base, blocked_irqs);
	}

	/*
	 * Since interrupts are disabled, we don't have to be too careful
	 * about these.
	 */
	if (atomic_nand_32_nv(&pic->pic_pending_ipls, ipl_mask) == 0)
		atomic_nand_32(&pic_pending_pics[pic->pic_id >> 5],
		    __BIT(pic->pic_id & 0x1f));
	if (atomic_dec_32_nv(&pic_pending_ipl_refcnt[ipl]) == 0)
		atomic_nand_32(&pic_pending_ipls, ipl_mask);
}

struct pic_softc *
pic_list_find_pic_by_pending_ipl(size_t pic_base, uint32_t ipl_mask)
{
	uint32_t pending = pic_pending_pics[pic_base >> 5];
	struct pic_softc *pic;

	for (;;) {
		int pic_id = 31 - __builtin_clz(pending);
		if (pic_id < 0)
			return NULL;

		pic = pic_list[pic_base + pic_id];
		KASSERT(pic != NULL);
		if (pic->pic_pending_ipls & ipl_mask)
			return pic;
		pending &= ~ipl_mask;
	}
}

void
pic_list_deliver_irqs(register_t psw, int ipl, void *frame)
{	
	const uint32_t ipl_mask = __BIT(ipl);
	struct pic_softc *pic;
	size_t pic_base = 0;
	size_t pic_count = 0;
	
	for (;;) {
		pic = pic_list_find_pic_by_pending_ipl(pic_base, ipl_mask);
		if (pic != NULL) {
			pic_deliver_irqs(pic, psw, ipl, frame);
			KASSERT((pic->pic_pending_ipls & ipl_mask) == 0);
			pic_count = 0;
			continue;
		}
#if PIC_MAXPICS > 32
		pic_count += 32;
		if (pic_count >= PIC_MAXPICS)
			break;
		pic_base = (pic_base + 32) % PIC_MAXPICS;
#endif
	}
	KASSERT((pic_pending_ipls & ipl_mask) == 0);
}

void
pic_do_pending_ints(register_t psw, int newipl)
{
	struct cpu_info * const ci = curcpu();
	while ((pic_pending_ipls & ~__BIT(newipl)) > __BIT(newipl)) {
		KASSERT(pic_pending_ipls < __BIT(NIPL));
		for (;;) {
			int ipl = 31 - __builtin_clz(pic_pending_ipls);
			KASSERT(ipl < NIPL);
			if (ipl <= newipl)
				break;

			ci->ci_cpl = ipl;
			pic_list_deliver_irqs(psw, ipl, NULL);
		}
	}
	ci->ci_cpl = newipl;
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
	KASSERT(pic_sourcebase + pic->pic_maxsources <= PIC_MAXMAXSOURCES);

	pic->pic_sources = &pic_sources[pic_sourcebase];
	pic->pic_irqbase = irqbase;
	pic_sourcebase += pic->pic_maxsources;
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

	is = malloc(sizeof(*is), M_INTRSOURCE, M_ZERO);
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

int
_splraise(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(newipl < NIPL);
	if (newipl > ci->ci_cpl)
		ci->ci_cpl = newipl;
	return oldipl;
}
int
_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(panicstr || newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		pic_do_pending_ints(psw, newipl);
		restore_interrupts(psw);
	}
	return oldipl;
}

void
splx(int savedipl)
{
	struct cpu_info * const ci = curcpu();
	KASSERT(savedipl < NIPL);
	if (savedipl < ci->ci_cpl) {
		register_t psw = disable_interrupts(I32_bit);
		pic_do_pending_ints(psw, savedipl);
		restore_interrupts(psw);
	}
	ci->ci_cpl = savedipl;
}
