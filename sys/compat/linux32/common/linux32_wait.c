/*	$NetBSD: linux32_wait.c,v 1.3.10.1 2008/01/09 01:51:24 matt Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_wait.c,v 1.3.10.1 2008/01/09 01:51:24 matt Exp $");

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
#include <sys/wait.h>

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
linux32_sys_waitpid(struct lwp *l, const struct linux32_sys_waitpid_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
	} */
	struct linux32_sys_wait4_args ua;

	SCARG(&ua, pid) = SCARG(uap, pid);
	SCARG(&ua, status) = SCARG(uap, status);
	SCARG(&ua, options) = SCARG(uap, options);
	NETBSD32PTR32(SCARG(&ua, rusage), NULL);

	return linux32_sys_wait4(l, &ua, retval);	
}

int
linux32_sys_wait4(struct lwp *l, const struct linux32_sys_wait4_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) pid;
		syscallarg(netbsd32_intp) status;
		syscallarg(int) options;
		syscallarg(netbsd32_rusagep_t) rusage;
	} */
	int error, status, linux_options, options, was_zombie;
	struct rusage ru;
	struct netbsd32_rusage ru32;
	int pid;

	linux_options = SCARG(uap, options);
	options = WOPTSCHECKED;
	if (linux_options & ~(LINUX_WAIT4_KNOWNFLAGS))
		return EINVAL;

	if (linux_options & LINUX_WAIT4_WNOHANG)
		options |= WNOHANG;
	if (linux_options & LINUX_WAIT4_WUNTRACED)
		options |= WUNTRACED;
	if (linux_options & LINUX_WAIT4_WALL)
		options |= WALLSIG;
	if (linux_options & LINUX_WAIT4_WCLONE)
		options |= WALTSIG;

	pid = SCARG(uap, pid);
	error = do_sys_wait(l, &pid, &status, options,
	    SCARG_P32(uap, rusage) != NULL ? &ru : NULL, &was_zombie);
	retval[0] = pid;
	if (pid == 0)
		return error;

	/* XXXAD ksiginfo leak */
	sigdelset(&l->l_proc->p_sigpend.sp_set, SIGCHLD);

	if (SCARG_P32(uap, rusage) != NULL) {
		netbsd32_from_rusage(&ru, &ru32);
		error = copyout(&ru32, SCARG_P32(uap, rusage), sizeof(ru32));
	}

	if (error == 0 && SCARG_P32(uap, status) != NULL) {
		status = bsd_to_linux_wstat(status);
		error = copyout(&status, SCARG_P32(uap, status), sizeof(status));
	}

	return error;
}
