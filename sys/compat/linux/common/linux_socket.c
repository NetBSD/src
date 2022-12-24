/*	$NetBSD: linux_socket.c,v 1.155 2022/12/24 15:23:02 andvar Exp $	*/

/*-
 * Copyright (c) 1995, 1998, 2008 The NetBSD Foundation, Inc.
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
 * Functions in multiarch:
 *	linux_sys_socketcall		: linux_socketcall.c
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_socket.c,v 1.155 2022/12/24 15:23:02 andvar Exp $");

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif /* defined(_KERNEL_OPT) */

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/domain.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/mount.h>
#include <sys/proc.h>
#include <sys/vnode.h>
#include <sys/device.h>
#include <sys/protosw.h>
#include <sys/mbuf.h>
#include <sys/syslog.h>
#include <sys/exec.h>
#include <sys/kauth.h>
#include <sys/syscallargs.h>
#include <sys/ktrace.h>

#include <lib/libkern/libkern.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>

#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_sched.h>
#include <compat/linux/common/linux_socket.h>
#include <compat/linux/common/linux_fcntl.h>
#if !defined(__aarch64__) && !defined(__alpha__) && !defined(__amd64__)
#include <compat/linux/common/linux_socketcall.h>
#endif
#include <compat/linux/common/linux_sockio.h>
#include <compat/linux/common/linux_ipc.h>
#include <compat/linux/common/linux_sem.h>

#include <compat/linux/linux_syscallargs.h>

#ifdef DEBUG_LINUX
#define DPRINTF(a) uprintf a
#else
#define DPRINTF(a)
#endif

/*
 * The calls in this file are entered either via the linux_socketcall()
 * interface or, on the Alpha, as individual syscalls.  The
 * linux_socketcall function does any massaging of arguments so that all
 * the calls in here need not think that they are anything other
 * than a normal syscall.
 */

static int linux_to_bsd_domain(int);
static int bsd_to_linux_domain(int);
static int linux_to_bsd_type(int);
int linux_to_bsd_sopt_level(int);
int linux_to_bsd_so_sockopt(int);
int linux_to_bsd_ip_sockopt(int);
int linux_to_bsd_ipv6_sockopt(int);
int linux_to_bsd_tcp_sockopt(int);
int linux_to_bsd_udp_sockopt(int);
int linux_getifname(struct lwp *, register_t *, void *);
int linux_getifconf(struct lwp *, register_t *, void *);
int linux_getifhwaddr(struct lwp *, register_t *, u_int, void *);
static int linux_get_sa(struct lwp *, int, struct sockaddr_big *,
		const struct osockaddr *, socklen_t);
static int linux_sa_put(struct osockaddr *osa);
static int linux_to_bsd_msg_flags(int);
static int bsd_to_linux_msg_flags(int);
static void linux_to_bsd_msghdr(const struct linux_msghdr *, struct msghdr *);
static void bsd_to_linux_msghdr(const struct msghdr *, struct linux_msghdr *);

static const int linux_to_bsd_domain_[LINUX_AF_MAX] = {
	AF_UNSPEC,
	AF_UNIX,
	AF_INET,
	AF_CCITT,	/* LINUX_AF_AX25 */
	AF_IPX,
	AF_APPLETALK,
	-1,		/* LINUX_AF_NETROM */
	-1,		/* LINUX_AF_BRIDGE */
	-1,		/* LINUX_AF_ATMPVC */
	AF_CCITT,	/* LINUX_AF_X25 */
	AF_INET6,
	-1,		/* LINUX_AF_ROSE */
	AF_DECnet,
	-1,		/* LINUX_AF_NETBEUI */
	-1,		/* LINUX_AF_SECURITY */
	pseudo_AF_KEY,
	AF_ROUTE,	/* LINUX_AF_NETLINK */
	-1,		/* LINUX_AF_PACKET */
	-1,		/* LINUX_AF_ASH */
	-1,		/* LINUX_AF_ECONET */
	-1,		/* LINUX_AF_ATMSVC */
	AF_SNA,
	/* rest up to LINUX_AF_MAX-1 is not allocated */
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};

static const int bsd_to_linux_domain_[AF_MAX] = {
	LINUX_AF_UNSPEC,
	LINUX_AF_UNIX,
	LINUX_AF_INET,
	-1,		/* AF_IMPLINK */
	-1,		/* AF_PUP */
	-1,		/* AF_CHAOS */
	-1,		/* AF_NS */
	-1,		/* AF_ISO */
	-1,		/* AF_ECMA */
	-1,		/* AF_DATAKIT */
	LINUX_AF_AX25,	/* AF_CCITT */
	LINUX_AF_SNA,
	LINUX_AF_DECnet,
	-1,		/* AF_DLI */
	-1,		/* AF_LAT */
	-1,		/* AF_HYLINK */
	LINUX_AF_APPLETALK,
	LINUX_AF_NETLINK,
	-1,		/* AF_LINK */
	-1,		/* AF_XTP */
	-1,		/* AF_COIP */
	-1,		/* AF_CNT */
	-1,		/* pseudo_AF_RTIP */
	LINUX_AF_IPX,
	LINUX_AF_INET6,
	-1,		/* pseudo_AF_PIP */
	-1,		/* AF_ISDN */
	-1,		/* AF_NATM */
	-1,		/* AF_ARP */
	LINUX_pseudo_AF_KEY,
	-1,		/* pseudo_AF_HDRCMPLT */
};

static const struct {
	int bfl;
	int lfl;
} bsd_to_linux_msg_flags_[] = {
	{MSG_OOB,		LINUX_MSG_OOB},
	{MSG_PEEK,		LINUX_MSG_PEEK},
	{MSG_DONTROUTE,		LINUX_MSG_DONTROUTE},
	{MSG_EOR,		LINUX_MSG_EOR},
	{MSG_TRUNC,		LINUX_MSG_TRUNC},
	{MSG_CTRUNC,		LINUX_MSG_CTRUNC},
	{MSG_WAITALL,		LINUX_MSG_WAITALL},
	{MSG_DONTWAIT,		LINUX_MSG_DONTWAIT},
	{MSG_BCAST,		0},		/* not supported, clear */
	{MSG_MCAST,		0},		/* not supported, clear */
	{MSG_NOSIGNAL,		LINUX_MSG_NOSIGNAL},
	{-1, /* not supp */	LINUX_MSG_PROBE},
	{-1, /* not supp */	LINUX_MSG_FIN},
	{-1, /* not supp */	LINUX_MSG_SYN},
	{-1, /* not supp */	LINUX_MSG_CONFIRM},
	{-1, /* not supp */	LINUX_MSG_RST},
	{-1, /* not supp */	LINUX_MSG_ERRQUEUE},
	{-1, /* not supp */	LINUX_MSG_MORE},
};

/*
 * Convert between Linux and BSD socket domain values
 */
static int
linux_to_bsd_domain(int ldom)
{
	if (ldom < 0 || ldom >= LINUX_AF_MAX)
		return (-1);

	return linux_to_bsd_domain_[ldom];
}

/*
 * Convert between BSD and Linux socket domain values
 */
static int
bsd_to_linux_domain(int bdom)
{
	if (bdom < 0 || bdom >= AF_MAX)
		return (-1);

	return bsd_to_linux_domain_[bdom];
}

