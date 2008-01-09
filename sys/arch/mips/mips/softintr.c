/*	$NetBSD: softintr.c,v 1.5.24.1 2008/01/09 01:47:19 matt Exp $	*/

/*
 * Copyright (c) 2001, 2007 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: softintr.c,v 1.5.24.1 2008/01/09 01:47:19 matt Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/intr.h>

#ifdef notyet	/* __HAVE_FAST_SOFTINTS stuff */
void
softint_init_md(lwp_t *l, u_int level, uintptr_t *machdep)
{

	switch (level) {
	case SOFTINT_BIO:
		*machdep = mips_ipl_si_to_sr[IPL_SOFTBIO];
		break;
	case SOFTINT_NET:
		*machdep = mips_ipl_si_to_sr[IPL_SOFTNET];
		break;
	case SOFTINT_SERIAL:
		*machdep = mips_ipl_si_to_sr[IPL_SOFTSERIAL];
		break;
	case SOFTINT_CLOCK:
		*machdep = mips_ipl_si_to_sr[IPL_SOFTCLOCK];
		break;
	default:
		panic("softint_init_md");
	}
}

void
mips_softintr_dispatch(u_int32_t ipending)
{
	int i;

	for (i = SI_NQUEUES - 1; i >= 0; i--) {
		if ((ipending & mips_ipl_si_to_sr[i]) == 0)
			continue;
		/*
		 * XXX
		 * splhigh
		 * save s0-s9 in curlwp's PCB
		 * switch stack
		 * switch curlwp
		 * -> softint_dispatch
		 * back to old stack
		 * restore curlwp
		 */
	}
}
#endif
