/*	$NetBSD: linux32_resource.c,v 1.6.8.1 2008/01/09 01:51:22 matt Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_resource.c,v 1.6.8.1 2008/01/09 01:51:22 matt Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/proc.h>
#include <sys/resourcevar.h>
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
#include <compat/linux/common/linux_limit.h>
#include <compat/linux/common/linux_oldolduname.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

int
linux32_sys_getrlimit(struct lwp *l, const struct linux32_sys_getrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */
	struct orlimit orl;
	int which;

	which = linux_to_bsd_limit(SCARG(uap, which));
	if (which < 0)
		return -which;

	bsd_to_linux_rlimit(&orl, &l->l_proc->p_rlimit[which]);

	return copyout(&orl, SCARG_P32(uap, rlp), sizeof(orl));
}

int
linux32_sys_setrlimit(struct lwp *l, const struct linux32_sys_setrlimit_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */
	struct rlimit rl;
	struct orlimit orl;
	int error;
	int which;

	if ((error = copyin(SCARG_P32(uap, rlp), &orl, sizeof(orl))) != 0)
		return error;

	which = linux_to_bsd_limit(SCARG(uap, which));
	if (which < 0)
		return -which;

	linux_to_bsd_rlimit(&rl, &orl);

	return dosetrlimit(l, l->l_proc, which, &rl);
}

int
linux32_sys_ugetrlimit(struct lwp *l, const struct linux32_sys_ugetrlimit_args *uap, register_t *retval)
{
	return linux32_sys_getrlimit(l, (const void *)uap, retval);
}

int
linux32_sys_getpriority(struct lwp *l, const struct linux32_sys_getpriority_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) who;
	} */
	struct sys_getpriority_args bsa;
	int error;
		 
	SCARG(&bsa, which) = SCARG(uap, which);
	SCARG(&bsa, who) = SCARG(uap, who);

	if ((error = sys_getpriority(l, &bsa, retval)))
		return error;
   
	*retval = NZERO - *retval;
	
	return 0;
} 

int
linux32_sys_setpriority(struct lwp *l, const struct linux32_sys_setpriority_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) which;
		syscallarg(int) who;
		syscallarg(int) prio;
	} */
	struct sys_setpriority_args bsa;
		 
	SCARG(&bsa, which) = SCARG(uap, which);
	SCARG(&bsa, who) = SCARG(uap, who);
	SCARG(&bsa, prio) = SCARG(uap, prio);

	return sys_setpriority(l, &bsa, retval);
} 
