/*	$NetBSD: netbsd32_wait.c,v 1.22.14.1 2016/10/05 20:55:39 skrll Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_wait.c,v 1.22.14.1 2016/10/05 20:55:39 skrll Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/ptrace.h>
#include <sys/resourcevar.h>
#include <sys/vnode.h>
#include <sys/pool.h>
#include <sys/proc.h>
#include <sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

int
netbsd32___wait450(struct lwp *l, const struct netbsd32___wait450_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */
	int error, status, pid = SCARG(uap, pid);
	struct netbsd32_rusage ru32;
	struct rusage ru;

	error = do_sys_wait(&pid, &status, SCARG(uap, options),
	    SCARG_P32(uap, rusage) != NULL ? &ru : NULL);

	retval[0] = pid;
	if (pid == 0)
		return error;

	if (SCARG_P32(uap, rusage)) {
		netbsd32_from_rusage(&ru, &ru32);
		error = copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
	}

	if (error == 0 && SCARG_P32(uap, status))
		error = copyout(&status, SCARG_P32(uap, status), sizeof(status));

	return error;
}

int
netbsd32_wait6(struct lwp *l, const struct netbsd32_wait6_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(idtype_t) idtype;
		syscallarg(id_t) id;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_wrusagep_t) wru;
		syscallarg(netbsd32_siginfop_t) info;
	} */
	idtype_t idtype = SCARG(uap, idtype);
	id_t id = SCARG(uap, id);
	struct wrusage wru, *wrup;
	siginfo_t si, *sip;
	int status;
	int pid;

	if (SCARG_P32(uap, wru) != NULL)
		wrup = &wru;
	else
		wrup = NULL;

	if (SCARG_P32(uap, info) != NULL)
		sip = &si;
	else
		sip = NULL;

	/*
	 *  We expect all callers of wait6() to know about WEXITED and
	 *  WTRAPPED.
	 */
	int error = do_sys_waitid(idtype, id, &pid, &status,
	    SCARG(uap, options), wrup, sip);

	retval[0] = pid; 	/* tell userland who it was */

#if 0
	/*
	 * should we copyout if there was no process, hence no useful data?
	 * We don't for an old sytle wait4() (etc) but I believe
	 * FreeBSD does for wait6(), so a tossup...  Go with FreeBSD for now.
	 */
	if (pid == 0)
		return error;
#endif


	if (error == 0 && SCARG_P32(uap, status))
		error = copyout(&status, SCARG_P32(uap, status),
		    sizeof(status));
	if (wrup != NULL && error == 0) {
		struct netbsd32_wrusage wru32;

		netbsd32_from_rusage(&wrup->wru_self, &wru32.wru_self);
		netbsd32_from_rusage(&wrup->wru_children, &wru32.wru_children);
		error = copyout(&wru32, SCARG_P32(uap, wru), sizeof(wru32));
	}
	if (sip != NULL && error == 0) {
	    	siginfo32_t si32;

		netbsd32_si_to_si32(&si32, sip);
		error = copyout(&si32, SCARG_P32(uap, info), sizeof(si32));
	}

	return error;
}

int
netbsd32___getrusage50(struct lwp *l,
    const struct netbsd32___getrusage50_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) who;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */
	int error;
	struct proc *p = l->l_proc;
	struct rusage ru;
	struct netbsd32_rusage ru32;

	error = getrusage1(p, SCARG(uap, who), &ru);
	if (error != 0)
		return error;

	netbsd32_from_rusage(&ru, &ru32);
	return copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
}
