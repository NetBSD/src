/*	$NetBSD: linux_socketcall.h,v 1.16.22.2 2017/12/03 11:36:55 jdolecek Exp $	*/

/*-
 * Copyright (c) 1995, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Frank van der Linden and Eric Haszlakiewicz.
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

/*
 * Copyright (c) 1995 Frank van der Linden
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed for the NetBSD Project
 *      by Frank van der Linden
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LINUX_SOCKETCALL_H
#define _LINUX_SOCKETCALL_H

/* Alpha does not use the socketcall multiplexer */
#if !defined(__alpha__) && !defined(__amd64__) && !defined(COMPAT_LINUX32)
/* Used on: arm, i386, m68k, mips, ppc, sparc, sparc64 */
/* Used for COMPAT_LINUX32 on amd64 */
/* Not used on: alpha */
#include <compat/linux/linux_syscall.h>
#include <compat/linux/linux_syscallargs.h>

/*
 * Values passed to the Linux socketcall() syscall, determining the actual
 * action to take.
 */

#define LINUX_SYS_SOCKET	1
#define LINUX_SYS_BIND		2
#define LINUX_SYS_CONNECT	3
#define LINUX_SYS_LISTEN	4
#define LINUX_SYS_ACCEPT	5
#define LINUX_SYS_GETSOCKNAME	6
#define LINUX_SYS_GETPEERNAME	7
#define LINUX_SYS_SOCKETPAIR	8
#define LINUX_SYS_SEND		9
#define LINUX_SYS_RECV		10
#define LINUX_SYS_SENDTO	11
#define LINUX_SYS_RECVFROM	12
#define LINUX_SYS_SHUTDOWN	13
#define LINUX_SYS_SETSOCKOPT	14
#define LINUX_SYS_GETSOCKOPT	15
#define LINUX_SYS_SENDMSG	16
#define LINUX_SYS_RECVMSG	17
#define LINUX_SYS_ACCEPT4	18
#define LINUX_SYS_RECVMMSG	19
#define LINUX_SYS_SENDMMSG	20

#define LINUX_MAX_SOCKETCALL	20


/*
 * Structures for the arguments of the different system calls. This looks
 * a little better than copyin() of all values one by one.
 */

/* !!!: This should be at least as large as any other struct here. */
struct linux_socketcall_dummy_args {
	int dummy_ints[4];		/* Max 4 ints */
	void *dummy_ptrs[3];		/* Max 3 pointers */
};

#ifndef LINUX_SYS_socket
struct linux_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};
#endif

#ifndef LINUX_SYS_socketpair
struct linux_sys_socketpair_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
	syscallarg(int *) rsv;
};
#endif

#ifndef LINUX_SYS_sendto
struct linux_sys_sendto_args {
	syscallarg(int) s;
	syscallarg(void *) msg;
	syscallarg(int) len;
	syscallarg(int) flags;
	syscallarg(struct osockaddr *) to;
	syscallarg(int) tolen;
};
#endif

#ifndef LINUX_SYS_recvfrom
struct linux_sys_recvfrom_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
	syscallarg(struct osockaddr *) from;
	syscallarg(int *) fromlenaddr;
};
#endif

#ifndef LINUX_SYS_setsockopt
struct linux_sys_setsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(void *) optval;
	syscallarg(int) optlen;
};
#endif

#ifndef LINUX_SYS_getsockopt
struct linux_sys_getsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(void *) optval;
	syscallarg(int *) optlen;
};
#endif

#ifndef LINUX_SYS_bind
struct linux_sys_bind_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int) namelen;
};
#endif

#ifndef LINUX_SYS_connect
struct linux_sys_connect_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int) namelen;
};
#endif

#ifndef LINUX_SYS_accept
struct linux_sys_accept_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int *) anamelen;
};
#endif

#ifndef LINUX_SYS_getsockname
struct linux_sys_getsockname_args {
	syscallarg(int) fdes;
	syscallarg(struct osockaddr *) asa;
	syscallarg(int *) alen;
};
#endif

#ifndef LINUX_SYS_getpeername
struct linux_sys_getpeername_args {
	syscallarg(int) fdes;
	syscallarg(struct osockaddr *) asa;
	syscallarg(int *) alen;
};
#endif

#ifndef LINUX_SYS_sendmsg
struct linux_sys_sendmsg_args {
	syscallarg(int) s;
	syscallarg(struct linux_msghdr *) msg;
	syscallarg(u_int) flags;
};
#endif

#ifndef LINUX_SYS_recvmsg
struct linux_sys_recvmsg_args {
	syscallarg(int) s;
	syscallarg(struct linux_msghdr *) msg;
	syscallarg(u_int) flags;
};
#endif

#ifndef LINUX_SYS_send
struct linux_sys_send_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
};
#endif

#ifndef LINUX_SYS_recv
struct linux_sys_recv_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
};
#endif

#ifndef LINUX_SYS_accept4
struct linux_sys_accept4_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int *) anamelen;
	syscallarg(int) flags;
};
#endif

#ifndef LINUX_SYS_recvmmsg
struct linux_sys_recvmmsg_args {
	syscallarg(int) s;
	syscallarg(struct linux_mmsghdr *) msgvec;
	syscallarg(unsigned int) vlen;
	syscallarg(unsigned int) flags;
	syscallarg(struct linux_timespec *) timeout;
};
#endif

#ifndef LINUX_SYS_sendmmsg
struct linux_sys_sendmmsg_args {
	syscallarg(int) s;
	syscallarg(struct linux_mmsghdr *) msgvec;
	syscallarg(unsigned int) vlen;
	syscallarg(unsigned int) flags;
};
#endif

# ifdef _KERNEL
__BEGIN_DECLS
#define SYS_DEF(foo) int foo(struct lwp *, const struct foo##_args *, register_t *);
SYS_DEF(linux_sys_socket)
SYS_DEF(linux_sys_socketpair)
SYS_DEF(linux_sys_sendto)
SYS_DEF(linux_sys_recvfrom)
SYS_DEF(linux_sys_setsockopt)
SYS_DEF(linux_sys_getsockopt)
SYS_DEF(linux_sys_connect)
SYS_DEF(linux_sys_bind)
SYS_DEF(linux_sys_getsockname)
SYS_DEF(linux_sys_getpeername)
SYS_DEF(linux_sys_sendmsg)
SYS_DEF(linux_sys_recvmsg)
SYS_DEF(linux_sys_recv)
SYS_DEF(linux_sys_send)
SYS_DEF(linux_sys_accept)
SYS_DEF(linux_sys_accept4)
SYS_DEF(linux_sys_recvmmsg)
SYS_DEF(linux_sys_sendmmsg)
#undef SYS_DEF
__END_DECLS
# endif /* !_KERNEL */

# endif

#endif /* !_LINUX_SOCKETCALL_H */
