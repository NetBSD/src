/*	$NetBSD: softintr.c,v 1.3 2003/07/15 02:43:39 lukem Exp $	*/

/*
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.3 2003/07/15 02:43:39 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/device.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <net/netisr.h>			/* Legacy softnet support */

#include <machine/intr.h>

/* XXX For legacy software interrupts. */
struct mips_soft_intrhand *softnet_intrhand;

struct mips_soft_intr mips_soft_intrs[_IPL_NSOFT];

/*
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
void
softintr_init(void)
{
	static const char *softintr_names[] = IPL_SOFTNAMES;
	struct mips_soft_intr *msi;
	int i;

	for (i = 0; i < _IPL_NSOFT; i++) {
		msi = &mips_soft_intrs[i];
		TAILQ_INIT(&msi->softintr_q);
		simple_lock_init(&msi->softintr_slock);
		msi->softintr_ipl = IPL_SOFT + i;
		evcnt_attach_dynamic(&msi->softintr_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* XXX Establish legacy soft interrupt handlers. */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);

	KASSERT(softnet_intrhand != NULL);
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct mips_soft_intr *msi;
	struct mips_soft_intrhand *sih;

	if (__predict_false(ipl >= (IPL_SOFT + _IPL_NSOFT) ||
			    ipl < IPL_SOFT))
		panic("softintr_establish");

	msi = &mips_soft_intrs[ipl - IPL_SOFT];

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = msi;
		sih->sih_func = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	return sih;
}

/*
 * softintr_disestablish:	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct mips_soft_intrhand *sih = arg;
	struct mips_soft_intr *msi = sih->sih_intrhead;
	int s;

	s = splhigh();
	simple_lock(&msi->softintr_slock);
	if (sih->sih_pending) {
		TAILQ_REMOVE(&msi->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	simple_unlock(&msi->softintr_slock);
	splx(s);

	free(sih, M_DEVBUF);
}

/*
 * softintr_dispatch:
 *
 *	Process pending software interrupts.
 *
 *	Called at splsoft()
 */
void
softintr_dispatch(ipending)
	u_int32_t ipending;
{
	struct mips_soft_intr *msi;
	struct mips_soft_intrhand *sih;
	int i, s;

	for (i = _IPL_NSOFT - 1; i >= 0; i--) {
		if ((ipending & mips_ipl_si_to_sr[i]) == 0)
			continue;

		msi = &mips_soft_intrs[i];

		if (TAILQ_FIRST(&msi->softintr_q) != NULL)
			msi->softintr_evcnt.ev_count++;

		for (;;) {
			s = splhigh();
			simple_lock(&msi->softintr_slock);

			sih = TAILQ_FIRST(&msi->softintr_q);
			if (sih != NULL) {
				TAILQ_REMOVE(&msi->softintr_q, sih, sih_q);
				sih->sih_pending = 0;
			}

			simple_unlock(&msi->softintr_slock);
			splx(s);

			if (sih == NULL)
				break;

			uvmexp.softs++;
			(*sih->sih_func)(sih->sih_arg);
		}
	}
}
