/*	$NetBSD: softintr.c,v 1.1.2.2 2002/06/23 17:36:24 jdolecek Exp $	*/

/*-
 * Copyright (c) 2000, 2001, 2002 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe, and by Matthew Fredette.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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
 * Generic soft interrupt implementation for NetBSD/hp700.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.1.2.2 2002/06/23 17:36:24 jdolecek Exp $");

#include <sys/param.h>
#include <sys/malloc.h>

#include <machine/intr.h>

#include <net/netisr.h>

#include <uvm/uvm_extern.h>

#include <hp700/hp700/intr.h>

static struct hp700_int_reg int_reg_soft;
static struct hp700_soft_intr hp700_soft_intrs[HP700_NSOFTINTR];
int softnetmask;

const struct {
	int ipl;
	const char *name;
} hp700_soft_intr_info[HP700_NSOFTINTR] = {
	{ IPL_SOFTCLOCK,	"softclock"	},
	{ IPL_SOFTNET,		"softnet"	},
	{ IPL_SOFTSERIAL,	"softserial"	},
};

/*
 * softintr_bootstrap:
 *
 *	Bootstrap the software interrupt system.
 */
void
softintr_bootstrap(void)
{
	/* We don't know softnetmask yet. */
	softnetmask = 0;
}

/*
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
int
softintr_init(int spl_free)
{
	struct hp700_soft_intr *si;
	struct device dummy_device;
	int i, bit_pos, bit_mask;

	/* Initialize the soft interrupt "register". */
	hp700_intr_reg_establish(&int_reg_soft);
	int_reg_soft.int_reg_dev = "soft";

	/* Initialize the soft interrupt "bits". */
	for (i = 0; i < HP700_NSOFTINTR; i++) {
	
		/* Allocate an spl bit. */
		bit_pos = ffs(spl_free);
		if (bit_pos-- == 0)
			panic("softintr_init: spl full");
		bit_mask = (1 << bit_pos);
		spl_free &= ~bit_mask;

		/* Register our interrupt handler for this bit. */
		strcpy(dummy_device.dv_xname, hp700_soft_intr_info[i].name);
		hp700_intr_establish(&dummy_device, hp700_soft_intr_info[i].ipl,
		    softintr_dispatch, (void *) (i + 1),
		    &int_reg_soft, bit_pos);
		
		si = &hp700_soft_intrs[i];
		TAILQ_INIT(&si->softintr_q);
		si->softintr_ssir = bit_mask;
	}

	softnetmask = hp700_soft_intrs[HP700_SOFTINTR_SOFTNET].softintr_ssir;

	return spl_free;
}

/*
 * softintr_dispatch:
 *
 *	Process pending software interrupts.
 */
int
softintr_dispatch(void *arg)
{
	int which = ((int) arg) - 1;
	struct hp700_soft_intr *si = &hp700_soft_intrs[which];
	struct hp700_soft_intrhand *sih;
	int s;
	int ni;

	/* Special handling for softnet. */
	if (which == HP700_SOFTINTR_SOFTNET) {
		/* use atomic "load & clear" */
		/*
		 * XXX netisr must be 16-byte aligned, but
		 * we can't control its declaration.
		 */
		 __asm __volatile ("ldcws 0(%1), %0"
				    : "=r" (ni) : "r" (&netisr));
#define DONETISR(m,c) if (ni & (1 << (m))) c()
#include <net/netisr_dispatch.h>
	}

	for (;;) {
		hp700_softintr_lock(si, s);
		sih = TAILQ_FIRST(&si->softintr_q);
		if (sih == NULL) {
			hp700_softintr_unlock(si, s);
			break;
		}
		TAILQ_REMOVE(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
		hp700_softintr_unlock(si, s);

		uvmexp.softs++;
		(*sih->sih_fn)(sih->sih_arg);
	}

	return 1;
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct hp700_soft_intr *si;
	struct hp700_soft_intrhand *sih;
	int which;

	switch (ipl) {
	case IPL_SOFTCLOCK:
		which = HP700_SOFTINTR_SOFTCLOCK;
		break;

	case IPL_SOFTNET:
		which = HP700_SOFTINTR_SOFTNET;
		break;

	case IPL_SOFTSERIAL:
		which = HP700_SOFTINTR_SOFTSERIAL;
		break;

	default:
		panic("softintr_establish");
	}

	si = &hp700_soft_intrs[which];

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = si;
		sih->sih_fn = func;
		sih->sih_arg = arg;
		sih->sih_pending = 0;
	}
	return (sih);
}

/*
 * softintr_disestablish:	[interface]
 *
 *	Unregister a software interrupt handler.
 */
void
softintr_disestablish(void *arg)
{
	struct hp700_soft_intrhand *sih = arg;
	struct hp700_soft_intr *si = sih->sih_intrhead;
	int s;

	hp700_softintr_lock(si, s);
	if (sih->sih_pending) {
		TAILQ_REMOVE(&si->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	hp700_softintr_unlock(si, s);

	free(sih, M_DEVBUF);
}