static int
linux_to_bsd_type(int ltype)
{
	int type, flags;

	/* Real types are identical between Linux and NetBSD */
	type = ltype & LINUX_SOCK_TYPE_MASK;

	/* But flags are not .. */
	flags = ltype & ~LINUX_SOCK_TYPE_MASK;
	if (flags & ~(LINUX_SOCK_CLOEXEC|LINUX_SOCK_NONBLOCK))
		return -1;

	if (flags & LINUX_SOCK_CLOEXEC)
		type |= SOCK_CLOEXEC;
	if (flags & LINUX_SOCK_NONBLOCK)
		type |= SOCK_NONBLOCK;

	return type;
}

static int
linux_to_bsd_msg_flags(int lflag)
{
	int i, lfl, bfl;
	int bflag = 0;

	if (lflag == 0)
		return (0);

	for(i = 0; i < __arraycount(bsd_to_linux_msg_flags_); i++) {
		bfl = bsd_to_linux_msg_flags_[i].bfl;
		lfl = bsd_to_linux_msg_flags_[i].lfl;

		if (lfl == 0)
			continue;

		if (lflag & lfl) {
			if (bfl < 0)
				return (-1);

			bflag |= bfl;
		}
	}

	return (bflag);
}

static int
bsd_to_linux_msg_flags(int bflag)
{
	int i, lfl, bfl;
	int lflag = 0;

	if (bflag == 0)
		return (0);

	for(i = 0; i < __arraycount(bsd_to_linux_msg_flags_); i++) {
		bfl = bsd_to_linux_msg_flags_[i].bfl;
		lfl = bsd_to_linux_msg_flags_[i].lfl;

		if (bfl <= 0)
			continue;

		if (bflag & bfl) {
			if (lfl < 0)
				return (-1);

			lflag |= lfl;
		}
	}

	return (lflag);
}

int
linux_sys_socket(struct lwp *l, const struct linux_sys_socket_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)	domain;
		syscallarg(int)	type;
		syscallarg(int) protocol;
	} */
	struct sys___socket30_args bsa;
	int error;


	SCARG(&bsa, protocol) = SCARG(uap, protocol);
	SCARG(&bsa, domain) = linux_to_bsd_domain(SCARG(uap, domain));
	if (SCARG(&bsa, domain) == -1)
		return EINVAL;
	SCARG(&bsa, type) = linux_to_bsd_type(SCARG(uap, type));
	if (SCARG(&bsa, type) == -1)
		return EINVAL;
	/*
	 * Apparently linux uses this to talk to ISDN sockets. If we fail
	 * now programs seems to handle it, but if we don't we are going
	 * to fail when we bind and programs don't handle this well.
	 */
	if (SCARG(&bsa, domain) == AF_ROUTE && SCARG(&bsa, type) == SOCK_RAW)
		return ENOTSUP;
	error = sys___socket30(l, &bsa, retval);

#ifdef INET6
	/*
	 * Linux AF_INET6 socket has IPV6_V6ONLY setsockopt set to 0 by
	 * default and some apps depend on this. So, set V6ONLY to 0
	 * for Linux apps if the sysctl value is set to 1.
	 */
	if (!error && ip6_v6only && SCARG(&bsa, domain) == PF_INET6) {
		struct socket *so;

		if (fd_getsock(*retval, &so) == 0) {
			int val = 0;

			/* ignore error */
			(void)so_setsockopt(l, so, IPPROTO_IPV6, IPV6_V6ONLY,
			    &val, sizeof(val));

			fd_putfile(*retval);
		}
	}
#endif

	return (error);
}

int
linux_sys_socketpair(struct lwp *l, const struct linux_sys_socketpair_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(int *) rsv;
	} */
	struct sys_socketpair_args bsa;

	SCARG(&bsa, domain) = linux_to_bsd_domain(SCARG(uap, domain));
	if (SCARG(&bsa, domain) == -1)
		return EINVAL;
	SCARG(&bsa, type) = linux_to_bsd_type(SCARG(uap, type));
	if (SCARG(&bsa, type) == -1)
		return EINVAL;
	SCARG(&bsa, protocol) = SCARG(uap, protocol);
	SCARG(&bsa, rsv) = SCARG(uap, rsv);

	return sys_socketpair(l, &bsa, retval);
}

int
linux_sys_sendto(struct lwp *l, const struct linux_sys_sendto_args *uap, register_t *retval)
{
	/* {
		syscallarg(int)				s;
		syscallarg(void *)			msg;
		syscallarg(int)				len;
		syscallarg(int)				flags;
		syscallarg(struct osockaddr *)		to;
		syscallarg(int)				tolen;
	} */
	struct msghdr   msg;
	struct iovec    aiov;
	struct sockaddr_big nam;
	struct mbuf *m;
	int bflags;
	int error;

	/* Translate message flags.  */
	bflags = linux_to_bsd_msg_flags(SCARG(uap, flags));
	if (bflags < 0)
		/* Some supported flag */
		return EINVAL;

	msg.msg_flags = 0;
	msg.msg_name = NULL;
	msg.msg_control = NULL;

	if (SCARG(uap, tolen)) {
		/* Read in and convert the sockaddr */
		error = linux_get_sa(l, SCARG(uap, s), &nam, SCARG(uap, to),
		    SCARG(uap, tolen));
		if (error)
			return error;
		error = sockargs(&m, &nam, nam.sb_len, UIO_SYSSPACE, MT_SONAME);
		if (error)
			return error;
		msg.msg_flags |= MSG_NAMEMBUF;
		msg.msg_name = m;
		msg.msg_namelen = nam.sb_len;
	}

	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = __UNCONST(SCARG(uap, msg));
	aiov.iov_len = SCARG(uap, len);

	return do_sys_sendmsg(l, SCARG(uap, s), &msg, bflags, retval);
}

static void
linux_to_bsd_msghdr(const struct linux_msghdr *lmsg, struct msghdr *bmsg)
{
	memset(bmsg, 0, sizeof(*bmsg));
	bmsg->msg_name = lmsg->msg_name;
	bmsg->msg_namelen = lmsg->msg_namelen;
	bmsg->msg_iov = lmsg->msg_iov;
	bmsg->msg_iovlen = lmsg->msg_iovlen;
	bmsg->msg_control = lmsg->msg_control;
	bmsg->msg_controllen = lmsg->msg_controllen;
	bmsg->msg_flags = lmsg->msg_flags;
}

static void
bsd_to_linux_msghdr(const struct msghdr *bmsg, struct linux_msghdr *lmsg)
{
	memset(lmsg, 0, sizeof(*lmsg));
	lmsg->msg_name = bmsg->msg_name;
	lmsg->msg_namelen = bmsg->msg_namelen;
	lmsg->msg_iov = bmsg->msg_iov;
	lmsg->msg_iovlen = bmsg->msg_iovlen;
	lmsg->msg_control = bmsg->msg_control;
	lmsg->msg_controllen = bmsg->msg_controllen;
	lmsg->msg_flags = bmsg->msg_flags;
}

