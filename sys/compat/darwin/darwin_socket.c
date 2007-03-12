/*	$NetBSD: darwin_socket.c,v 1.12.2.1 2007/03/12 05:51:56 rmind Exp $ */

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
__KERNEL_RCSID(0, "$NetBSD: darwin_socket.c,v 1.12.2.1 2007/03/12 05:51:56 rmind Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <sys/lwp.h>
#include <sys/socketvar.h>
#include <sys/un.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <compat/sys/signal.h>

#include <compat/common/compat_util.h>
#include <compat/common/compat_file.h>

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

int
native_to_darwin_sockaddr(nsa, dsa)
	struct sockaddr *nsa;
	struct sockaddr_storage *dsa;
{
	size_t len;

	if ((len = nsa->sa_len) > _SS_MAXSIZE) {
		printf("native_to_darwin_sockaddr: sa_len too big");
		return EINVAL;
	}

	memcpy(dsa, nsa, len);
	/* Array dereference is safe. sa_family is type unsigned */
	dsa->ss_family = native_to_darwin_af[nsa->sa_family];

	return 0;
}

int
darwin_to_native_sockaddr(dsa, nsa)
	struct sockaddr *dsa;
	struct sockaddr_storage *nsa;
{
	size_t len;

	if ((len = dsa->sa_len) > _SS_MAXSIZE) {
		printf("darwin_to_native_sockaddr: sa_len too big");
		return EINVAL;
	}

	if (len == 0) {
		/*
		 * It happens for AF_LOCAL sockets, where the
		 * size must be computed by hand...
		 */
		switch (dsa->sa_family) {
		case DARWIN_AF_LOCAL: {
			struct sockaddr_un *sun = (struct sockaddr_un *)dsa;

			len = sizeof(*sun)
			    - sizeof(sun->sun_path)
			    + strlen(sun->sun_path)
			    + 1; /* For trailing \0 */
			if (len > _SS_MAXSIZE) {
				printf("darwin_to_native_sockaddr: "
				    "sa_len too big");
				return EINVAL;
			}
			break;
		}

		default:
			printf("darwin_to_native_sockaddr: sa_len not set");
			return EINVAL;
			break;
		}
	}

	memcpy(nsa, dsa, len);
	/* Array dereference is safe. sa_family is type unsigned */
	nsa->ss_family = darwin_to_native_af[dsa->sa_family];

	return 0;
}

int
darwin_sys_socket(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_socket_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct compat_30_sys_socket_args cup;

	if (SCARG(uap, domain) < 0)
		return (EPROTONOSUPPORT);

	SCARG(&cup, domain) = darwin_to_native_af[SCARG(uap, domain)];
	SCARG(&cup, type) = SCARG(uap, type);
	SCARG(&cup, protocol) = SCARG(uap, protocol);

	return compat_30_sys_socket(l, &cup, retval);
}

int
darwin_sys_recvfrom(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(size_t) len;
		syscallarg(int) flags;
		syscallarg(struct sockaddr *) from;
		syscallarg(unsigned int *) fromlenaddr;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_recvfrom_args cup;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	if (SCARG(uap, from) != NULL)
		nssp = stackgap_alloc(p, &sg, sizeof(*nssp));
	else
		nssp = NULL;

	SCARG(&cup, s) = SCARG(uap, s);
	SCARG(&cup, buf) = SCARG(uap, buf);
	SCARG(&cup, len) = SCARG(uap, len);
	SCARG(&cup, flags) = SCARG(uap, flags);
	SCARG(&cup, from) = (struct sockaddr *)nssp;
	SCARG(&cup, fromlenaddr) = SCARG(uap, fromlenaddr);

	if ((error = sys_recvfrom(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(nssp, &nss, sizeof(nss))) != 0)
		return error;

	if ((error = native_to_darwin_sockaddr((struct sockaddr *)&nss,
					       &dss)) != 0)
		return error;

	if ((error = copyin(SCARG(uap, fromlenaddr), &len, sizeof(len))) != 0)
		return error;

	if (len > _SS_MAXSIZE)
		len = _SS_MAXSIZE;
	if (dss.ss_len < len)
		len = dss.ss_len;

	if ((error = copyout(&dss, SCARG(uap, from), len)) != 0)
		return error;

	if ((error = copyout(&len, SCARG(uap, fromlenaddr), sizeof(len))) != 0)
		return error;

	return 0;
}

int
darwin_sys_accept(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_accept_args /* {
		syscallarg(int) s;
		syscallarg(struct sockaddr *) name;
		syscallarg(unsigned int *) anamelen;
	} */ *uap = v;
	struct sys_accept_args cup;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	nssp = stackgap_alloc(p, &sg, sizeof(*nssp));

	SCARG(&cup, s) = SCARG(uap, s);
	SCARG(&cup, name) = (struct sockaddr *)nssp;
	SCARG(&cup, anamelen) = SCARG(uap, anamelen);

	if ((error = sys_accept(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(nssp, &nss, sizeof(nss))) != 0)
		return error;

	if ((error = native_to_darwin_sockaddr((struct sockaddr *)&nss,
					       &dss)) != 0)
		return error;

	if ((error = copyin(SCARG(uap, anamelen), &len, sizeof(len))) != 0)
		return error;

	if (len > _SS_MAXSIZE)
		len = _SS_MAXSIZE;
	if (dss.ss_len < len)
		len = dss.ss_len;

	if ((error = copyout(&dss, SCARG(uap, name), len)) != 0)
		return error;

	if ((error = copyout(&len, SCARG(uap, anamelen), sizeof(len))) != 0)
		return error;

	return 0;
}

int
darwin_sys_getpeername(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(struct sockaddr *) asa;
		syscallarg(unsigned int *) alen;
	} */ *uap = v;
	struct sys_getpeername_args cup;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	nssp = stackgap_alloc(p, &sg, sizeof(*nssp));

	SCARG(&cup, fdes) = SCARG(uap, fdes);
	SCARG(&cup, asa) = (struct sockaddr *)nssp;
	SCARG(&cup, alen) = SCARG(uap, alen);

	if ((error = sys_getpeername(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(nssp, &nss, sizeof(nss))) != 0)
		return error;

	if ((error = native_to_darwin_sockaddr((struct sockaddr *)&nss,
					       &dss)) != 0)
		return error;

	if ((error = copyin(SCARG(uap, alen), &len, sizeof(len))) != 0)
		return error;

	if (len > _SS_MAXSIZE)
		len = _SS_MAXSIZE;
	if (dss.ss_len < len)
		len = dss.ss_len;

	if ((error = copyout(&dss, SCARG(uap, asa), len)) != 0)
		return error;

	if ((error = copyout(&len, SCARG(uap, alen), sizeof(len))) != 0)
		return error;

	return 0;
}

int
darwin_sys_getsockname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(struct sockaddr *) asa;
		syscallarg(unsigned int *) alen;
	} */ *uap = v;
	struct sys_getsockname_args cup;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	nssp = stackgap_alloc(p, &sg, sizeof(*nssp));

	SCARG(&cup, fdes) = SCARG(uap, fdes);
	SCARG(&cup, asa) = (struct sockaddr *)nssp;
	SCARG(&cup, alen) = SCARG(uap, alen);

	if ((error = sys_getsockname(l, &cup, retval)) != 0)
		return error;

	if ((error = copyin(nssp, &nss, sizeof(nss))) != 0)
		return error;

	if ((error = native_to_darwin_sockaddr((struct sockaddr *)&nss,
					       &dss)) != 0)
		return error;

	if ((error = copyin(SCARG(uap, alen), &len, sizeof(len))) != 0)
		return error;

	if (len > _SS_MAXSIZE)
		len = _SS_MAXSIZE;
	if (dss.ss_len < len)
		len = dss.ss_len;

	if ((error = copyout(&dss, SCARG(uap, asa), len)) != 0)
		return error;

	if ((error = copyout(&len, SCARG(uap, alen), sizeof(len))) != 0)
		return error;

	return 0;
}

int
darwin_sys_connect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_connect_args /* {
		syscallarg(int) s;
		syscallarg(struct sockaddr *) name;
		syscallarg(unsigned int *) namelen;
	} */ *uap = v;
	struct sys_connect_args cup;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	nssp = stackgap_alloc(p, &sg, sizeof(*nssp));

	if ((error = copyin(SCARG(uap, name), &dss, sizeof(dss))) != 0)
		return error;

	if ((error = darwin_to_native_sockaddr((struct sockaddr *)&dss,
					       &nss)) != 0)
		return error;

	len = SCARG(uap, namelen);
	if (nss.ss_len < len)
		len = nss.ss_len;

	if ((error = copyout(&nss, nssp, sizeof(nss))) != 0)
		return error;

	SCARG(&cup, s) = SCARG(uap, s);
	SCARG(&cup, name) = (struct sockaddr *)nssp;
	SCARG(&cup, namelen) = len;

	return bsd_sys_connect(l, &cup, retval);
}

