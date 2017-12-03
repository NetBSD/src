/*	$NetBSD: userret.h,v 1.14.2.1 2017/12/03 11:36:48 jdolecek Exp $	*/

/*
 * Copyright (c) 1994 Ludd, University of Lule}, Sweden.
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/userret.h>
#include <sys/ras.h>

/*
 *	Common code used by various exception handlers to
 *	return to usermode.
 */
static __inline void
userret(struct lwp *l, struct trapframe *tf, u_quad_t oticks)
{
	struct proc * const p = l->l_proc;

	mi_userret(l);

	/*
	 * Check to see if a RAS was interrupted and restart it if it was.
	 */
	if (__predict_false(p->p_raslist != NULL)) {
		void * const ras_pc = ras_lookup(p, (void *) tf->tf_pc);
		if (ras_pc != (void *) -1)
			tf->tf_pc = (vaddr_t) ras_pc;
	}

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if ((p->p_stflag & PST_PROFIL) != 0) {
		extern int psratio;

		addupc_task(l, tf->tf_pc,
		    (int)(p->p_sticks - oticks) * psratio);
	}
}