int
linux_sys_sendmsg(struct lwp *l, const struct linux_sys_sendmsg_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(struct linux_msghdr *) msg;
		syscallarg(u_int) flags;
	} */
	struct msghdr	msg;
	struct linux_msghdr lmsg;
	int		error;
	int		bflags;
	struct sockaddr_big nam;
	u_int8_t	*control;
	struct mbuf     *ctl_mbuf = NULL;

	error = copyin(SCARG(uap, msg), &lmsg, sizeof(lmsg));
	if (error)
		return error;
	linux_to_bsd_msghdr(&lmsg, &msg);

	msg.msg_flags = MSG_IOVUSRSPACE;

	/*
	 * Translate message flags.
	 */
	bflags = linux_to_bsd_msg_flags(SCARG(uap, flags));
	if (bflags < 0)
		/* Some supported flag */
		return EINVAL;

	if (lmsg.msg_name) {
		/* Read in and convert the sockaddr */
		error = linux_get_sa(l, SCARG(uap, s), &nam, msg.msg_name,
		    msg.msg_namelen);
		if (error)
			return (error);
		msg.msg_name = &nam;
	}

	/*
	 * Handle cmsg if there is any.
	 */
	if (LINUX_CMSG_FIRSTHDR(&lmsg)) {
		struct linux_cmsghdr l_cmsg, *l_cc;
		struct cmsghdr *cmsg;
		ssize_t resid = msg.msg_controllen;
		size_t clen, cidx = 0, cspace;

		ctl_mbuf = m_get(M_WAIT, MT_CONTROL);
		clen = MLEN;
		control = mtod(ctl_mbuf, void *);

		l_cc = LINUX_CMSG_FIRSTHDR(&lmsg);
		do {
			error = copyin(l_cc, &l_cmsg, sizeof(l_cmsg));
			if (error)
				goto done;

			/*
			 * Sanity check the control message length.
			 */
			if (l_cmsg.cmsg_len > resid
			    || l_cmsg.cmsg_len < sizeof l_cmsg) {
				error = EINVAL;
				goto done;
			}

			/*
			 * Refuse unsupported control messages, and
			 * translate fields as appropriate.
			 */
			switch (l_cmsg.cmsg_level) {
			case LINUX_SOL_SOCKET:
				/* It only differs on some archs */
				if (LINUX_SOL_SOCKET != SOL_SOCKET)
					l_cmsg.cmsg_level = SOL_SOCKET;

				switch(l_cmsg.cmsg_type) {
				case LINUX_SCM_RIGHTS:
					/* Linux SCM_RIGHTS is same as NetBSD */
					break;

				case LINUX_SCM_CREDENTIALS:
					/* no native equivalent, just drop it */
					if (control != mtod(ctl_mbuf, void *))
						free(control, M_MBUF);
					m_free(ctl_mbuf);
					ctl_mbuf = NULL;
					msg.msg_control = NULL;
					msg.msg_controllen = 0;
					goto skipcmsg;

				default:
					/* other types not supported */
					error = EINVAL;
					goto done;
				}
				break;
			default:
				/* pray and leave intact */
				break;
			}

			cspace = CMSG_SPACE(l_cmsg.cmsg_len - sizeof(l_cmsg));

			/* Check the buffer is big enough */
			if (__predict_false(cidx + cspace > clen)) {
				u_int8_t *nc;
				size_t nclen;

				nclen = cidx + cspace;
				if (nclen >= PAGE_SIZE) {
					error = EINVAL;
					goto done;
				}
				nc = realloc(clen <= MLEN ? NULL : control,
						nclen, M_TEMP, M_WAITOK);
				if (!nc) {
					error = ENOMEM;
					goto done;
				}
				if (cidx <= MLEN)
					/* Old buffer was in mbuf... */
					memcpy(nc, control, cidx);
				control = nc;
				clen = nclen;
			}

			/* Copy header */
			cmsg = (void *)&control[cidx];
			cmsg->cmsg_len = l_cmsg.cmsg_len + LINUX_CMSG_ALIGN_DELTA;
			cmsg->cmsg_level = l_cmsg.cmsg_level;
			cmsg->cmsg_type = l_cmsg.cmsg_type;

			/* Zero area between header and data */
			memset(cmsg + 1, 0, 
				CMSG_ALIGN(sizeof(*cmsg)) - sizeof(*cmsg));

			/* Copyin the data */
			error = copyin(LINUX_CMSG_DATA(l_cc),
				CMSG_DATA(cmsg),
				l_cmsg.cmsg_len - sizeof(l_cmsg));
			if (error)
				goto done;

			resid -= LINUX_CMSG_ALIGN(l_cmsg.cmsg_len);
			cidx += cspace;
		} while ((l_cc = LINUX_CMSG_NXTHDR(&msg, l_cc, &l_cmsg)) && resid > 0);

		/* If we allocated a buffer, attach to mbuf */
		if (cidx > MLEN) {
			MEXTADD(ctl_mbuf, control, clen, M_MBUF, NULL, NULL);
			ctl_mbuf->m_flags |= M_EXT_RW;
		}
		control = NULL;
		ctl_mbuf->m_len = cidx;

		msg.msg_control = ctl_mbuf;
		msg.msg_flags |= MSG_CONTROLMBUF;

		ktrkuser("mbcontrol", mtod(ctl_mbuf, void *),
		    msg.msg_controllen);
	}

skipcmsg:
	error = do_sys_sendmsg(l, SCARG(uap, s), &msg, bflags, retval);
	/* Freed internally */
	ctl_mbuf = NULL;

done:
	if (ctl_mbuf != NULL) {
		if (control != NULL && control != mtod(ctl_mbuf, void *))
			free(control, M_MBUF);
		m_free(ctl_mbuf);
	}
	return (error);
}

int
linux_sys_recvfrom(struct lwp *l, const struct linux_sys_recvfrom_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(struct osockaddr *) from;
		syscallarg(int *) fromlenaddr;
	} */
	int		error;
	struct sys_recvfrom_args bra;

	SCARG(&bra, s) = SCARG(uap, s);
	SCARG(&bra, buf) = SCARG(uap, buf);
	SCARG(&bra, len) = SCARG(uap, len);
	SCARG(&bra, flags) = SCARG(uap, flags);
	SCARG(&bra, from) = (struct sockaddr *) SCARG(uap, from);
	SCARG(&bra, fromlenaddr) = (socklen_t *)SCARG(uap, fromlenaddr);

	if ((error = sys_recvfrom(l, &bra, retval)))
		return (error);

	if (SCARG(uap, from) && (error = linux_sa_put(SCARG(uap, from))))
		return (error);

	return (0);
}

static int
linux_copyout_msg_control(struct lwp *l, struct msghdr *mp, struct mbuf *control)
{
	int dlen, error = 0;
	struct cmsghdr *cmsg;
	struct linux_cmsghdr linux_cmsg;
	struct mbuf *m;
	char *q, *q_end;

	if (mp->msg_controllen <= 0 || control == 0) {
		mp->msg_controllen = 0;
		free_control_mbuf(l, control, control);
		return 0;
	}

	ktrkuser("msgcontrol", mtod(control, void *), mp->msg_controllen);

	q = (char *)mp->msg_control;
	q_end = q + mp->msg_controllen;

	for (m = control; m != NULL; ) {
		cmsg = mtod(m, struct cmsghdr *);

		/*
		 * Fixup cmsg. We handle two things:
		 * 0. different sizeof cmsg_len.
		 * 1. different values for level/type on some archs
		 * 2. different alignment of CMSG_DATA on some archs
		 */
		memset(&linux_cmsg, 0, sizeof(linux_cmsg));
		linux_cmsg.cmsg_len = cmsg->cmsg_len - LINUX_CMSG_ALIGN_DELTA;
		linux_cmsg.cmsg_level = cmsg->cmsg_level;
		linux_cmsg.cmsg_type = cmsg->cmsg_type;

		dlen = q_end - q;
		if (linux_cmsg.cmsg_len > dlen) {
			/* Not enough room for the parameter */
			dlen -= sizeof linux_cmsg;
			if (dlen <= 0)
				/* Discard if header wont fit */
				break;
			mp->msg_flags |= MSG_CTRUNC;
			if (linux_cmsg.cmsg_level == SOL_SOCKET
			    && linux_cmsg.cmsg_type == SCM_RIGHTS)
				/* Do not truncate me ... */
				break;
		} else
			dlen = linux_cmsg.cmsg_len - sizeof linux_cmsg;

		switch (linux_cmsg.cmsg_level) {
		case SOL_SOCKET:
			linux_cmsg.cmsg_level = LINUX_SOL_SOCKET;
			switch (linux_cmsg.cmsg_type) {
			case SCM_RIGHTS:
				/* Linux SCM_RIGHTS is same as NetBSD */
				break;

			default:
				/* other types not supported */
				error = EINVAL;
				goto done;
			}
			/* machine dependent ! */
			break;
		default:
			/* pray and leave intact */
			break;
		}

		/* There can be padding between the header and data... */
		error = copyout(&linux_cmsg, q, sizeof linux_cmsg);
		if (error != 0) {
			error = copyout(CCMSG_DATA(cmsg), q + sizeof linux_cmsg,
			    dlen);
		}
		if (error != 0) {
			/* We must free all the SCM_RIGHTS */
			m = control;
			break;
		}
		m = m->m_next;
		if (m == NULL || q + LINUX_CMSG_SPACE(dlen) > q_end) {
			q += LINUX_CMSG_LEN(dlen);
			break;
		}
		q += LINUX_CMSG_SPACE(dlen);
	}

  done:
	free_control_mbuf(l, control, m);

	mp->msg_controllen = q - (char *)mp->msg_control;
	return error;
}

