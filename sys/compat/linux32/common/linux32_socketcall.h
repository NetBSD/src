/*	$NetBSD: linux32_socketcall.h,v 1.2.4.1 2008/01/02 21:52:52 bouyer Exp $ */

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
#ifndef _LINUX32_SOCKETCALL_H
#define _LINUX32_SOCKETCALL_H

#define LINUX32_MAX_SOCKETCALL    17

#ifdef  syscallarg
#undef  syscallarg
#endif

#define syscallarg(x)							\
	union {								\
		register32_t pad;					\
									\
		struct { x datum; } le;					\
		struct { /* LINTED zero array dimension */		\
			int8_t pad[  /* CONSTCOND */			\
				(sizeof (register32_t) < sizeof (x))	\
				? 0					\
				: sizeof (register32_t) - sizeof (x)];	\
			x datum;					\
		} be;							\
	}

struct linux32_sys_socket_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
};

struct linux32_sys_socketpair_args {
	syscallarg(int) domain;
	syscallarg(int) type;
	syscallarg(int) protocol;
	syscallarg(netbsd32_intp) rsv;
};

struct linux32_sys_sendto_args {
	syscallarg(int) s;
	syscallarg(netbsd32_voidp) msg;
	syscallarg(int) len;
	syscallarg(int) flags;
	syscallarg(netbsd32_sockaddrp_t) to;
	syscallarg(int) tolen;
};

struct linux32_sys_recvfrom_args {
	syscallarg(int) s;
	syscallarg(netbsd32_voidp) buf;
	syscallarg(netbsd32_size_t) len;
	syscallarg(int) flags;
	syscallarg(netbsd32_osockaddrp_t) from;
	syscallarg(netbsd32_intp) fromlenaddr;
};

struct linux32_sys_setsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(netbsd32_voidp) optval;
	syscallarg(int) optlen;
};

struct linux32_sys_getsockopt_args {
	syscallarg(int) s;
	syscallarg(int) level;
	syscallarg(int) optname;
	syscallarg(netbsd32_voidp) optval;
	syscallarg(netbsd32_intp) optlen;
};

struct linux32_sys_bind_args {
	syscallarg(int) s;
	syscallarg(netbsd32_osockaddrp_t) name;
	syscallarg(int) namelen;
};

struct linux32_sys_connect_args {
	syscallarg(int) s;
	syscallarg(netbsd32_osockaddrp_t) name;
	syscallarg(int) namelen;
};

struct linux32_sys_accept_args {
	syscallarg(int) s;
	syscallarg(netbsd32_osockaddrp_t) name;
	syscallarg(netbsd32_intp) anamelen;
};

struct linux32_sys_getsockname_args {
	syscallarg(int) fdec;
	syscallarg(netbsd32_charp) asa;
	syscallarg(netbsd32_intp) alen;
};

struct linux32_sys_getpeername_args {
	syscallarg(int) fdes;
	syscallarg(netbsd32_sockaddrp_t) asa;
	syscallarg(netbsd32_intp) alen;
};

struct linux32_sys_sendmsg_args {
	syscallarg(int) s;
	syscallarg(netbsd32_msghdrp_t) msg;
	syscallarg(int) flags;
};

struct linux32_sys_recvmsg_args {
	syscallarg(int) s;
	syscallarg(netbsd32_msghdrp_t) msg;
	syscallarg(int) flags;
};

struct linux32_sys_send_args {
	syscallarg(int) s;
	syscallarg(netbsd32_voidp) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
};

struct linux32_sys_recv_args {
	syscallarg(int) s;
	syscallarg(netbsd32_voidp) buf;
	syscallarg(int) len;
	syscallarg(int) flags;
};

union linux32_socketcall_args {
	struct 	linux_sys_socket_args socket_args;
	struct 	linux32_sys_bind_args bind_args;
	struct 	linux32_sys_connect_args connect_args;
	struct 	sys_listen_args listen_args;
	struct 	linux32_sys_accept_args accept_args;
	struct 	linux32_sys_getsockname_args getsockname_args;
	struct 	linux32_sys_getpeername_args getpeername_args;
	struct 	linux32_sys_socketpair_args socketpair_args;
	struct 	linux32_sys_send_args send_args;
	struct 	linux32_sys_recv_args recv_args;
	struct 	linux32_sys_sendto_args sendto_args;
	struct 	linux32_sys_recvfrom_args recvfrom_args;
	struct 	sys_shutdown_args shutdown_args;
	struct 	linux32_sys_setsockopt_args setsockopt_args;
	struct 	linux32_sys_getsockopt_args getsockopt_args;
	struct 	linux32_sys_sendmsg_args sendmsg_args;
	struct 	linux32_sys_recvmsg_args recvmsg_args;
};

# ifdef _KERNEL
__BEGIN_DECLS
#define SYS_DEF(foo) struct foo##_args; \
    int foo(struct lwp *, const struct foo##_args *, register_t *)
SYS_DEF(linux32_sys_socketpair);
SYS_DEF(linux32_sys_sendto);
SYS_DEF(linux32_sys_recvfrom);
SYS_DEF(linux32_sys_setsockopt);
SYS_DEF(linux32_sys_getsockopt);
SYS_DEF(linux32_sys_connect);
SYS_DEF(linux32_sys_socket);
SYS_DEF(linux32_sys_bind);
SYS_DEF(linux32_sys_getsockname);
SYS_DEF(linux32_sys_getpeername);
SYS_DEF(linux32_sys_sendmsg);
SYS_DEF(linux32_sys_recvmsg);
SYS_DEF(linux32_sys_recv);
SYS_DEF(linux32_sys_send);
SYS_DEF(linux32_sys_accept);
#undef SYS_DEF
__END_DECLS
# endif /* !_KERNEL */

#endif /* !_LINUX32_SOCKETCALL_H */
