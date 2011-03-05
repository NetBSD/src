/*	$NetBSD: userret.h,v 1.17.2.1 2011/03/05 15:09:58 bouyer Exp $	*/

/*
 * Copyright (C) 1995, 1996 Wolfgang Solfrank.
 * Copyright (C) 1995, 1996 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "opt_ppcarch.h"
#include "opt_altivec.h"

#include <sys/userret.h>

#include <powerpc/fpu.h>

#ifdef PPC_BOOKE
#include <powerpc/spr.h>
#include <powerpc/booke/spr.h>
#endif

/*
 * Define the code needed before returning to user mode, for
 * trap and syscall.
 */
static __inline void
userret(struct lwp *l, struct trapframe *tf)
{
#if defined(PPC_HAVE_FPU) || defined(ALTIVEC) || defined(PPC_HAVE_SPE)
	struct cpu_info * const ci = curcpu();
#endif

	KASSERTMSG((tf == trapframe(curlwp)),
	    ("tf=%p, trapframe(curlwp)=%p\n", tf, trapframe(curlwp)));

	/* Invoke MI userret code */
	mi_userret(l);

	tf->tf_srr1 &= PSL_USERSRR1;	/* clear SRR1 status bits */

	/*
	 * If someone stole the fp or vector unit while we were away,
	 * disable it.  Note that if the PSL FP/VEC bits aren't set, then
	 * we don't own it.
	 */
#ifdef PPC_HAVE_FPU
	if ((tf->tf_srr1 & PSL_FP) &&
	    (l != ci->ci_fpulwp || l->l_md.md_fpucpu != ci)) {
		tf->tf_srr1 &= ~(PSL_FP|PSL_FE0|PSL_FE1);
	}
#endif
#ifdef ALTIVEC
	/*
	 * We need to manually restore PSL_VEC each time we return
	 * to user mode since PSL_VEC is not preserved in SRR1.
	 */
	if (tf->tf_srr1 & PSL_VEC) {
		if (l != ci->ci_veclwp)
			tf->tf_srr1 &= ~PSL_VEC;
	} else {
		if (l == ci->ci_veclwp)
			tf->tf_srr1 |= PSL_VEC;
	}

	/*
	 * If the new process isn't the current AltiVec process on this
	 * CPU, we need to stop any data streams that are active (since
	 * it will be a different address space).
	 */
	if (ci->ci_veclwp != &lwp0 && ci->ci_veclwp != l) {
		__asm volatile("dssall;sync");
	}
#endif
#ifdef PPC_BOOKE
	/*
	 * BookE doesn't PSL_SE but it does have a debug instruction completion
	 * exception but it needs PSL_DE to fire.  Since we don't want it to
	 * happen in the kernel, we must disable PSL_DE and let it get
	 * restored by rfi/rfci.
	 */
	if (__predict_false(tf->tf_srr1 & PSL_SE)) {
		extern void booke_sstep(struct trapframe *); /* ugly */
		booke_sstep(tf);
	}
#endif
#ifdef PPC_HAVE_SPE
	/*
	 * We need to manually restore PSL_SPV each time we return
	 * to user mode since PSL_SPV is not preserved in SRR1 since
	 * we don't include it in PSL_USERSRR1 to control its setting.
	 */
	if (tf->tf_srr1 & PSL_SPV) {
		if (l != ci->ci_veclwp)
			tf->tf_srr1 &= ~PSL_SPV;
	} else {
		if (l == ci->ci_veclwp)
			tf->tf_srr1 |= PSL_SPV;
	}
#endif
}