int
linux_sys_recvmsg(struct lwp *l, const struct linux_sys_recvmsg_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(struct linux_msghdr *) msg;
		syscallarg(u_int) flags;
	} */
	struct msghdr	msg;
	struct linux_msghdr lmsg;
	int		error;
	struct mbuf	*from, *control;

	error = copyin(SCARG(uap, msg), &lmsg, sizeof(lmsg));
	if (error)
		return (error);
	linux_to_bsd_msghdr(&lmsg, &msg);

	msg.msg_flags = linux_to_bsd_msg_flags(SCARG(uap, flags));
	if (msg.msg_flags < 0) {
		/* Some unsupported flag */
		return (EINVAL);
	}
	msg.msg_flags |= MSG_IOVUSRSPACE;

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, &from,
	    msg.msg_control != NULL ? &control : NULL, retval);
	if (error != 0)
		return error;

	if (msg.msg_control != NULL)
		error = linux_copyout_msg_control(l, &msg, control);

	if (error == 0 && from != 0) {
		mtod(from, struct osockaddr *)->sa_family =
		    bsd_to_linux_domain(mtod(from, struct sockaddr *)->sa_family);
		error = copyout_sockname(msg.msg_name, &msg.msg_namelen, 0,
			from);
	} else
		msg.msg_namelen = 0;

	if (from != NULL)
		m_free(from);

	if (error == 0) {
		msg.msg_flags = bsd_to_linux_msg_flags(msg.msg_flags);
		if (msg.msg_flags < 0)
			/* Some flag unsupported by Linux */
			error = EINVAL;
		else {
			ktrkuser("msghdr", &msg, sizeof(msg));
			bsd_to_linux_msghdr(&msg, &lmsg);
			error = copyout(&lmsg, SCARG(uap, msg), sizeof(lmsg));
		}
	}

	return (error);
}

/*
 * Convert socket option level from Linux to NetBSD value. Only SOL_SOCKET
 * is different, the rest matches IPPROTO_* on both systems.
 */
int
linux_to_bsd_sopt_level(int llevel)
{

	switch (llevel) {
	case LINUX_SOL_SOCKET:
		return SOL_SOCKET;
	case LINUX_SOL_IP:
		return IPPROTO_IP;
#ifdef INET6
	case LINUX_SOL_IPV6:
		return IPPROTO_IPV6;
#endif
	case LINUX_SOL_TCP:
		return IPPROTO_TCP;
	case LINUX_SOL_UDP:
		return IPPROTO_UDP;
	default:
		return -1;
	}
}

/*
 * Convert Linux socket level socket option numbers to NetBSD values.
 */
int
linux_to_bsd_so_sockopt(int lopt)
{

	switch (lopt) {
	case LINUX_SO_DEBUG:
		return SO_DEBUG;
	case LINUX_SO_REUSEADDR:
		/*
		 * Linux does not implement SO_REUSEPORT, but allows reuse of
		 * a host:port pair through SO_REUSEADDR even if the address
		 * is not a multicast-address. Effectively, this means that we
		 * should use SO_REUSEPORT to allow Linux applications to not
		 * exit with EADDRINUSE
		 */
		return SO_REUSEPORT;
	case LINUX_SO_TYPE:
		return SO_TYPE;
	case LINUX_SO_ERROR:
		return SO_ERROR;
	case LINUX_SO_DONTROUTE:
		return SO_DONTROUTE;
	case LINUX_SO_BROADCAST:
		return SO_BROADCAST;
	case LINUX_SO_SNDBUF:
		return SO_SNDBUF;
	case LINUX_SO_RCVBUF:
		return SO_RCVBUF;
	case LINUX_SO_KEEPALIVE:
		return SO_KEEPALIVE;
	case LINUX_SO_OOBINLINE:
		return SO_OOBINLINE;
	case LINUX_SO_NO_CHECK:
	case LINUX_SO_PRIORITY:
		return -1;
	case LINUX_SO_LINGER:
		return SO_LINGER;
	case LINUX_SO_BSDCOMPAT:
	case LINUX_SO_PASSCRED:
	case LINUX_SO_PEERCRED:
		return -1;
	case LINUX_SO_RCVLOWAT:
		return SO_RCVLOWAT;
	case LINUX_SO_SNDLOWAT:
		return SO_SNDLOWAT;
	case LINUX_SO_RCVTIMEO:
		return SO_RCVTIMEO;
	case LINUX_SO_SNDTIMEO:
		return SO_SNDTIMEO;
	case LINUX_SO_SECURITY_AUTHENTICATION:
	case LINUX_SO_SECURITY_ENCRYPTION_TRANSPORT:
	case LINUX_SO_SECURITY_ENCRYPTION_NETWORK:
	case LINUX_SO_BINDTODEVICE:
	case LINUX_SO_ATTACH_FILTER:
	case LINUX_SO_DETACH_FILTER:
	case LINUX_SO_PEERNAME:
		return -1;
	case LINUX_SO_TIMESTAMP:
		return SO_TIMESTAMP;
	case LINUX_SO_ACCEPTCONN:
	case LINUX_SO_PEERSEC:
	case LINUX_SO_SNDBUFFORCE:
	case LINUX_SO_RCVBUFFORCE:
	case LINUX_SO_PASSSEC:
	case LINUX_SO_TIMESTAMPNS:
	case LINUX_SO_MARK:
	case LINUX_SO_TIMESTAMPING:
	case LINUX_SO_PROTOCOL:
	case LINUX_SO_DOMAIN:
	case LINUX_SO_RXQ_OVFL:
	case LINUX_SO_WIFI_STATUS:
	case LINUX_SO_PEEK_OFF:
	case LINUX_SO_NOFCS:
	default:
		return -1;
	}
}

/*
 * Convert Linux IP level socket option number to NetBSD values.
 */
