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
__KERNEL_RCSID(0, "$NetBSD: pic.c,v 1.1.2.2 2007/08/30 07:05:23 matt Exp $");

#define _INTR_PRIVATE
#include <sys/param.h>
#include <sys/evcnt.h>

#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>

#include <arm/pic/picvar.h>

uint32_t atomic_cas_32(volatile_uint32_t *, uint32_t, uint32_t);

uint32_t atomic_or_32(volatile uint32_t *, uint32_t);
uint32_t atomic_and_32(volatile uint32_t *, uint32_t);
uint32_t atomic_nand_32(volatile uint32_t *, uint32_t);
uint32_t atomic_inc_32_nv(volatile uint32_t *);
uint32_t atomic_dec_32_nv(volatile uint32_t *);

static inline uint32_t
atomic_cas_32(volatile uint32_t *p, uint32_t old, uint32_t new)
{
	uint32_t rv, tmp;

	__asm(
	"1:	ldrex	%0, [%2]\n" 
	"	teq	%0, %3\n"
	"	bxne	lr\n"
	"	strex	%1, %4, [%2]\n"
	"	cmp	%1, #0\n"
	"	bne	1b\n"
	"	bx	lr"
		: "=&r"(rv), "=&r"(tmp)
		: "r"(p), "r"(old), "r"(new)
		: "memory");

	return rv;
}

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
uint32_t pic_pending_ipl_refcnt[NIPL];
struct intrsources *pic_sources[PIC_MAXMAXSOURCES];



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
	atomic_or_32(&pic->pic_pending_irqs[is->is_irq >> 5],
	    __BIT(is->is_irq & 0x1f);

	atomic_or_32(&pic_pending_ipls, 
	    atomic_or_32_nv(&pic->pic_pending_ipls,
		__BIT(is->is_ipl)));

	atomic_or_32(&pic_pending_pics[pic->pic_id >> 5],
	    __BIT(pic->pic_id));
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
	int rv;
	
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
				if (__predict_false(frame != NULL
				    && is->is_arg == NULL)) {
					rv = (*is->is_func)(frame);
				} else {
					if ((psw & I32_bit) == 0)
						cpsie(I32_bit);
					rv = (*is->is_func)(frame);
					cpsid(I32_bit);
				}
				is->is_ev.ev_count++;
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
		atomic_nand(pic_pending_ipls, ipl_mask);
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

int
pic_add(struct pic_softc *pic, int irqbase)
{
	int idx, maybe_idx = -1;
	int irqbase = -1;
	for (idx = 0; idx < PIC_MAXPICS; idx++) {
		if (maybe_idx == -1 && (xpic = pic_list[idx]) == NULL) {
			maybe_idx = idx;
			if (maybe_irq_base < xpic->pic_irqbase + xpic->pic_maxsoruces)
				maybe_irqbase = xpic->pic_irqbase + xpic->pic_maxsoruces;
			if (irqbase < 0)
				break;
			continues;
		}
		if (irqbase >= xpic->pic_irqbase + xpic->pic_maxsources)
			continue;
		if (irqbase + pic->pic_maxsources <= xpic->pic_irqbase)
			continue;
		printf("pic_add: pic %s (%u sources @ irq %u) conflicts"
		    " with pic %s (%u sources @ irq %u)\n",
		    pic->pic_name, pic->pic_maxsources, irqbase,
		return -1;
	}
	if (irqbase < 0)
		irqbase = maybe_irqbase;
	idx = maybe_idx;
	KASSERT(idx + pic->pic_maxsources <= PIC_MAXMAXSOURCES);

	pic->pic_sources = &pic_sources[irqbase];
	pic->pic_irqbase = irqbase;
	pic_list[idx] = pic;
}

int
pic_alloc_irq(struct pic_softc *pic)
{
}

void *
pic_establish(struct pic_softc *pic, int irq, int ipl, int level,
	int (*func)(void *), void *arg)
{
}

void *
pic_disestablish(void *arg)
{
	struct intrsource * const is = arg;
	struct pic_softc * const pic = is->is_pic;
	const int irq = is->is_irq;

	(*pic->pic_ops->pic_block_irqs)(pic, irq & ~31, __BIT(irq));
	pic->pic_sources[irq] = NULL;
}
