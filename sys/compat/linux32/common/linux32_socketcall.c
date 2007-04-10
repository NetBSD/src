/*	$NetBSD: linux32_socketcall.c,v 1.1.32.1 2007/04/10 13:26:25 ad Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: linux32_socketcall.c,v 1.1.32.1 2007/04/10 13:26:25 ad Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ucred.h>

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

#define s(f) sizeof(struct s1(f))
#define s1(f) f ## _args

static const struct {
	const char *name;
	int argsize;
	int (*syscall)(struct lwp *, void *, register_t *);
} linux32_socketcall[LINUX32_MAX_SOCKETCALL+1] = {
	{"invalid",	-1, NULL},
	{"socket",	s(linux32_sys_socket), linux32_sys_socket},
	{"bind",	s(linux32_sys_bind), linux32_sys_bind},
	{"connect",	s(linux32_sys_connect), linux32_sys_connect},
	{"listen",	s(netbsd32_listen), netbsd32_listen},
	{"accept",	s(linux32_sys_accept), linux32_sys_accept},
	{"getsockname",	s(linux32_sys_getsockname), linux32_sys_getsockname},
	{"getpeername",	s(linux32_sys_getpeername), linux32_sys_getpeername},
	{"socketpair",	s(linux32_sys_socketpair), linux32_sys_socketpair},
	{"send",	s(linux32_sys_send), linux32_sys_send},
	{"recv",	s(linux32_sys_recv), linux32_sys_recv},
	{"sendto",	s(linux32_sys_sendto), linux32_sys_sendto},
	{"recvfrom",	s(linux32_sys_recvfrom), linux32_sys_recvfrom},
	{"shutdown",	s(netbsd32_shutdown), netbsd32_shutdown},
	{"setsockopt",	s(linux32_sys_setsockopt), linux32_sys_setsockopt},
	{"getsockopt",	s(linux32_sys_getsockopt), linux32_sys_getsockopt},
	{"sendmsg",	s(linux32_sys_sendmsg), linux32_sys_sendmsg},
	{"recvmsg",	s(linux32_sys_recvmsg), linux32_sys_recvmsg},
};
#undef s

int
linux32_sys_socketcall(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_socketcall_args /* {
		syscallarg(int) what;
		syscallarg(netbsd32_voidp) args;
	} */ *uap = v;
	union linux32_socketcall_args ua;
	int error;

	if (SCARG(uap, what) < 0 || SCARG(uap, what) > LINUX32_MAX_SOCKETCALL)
		return ENOSYS;

	if ((error = copyin(SCARG_P32(uap, args), &ua,
	    linux32_socketcall[SCARG(uap, what)].argsize)) != 0)
		return error;

	return linux32_socketcall[SCARG(uap, what)].syscall(l, &ua, retval);
}