int
linux_to_bsd_ip_sockopt(int lopt)
{

	switch (lopt) {
	case LINUX_IP_TOS:
		return IP_TOS;
	case LINUX_IP_TTL:
		return IP_TTL;
	case LINUX_IP_HDRINCL:
		return IP_HDRINCL;
	case LINUX_IP_MULTICAST_TTL:
		return IP_MULTICAST_TTL;
	case LINUX_IP_MULTICAST_LOOP:
		return IP_MULTICAST_LOOP;
	case LINUX_IP_MULTICAST_IF:
		return IP_MULTICAST_IF;
	case LINUX_IP_ADD_MEMBERSHIP:
		return IP_ADD_MEMBERSHIP;
	case LINUX_IP_DROP_MEMBERSHIP:
		return IP_DROP_MEMBERSHIP;
	case LINUX_IP_RECVERR:
		return -2;	/* ignored */
	default:
		return -1;
	}
}

/*
 * Convert Linux IPV6 level socket option number to NetBSD values.
 */
#ifdef INET6
int
linux_to_bsd_ipv6_sockopt(int lopt)
{

	switch (lopt) {
	case LINUX_IPV6_V6ONLY:
		return IPV6_V6ONLY;
	default:
		return -1;
	}
}
#endif

/*
 * Convert Linux TCP level socket option number to NetBSD values.
 */
int
linux_to_bsd_tcp_sockopt(int lopt)
{

	switch (lopt) {
	case LINUX_TCP_NODELAY:
		return TCP_NODELAY;
	case LINUX_TCP_MAXSEG:
		return TCP_MAXSEG;
	default:
		return -1;
	}
}

/*
 * Convert Linux UDP level socket option number to NetBSD values.
 */
int
linux_to_bsd_udp_sockopt(int lopt)
{

	switch (lopt) {
	default:
		return -1;
	}
}

/*
 * Another reasonably straightforward function: setsockopt(2).
 * The level and option numbers are converted; the values passed
 * are not (yet) converted, the ones currently implemented don't
 * need conversion, as they are the same on both systems.
 */
int
linux_sys_setsockopt(struct lwp *l, const struct linux_sys_setsockopt_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(void *) optval;
		syscallarg(int) optlen;
	} */
	struct sys_setsockopt_args bsa;
	int name;

	SCARG(&bsa, s) = SCARG(uap, s);
	SCARG(&bsa, level) = linux_to_bsd_sopt_level(SCARG(uap, level));
	SCARG(&bsa, val) = SCARG(uap, optval);
	SCARG(&bsa, valsize) = SCARG(uap, optlen);

	/*
	 * Linux supports only SOL_SOCKET for AF_LOCAL domain sockets
	 * and returns EOPNOTSUPP for other levels
	 */
	if (SCARG(&bsa, level) != SOL_SOCKET) {
		struct socket *so;
		int error, family;

		/* fd_getsock() will use the descriptor for us */
	    	if ((error = fd_getsock(SCARG(&bsa, s), &so)) != 0)
		    	return error;
		family = so->so_proto->pr_domain->dom_family;
		fd_putfile(SCARG(&bsa, s));

		if (family == AF_LOCAL)
			return EOPNOTSUPP;
	}

	switch (SCARG(&bsa, level)) {
	case SOL_SOCKET:
		name = linux_to_bsd_so_sockopt(SCARG(uap, optname));
		break;
	case IPPROTO_IP:
		name = linux_to_bsd_ip_sockopt(SCARG(uap, optname));
		break;
#ifdef INET6
	case IPPROTO_IPV6:
		name = linux_to_bsd_ipv6_sockopt(SCARG(uap, optname));
		break;
#endif
	case IPPROTO_TCP:
		name = linux_to_bsd_tcp_sockopt(SCARG(uap, optname));
		break;
	case IPPROTO_UDP:
		name = linux_to_bsd_udp_sockopt(SCARG(uap, optname));
		break;
	default:
		return EINVAL;
	}

	if (name == -1)
		return EINVAL;
	if (name == -2)
		return 0;
	SCARG(&bsa, name) = name;

	return sys_setsockopt(l, &bsa, retval);
}

/*
 * getsockopt(2) is very much the same as setsockopt(2) (see above)
 */
int
linux_sys_getsockopt(struct lwp *l, const struct linux_sys_getsockopt_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(void *) optval;
		syscallarg(int *) optlen;
	} */
	struct sys_getsockopt_args bga;
	int name;

	SCARG(&bga, s) = SCARG(uap, s);
	SCARG(&bga, level) = linux_to_bsd_sopt_level(SCARG(uap, level));
	SCARG(&bga, val) = SCARG(uap, optval);
	SCARG(&bga, avalsize) = (socklen_t *)SCARG(uap, optlen);

	switch (SCARG(&bga, level)) {
	case SOL_SOCKET:
		name = linux_to_bsd_so_sockopt(SCARG(uap, optname));
		break;
	case IPPROTO_IP:
		name = linux_to_bsd_ip_sockopt(SCARG(uap, optname));
		break;
#ifdef INET6
	case IPPROTO_IPV6:
		name = linux_to_bsd_ipv6_sockopt(SCARG(uap, optname));
		break;
#endif
	case IPPROTO_TCP:
		name = linux_to_bsd_tcp_sockopt(SCARG(uap, optname));
		break;
	case IPPROTO_UDP:
		name = linux_to_bsd_udp_sockopt(SCARG(uap, optname));
		break;
	default:
		return EINVAL;
	}

	if (name == -1)
		return EINVAL;
	SCARG(&bga, name) = name;

	return sys_getsockopt(l, &bga, retval);
}

int
linux_getifname(struct lwp *l, register_t *retval, void *data)
{
	struct ifnet *ifp;
	struct linux_ifreq ifr;
	int error;
	int s;

	error = copyin(data, &ifr, sizeof(ifr));
	if (error)
		return error;

	s = pserialize_read_enter();
	ifp = if_byindex(ifr.ifr_ifru.ifru_ifindex);
	if (ifp == NULL) {
		pserialize_read_exit(s);
		return ENODEV;
	}

	strncpy(ifr.ifr_name, ifp->if_xname, sizeof(ifr.ifr_name));
	pserialize_read_exit(s);

	return copyout(&ifr, data, sizeof(ifr));
}

int
linux_getifconf(struct lwp *l, register_t *retval, void *data)
{
	struct linux_ifreq ifr, *ifrp = NULL;
	struct linux_ifconf ifc;
	struct ifnet *ifp;
	struct sockaddr *sa;
	struct osockaddr *osa;
	int space = 0, error;
	const int sz = (int)sizeof(ifr);
	bool docopy;
	int s;
	int bound;
	struct psref psref;

	error = copyin(data, &ifc, sizeof(ifc));
	if (error)
		return error;

	docopy = ifc.ifc_req != NULL;
	if (docopy) {
		if (ifc.ifc_len < 0)
			return EINVAL;

		space = ifc.ifc_len;
		ifrp = ifc.ifc_req;
	}
	memset(&ifr, 0, sizeof(ifr));

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		struct ifaddr *ifa;
		if_acquire(ifp, &psref);
		pserialize_read_exit(s);

		(void)strncpy(ifr.ifr_name, ifp->if_xname,
		    sizeof(ifr.ifr_name));
		if (ifr.ifr_name[sizeof(ifr.ifr_name) - 1] != '\0') {
			error = ENAMETOOLONG;
			goto release_exit;
		}

		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			struct psref psref_ifa;
			ifa_acquire(ifa, &psref_ifa);
			pserialize_read_exit(s);

			sa = ifa->ifa_addr;
			if (sa->sa_family != AF_INET ||
			    sa->sa_len > sizeof(*osa))
				goto next;
			memcpy(&ifr.ifr_addr, sa, sa->sa_len);
			osa = (struct osockaddr *)&ifr.ifr_addr;
			osa->sa_family = sa->sa_family;
			if (space >= sz) {
				error = copyout(&ifr, ifrp, sz);
				if (error != 0) {
					ifa_release(ifa, &psref_ifa);
					goto release_exit;
				}
				ifrp++;
			}
			space -= sz;
		next:
			s = pserialize_read_enter();
			ifa_release(ifa, &psref_ifa);
		}

		KASSERT(pserialize_in_read_section());
		if_release(ifp, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	if (docopy)
		ifc.ifc_len -= space;
	else
		ifc.ifc_len = -space;

	return copyout(&ifc, data, sizeof(ifc));

release_exit:
	if_release(ifp, &psref);
	curlwp_bindx(bound);
	return error;
}

