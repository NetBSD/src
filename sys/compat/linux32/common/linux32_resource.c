/*	$NetBSD: linux32_resource.c,v 1.4.6.1 2007/03/29 19:27:38 reinoud Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_resource.c,v 1.4.6.1 2007/03/29 19:27:38 reinoud Exp $");

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
linux32_sys_getrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sys_getrlimit_args ap;
	struct orlimit orl;
	struct rlimit rl;
	int error;

	SCARG(&ap, which) = linux_to_bsd_limit(SCARG(uap, which));
	if ((error = SCARG(&ap, which)) < 0)
		return -error;

	SCARG(&ap, rlp) = stackgap_alloc(p, &sg, sizeof rl);

	if ((error = sys_getrlimit(l, &ap, retval)) != 0)
		return error;

	if ((error = copyin(SCARG(&ap, rlp), &rl, sizeof(rl))) != 0)
		return error;

	bsd_to_linux_rlimit(&orl, &rl);

	return copyout(&orl, SCARG_P32(uap, rlp), sizeof(orl));
}

int
linux32_sys_setrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_setrlimit_args /* {
		syscallarg(int) which;
		syscallarg(netbsd32_orlimitp_t) rlp;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sys_getrlimit_args ap;
	struct rlimit rl;
	struct orlimit orl;
	int error;

	SCARG(&ap, which) = linux_to_bsd_limit(SCARG(uap, which));
	SCARG(&ap, rlp) = stackgap_alloc(p, &sg, sizeof rl);
	if ((error = SCARG(&ap, which)) < 0)
		return -error;

	if ((error = copyin(SCARG_P32(uap, rlp), &orl, sizeof(orl))) != 0)
		return error;

	linux_to_bsd_rlimit(&rl, &orl);

	if ((error = copyout(&rl, SCARG(&ap, rlp), sizeof(rl))) != 0)
		return error;

	return sys_setrlimit(l, &ap, retval);
}

int
linux32_sys_ugetrlimit(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	return linux32_sys_getrlimit(l, v, retval);
}

int
linux32_sys_getpriority(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
	} */ *uap = v;
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
linux32_sys_setpriority(l, v, retval) 
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_setpriority_args /* {
		syscallarg(int) which;
		syscallarg(int) who;
		syscallarg(int) prio;
	} */ *uap = v;
	struct sys_setpriority_args bsa;
		 
	SCARG(&bsa, which) = SCARG(uap, which);
	SCARG(&bsa, who) = SCARG(uap, who);
	SCARG(&bsa, prio) = SCARG(uap, prio);

	return sys_setpriority(l, &bsa, retval);
} 
