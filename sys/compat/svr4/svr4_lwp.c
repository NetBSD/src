/*	$NetBSD: svr4_lwp.c,v 1.16 2007/12/20 23:03:05 dsl Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: svr4_lwp.c,v 1.16 2007/12/20 23:03:05 dsl Exp $");

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
#include <sys/syscallargs.h>
#include <sys/un.h>
#include <sys/stat.h>

#include <compat/svr4/svr4_types.h>
#include <compat/svr4/svr4_util.h>
#include <compat/svr4/svr4_socket.h>
#include <compat/svr4/svr4_signal.h>
#include <compat/svr4/svr4_sockmod.h>
#include <compat/svr4/svr4_lwp.h>
#include <compat/svr4/svr4_ucontext.h>
#include <compat/svr4/svr4_syscallargs.h>


int
svr4_sys__lwp_self(struct lwp *l, const void *uap, register_t *retval)
{
	return sys__lwp_self(l, uap, retval);
}

int
svr4_sys__lwp_create(struct lwp *l, const struct svr4_sys__lwp_create_args *uap, register_t *retval)
{
	struct sys__lwp_create_args lc;
	int flags;

	flags = 0;

	if (SCARG(uap, flags) & SVR4_LWP_DETACHED)
	    flags  &= LWP_DETACHED;

	if (SCARG(uap, flags) & SVR4_LWP_SUSPENDED)
	    flags  &= LWP_SUSPENDED;

	if (SCARG(uap, flags) & SVR4___LWP_ASLWP) {
		/* XXX Punt! */
	}


	/* XXX At the moment, svr4_ucontext_t and ucontext_t are the same */
	SCARG(&lc, ucp) = (ucontext_t *)SCARG(uap, uc);
	SCARG(&lc, flags) = flags;
	SCARG(&lc, new_lwp) = (lwpid_t *)SCARG(uap, lwpid);


	return sys__lwp_create(l, &lc, retval);
}

int
svr4_sys__lwp_kill(struct lwp *l, const struct svr4_sys__lwp_kill_args *uap, register_t *retval)
{
	struct sys_kill_args ap;
	SCARG(&ap, pid) = SCARG(uap, lwpid);
	SCARG(&ap, signum) = SCARG(uap, signum);

	/* XXX NJWLWP */
	return sys_kill(l, &ap, retval);
}

int
svr4_sys__lwp_info(struct lwp *l, const struct svr4_sys__lwp_info_args *uap, register_t *retval)
{
	struct svr4_lwpinfo lwpinfo;
	int error;

	/* XXX NJWLWP */
	TIMEVAL_TO_TIMESPEC(&l->l_proc->p_stats->p_ru.ru_stime, &lwpinfo.lwp_stime);
	TIMEVAL_TO_TIMESPEC(&l->l_proc->p_stats->p_ru.ru_utime, &lwpinfo.lwp_utime);

	if ((error = copyout(&lwpinfo, SCARG(uap, lwpinfo), sizeof(lwpinfo))) ==
	    -1)
	       return error;
	return 0;
}

int
svr4_sys__lwp_exit(struct lwp *l, const void *v, register_t *retval)
{

	return sys__lwp_exit(l, NULL, retval);
}

int
svr4_sys__lwp_wait(struct lwp *l, const struct svr4_sys__lwp_wait_args *uap, register_t *retval)
{
	struct sys__lwp_wait_args ap;

	SCARG(&ap, wait_for) = SCARG(uap, wait_for);
	SCARG(&ap, departed) = (lwpid_t *)SCARG(uap, departed_lwp);

	return sys__lwp_wait(l, &ap, retval);
}

int
svr4_sys__lwp_suspend(struct lwp *l, const struct svr4_sys__lwp_suspend_args *uap, register_t *retval)
{
	struct sys__lwp_suspend_args ap;

	SCARG(&ap, target) = SCARG(uap, lwpid);

	return sys__lwp_suspend(l, &ap, retval);
}


int
svr4_sys__lwp_continue(struct lwp *l, const struct svr4_sys__lwp_continue_args *uap, register_t *retval)
{
	struct sys__lwp_continue_args ap;

	SCARG(&ap, target) = SCARG(uap, lwpid);

	return sys__lwp_continue(l, &ap, retval);
}

int
svr4_sys__lwp_getprivate(struct lwp *l, const void *v, register_t *retval)
{
	/* XXX NJWLWP: Replace with call to native version if we ever
	 * implement that. */

	*retval = (register_t)l->l_private;
	return 0;
}

int
svr4_sys__lwp_setprivate(struct lwp *l, const struct svr4_sys__lwp_setprivate_args *uap, register_t *retval)
{

	/* XXX NJWLWP: Replace with call to native version if we ever
	 * implement that. */

	return copyin(SCARG(uap, buffer), &l->l_private, sizeof(void *));
}
