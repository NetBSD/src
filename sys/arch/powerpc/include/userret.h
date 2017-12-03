/*	$NetBSD: userret.h,v 1.24.2.2 2017/12/03 11:36:37 jdolecek Exp $	*/

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
#include <sys/ras.h>

#include <powerpc/fpu.h>
#include <powerpc/psl.h>

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
	KASSERTMSG((tf == trapframe(curlwp)),
	   "tf=%p, trapframe(curlwp)=%p\n", tf, trapframe(curlwp));

	/* Invoke MI userret code */
	mi_userret(l);

	KASSERTMSG((tf->tf_srr1 & PSL_PR) != 0,
	    "tf=%p: srr1 (%#lx): PSL_PR isn't set!",
	    tf, tf->tf_srr1);
	KASSERTMSG((tf->tf_srr1 & PSL_FP) == 0
	    || l->l_cpu->ci_data.cpu_pcu_curlwp[PCU_FPU] == l,
	    "tf=%p: srr1 (%#lx): PSL_FP set but FPU curlwp %p is not curlwp %p!",
	    tf, tf->tf_srr1, l->l_cpu->ci_data.cpu_pcu_curlwp[PCU_FPU], l);

	/* clear SRR1 status bits */
	tf->tf_srr1 &= (PSL_USERSRR1|PSL_FP|PSL_VEC);

#ifdef ALTIVEC
	/*
	 * We need to manually restore PSL_VEC each time we return
	 * to user mode since PSL_VEC isn't always preserved in SRR1.
	 * We keep a copy of it in md_flags to make restoring easier.
	 */
	tf->tf_srr1 |= l->l_md.md_flags & PSL_VEC;
#endif
#ifdef PPC_BOOKE
	/*
	 * BookE doesn't have PSL_SE but it does have a debug instruction
	 * completion exception but it needs PSL_DE to fire.  Instead we
	 * use IAC1/IAC2 to match the next PC.
	 */
	if (__predict_false(tf->tf_srr1 & PSL_SE)) {
		tf->tf_srr1 &= ~PSL_SE;
		extern void booke_sstep(struct trapframe *); /* ugly */
		booke_sstep(tf);
	}
#endif

#ifdef __HAVE_RAS
	/*
	 * Check to see if a RAS was interrupted and restart it if it was.      
	 */
	struct proc * const p = l->l_proc;
	if (__predict_false(p->p_raslist != NULL)) {
		void * const ras_pc = ras_lookup(p, (void *) tf->tf_srr0);
		if (ras_pc != (void *) -1)
			tf->tf_srr0 = (vaddr_t) ras_pc;
	}
#endif
}
