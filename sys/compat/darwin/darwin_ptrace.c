/*	$NetBSD: darwin_ptrace.c,v 1.1 2003/11/20 07:12:34 manu Exp $ */

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus.
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
__KERNEL_RCSID(0, "$NetBSD: darwin_ptrace.c,v 1.1 2003/11/20 07:12:34 manu Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/ptrace.h>
#include <sys/sa.h>

#include <sys/syscallargs.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_exec.h>
#include <compat/darwin/darwin_ptrace.h>
#include <compat/darwin/darwin_syscallargs.h>

#if 0
#define ISSET(t, f)     ((t) & (f))

static inline int ptrace_sanity_check(struct proc *, struct proc *);

/* Sanity checks copied from native sys_ptrace() */
static inline int 
ptrace_sanity_check(p, t)
	struct proc *p;
	struct proc *t;
{
	/*
	 * You can't do what you want to the process if:
	 *	(1) It's not being traced at all,
	 */
	if (!ISSET(t->p_flag, P_TRACED))
		return (EPERM);

	/*
	 *	(2) it's being traced by procfs (which has
	 *	    different signal delivery semantics),
	 */
	if (ISSET(t->p_flag, P_FSTRACE))
		return (EBUSY);

	/*
	 *	(3) it's not being traced by _you_, or
	 */
	if (t->p_pptr != p)
		return (EBUSY);

	/*
	 *	(4) it's not currently stopped.
	 */
	if (t->p_stat != SSTOP || !ISSET(t->p_flag, P_WAITED))
		return (EBUSY);

	return 0;
}
#endif

int
darwin_sys_ptrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(caddr_t) addr;
		syscallarg(int) data;
	} */ *uap = v;
	int req = SCARG(uap, req);
	struct proc *p = l->l_proc;
	struct darwin_emuldata *ded = NULL;
	struct proc *t;			/* target process */
	int error;

	switch (req) {
	case DARWIN_PT_SIGEXC:
		ded = (struct darwin_emuldata *)p->p_emuldata;
		ded->ded_flags |= DARWIN_DED_SIGEXC;
		break;

	case DARWIN_PT_DETACH:
		if ((t = pfind(SCARG(uap, pid))) == NULL)
			return (ESRCH);

		/* 
		 * Clear signal-as-exceptions flag if detaching is 
		 * successful and if it is a Darwin process.
		 */
		if (((error = sys_ptrace(l, v, retval)) == 0) &&
		    (t->p_emul != &emul_darwin)) {
			ded = (struct darwin_emuldata *)t->p_emuldata;
			ded->ded_flags &= ~DARWIN_DED_SIGEXC;
		}
		break;

	case DARWIN_PT_READ_U:
	case DARWIN_PT_WRITE_U:
	case DARWIN_PT_STEP:
	case DARWIN_PT_THUPDATE:
	case DARWIN_PT_ATTACHEXC:
	case DARWIN_PT_FORCEQUOTA:
	case DARWIN_PT_DENY_ATTACH:
		printf("darwin_sys_ptrace: unimplemented command %d\n", req);
		break;

	/* The other ptrace commands are the same on NetBSD */
	default:
		return sys_ptrace(l, v, retval);
		break;
	}
	
	return 0;
}
