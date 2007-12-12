/*	$NetBSD: darwin_socket.c,v 1.17 2007/12/12 21:37:31 dsl Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: darwin_socket.c,v 1.17 2007/12/12 21:37:31 dsl Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/lwp.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <sys/mbuf.h>
#include <sys/filedesc.h>
#include <sys/protosw.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/common/compat_util.h>

#include <compat/mach/mach_vm.h>

#include <compat/darwin/darwin_audit.h>
#include <compat/darwin/darwin_socket.h>
#include <compat/darwin/darwin_syscallargs.h>

unsigned char native_to_darwin_af[] = {
	0,
	DARWIN_AF_LOCAL,
	DARWIN_AF_INET,
	DARWIN_AF_IMPLINK,
	DARWIN_AF_PUP,
	DARWIN_AF_CHAOS,	/* 5 */
	DARWIN_AF_NS,
	DARWIN_AF_ISO,
	DARWIN_AF_ECMA,
	DARWIN_AF_DATAKIT,
	DARWIN_AF_CCITT,	/* 10 */
	DARWIN_AF_SNA,
	DARWIN_AF_DECnet,
	DARWIN_AF_DLI,
	DARWIN_AF_LAT,
	DARWIN_AF_HYLINK,	/* 15 */
	DARWIN_AF_APPLETALK,
	DARWIN_AF_ROUTE,
	DARWIN_AF_LINK,
	DARWIN_AF_XTP,
	DARWIN_AF_COIP,		/* 20 */
	DARWIN_AF_CNT,
	DARWIN_AF_RTIP,
	DARWIN_AF_IPX,
	DARWIN_AF_INET6,
	DARWIN_AF_PIP,		/* 25 */
	DARWIN_AF_ISDN,
	DARWIN_AF_NATM,
	0,
	DARWIN_AF_KEY,
	DARWIN_AF_HDRCMPLT,	/* 30 */
	0,
	0,
	0,
	0,
	0,			/* 35 */
	0,
};

unsigned char darwin_to_native_af[] = {
	0,
	AF_LOCAL,
	AF_INET,
	AF_IMPLINK,
	AF_PUP,
	AF_CHAOS,	/* 5 */
	AF_NS,
	AF_ISO,
	AF_ECMA,
	AF_DATAKIT,
	AF_CCITT,	/* 10 */
	AF_SNA,
	AF_DECnet,
	AF_DLI,
	AF_LAT,
	AF_HYLINK,	/* 15 */
	AF_APPLETALK,
	AF_ROUTE,
	AF_LINK,
	0,
	AF_COIP,	/* 20 */
	AF_CNT,
	0,
	AF_IPX,
	0,
	0,		/* 25 */
	0,
	0,
	AF_ISDN,
	pseudo_AF_KEY,
	AF_INET6,	/* 30 */
	AF_NATM,
	0,
	0,
	0,
	pseudo_AF_HDRCMPLT,	/* 35 */
	0,
};

static int
native_to_darwin_sockaddr(struct mbuf *nam)
{
	struct sockaddr *sa = mtod(nam, void *);

	/* We only need to translate the address family */
	if ((unsigned)sa->sa_family >= __arraycount(native_to_darwin_af))
		return EPROTONOSUPPORT;

	sa->sa_family = native_to_darwin_af[sa->sa_family];
	return 0;
}

static int
darwin_to_native_sockaddr(struct mbuf *nam)
{
	struct sockaddr *sa = mtod(nam, void *);

	if ((unsigned)sa->sa_family >= __arraycount(darwin_to_native_af)) {
		m_free(nam);
		return EPROTONOSUPPORT;
	}
	sa->sa_family = darwin_to_native_af[sa->sa_family];

	/*
	 * sa_len is zero for AF_LOCAL sockets, believe size we copied in!
	 * The code used to strlen the filename, but that way lies madness!
	 */

	if (sa->sa_len > nam->m_len || sa->sa_len == 0)
		sa->sa_len = nam->m_len;
	else
		nam->m_len = sa->sa_len;

	return 0;
}

int
darwin_sys_socket(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct compat_30_sys_socket_args cup;

	if ((unsigned)SCARG(uap, domain) >= __arraycount(darwin_to_native_af))
		return (EPROTONOSUPPORT);

	SCARG(&cup, domain) = darwin_to_native_af[SCARG(uap, domain)];
	SCARG(&cup, type) = SCARG(uap, type);
	SCARG(&cup, protocol) = SCARG(uap, protocol);

	return compat_30_sys_socket(l, &cup, retval);
}

