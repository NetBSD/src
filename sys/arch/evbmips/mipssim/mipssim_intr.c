/* $NetBSD: mipssim_intr.c,v 1.2 2021/02/15 22:39:46 reinoud Exp $ */

/*-
 * Copyright (c) 2014 Michael Lorenz
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
__KERNEL_RCSID(0, "$NetBSD: mipssim_intr.c,v 1.2 2021/02/15 22:39:46 reinoud Exp $");

#define __INTR_PRIVATE

#include <sys/param.h>
#include <sys/cpu.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/kmem.h>

#include <mips/locore.h>
#include <machine/intr.h>

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given hardware interrupt priority level.
 */
static const struct ipl_sr_map mipssim_ipl_sr_map = {
    .sr_bits = {
	[IPL_NONE] =		0,
	[IPL_SOFTCLOCK] =	MIPS_SOFT_INT_MASK_0,
	[IPL_SOFTNET] =		MIPS_SOFT_INT_MASK,
	[IPL_VM] =		MIPS_SOFT_INT_MASK
				    | MIPS_INT_MASK_0 | MIPS_INT_MASK_1
				    | MIPS_INT_MASK_2,
	[IPL_SCHED] =		MIPS_SOFT_INT_MASK
				    | MIPS_INT_MASK_0 | MIPS_INT_MASK_1
				    | MIPS_INT_MASK_2 | MIPS_INT_MASK_5,
	[IPL_DDB] =		MIPS_INT_MASK,
	[IPL_HIGH] =		MIPS_INT_MASK,
    },
};

/* XXX - add evcnt bits to <machine/intr.h> struct evbmips_intrhand */
struct intrhand {
	LIST_ENTRY(intrhand) ih_q;
	int (*ih_func)(void *);
	void *ih_arg;
	int ih_irq;
};


/*
 * Use CPU interrupts INT0 .. INT4.  Clock interrupts (INT5)
 * are handled in cpu_intr() before evbmips_iointr() is called.
 */
#define	NINTR		5	/* MIPS INT0 - INT4 */

LIST_HEAD(intrlist, intrhand) intrs[NINTR];
struct evcnt ih_count[NINTR];

const char * const intrnames[NINTR] = {
	"int 0 (mipsnet)",
	"int 1 (virtio)",
	"int 2 (uart)",
	"int 3 (unused)",
	"int 4 (unused)",
};

void mipssim_irq(int);

void
evbmips_intr_init(void)
{
	int i;

	ipl_sr_map = mipssim_ipl_sr_map;

	/* zero all handlers */
	for (i = 0; i < NINTR; i++) {
		LIST_INIT(&intrs[i]);
		evcnt_attach_dynamic(&ih_count[i], EVCNT_TYPE_INTR,
		    NULL, "cpu", intrnames[i]);
	}
}

void
evbmips_iointr(int ipl, uint32_t ipending, struct clockframe *cf)
{
	struct intrlist *list;

	for (int level = NINTR - 1; level >= 0; level--) {
		struct intrhand *ih;

		if ((ipending & (MIPS_INT_MASK_0 << level)) == 0)
			continue;

		ih_count[level].ev_count++;
		list = &intrs[level];

		LIST_FOREACH(ih, list, ih_q) {
			if (ih->ih_func) {
				(*ih->ih_func)(ih->ih_arg);
			}
		}
	}
}

void *
evbmips_intr_establish(int irq, int (*func)(void *), void *arg)
{
	struct intrlist *list;
	struct intrhand *ih;
	int s;

	if ((irq < 0) || (irq >= NINTR)) {
		aprint_error("%s: invalid irq %d\n", __func__, irq);
		return NULL;
	}

	list = &intrs[irq];
	ih = kmem_alloc(sizeof(struct intrhand), KM_SLEEP);

	s = splhigh();

	ih->ih_func = func;
	ih->ih_arg = arg;
	ih->ih_irq = irq;
	LIST_INSERT_HEAD(list, ih, ih_q);

	/* now enable the IRQ (nothing to do here?) */

	splx(s);

	return ih;
}

void
evbmips_intr_disestablish(void *cookie)
{
	panic("untested %s", __func__);	/* XXX! */

	struct intrhand *ih = cookie;
	int s;

	s = splhigh();

	/* now disable the IRQ (nothing to do here?) */

	ih->ih_func = NULL;
	ih->ih_arg = NULL;
	LIST_REMOVE(ih, ih_q);
	kmem_free(ih, sizeof(struct intrhand));

	splx(s);
}
