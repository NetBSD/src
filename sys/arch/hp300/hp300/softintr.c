/*	$NetBSD: softintr.c,v 1.1.2.3 2002/04/01 07:39:58 nathanw Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Steve C. Woodford and Jason R. Thorpe.
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
 * Generic soft interrupt implementation for hp300.
 * Based heavily on the alpha implementation by Jason Thorpe.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.1.2.3 2002/04/01 07:39:58 nathanw Exp $");                                                  

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/malloc.h>
#include <sys/sched.h>
#include <sys/vmmeter.h>

#include <uvm/uvm_extern.h>

#include <machine/cpu.h>
#include <machine/intr.h>

struct hp300_soft_intrhand	*softnet_intrhand;
volatile u_int8_t ssir;

static struct hp300_soft_intr hp300_soft_intrs[IPL_NSOFT];

/*
 * softintr_init()
 *
 *	Initialise hp300 software interrupt subsystem.
 */
void
softintr_init()
{
	static const char *softintr_names[] = IPL_SOFTNAMES;
	struct hp300_soft_intr *hsi;
	int i;

	for (i = 0; i < IPL_NSOFT; i++) {
		hsi = &hp300_soft_intrs[i];
		LIST_INIT(&hsi->hsi_q);
		hsi->hsi_ipl = i;
		evcnt_attach_dynamic(&hsi->hsi_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* Establish legacy software interrupt handlers */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *)) netintr, NULL);

	assert(softnet_intrhand != NULL);
}

/*
 * softintr_dispatch()
 *
 *	Internal function for running queued soft interrupts.
 */
void
softintr_dispatch()
{
	struct hp300_soft_intr *hsi;
	struct hp300_soft_intrhand *sih;
	int handled;
	u_int8_t mask;

	do {
		mask = 0x1;
		for (mask = 1, hsi = hp300_soft_intrs, handled = 0;
		    hsi < &hp300_soft_intrs[IPL_NSOFT];
		    hsi++, mask <<= 1) {

			if ((ssir & mask) == 0)
				continue;

			hsi->hsi_evcnt.ev_count++;
			handled++;

			for (sih = LIST_FIRST(&hsi->hsi_q);
			     sih != NULL;
			     sih = LIST_NEXT(sih, sih_q)) {
				if (sih->sih_pending) {
					uvmexp.softs++;
					sih->sih_pending = 0;
					(*sih->sih_fn)(sih->sih_arg);
				}
			}

			ssir &= ~mask;
		}
	} while (handled);
}

/*
 * softintr_establish()		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct hp300_soft_intr *hsi;
	struct hp300_soft_intrhand *sih;
	int s;

	if (__predict_false(ipl >= IPL_NSOFT || ipl < 0 || func == NULL))
		panic("softintr_establish");

	hsi = &hp300_soft_intrs[ipl];

	MALLOC(sih, struct hp300_soft_intrhand *,
	   sizeof(struct hp300_soft_intrhand), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = hsi;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
		s = splsoft();
		LIST_INSERT_HEAD(&hsi->hsi_q, sih, sih_q);
		splx(s);
	}
	return (sih);
}

/*
 * softintr_disestablish()	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct hp300_soft_intrhand *sih = arg;
	int s;

	s = splsoft();
	LIST_REMOVE(sih, sih_q);
	splx(s);

	free(sih, M_DEVBUF);
}
