/*	$NetBSD: linux32_dirent.c,v 1.1.16.4 2008/01/21 09:41:34 yamt Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_dirent.c,v 1.1.16.4 2008/01/21 09:41:34 yamt Exp $");

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
linux32_sys_getdents(struct lwp *l, const struct linux32_sys_getdents_args *uap, register_t *retval)
{
	/* {
		syscallcarg(int) fd;
		syscallcarg(linux32_direntp_t) dent;
		syscallcarg(unsigned int) count;
	} */
	struct linux_sys_getdents_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(dent, struct linux_dirent);
	NETBSD32TO64_UAP(count);

	return linux_sys_getdents(l, &ua, retval);
}


int
linux32_sys_getdents64(struct lwp *l, const struct linux32_sys_getdents64_args *uap, register_t *retval)
{
	/* {
		syscallcarg(int) fd;
		syscallcarg(linux32_dirent64p_t) dent;
		syscallcarg(unsigned int) count;
	} */
	struct linux_sys_getdents64_args ua;

	NETBSD32TO64_UAP(fd);
	NETBSD32TOP_UAP(dent, struct linux_dirent64);
	NETBSD32TO64_UAP(count);

	return linux_sys_getdents64(l, &ua, retval);
}

int
linux32_sys_readdir(struct lwp *l, const struct linux32_sys_readdir_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(struct linux32_direntp_t) dent;
		syscallarg(unsigned int) count;
	} */
	int error;
	struct linux32_sys_getdents_args da;

	SCARG(&da, fd) = SCARG(uap, fd);
	SCARG(&da, dent) = SCARG(uap, dent);
	SCARG(&da, count) = 1;

	error = linux32_sys_getdents(l, &da, retval);
	if (error == 0 && *retval > 1)
		*retval = 1;

	return error;
}
