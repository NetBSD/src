/*	$NetBSD: svr4_32_lwp.c,v 1.2.4.2 2002/05/29 21:33:04 nathanw Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
__KERNEL_RCSID(0, "$NetBSD: svr4_32_lwp.c,v 1.2.4.2 2002/05/29 21:33:04 nathanw Exp $");

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/queue.h>
#include <sys/mbuf.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <sys/resourcevar.h>
#include <sys/proc.h>
#include <sys/user.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sa.h>
#include <sys/syscallargs.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <compat/svr4_32/svr4_32_types.h>
#include <compat/svr4_32/svr4_32_util.h>
#include <compat/svr4_32/svr4_32_socket.h>
#include <compat/svr4_32/svr4_32_signal.h>
#include <compat/svr4/svr4_sockmod.h>
#include <compat/svr4_32/svr4_32_lwp.h>
#include <compat/svr4_32/svr4_32_ucontext.h>
#include <compat/svr4_32/svr4_32_syscallargs.h>


#if 0
int
svr4_32_sys__lwp_self(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	return sys_getpid(p, v, retval);
}
#endif

int
svr4_32_sys__lwp_create(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_create_args *uap = v;
	int error;
	struct proc *pt;
	svr4_32_ucontext_t uc;
#define SVR4_FORK_FLAGS \
    (FORK_SHAREVM|FORK_SHARECWD|FORK_SHAREFILES|FORK_SHARESIGS)


	if ((error = fork1(p, SVR4_FORK_FLAGS, SIGCHLD, NULL, 0,
	     NULL, NULL, retval, &pt)) == -1)
		return error;

	if (SCARG(uap, flags) & SVR4_LWP_DETACHED)
	    pt->p_flag &= ~P_CONTROLT;

	if (SCARG(uap, flags) & SVR4_LWP_SUSPENDED)
	    pt->p_stat = SSTOP;

	if (SCARG(uap, flags) & SVR4___LWP_ASLWP) {
		/*
		 * XXX: This does not really work, we don't have
		 *	the facility to deliver all async signals
		 *	to a single lwp, and also we don't keep
		 *	track of having only one ASLWP. For now
		 *	we just block all signals as we are supposed
		 *	to.
		 */
		sigset_t ss;
		sigfillset(&ss);
		(void)sigprocmask1(pt, SIG_BLOCK, &ss, 0);
	} else {
		/*
		 * XXX: We block all signals to the rest of the of
		 * 	the lwp's, so that they don't get confused
		 *	as of the above. Will that work? What does
		 *	sharing s
		 */
		sigset_t ss;
		sigfillset(&ss);
		(void)sigprocmask1(pt, SIG_BLOCK, &ss, 0);
	}

	if ((error = copyin((caddr_t)(u_long)SCARG(uap, uc), &uc, sizeof(uc))) != 0)
		return error;

	if ((error = svr4_32_setcontext(pt, &uc)) != 0)
		return error;

	if ((error = copyout(&pt->p_pid, (caddr_t)(u_long)SCARG(uap, lwpid),
	    sizeof(svr4_lwpid_t))) == -1)
		return error;
	return 0;
}

int
svr4_32_sys__lwp_kill(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_kill_args *uap = v;
	struct sys_kill_args ap;
	SCARG(&ap, pid) = SCARG(uap, lwpid);
	SCARG(&ap, signum) = SCARG(uap, signum);

	return sys_kill(p, &ap, retval);
}

int
svr4_32_sys__lwp_info(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_info_args *uap = v;
	struct svr4_32_lwpinfo lwpinfo;
	int error;

	TIMEVAL_TO_TIMESPEC(&p->p_stats->p_ru.ru_stime, &lwpinfo.lwp_stime);
	TIMEVAL_TO_TIMESPEC(&p->p_stats->p_ru.ru_utime, &lwpinfo.lwp_utime);

	if ((error = copyout(&lwpinfo, (caddr_t)(u_long)SCARG(uap, lwpinfo), 
			     sizeof(lwpinfo))) == -1)
	       return error;
	return 0;
}

int
svr4_32_sys__lwp_exit(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct sys_exit_args ap;
	int error;

	/* XXX: We don't handle the suspended case correctly here */
	SCARG(&ap, rval) = 0;

	if ((error = sys_exit(p, &ap, retval)) == -1)
		return error;

	*retval = 0;
	return 0;
}

int
svr4_32_sys__lwp_wait(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_wait_args *uap = v;
	struct sys_wait4_args ap;
	int error;

	SCARG(&ap, pid) = SCARG(uap, wait_for);
	SCARG(&ap, status) = NULL;
	SCARG(&ap, options) = 0;

	if ((error = sys_wait4(p, &ap, retval)) == -1)
		return error;

	if (SCARG(uap, departed_lwp) != NULL)
	    if ((error = copyout(retval, (caddr_t)(u_long)SCARG(uap, departed_lwp),
		sizeof(svr4_lwpid_t))) == -1)
		    return error;

	*retval = 0;
	return 0;
}

/* XXX Stolen from kern_sig.c */
#define CANSIGNAL(p, pc, q, signum) \
	((pc)->pc_ucred->cr_uid == 0 || \
	    (pc)->p_ruid == (q)->p_cred->p_ruid || \
	    (pc)->pc_ucred->cr_uid == (q)->p_cred->p_ruid || \
	    (pc)->p_ruid == (q)->p_ucred->cr_uid || \
	    (pc)->pc_ucred->cr_uid == (q)->p_ucred->cr_uid || \
	    ((signum) == SIGCONT && (q)->p_session == (p)->p_session))
int
svr4_32_sys__lwp_suspend(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_suspend_args *uap = v;
	struct proc *pt;

	/* Security implications here! */
	if ((pt = pfind(SCARG(uap, lwpid))) == NULL)
		return ESRCH;

	if (!CANSIGNAL(p, p->p_cred, pt, 0))
		return EPERM;

	pt->p_stat = SSTOP;
	return 0;
}

int
svr4_32_sys__lwp_continue(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_continue_args *uap = v;
	struct proc *pt;

	if ((pt = pfind(SCARG(uap, lwpid))) == NULL)
		return ESRCH;

	if (!CANSIGNAL(p, p->p_cred, pt, 0))
		return EPERM;

	pt->p_stat = SRUN;
	return 0;
}

int
svr4_32_sys__lwp_getprivate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	/* XXX: Use mach field! */
	*retval = (register_t)p->p_thread;
	return 0;
}

int
svr4_32_sys__lwp_setprivate(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct svr4_32_sys__lwp_setprivate_args *uap = v;

	/* 
	 * XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	 *
	 * This has many problems:
	 *
	 * 1) we are using an old, left over field from Mach
	 *  
	 * 2) the lwpid_t is an unsigned int but the p_thread
	 *	field is a (64-bit) pointer.
	 *
	 * 3) we're copying in 64-bits from a 32-bit field.
	 */
	return copyin((caddr_t)(u_long)SCARG(uap, buffer), 
		      &p->p_thread, sizeof(void *));
	return 0;
}