int
darwin_sys_recvfrom(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(size_t) len;
		syscallarg(int) flags;
		syscallarg(struct sockaddr *) from;
		syscallarg(unsigned int *) fromlenaddr;
	} */ *uap = v;
	struct msghdr	msg;
	struct iovec	aiov;
	int		error;
	struct mbuf	*from;

	msg.msg_name = NULL;;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_control = NULL;
	msg.msg_flags = SCARG(uap, flags) & MSG_USERFLAGS;

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, &from, NULL, retval);
	if (error != 0)
		return error;

	error = native_to_darwin_sockaddr(from);
	if (error == 0)
		error = copyout_sockname(SCARG(uap, from),
		    SCARG(uap, fromlenaddr), MSG_LENUSRSPACE, from);
	if (from != NULL)
		m_free(from);
	return error;
}

int
darwin_sys_accept(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_accept_args /* {
		syscallarg(int) s;
		syscallarg(struct sockaddr *) name;
		syscallarg(unsigned int *) anamelen;
	} */ *uap = v;
	int error;
	struct mbuf *name;

	error = do_sys_accept(l, SCARG(uap, s), &name, retval);
	if (error != 0)
		return error;

	error = native_to_darwin_sockaddr(name);
	if (error == 0)
		error = copyout_sockname(SCARG(uap, name), SCARG(uap, anamelen),
		    MSG_LENUSRSPACE, name);
	if (name != NULL)
		m_free(name);
	if (error != 0)
		fdrelease(l, *retval);
	return error;
}

int
darwin_sys_getpeername(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(struct sockaddr *) asa;
		syscallarg(unsigned int *) alen;
	} */ *uap = v;
	struct mbuf	*m;
	int		error;

	error = do_sys_getsockname(l, SCARG(uap, fdes), PRU_PEERADDR, &m);
	if (error != 0)
		return error;

	error = native_to_darwin_sockaddr(m);
	if (error != 0)
		error = copyout_sockname(SCARG(uap, asa), SCARG(uap, alen),
		    MSG_LENUSRSPACE, m);
	if (m != NULL)
		m_free(m);
	return error;
}

int
darwin_sys_getsockname(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(struct sockaddr *) asa;
		syscallarg(unsigned int *) alen;
	} */ *uap = v;
	struct mbuf	*m;
	int		error;

	error = do_sys_getsockname(l, SCARG(uap, fdes), PRU_SOCKADDR, &m);
	if (error != 0)
		return error;

	error = native_to_darwin_sockaddr(m);
	if (error != 0)
		error = copyout_sockname(SCARG(uap, asa), SCARG(uap, alen),
		    MSG_LENUSRSPACE, m);
	if (m != NULL)
		m_free(m);
	return error;
}

int
darwin_sys_connect(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_connect_args /* {
		syscallarg(int) s;
		syscallarg(struct sockaddr *) name;
		syscallarg(unsigned int *) namelen;
	} */ *uap = v;
	struct mbuf *nam;
	int error;

	error = sockargs(&nam, SCARG(uap, name), SCARG(uap, namelen),
	    MT_SONAME);
	if (error == 0)
		error = darwin_to_native_sockaddr(nam);
	if (error == 0)
		error = do_sys_connect(l, SCARG(uap, s), nam);

	return error;
}

int
darwin_sys_bind(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_bind_args /* {
		syscallarg(int) s;
		syscallarg(struct sockaddr *) name;
		syscallarg(unsigned int *) namelen;
	} */ *uap = v;
	struct mbuf *nam;
	int error;

	error = sockargs(&nam, SCARG(uap, name), SCARG(uap, namelen),
	    MT_SONAME);
	if (error == 0)
		error = darwin_to_native_sockaddr(nam);
	if (error == 0)
		error = do_sys_bind(l, SCARG(uap, s), nam);

	return error;
}

int
darwin_sys_sendto(struct lwp *l, void *v, register_t *retval)
{
	struct darwin_sys_sendto_args /* {
		syscallarg(int) s;
		syscallarg(const void *) buf;
		syscallarg(size_t) len;
		syscallarg(int) flags;
		syscallarg(struct sockaddr *) to;
		syscallarg(unsigned int) tolen;
	} */ *uap = v;

	struct msghdr	msg;
	struct iovec	aiov;
	struct mbuf *nam;
	int error;

	error = sockargs(&nam, SCARG(uap, to), SCARG(uap, tolen), MT_SONAME);
	if (error != 0)
		return error;
	error = darwin_to_native_sockaddr(nam);
	if (error != 0)
		return error;

	msg.msg_name = nam;
	msg.msg_namelen = 0;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_control = 0;
	msg.msg_flags = MSG_NAMEMBUF;
	aiov.iov_base = __UNCONST(SCARG(uap, buf)); /* XXXUNCONST kills const */
	aiov.iov_len = SCARG(uap, len);
	return do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
}
