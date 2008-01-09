/*	$NetBSD: linux32_sched.c,v 1.3.20.2 2008/01/09 01:51:22 matt Exp $ */

/*-
 * Copyright (c) 2006 Emmanuel Dreyfus, all rights reserved.
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
 *	This product includes software developed by Emmanuel Dreyfus
 * 4. The name of the author may not be used to endorse or promote 
 *    products derived from this software without specific prior written 
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE THE AUTHOR AND CONTRIBUTORS ``AS IS'' 
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, 
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS 
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>

__KERNEL_RCSID(0, "$NetBSD: linux32_sched.c,v 1.3.20.2 2008/01/09 01:51:22 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/swap.h>

#include <machine/types.h>

#include <sys/syscallargs.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_conv.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_machdep.h>
#include <compat/linux/common/linux_misc.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

int
linux32_sys_clone(struct lwp *l, const struct linux32_sys_clone_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) flags;
		syscallarg(netbsd32_voidp) stack;
	} */
	struct linux_sys_clone_args ua;
	
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(stack, void *);
#ifdef LINUX_NPTL
	SCARG(&ua, parent_tidptr) = NULL;
	SCARG(&ua, child_tidptr) = NULL;
#endif

	return linux_sys_clone(l, &ua, retval);
}

int
linux32_sys_sched_getscheduler(struct lwp *l, const struct linux32_sys_sched_getscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
	} */
	struct linux_sys_sched_getscheduler_args ua;

	NETBSD32TO64_UAP(pid);

	return linux_sys_sched_getscheduler(l, &ua, retval);
}

int
linux32_sys_sched_setscheduler(struct lwp *l, const struct linux32_sys_sched_setscheduler_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(int) policy;
		syscallarg(const linux32_sched_paramp_t) sp;
	} */
	struct linux_sys_sched_setscheduler_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TO64_UAP(policy);
	NETBSD32TOP_UAP(sp, const struct linux_sched_param);

	return linux_sys_sched_setscheduler(l, &ua, retval);
}

int
linux32_sys_sched_getparam(struct lwp *l, const struct linux32_sys_sched_getparam_args *uap, register_t *retval)
{
	/* {
		syscallarg(pid_t) pid;
		syscallarg(linux32_sched_paramp_t *) sp;
	} */
	struct linux_sys_sched_getparam_args ua;

	NETBSD32TO64_UAP(pid);
	NETBSD32TOP_UAP(sp, struct linux_sched_param);
	
	return linux_sys_sched_getparam(l, &ua, retval);
}

int
linux32_sys_exit_group(struct lwp *l, const struct linux32_sys_exit_group_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) error_code;
	} */
	struct linux_sys_exit_group_args ua;

	NETBSD32TO64_UAP(error_code);

	return linux_sys_exit_group(l, &ua, retval);
}