int
darwin_sys_bind(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_bind_args /* {
		syscallarg(int) s;
		syscallarg(struct sockaddr *) name;
		syscallarg(unsigned int *) namelen;
	} */ *uap = v;
	struct sys_bind_args cup;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	nssp = stackgap_alloc(p, &sg, sizeof(*nssp));

	if ((error = copyin(SCARG(uap, name), &dss, sizeof(dss))) != 0)
		return error;

	if ((error = darwin_to_native_sockaddr((struct sockaddr *)&dss,
					       &nss)) != 0)
		return error;

	len = SCARG(uap, namelen);
	if (nss.ss_len < len)
		len = nss.ss_len;

	if ((error = copyout(&nss, nssp, sizeof(nss))) != 0)
		return error;

	SCARG(&cup, s) = SCARG(uap, s);
	SCARG(&cup, name) = (struct sockaddr *)nssp;
	SCARG(&cup, namelen) = len;

	return bsd_sys_bind(l, &cup, retval);
}

int
darwin_sys_sendto(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct darwin_sys_sendto_args /* {
		syscallarg(int) s;
		syscallarg(const void *) buf;
		syscallarg(size_t) len;
		syscallarg(int) flags;
		syscallarg(struct sockaddr *) to;
		syscallarg(unsigned int) tolen;
	} */ *uap = v;
	struct sys_sendto_args cup;
	struct proc *p = l->l_proc;
	void *sg = stackgap_init(p, 0);
	struct sockaddr_storage nss;
	struct sockaddr_storage dss;
	struct sockaddr_storage *nssp;
	size_t len;
	int error;

	nssp = stackgap_alloc(p, &sg, sizeof(*nssp));

	if ((error = copyin(SCARG(uap, to), &dss, sizeof(dss))) != 0)
		return error;

	if ((error = darwin_to_native_sockaddr((struct sockaddr *)&dss,
					       &nss)) != 0)
		return error;

	len = SCARG(uap, tolen);
	if (nss.ss_len < len)
		len = nss.ss_len;

	if ((error = copyout(&nss, nssp, sizeof(nss))) != 0)
		return error;

	SCARG(&cup, s) = SCARG(uap, s);
	SCARG(&cup, buf) = SCARG(uap, buf);
	SCARG(&cup, len) = SCARG(uap, len);
	SCARG(&cup, flags) = SCARG(uap, flags);
	SCARG(&cup, to) = (struct sockaddr *)nssp;
	SCARG(&cup, tolen) = len;

	return sys_sendto(l, &cup, retval);
}
