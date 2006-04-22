/*	$NetBSD: userret.h,v 1.1.8.2 2006/04/22 11:38:06 simonb Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *     This product includes software developed at Ludd, University of Lule}.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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

static __inline void
userret(struct lwp *, struct trapframe *, u_quad_t);

/*
 *	Common code used by various execption handlers to
 *	return to usermode.
 */
static __inline void
userret(struct lwp *l, struct trapframe *frame, u_quad_t oticks)
{
	int sig;
	struct proc *p = l->l_proc;

	/* Generate UNBLOCKED upcall. */
	if (l->l_flag & L_SA_BLOCKING)
		sa_unblock_userret(l);

	/* Take pending signals. */
	while ((sig = CURSIG(l)) != 0)
		postsig(sig);
	l->l_priority = l->l_usrpri;
	if (curcpu()->ci_want_resched) {
		/*
		 * We are being preempted.
		 */
		preempt(0);
		while ((sig = CURSIG(l)) != 0)
			postsig(sig);
	}

	/* Invoke per-process kernel-exit handling, if any */
	if (p->p_userret)
		(p->p_userret)(l, p->p_userret_arg);

	/*
	 * If profiling, charge system time to the trapped pc.
	 */
	if (p->p_flag & P_PROFIL) {
		extern int psratio;

		addupc_task(p, frame->pc,
		    (int)(p->p_sticks - oticks) * psratio);
	}
	/* Invoke any pending upcalls. */
	if (l->l_flag & L_SA_UPCALL)
		sa_upcall_userret(l);

	curcpu()->ci_schedstate.spc_curpriority = l->l_priority;
}