int
linux_getifhwaddr(struct lwp *l, register_t *retval, u_int fd,
    void *data)
{
	/* Not the full structure, just enough to map what we do here */
	struct linux_ifreq lreq;
	file_t *fp;
	struct ifaddr *ifa;
	struct ifnet *ifp;
	struct sockaddr_dl *sadl;
	int error, found;
	int index, ifnum;
	int s;

	/*
	 * We can't emulate this ioctl by calling sys_ioctl() to run
	 * SIOCGIFCONF, because the user buffer is not of the right
	 * type to take those results.  We can't use kernel buffers to
	 * receive the results, as the implementation of sys_ioctl()
	 * and ifconf() [which implements SIOCGIFCONF] use
	 * copyin()/copyout() which will fail on kernel addresses.
	 *
	 * So, we must duplicate code from sys_ioctl() and ifconf().  Ugh.
	 */

	if ((fp = fd_getfile(fd)) == NULL)
		return (EBADF);

	KERNEL_LOCK(1, NULL);

	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	error = copyin(data, &lreq, sizeof(lreq));
	if (error)
		goto out;
	lreq.ifr_name[LINUX_IFNAMSIZ-1] = '\0';		/* just in case */

	/*
	 * Try real interface name first, then fake "ethX"
	 */
	found = 0;
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		if (found)
			break;
		if (strcmp(lreq.ifr_name, ifp->if_xname))
			/* not this interface */
			continue;

		found=1;
		if (IFADDR_READER_EMPTY(ifp)) {
			pserialize_read_exit(s);
			error = ENODEV;
			goto out;
		}
		IFADDR_READER_FOREACH(ifa, ifp) {
			sadl = satosdl(ifa->ifa_addr);
			/* only return ethernet addresses */
			/* XXX what about FDDI, etc. ? */
			if (sadl->sdl_family != AF_LINK ||
			    sadl->sdl_type != IFT_ETHER)
				continue;
			memcpy(&lreq.ifr_hwaddr.sa_data, CLLADDR(sadl),
			       MIN(sadl->sdl_alen,
				   sizeof(lreq.ifr_hwaddr.sa_data)));
			lreq.ifr_hwaddr.sa_family =
				sadl->sdl_family;
			pserialize_read_exit(s);

			error = copyout(&lreq, data, sizeof(lreq));
			goto out;
		}
	}
	pserialize_read_exit(s);

	if (strncmp(lreq.ifr_name, "eth", 3) != 0) {
		/* unknown interface, not even an "eth*" name */
		error = ENODEV;
		goto out;
	}

	for (ifnum = 0, index = 3;
	     index < LINUX_IFNAMSIZ && lreq.ifr_name[index] != '\0';
	     index++) {
		ifnum *= 10;
		ifnum += lreq.ifr_name[index] - '0';
	}

	error = EINVAL;			/* in case we don't find one */
	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		memcpy(lreq.ifr_name, ifp->if_xname,
		       MIN(LINUX_IFNAMSIZ, IFNAMSIZ));
		IFADDR_READER_FOREACH(ifa, ifp) {
			sadl = satosdl(ifa->ifa_addr);
			/* only return ethernet addresses */
			/* XXX what about FDDI, etc. ? */
			if (sadl->sdl_family != AF_LINK ||
			    sadl->sdl_type != IFT_ETHER)
				continue;
			if (ifnum--)
				/* not the requested iface */
				continue;
			memcpy(&lreq.ifr_hwaddr.sa_data,
			       CLLADDR(sadl),
			       MIN(sadl->sdl_alen,
				   sizeof(lreq.ifr_hwaddr.sa_data)));
			lreq.ifr_hwaddr.sa_family =
				sadl->sdl_family;
			pserialize_read_exit(s);

			error = copyout(&lreq, data, sizeof(lreq));
			goto out;
		}
	}
	pserialize_read_exit(s);

out:
	KERNEL_UNLOCK_ONE(NULL);
	fd_putfile(fd);
	return error;
}

int
linux_ioctl_socket(struct lwp *l, const struct linux_sys_ioctl_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(void *) data;
	} */
	u_long com;
	int error = 0, isdev = 0, dosys = 1;
	struct sys_ioctl_args ia;
	file_t *fp;
	struct vnode *vp;
	int (*ioctlf)(file_t *, u_long, void *);
	struct ioctl_pt pt;

	if ((fp = fd_getfile(SCARG(uap, fd))) == NULL)
		return (EBADF);

	if (fp->f_type == DTYPE_VNODE) {
		vp = (struct vnode *)fp->f_data;
		isdev = vp->v_type == VCHR;
	}

	/*
	 * Don't try to interpret socket ioctl calls that are done
	 * on a device filedescriptor, just pass them through, to
	 * emulate Linux behaviour. Use PTIOCLINUX so that the
	 * device will only handle these if it's prepared to do
	 * so, to avoid unexpected things from happening.
	 */
	if (isdev) {
		dosys = 0;
		ioctlf = fp->f_ops->fo_ioctl;
		pt.com = SCARG(uap, com);
		pt.data = SCARG(uap, data);
		error = ioctlf(fp, PTIOCLINUX, &pt);
		/*
		 * XXX hack: if the function returns EJUSTRETURN,
		 * it has stuffed a sysctl return value in pt.data.
		 */
		if (error == EJUSTRETURN) {
			retval[0] = (register_t)pt.data;
			error = 0;
		}
		goto out;
	}

	com = SCARG(uap, com);
	retval[0] = 0;

	switch (com) {
	case LINUX_SIOCGIFNAME:
		error = linux_getifname(l, retval, SCARG(uap, data));
		dosys = 0;
		break;
	case LINUX_SIOCGIFCONF:
		error = linux_getifconf(l, retval, SCARG(uap, data));
		dosys = 0;
		break;
	case LINUX_SIOCGIFFLAGS:
		SCARG(&ia, com) = OSIOCGIFFLAGS;
		break;
	case LINUX_SIOCSIFFLAGS:
		SCARG(&ia, com) = OSIOCSIFFLAGS;
		break;
	case LINUX_SIOCGIFADDR:
		SCARG(&ia, com) = OOSIOCGIFADDR;
		break;
	case LINUX_SIOCGIFDSTADDR:
		SCARG(&ia, com) = OOSIOCGIFDSTADDR;
		break;
	case LINUX_SIOCGIFBRDADDR:
		SCARG(&ia, com) = OOSIOCGIFBRDADDR;
		break;
	case LINUX_SIOCGIFNETMASK:
		SCARG(&ia, com) = OOSIOCGIFNETMASK;
		break;
	case LINUX_SIOCGIFMTU:
		SCARG(&ia, com) = OSIOCGIFMTU;
		break;
	case LINUX_SIOCADDMULTI:
		SCARG(&ia, com) = OSIOCADDMULTI;
		break;
	case LINUX_SIOCDELMULTI:
		SCARG(&ia, com) = OSIOCDELMULTI;
		break;
	case LINUX_SIOCGIFHWADDR:
		error = linux_getifhwaddr(l, retval, SCARG(uap, fd),
		    SCARG(uap, data));
		dosys = 0;
		break;
	default:
		error = EINVAL;
	}

 out:
 	fd_putfile(SCARG(uap, fd));

	if (error ==0 && dosys) {
		SCARG(&ia, fd) = SCARG(uap, fd);
		SCARG(&ia, data) = SCARG(uap, data);
		error = sys_ioctl(curlwp, &ia, retval);
	}

	return error;
}

