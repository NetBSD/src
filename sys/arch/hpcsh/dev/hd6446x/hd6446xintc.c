/*	$NetBSD: hd6446xintc.c,v 1.7.60.1 2008/05/18 12:32:06 yamt Exp $	*/

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by UCHIYAMA Yasushi.
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
__KERNEL_RCSID(0, "$NetBSD: hd6446xintc.c,v 1.7.60.1 2008/05/18 12:32:06 yamt Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <sh3/devreg.h>
#include <hpcsh/dev/hd6446x/hd6446xintcreg.h>
#include <hpcsh/dev/hd6446x/hd6446xintcvar.h>

struct hd6446x_intrhand hd6446x_intrhand[_HD6446X_INTR_N];
uint16_t hd6446x_imask[_IPL_N];
uint16_t hd6446x_ienable;

static void hd6446x_intr_priority_update(void);


void
hd6446x_intr_init(void)
{

	/* Initialize interrupt priority masks. */
	hd6446x_intr_priority_update();
}

void *
hd6446x_intr_establish(int irq, int mode, int level,
    int (*func)(void *), void *arg)
{
	struct hd6446x_intrhand *hh = &hd6446x_intrhand[ffs(irq) - 1];
	int s;

	s = splhigh();

	/* Register interrupt handler */
	hh->hh_func = func;
	hh->hh_arg = arg;
	hh->hh_ipl = level << 4;
	hh->hh_imask = irq;
	hd6446x_ienable |= hh->hh_imask;

	/* Update interrupt priority masks. */
	hd6446x_intr_priority_update();

	splx(s);

	return (hh);
}

void
hd6446x_intr_disestablish(void *handle)
{
	struct hd6446x_intrhand *hh = handle;
	int s;
	
	s = splhigh();

	/* Update interrupt priority masks */
	hd6446x_ienable &= ~hh->hh_imask;
	memset(hh, 0, sizeof(*hh));
	hd6446x_intr_priority_update();

	splx(s);
}

void
hd6446x_intr_priority(int irq, int level)
{
	struct hd6446x_intrhand *hh = &hd6446x_intrhand[ffs(irq) - 1];	
	int s;

	KDASSERT(hh->hh_func != NULL);
	s = splhigh();
	hh->hh_ipl = level << 4;
	hd6446x_intr_priority_update();
	splx(s);
}

static void
hd6446x_intr_priority_update(void)
{
	int ipl, src;

	for (ipl = 0; ipl < _IPL_N; ipl++) {
		uint16_t mask = ~hd6446x_ienable; /* mask disabled */

		/* mask sources interrupting at <= ipl */
		for (src = 0; src < _HD6446X_INTR_N; ++src) {
			struct hd6446x_intrhand *hh = &hd6446x_intrhand[src];

			if (hh->hh_func != NULL && hh->hh_ipl <= (ipl << 4))
				mask |= 1 << src;
		}

		hd6446x_imask[ipl] = mask;
	}
}
