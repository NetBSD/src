/*	$NetBSD: linux_socketcall.h,v 1.15.14.1 2009/07/23 23:31:41 jym Exp $	*/

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

/*
 * Values passed to the Linux socketcall() syscall, determining the actual
 * action to take.
 */

#define LINUX_SYS_socket	1
#define LINUX_SYS_bind		2
#define LINUX_SYS_connect	3
#define LINUX_SYS_listen	4
#define LINUX_SYS_accept	5
#define LINUX_SYS_getsockname	6
#define LINUX_SYS_getpeername	7
#define LINUX_SYS_socketpair	8
#define LINUX_SYS_send		9
#define LINUX_SYS_recv		10
#define LINUX_SYS_sendto	11
#define LINUX_SYS_recvfrom	12
#define LINUX_SYS_shutdown	13
#define LINUX_SYS_setsockopt	14
#define LINUX_SYS_getsockopt	15
#define LINUX_SYS_sendmsg	16
#define LINUX_SYS_recvmsg	17

#define LINUX_MAX_SOCKETCALL	17


/*
 * Structures for the arguments of the different system calls. This looks
 * a little better than copyin() of all values one by one.
 */

/* !!!: This should be at least as large as any other struct here. */
struct linux_socketcall_dummy_args {
	int dummy_ints[4];		/* Max 4 ints */
	void *dummy_ptrs[3];		/* Max 3 pointers */
};

struct linux_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};

struct linux_sys_socketpair_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
	syscallarg(int *) rsv;
};

struct linux_sys_sendto_args {
	syscallarg(int) s;
	syscallarg(void *) msg;
	syscallarg(int) len;
	syscallarg(int) flags;
	syscallarg(struct osockaddr *) to;
	syscallarg(int) tolen;
};

struct linux_sys_recvfrom_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
	syscallarg(struct osockaddr *) from;
	syscallarg(int *) fromlenaddr;
};

struct linux_sys_setsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(void *) optval;
	syscallarg(int) optlen;
};

struct linux_sys_getsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(void *) optval;
	syscallarg(int *) optlen;
};

struct linux_sys_bind_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int) namelen;
};

struct linux_sys_connect_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int) namelen;
};

struct linux_sys_accept_args {
	syscallarg(int) s;
	syscallarg(struct osockaddr *) name;
	syscallarg(int *) anamelen;
};

struct linux_sys_getsockname_args {
	syscallarg(int) fdes;
	syscallarg(struct osockaddr *) asa;
	syscallarg(int *) alen;
};

struct linux_sys_getpeername_args {
	syscallarg(int) fdes;
	syscallarg(struct osockaddr *) asa;
	syscallarg(int *) alen;
};

struct linux_sys_sendmsg_args {
	syscallarg(int) s;
	syscallarg(struct linux_msghdr *) msg;
	syscallarg(u_int) flags;
};

struct linux_sys_recvmsg_args {
	syscallarg(int) s;
	syscallarg(struct linux_msghdr *) msg;
	syscallarg(u_int) flags;
};

struct linux_sys_send_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
};

struct linux_sys_recv_args {
	syscallarg(int) s;
	syscallarg(void *) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
};

/* These are only used for their size: */

struct linux_sys_listen_args {
	syscallarg(int) s;
	syscallarg(int) backlog;
};

struct linux_sys_shutdown_args {
	syscallarg(int) s;
	syscallarg(int) how;
};

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
#undef SYS_DEF
__END_DECLS
# endif /* !_KERNEL */

# endif

#endif /* !_LINUX_SOCKETCALL_H */