int
linux_sys_connect(struct lwp *l, const struct linux_sys_connect_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(const struct sockaddr *) name;
		syscallarg(int) namelen;
	} */
	int		error;
	struct sockaddr_big sb;

	error = linux_get_sa(l, SCARG(uap, s), &sb, SCARG(uap, name),
	    SCARG(uap, namelen));
	if (error)
		return (error);

	error = do_sys_connect(l, SCARG(uap, s), (struct sockaddr *)&sb);

	if (error == EISCONN) {
		struct socket *so;
		int state, prflags;

		/* fd_getsock() will use the descriptor for us */
	    	if (fd_getsock(SCARG(uap, s), &so) != 0)
		    	return EISCONN;

		solock(so);
		state = so->so_state;
		prflags = so->so_proto->pr_flags;
		sounlock(so);
		fd_putfile(SCARG(uap, s));
		/*
		 * We should only let this call succeed once per
		 * non-blocking connect; however we don't have
		 * a convenient place to keep that state..
		 */
		if ((state & (SS_ISCONNECTED|SS_NBIO)) ==
		    (SS_ISCONNECTED|SS_NBIO) &&
		    (prflags & PR_CONNREQUIRED))
			return 0;
	}

	return (error);
}

int
linux_sys_bind(struct lwp *l, const struct linux_sys_bind_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(const struct osockaddr *) name;
		syscallarg(int) namelen;
	} */
	int		error;
	struct sockaddr_big sb;

	error = linux_get_sa(l, SCARG(uap, s), &sb, SCARG(uap, name),
	    SCARG(uap, namelen));
	if (error)
		return (error);

	return do_sys_bind(l, SCARG(uap, s), (struct sockaddr *)&sb);
}

int
linux_sys_getsockname(struct lwp *l, const struct linux_sys_getsockname_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fdes;
		syscallarg(void *) asa;
		syscallarg(int *) alen;
	} */
	int error;

	if ((error = sys_getsockname(l, (const void *)uap, retval)) != 0)
		return (error);

	if ((error = linux_sa_put((struct osockaddr *)SCARG(uap, asa))))
		return (error);

	return (0);
}

int
linux_sys_getpeername(struct lwp *l, const struct linux_sys_getpeername_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) fdes;
		syscallarg(void *) asa;
		syscallarg(int *) alen;
	} */
	int error;

	if ((error = sys_getpeername(l, (const void *)uap, retval)) != 0)
		return (error);

	if ((error = linux_sa_put((struct osockaddr *)SCARG(uap, asa))))
		return (error);

	return (0);
}

/*
 * Copy the osockaddr structure pointed to by name to sb, adjust
 * family and convert to sockaddr.
 */
static int
linux_get_sa(struct lwp *l, int s, struct sockaddr_big *sb,
    const struct osockaddr *name, socklen_t namelen)
{
	int error, bdom;

	if (namelen > UCHAR_MAX ||
	    namelen <= offsetof(struct sockaddr_big, sb_data))
		return EINVAL;

	error = copyin(name, sb, namelen);
	if (error)
		return error;

	bdom = linux_to_bsd_domain(sb->sb_family);
	if (bdom == -1)
		return EINVAL;

	/*
	 * If the family is unspecified, use address family of the socket.
	 * This avoid triggering strict family checks in netinet/in_pcb.c et.al.
	 */
	if (bdom == AF_UNSPEC) {
		struct socket *so;

		/* fd_getsock() will use the descriptor for us */
		if ((error = fd_getsock(s, &so)) != 0)
			return error;

		bdom = so->so_proto->pr_domain->dom_family;
		fd_putfile(s);
	}

	/*
	 * Older Linux IPv6 code uses obsolete RFC2133 struct sockaddr_in6,
	 * which lacks the scope id compared with RFC2553 one. If we detect
	 * the situation, reject the address and write a message to system log.
	 *
	 * Still accept addresses for which the scope id is not used.
	 */
	if (bdom == AF_INET6 &&
	    namelen == sizeof(struct sockaddr_in6) - sizeof(uint32_t)) {
		struct sockaddr_in6 *sin6 = (struct sockaddr_in6 *)sb;
		if (!IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr) &&
		    (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) ||
		     IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr) ||
		     IN6_IS_ADDR_V4COMPAT(&sin6->sin6_addr) ||
		     IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) ||
		     IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr))) {
			struct proc *p = l->l_proc;
			int uid = l->l_cred ? kauth_cred_geteuid(l->l_cred) : -1;

			log(LOG_DEBUG,
			    "pid %d (%s), uid %d: obsolete pre-RFC2553 "
			    "sockaddr_in6 rejected",
			    p->p_pid, p->p_comm, uid);
			return EINVAL;
		}
		namelen = sizeof(struct sockaddr_in6);
		sin6->sin6_scope_id = 0;
	}

	/*
	 * Linux is less strict than NetBSD and permits namelen to be larger
	 * than valid struct sockaddr_in*.  If this is the case, truncate
	 * the value to the correct size, so that NetBSD networking does not
	 * return an error.
	 */
	switch (bdom) {
	case AF_INET:
		namelen = MIN(namelen, sizeof(struct sockaddr_in));
		break;
	case AF_INET6:
		namelen = MIN(namelen, sizeof(struct sockaddr_in6));
		break;
	}

	sb->sb_family = bdom;
	sb->sb_len = namelen;
	ktrkuser("mbsoname", sb, namelen);
	return 0;
}

static int
linux_sa_put(struct osockaddr *osa)
{
	struct sockaddr sa;
	struct osockaddr *kosa;
	int error, bdom, len;

	/*
	 * Only read/write the sockaddr family and length part, the rest is
	 * not changed.
	 */
	len = sizeof(sa.sa_len) + sizeof(sa.sa_family);

	error = copyin(osa, &sa, len);
	if (error)
		return (error);

	bdom = bsd_to_linux_domain(sa.sa_family);
	if (bdom == -1)
		return (EINVAL);

	/* Note: we convert from sockaddr to osockaddr here, too */
	kosa = (struct osockaddr *) &sa;
	kosa->sa_family = bdom;
	error = copyout(kosa, osa, len);
	if (error)
		return (error);

	return (0);
}

#if !defined(__aarch64__) && !defined(__amd64__)
int
linux_sys_recv(struct lwp *l, const struct linux_sys_recv_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */
	struct sys_recvfrom_args bra;


	SCARG(&bra, s) = SCARG(uap, s);
	SCARG(&bra, buf) = SCARG(uap, buf);
	SCARG(&bra, len) = (size_t) SCARG(uap, len);
	SCARG(&bra, flags) = SCARG(uap, flags);
	SCARG(&bra, from) = NULL;
	SCARG(&bra, fromlenaddr) = NULL;

	return (sys_recvfrom(l, &bra, retval));
}

