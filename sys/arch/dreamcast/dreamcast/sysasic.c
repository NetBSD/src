/*	$NetBSD: sysasic.c,v 1.1.4.1 2002/06/23 17:35:36 jdolecek Exp $	*/

/*-
 * Copyright (c) 2001, 2002 The NetBSD Foundation, Inc.
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

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>

#include <sh3/exception.h>

#include <machine/intr.h>
#include <machine/sysasicvar.h>

#define SYSASIC_IRQ_LEVEL_13	0
#define SYSASIC_IRQ_LEVEL_11	1
#define SYSASIC_IRQ_LEVEL_9	2
#define SYSASIC_IRQ_LEVEL_MAX	2

struct sysasic_intrhand {
	void	*syh_intc;
	int	syh_event;
	int	syh_idx;
} sysasic_intrhand[SYSASIC_IRQ_LEVEL_MAX + 1];

void sysasic_intr_enable(struct sysasic_intrhand *, int);

/*
 * Set up an interrupt handler to start being called.
 */
void *
sysasic_intr_establish(int event, int (*ih_fun)(void *), void *ih_arg)
{
	struct sysasic_intrhand *syh;
	int evtcode, ipl, idx;

	if (event < 0 || event > SYSASIC_EVENT_MAX)
		panic("invalid sysasic event %d", event);

	/*
	 * Dreamcast use SH4 INTC as IRL mode. if INTEVT code is specified,
	 * its priority level is fixed.
	 */
	switch (event) {
	case SYSASIC_EVENT_EXT:
		idx = SYSASIC_IRQ_LEVEL_11;
		ipl = IPL_NET;
		evtcode = SH_INTEVT_IRL11;
		break;
	case SYSASIC_EVENT_GDROM:
		idx = SYSASIC_IRQ_LEVEL_9;
		ipl = IPL_BIO;
		evtcode = SH_INTEVT_IRL9;
		break;
	default:
		panic("vaild but unknown event %d\n", event);
	}

	syh = &sysasic_intrhand[idx];

	syh->syh_event	= event;
	syh->syh_idx	= idx;
	syh->syh_intc	= intc_intr_establish(evtcode, IST_LEVEL, ipl,
	    ih_fun, ih_arg);

	sysasic_intr_enable(syh, 1);

	return (void *)syh;
}

/*
 * Deregister an interrupt handler.
 */
void
sysasic_intr_disestablish(void *arg)
{
	struct sysasic_intrhand *syh = arg;
	int event;

	event = syh->syh_event;

	if (event < 0 || event > SYSASIC_EVENT_MAX)
		panic("invalid sysasic event %d", event);

	sysasic_intr_enable(syh, 0);
	intc_intr_disestablish(syh->syh_intc);
}

void
sysasic_intr_enable(struct sysasic_intrhand *syh, int on)
{
	int event = syh->syh_event;
	__volatile u_int32_t *masks =
	    (__volatile u_int32_t *)(0xa05f6910 + (syh->syh_idx << 4));

	masks[0] = 0;
	masks[1] = 0;
	if (on)
		masks[event >> 5] = 1 << (event & 31);
}
