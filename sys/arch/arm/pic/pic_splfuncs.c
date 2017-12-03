/*	$NetBSD: pic_splfuncs.c,v 1.4.2.2 2017/12/03 11:35:55 jdolecek Exp $	*/
/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: pic_splfuncs.c,v 1.4.2.2 2017/12/03 11:35:55 jdolecek Exp $");

#define _INTR_PRIVATE
#include <sys/param.h>
#include <sys/atomic.h>
#include <sys/evcnt.h>
#include <sys/lwp.h>
#include <sys/kernel.h>

#include <dev/cons.h>

#if defined(__arm__)
#include <arm/armreg.h>
#include <arm/cpu.h>
#include <arm/cpufunc.h>
#elif defined(__aarch64__)
#include <aarch64/locore.h>
#define I32_bit		DAIF_I
#define F32_bit		DAIF_F
#endif

#include <arm/pic/picvar.h>


int
_splraise(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(newipl < NIPL);
	if (newipl > ci->ci_cpl) {
		pic_set_priority(ci, newipl);
	}
	return oldipl;
}
int
_spllower(int newipl)
{
	struct cpu_info * const ci = curcpu();
	const int oldipl = ci->ci_cpl;
	KASSERT(panicstr || newipl <= ci->ci_cpl);
	if (newipl < ci->ci_cpl) {
		register_t psw = cpsid(I32_bit);
		ci->ci_intr_depth++;
		pic_do_pending_ints(psw, newipl, NULL);
		ci->ci_intr_depth--;
		if ((psw & I32_bit) == 0 || newipl == IPL_NONE)
			cpsie(I32_bit);
		cpu_dosoftints();
	}
	return oldipl;
}

void
splx(int savedipl)
{
	struct cpu_info * const ci = curcpu();
	KASSERT(savedipl < NIPL);

	if (__predict_false(savedipl == ci->ci_cpl)) {
		return;
	}

	register_t psw = cpsid(I32_bit);
	KASSERTMSG(panicstr != NULL || savedipl < ci->ci_cpl,
	    "splx(%d) to a higher ipl than %d", savedipl, ci->ci_cpl);

	ci->ci_intr_depth++;
	pic_do_pending_ints(psw, savedipl, NULL);
	ci->ci_intr_depth--;
	KASSERTMSG(ci->ci_cpl == savedipl, "cpl %d savedipl %d",
	    ci->ci_cpl, savedipl);
	if ((psw & I32_bit) == 0)
		cpsie(I32_bit);
	cpu_dosoftints();
	KASSERTMSG(ci->ci_cpl == savedipl, "cpl %d savedipl %d",
	    ci->ci_cpl, savedipl);
}
