/*	$NetBSD: linux32_socketcall.c,v 1.1.16.4 2008/01/21 09:41:36 yamt Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux32_socketcall.c,v 1.1.16.4 2008/01/21 09:41:36 yamt Exp $");

#include "opt_ktrace.h"
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/proc.h>
#include <sys/ucred.h>
#include <sys/ktrace.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/linux_syscallargs.h>

#include <compat/linux32/common/linux32_types.h>
#include <compat/linux32/common/linux32_signal.h>
#include <compat/linux32/common/linux32_machdep.h>
#include <compat/linux32/common/linux32_sysctl.h>
#include <compat/linux32/common/linux32_socketcall.h>
#include <compat/linux32/linux32_syscallargs.h>

#define sc(emul, fn) { #fn, sizeof (struct emul##_##fn##_args), \
	(int (*)(struct lwp *, const void *, register_t *))emul##_##fn }

static const struct {
	const char *name;
	int argsize;
	int (*syscall)(struct lwp *, const void *, register_t *);
} linux32_socketcall[LINUX32_MAX_SOCKETCALL+1] = {
	{"invalid",	-1, NULL},
	sc(linux32_sys, socket),
	sc(linux32_sys, bind),
	sc(linux32_sys, connect),
	sc(netbsd32, listen),
	sc(linux32_sys, accept),
	sc(linux32_sys, getsockname),
	sc(linux32_sys, getpeername),
	sc(linux32_sys, socketpair),
	sc(linux32_sys, send),
	sc(linux32_sys, recv),
	sc(linux32_sys, sendto),
	sc(linux32_sys, recvfrom),
	sc(netbsd32, shutdown),
	sc(linux32_sys, setsockopt),
	sc(linux32_sys, getsockopt),
	sc(linux32_sys, sendmsg),
	sc(linux32_sys, recvmsg),
};
#undef sc


int
linux32_sys_socketcall(struct lwp *l, const struct linux32_sys_socketcall_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) what;
		syscallarg(netbsd32_voidp) args;
	} */
	union linux32_socketcall_args ua;
	int error;

	if (SCARG(uap, what) < 0 || SCARG(uap, what) > LINUX32_MAX_SOCKETCALL)
		return ENOSYS;

	if ((error = copyin(SCARG_P32(uap, args), &ua,
	    linux32_socketcall[SCARG(uap, what)].argsize)) != 0)
		return error;

	/* Trace the socket-call arguments as 'GIO' on fd -1 */
	ktrkuser(linux32_socketcall[SCARG(uap, what)].name, &ua,
	    linux32_socketcall[SCARG(uap, what)].argsize);

	return linux32_socketcall[SCARG(uap, what)].syscall(l, &ua, retval);
}
