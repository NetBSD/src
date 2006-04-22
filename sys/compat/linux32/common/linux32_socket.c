/*	$NetBSD: linux32_socket.c,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $ */

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

__KERNEL_RCSID(0, "$NetBSD: linux32_socket.c,v 1.1.10.2 2006/04/22 11:38:14 simonb Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/fstypes.h>
#include <sys/signal.h>
#include <sys/dirent.h>
#include <sys/kernel.h>
#include <sys/fcntl.h>
#include <sys/select.h>
#include <sys/sa.h>
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
linux32_sys_socketpair(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(netbsd32_intp) rsv;
	} */ *uap = v;
	struct linux_sys_socketpair_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);
	NETBSD32TOP_UAP(rsv, int)

	return linux_sys_socketpair(l, &ua, retval);
}

int
linux32_sys_sendto(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_sendto_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) msg; 
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_osockaddrp_t) to;
		syscallarg(int) tolen;
	} */ *uap = v;
	struct linux_sys_sendto_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(msg, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(to, struct osockaddr);
	NETBSD32TO64_UAP(tolen);

	return linux_sys_sendto(l, &ua, retval);
}


int
linux32_sys_recvfrom(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf; 
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_osockaddrp_t) from;
		syscallarg(netbsd32_intp) fromlenaddr;
	} */ *uap = v;
	struct linux_sys_recvfrom_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	NETBSD32TOP_UAP(from, struct osockaddr);
	NETBSD32TOP_UAP(fromlenaddr, unsigned int);

	return linux_sys_recvfrom(l, &ua, retval);
}

int
linux32_sys_setsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(netbsd32_voidp) optval;
		syscallarg(int) optlen;
	} */ *uap = v;
	struct linux_sys_setsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(optname);
	NETBSD32TOP_UAP(optval, void);
	NETBSD32TO64_UAP(optlen);

	return linux_sys_setsockopt(l, &ua, retval);
}


int
linux32_sys_getsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(netbsd32_voidp) optval;
		syscallarg(netbsd32_intp) optlen;
	} */ *uap = v;
	struct linux_sys_getsockopt_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TO64_UAP(level);
	NETBSD32TO64_UAP(optname);
	NETBSD32TOP_UAP(optval, void);
	NETBSD32TOP_UAP(optlen, int);

	return linux_sys_getsockopt(l, &ua, retval);
}

int
linux32_sys_socket(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct linux_sys_socket_args ua;

	NETBSD32TO64_UAP(domain);
	NETBSD32TO64_UAP(type);
	NETBSD32TO64_UAP(protocol);

	return linux_sys_socket(l, &ua, retval);
}

int
linux32_sys_bind(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_bind_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_osockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct linux_sys_bind_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct osockaddr)
	NETBSD32TO64_UAP(namelen);

	return linux_sys_bind(l, &ua, retval);
}

int
linux32_sys_connect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_connect_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_osockaddrp_t) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct linux_sys_connect_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct osockaddr)
	NETBSD32TO64_UAP(namelen);

	printf("linux32_sys_connect: s = %d, name = %p, namelen = %d\n",
		SCARG(&ua, s), SCARG(&ua, name), SCARG(&ua, namelen));

	return linux_sys_connect(l, &ua, retval);
}

int
linux32_sys_accept(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_accept_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_osockaddrp_t) name;
		syscallarg(netbsd32_intp) anamelen;
	} */ *uap = v;
	struct linux_sys_accept_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(name, struct osockaddr)
	NETBSD32TOP_UAP(anamelen, int);

	return linux_sys_accept(l, &ua, retval);
}

int
linux32_sys_getpeername(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(netbsd32_sockaddrp_t) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct linux_sys_getpeername_args ua;

	NETBSD32TO64_UAP(fdes);
	NETBSD32TOP_UAP(asa, struct sockaddr)
	NETBSD32TOP_UAP(alen, int);

	return linux_sys_getpeername(l, &ua, retval);
}

int
linux32_sys_getsockname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_getsockname_args /* {
		syscallarg(int) fdec;
		syscallarg(netbsd32_charp) asa;
		syscallarg(netbsd32_intp) alen;
	} */ *uap = v;
	struct linux_sys_getsockname_args ua;

	NETBSD32TO64_UAP(fdec);
	NETBSD32TOP_UAP(asa, char)
	NETBSD32TOP_UAP(alen, int);

	return linux_sys_getsockname(l, &ua, retval);
}

int
linux32_sys_sendmsg(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_sendmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct linux_sys_sendmsg_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(msg, struct msghdr);
	NETBSD32TO64_UAP(flags);

	return linux_sys_sendmsg(l, &ua, retval);
}

int
linux32_sys_recvmsg(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_recvmsg_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct linux_sys_recvmsg_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(msg, struct msghdr);
	NETBSD32TO64_UAP(flags);

	return linux_sys_recvmsg(l, &ua, retval);
}

int
linux32_sys_send(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_send_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_sendto_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	SCARG(&ua, to) = NULL;
	SCARG(&ua, tolen) = 0;

	return sys_sendto(l, &ua, retval);
}

int
linux32_sys_recv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux32_sys_recv_args /* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_recvfrom_args ua;

	NETBSD32TO64_UAP(s);
	NETBSD32TOP_UAP(buf, void);
	NETBSD32TO64_UAP(len);
	NETBSD32TO64_UAP(flags);
	SCARG(&ua, from) = NULL;
	SCARG(&ua, fromlenaddr) = NULL;

	return sys_recvfrom(l, &ua, retval);
}
