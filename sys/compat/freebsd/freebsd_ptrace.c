/*	$NetBSD: freebsd_ptrace.c,v 1.15 2007/06/23 15:24:26 dsl Exp $	*/

/*-
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)sys_process.c	8.1 (Berkeley) 6/10/93
 */

/*-
 * Copyright (c) 1994 Christopher G. Demetriou.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)sys_process.c	8.1 (Berkeley) 6/10/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: freebsd_ptrace.c,v 1.15 2007/06/23 15:24:26 dsl Exp $");

#if defined(_KERNEL_OPT)
#include "opt_ptrace.h"
#endif

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/errno.h>
#include <sys/ptrace.h>
#include <sys/uio.h>
#include <sys/user.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>
#include <compat/sys/signalvar.h>

#include <machine/reg.h>
#include <machine/freebsd_machdep.h>

#include <compat/common/compat_util.h>
#include <compat/freebsd/freebsd_syscallargs.h>
#include <compat/freebsd/freebsd_ptrace.h>

/*
 * Process debugging system call.
 */
int
freebsd_sys_ptrace(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
#if defined(PTRACE) || defined(_LKM)
	struct freebsd_sys_ptrace_args /* {
		syscallarg(int) req;
		syscallarg(pid_t) pid;
		syscallarg(void *) addr;
		syscallarg(int) data;
	} */ *uap = v;
	struct sys_ptrace_args npa;
#ifdef _LKM
	sy_call_t sys_ptrace = sysent[SYS_ptrace].sy_call;
#endif

	switch (SCARG(uap, req)) {
#ifdef PT_STEP
	case FREEBSD_PT_STEP:
		SCARG(&npa, req) = PT_STEP;
		SCARG(&npa, pid) = SCARG(uap, pid);
		SCARG(&npa, addr) = SCARG(uap, addr);
		SCARG(&npa, data) = SCARG(uap, data);
		return sys_ptrace(l, &npa, retval);
#endif
	case FREEBSD_PT_TRACE_ME:
	case FREEBSD_PT_READ_I:
	case FREEBSD_PT_READ_D:
	case FREEBSD_PT_WRITE_I:
	case FREEBSD_PT_WRITE_D:
	case FREEBSD_PT_CONTINUE:
	case FREEBSD_PT_KILL:
		/* These requests are compatible with NetBSD */
		return sys_ptrace(l, uap, retval);

#if 0
/*
 * XXX: I've commented out this code, it is broken on too many fronts to fix.
 *	1) It is doing an unlocked read-modify-write cycle on process that
 *	   I assume might be running!
 *	   and in code that might sleep (due to a pagefault), never mind
 *	   what happens on an SMP system
 *      2) It accesses data in userspace without using copyin/out.
 *	3) It all looks like a nasty hack that isn't likely to work.
 *	4) It uses the stackgap.
 * dsl June 2007
 */
	case FREEBSD_PT_READ_U:
	case FREEBSD_PT_WRITE_U:
    {
	int error;
	struct {
		struct reg regs;
		struct fpreg fpregs;
	} *nrp;
	struct freebsd_ptrace_reg fr;
		sg = stackgap_init(p, 0);
		nrp = stackgap_alloc(p, &sg, sizeof(*nrp));
#ifdef PT_GETREGS
		SCARG(&npa, req) = PT_GETREGS;
		SCARG(&npa, pid) = SCARG(uap, pid);
		SCARG(&npa, addr) = (void *)&nrp->regs;
		if ((error = sys_ptrace(l, &npa, retval)) != 0)
			return error;
#endif
#ifdef PT_GETFPREGS
		SCARG(&npa, req) = PT_GETFPREGS;
		SCARG(&npa, pid) = SCARG(uap, pid);
		SCARG(&npa, addr) = (void *)&nrp->fpregs;
		if ((error = sys_ptrace(l, &npa, retval)) != 0)
			return error;
#endif
		netbsd_to_freebsd_ptrace_regs(&nrp->regs, &nrp->fpregs, &fr);
		switch (SCARG(uap, req)) {
		case FREEBSD_PT_READ_U:
			return freebsd_ptrace_getregs(&fr, SCARG(uap, addr),
						      retval);

		case FREEBSD_PT_WRITE_U:
			error = freebsd_ptrace_setregs(&fr,
			    SCARG(uap, addr), SCARG(uap, data));
			if (error)
				return error;
			freebsd_to_netbsd_ptrace_regs(&fr,
						&nrp->regs, &nrp->fpregs);
#ifdef PT_SETREGS
			SCARG(&npa, req) = PT_SETREGS;
			SCARG(&npa, pid) = SCARG(uap, pid);
			SCARG(&npa, addr) = (void *)&nrp->regs;
			if ((error = sys_ptrace(l, &npa, retval)) != 0)
				return error;
#endif
#ifdef PT_SETFPREGS
			SCARG(&npa, req) = PT_SETFPREGS;
			SCARG(&npa, pid) = SCARG(uap, pid);
			SCARG(&npa, addr) = (void *)&nrp->fpregs;
			if ((error = sys_ptrace(l, &npa, retval)) != 0)
				return error;
#endif
			return 0;
		}
    }
#endif

	default:			/* It was not a legal request. */
		return (EINVAL);
	}

#ifdef DIAGNOSTIC
	panic("freebsd_ptrace: impossible");
#endif
#else
	return (ENOSYS);
#endif /* PTRACE || _LKM */
}
