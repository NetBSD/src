/*	$NetBSD: interrupt.c,v 1.3 2001/09/15 19:51:38 uch Exp $	*/

/*-
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

#include "opt_vr41xx.h"
#include "opt_tx39xx.h"

#include <sys/param.h>
#include <sys/malloc.h>

#include <uvm/uvm_extern.h>

#include <machine/locore.h>	/* mips3_cp0_*() */
#include <machine/sysconf.h>

#ifdef DEBUG
#define STATIC
#else
#define STATIC	static
#endif

#if defined(VR41XX) && defined(TX39XX)
STATIC void (*__cpu_intr)(u_int32_t, u_int32_t, u_int32_t, u_int32_t);
#define VR_INTR	vr_intr
#define TX_INTR	tx_intr
#elif defined(VR41XX)
#define VR_INTR	cpu_intr
#elif defined(TX39XX)
#define TX_INTR	cpu_intr
#endif

/*
 * This is a mask of bits to clear in the SR when we go to a
 * given interrupt priority level.
 */
#ifdef VR41XX
const u_int32_t __ipl_sr_bits_vr[_IPL_N] = {
	0,					/* IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */

	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTSERIAL */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/* IPL_BIO */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/* IPL_NET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0,		/* IPL_{TTY,SERIAL} */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_0|
		MIPS_INT_MASK_1,		/* IPL_{CLOCK,HIGH} */
};
#endif /* VR41XX */

#ifdef TX39XX
const u_int32_t __ipl_sr_bits_tx[_IPL_N] = {
	0,					/* IPL_NONE */

	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */

	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTNET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1,		/* IPL_SOFTSERIAL */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_4,		/* IPL_BIO */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_4,		/* IPL_NET */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_4,		/* IPL_{TTY,SERIAL} */

	MIPS_SOFT_INT_MASK_0|
		MIPS_SOFT_INT_MASK_1|
		MIPS_INT_MASK_2|
		MIPS_INT_MASK_4,		/* IPL_{CLOCK,HIGH} */
};
#endif /* TX39XX */

const u_int32_t ipl_si_to_sr[_IPL_NSOFT] = {
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFT */
	MIPS_SOFT_INT_MASK_0,			/* IPL_SOFTCLOCK */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTNET */
	MIPS_SOFT_INT_MASK_1,			/* IPL_SOFTSERIAL */
};

const u_int32_t *ipl_sr_bits;
struct hpcmips_soft_intrhand *softnet_intrhand;
struct hpcmips_soft_intr hpcmips_soft_intrs[_IPL_NSOFT];
STATIC void softintr(u_int32_t);

void
intr_init()
{
#if defined(VR41XX) && defined(TX39XX)
	__cpu_intr = CPUISMIPS3 ? vr_intr : tx_intr;
	ipl_sr_bits = CPUISMIPS3 ? __ipl_sr_bits_vr : __ipl_sr_bits_tx;
#elif defined(VR41XX)
	ipl_sr_bits = __ipl_sr_bits_vr;
#elif defined(TX39XX)
	ipl_sr_bits = __ipl_sr_bits_tx;
#endif
}

#if defined(VR41XX) && defined(TX39XX)
void
cpu_intr(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{

	(*__cpu_intr)(status, caust, pc, ipending);
}
#endif /* VR41XX && TX39XX */

#ifdef VR41XX
void
VR_INTR(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	uvmexp.intrs++;
	
	if (ipending & MIPS_INT_MASK_5) {
		/*
		 *  Writing a value to the Compare register,
		 *  as a side effect, clears the timer
		 *  interrupt request.
		 */
		mips3_cp0_compare_write(mips3_cp0_count_read());
	}

	if (ipending & MIPS3_HARD_INT_MASK)
		_splset((*platform.iointr)(status, cause, pc, ipending));

	softintr(ipending);
}
#endif /* VR41XX */

#ifdef TX39XX
void
TX_INTR(u_int32_t status, u_int32_t cause, u_int32_t pc, u_int32_t ipending)
{
	uvmexp.intrs++;

	if (ipending & MIPS_HARD_INT_MASK)
		_splset((*platform.iointr)(status, cause, pc, ipending));

	softintr(ipending);
}
#endif /* TX39XX */

/*
 * softintr:
 *
 *	dispatch pending software interrupt handler.
 */
void
softintr(u_int32_t ipending)
{
	struct hpcmips_soft_intr *asi;
	struct hpcmips_soft_intrhand *sih;
	int i, s;

	ipending &= (MIPS_SOFT_INT_MASK_1 | MIPS_SOFT_INT_MASK_0);
	if (ipending == 0)
		return;

	_clrsoftintr(ipending);

	for (i = _IPL_NSOFT - 1; i >= 0; i--) {
		if ((ipending & ipl_si_to_sr[i]) == 0)
			continue;

		asi = &hpcmips_soft_intrs[i];

		if (TAILQ_FIRST(&asi->softintr_q) != NULL)
			asi->softintr_evcnt.ev_count++;

		for (;;) {
			s = splhigh();

			sih = TAILQ_FIRST(&asi->softintr_q);
			if (sih != NULL) {
				TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
				sih->sih_pending = 0;
			}

			splx(s);

			if (sih == NULL)
				break;

			uvmexp.softs++;
			(*sih->sih_fn)(sih->sih_arg);
		}
	}
}

/*
 * softintr_init:
 *
 *	Initialize the software interrupt system.
 */
void
softintr_init(void)
{
	static const char *softintr_names[] = IPL_SOFTNAMES;
	struct hpcmips_soft_intr *asi;
	int i;

	for (i = 0; i < _IPL_NSOFT; i++) {
		asi = &hpcmips_soft_intrs[i];
		TAILQ_INIT(&asi->softintr_q);
		asi->softintr_ipl = IPL_SOFT + i;
		evcnt_attach_dynamic(&asi->softintr_evcnt, EVCNT_TYPE_INTR,
		    NULL, "soft", softintr_names[i]);
	}

	/* XXX Establish legacy soft interrupt handlers. */
	softnet_intrhand = softintr_establish(IPL_SOFTNET,
	    (void (*)(void *))netintr, NULL);

	assert(softnet_intrhand != NULL);
}

/*
 * softintr_establish:		[interface]
 *
 *	Register a software interrupt handler.
 */
void *
softintr_establish(int ipl, void (*func)(void *), void *arg)
{
	struct hpcmips_soft_intr *asi;
	struct hpcmips_soft_intrhand *sih;

	if (__predict_false(ipl >= (IPL_SOFT + _IPL_NSOFT) ||
			    ipl < IPL_SOFT))
		panic("softintr_establish");

	asi = &hpcmips_soft_intrs[ipl - IPL_SOFT];

	sih = malloc(sizeof(*sih), M_DEVBUF, M_NOWAIT);
	if (__predict_true(sih != NULL)) {
		sih->sih_intrhead = asi;
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
	struct hpcmips_soft_intrhand *sih = arg;
	struct hpcmips_soft_intr *asi = sih->sih_intrhead;
	int s;

	s = splhigh();
	if (sih->sih_pending) {
		TAILQ_REMOVE(&asi->softintr_q, sih, sih_q);
		sih->sih_pending = 0;
	}
	splx(s);

	free(sih, M_DEVBUF);
}