int
linux_sys_send(struct lwp *l, const struct linux_sys_send_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */
	struct sys_sendto_args bsa;

	SCARG(&bsa, s)		= SCARG(uap, s);
	SCARG(&bsa, buf)	= SCARG(uap, buf);
	SCARG(&bsa, len)	= SCARG(uap, len);
	SCARG(&bsa, flags)	= SCARG(uap, flags);
	SCARG(&bsa, to)		= NULL;
	SCARG(&bsa, tolen)	= 0;

	return (sys_sendto(l, &bsa, retval));
}
#endif

int
linux_sys_accept(struct lwp *l, const struct linux_sys_accept_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(struct osockaddr *) name;
		syscallarg(int *) anamelen;
	} */
	int error;
	struct sys_accept_args baa;

	SCARG(&baa, s)		= SCARG(uap, s);
	SCARG(&baa, name)	= (struct sockaddr *) SCARG(uap, name);
	SCARG(&baa, anamelen)	= (unsigned int *) SCARG(uap, anamelen);

	if ((error = sys_accept(l, &baa, retval)))
		return (error);

	if (SCARG(uap, name) && (error = linux_sa_put(SCARG(uap, name))))
		return (error);

	return (0);
}

int
linux_sys_accept4(struct lwp *l, const struct linux_sys_accept4_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(struct osockaddr *) name;
		syscallarg(int *) anamelen;
		syscallarg(int) flags;
	} */
	int error, flags;
	struct sockaddr_big name;

	if ((flags = linux_to_bsd_type(SCARG(uap, flags))) == -1)
		return EINVAL;

	name.sb_len = UCHAR_MAX;
	error = do_sys_accept(l, SCARG(uap, s), (struct sockaddr *)&name,
	    retval, NULL, flags, 0);
	if (error != 0)
		return error;

	error = copyout_sockname_sb((struct sockaddr *)SCARG(uap, name),
	    SCARG(uap, anamelen), MSG_LENUSRSPACE, &name);
	if (error != 0) {
		int fd = (int)*retval;
		if (fd_getfile(fd) != NULL)
			(void)fd_close(fd);
		return error;
	}
	if (SCARG(uap, name) && (error = linux_sa_put(SCARG(uap, name))))
		return error;

	return 0;
}

int
linux_sys_sendmmsg(struct lwp *l, const struct linux_sys_sendmmsg_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(struct linux_mmsghdr *) msgvec;
		syscallarg(unsigned int) vlen;
		syscallarg(unsigned int) flags;
	} */
	struct linux_mmsghdr lmsg;
	struct mmsghdr bmsg;
	struct socket *so;
	file_t *fp;
	struct msghdr *msg = &bmsg.msg_hdr;
	int error, s;
	unsigned int vlen, flags, dg;

	if ((flags = linux_to_bsd_msg_flags(SCARG(uap, flags))) == -1)
		return EINVAL;

	flags = (flags & MSG_USERFLAGS) | MSG_IOVUSRSPACE;

	s = SCARG(uap, s);
	if ((error = fd_getsock1(s, &so, &fp)) != 0)
		return error;

	vlen = SCARG(uap, vlen);
	if (vlen > 1024)
		vlen = 1024;

	for (dg = 0; dg < vlen;) {
		error = copyin(SCARG(uap, msgvec) + dg, &lmsg, sizeof(lmsg));
		if (error)
			break;
		linux_to_bsd_msghdr(&lmsg.msg_hdr, &bmsg.msg_hdr);

		msg->msg_flags = flags;

		error = do_sys_sendmsg_so(l, s, so, fp, msg, flags, retval);
		if (error)
			break;

		ktrkuser("msghdr", msg, sizeof *msg);
		lmsg.msg_len = *retval;
		error = copyout(&lmsg, SCARG(uap, msgvec) + dg, sizeof(lmsg));
		if (error)
			break;
		dg++;

	}

	*retval = dg;

	fd_putfile(s);

	/*
	 * If we succeeded at least once, return 0.
	 */
	if (dg)
		return 0;
	return error;
}

int
linux_sys_recvmmsg(struct lwp *l, const struct linux_sys_recvmmsg_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(struct linux_mmsghdr *) msgvec;
		syscallarg(unsigned int) vlen;
		syscallarg(unsigned int) flags;
		syscallarg(struct linux_timespec *) timeout;
	} */
	struct linux_mmsghdr lmsg;
	struct mmsghdr bmsg;
	struct socket *so;
	struct msghdr *msg = &bmsg.msg_hdr;
	int error, s;
	struct mbuf *from, *control;
	struct timespec ts = {0}, now;
	struct linux_timespec lts;
	unsigned int vlen, flags, dg;

	if (SCARG(uap, timeout)) {
		error = copyin(SCARG(uap, timeout), &lts, sizeof(lts));
			return error;
		ts.tv_sec = lts.tv_sec;
		ts.tv_nsec = lts.tv_nsec;
		getnanotime(&now);
		timespecadd(&now, &ts, &ts);
	}

	s = SCARG(uap, s);
	if ((error = fd_getsock(s, &so)) != 0)
		return error;

	/*
	 * If so->so_rerror holds a deferred error return it now.
	 */
	if (so->so_rerror) {
		error = so->so_rerror;
		so->so_rerror = 0;
		fd_putfile(s);
		return error;
	}

	vlen = SCARG(uap, vlen);
	if (vlen > 1024)
		vlen = 1024;

	from = NULL;
	flags = (SCARG(uap, flags) & MSG_USERFLAGS) | MSG_IOVUSRSPACE;

	for (dg = 0; dg < vlen;) {
		error = copyin(SCARG(uap, msgvec) + dg, &lmsg, sizeof(lmsg));
		if (error)
			break;
		linux_to_bsd_msghdr(&lmsg.msg_hdr, &bmsg.msg_hdr);
		msg->msg_flags = flags & ~MSG_WAITFORONE;

		if (from != NULL) {
			m_free(from);
			from = NULL;
		}

		error = do_sys_recvmsg_so(l, s, so, msg, &from,
		    msg->msg_control != NULL ? &control : NULL, retval);
		if (error) {
			if (error == EAGAIN && dg > 0)
				error = 0;
			break;
		}

		if (msg->msg_control != NULL)
			error = linux_copyout_msg_control(l, msg, control);
		if (error)
			break;

		if (from != NULL) {
			mtod(from, struct osockaddr *)->sa_family =
			    bsd_to_linux_domain(mtod(from,
			    struct sockaddr *)->sa_family);
			error = copyout_sockname(msg->msg_name,
			    &msg->msg_namelen, 0, from);
			if (error)
				break;
		}


		lmsg.msg_len = *retval;
		ktrkuser("msghdr", msg, sizeof(*msg));
		bsd_to_linux_msghdr(msg, &lmsg.msg_hdr);
		error = copyout(&lmsg, SCARG(uap, msgvec) + dg, sizeof(lmsg));
		if (error)
			break;

		dg++;
		if (msg->msg_flags & MSG_OOB)
			break;

		if (SCARG(uap, timeout)) {
			getnanotime(&now);
			timespecsub(&now, &ts, &now);
			if (now.tv_sec > 0)
				break;
		}

		if (flags & MSG_WAITFORONE)
			flags |= MSG_DONTWAIT;

	}

	if (from != NULL)
		m_free(from);

	*retval = dg;

	/*
	 * If we succeeded at least once, return 0, hopefully so->so_rerror
	 * will catch it next time.
	 */
	if (error && dg > 0) {
		so->so_rerror = error;
		error = 0;
	}

	fd_putfile(s);

	return error;
}
