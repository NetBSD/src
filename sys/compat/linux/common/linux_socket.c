/*	$NetBSD: linux_socket.c,v 1.48 2003/07/27 05:04:02 mrg Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Functions in multiarch:
 *	linux_sys_socketcall		: linux_socketcall.c
 *
 * XXX Note: Linux CMSG_ALIGN() uses (sizeof(long)-1). For architectures
 * where our CMSG_ALIGN() differs (like powerpc, sparc, sparc64), the passed
 * control structure would need to be adjusted accordingly in sendmsg() and
 * recvmsg().
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: linux_socket.c,v 1.48 2003/07/27 05:04:02 mrg Exp $");

#if defined(_KERNEL_OPT)
#include "opt_inet.h"
#endif

#include <sys/param.h>
#include <sys/kernel.h>
#include <sys/systm.h>
#include <sys/buf.h>
#include <sys/malloc.h>
#include <sys/ioctl.h>
#include <sys/tty.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
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

#include <sys/sa.h>
#include <sys/syscallargs.h>

#ifdef INET6
#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#endif

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_socket.h>
#include <compat/linux/common/linux_socketcall.h>
#include <compat/linux/common/linux_sockio.h>

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

static int linux_to_bsd_domain __P((int));
static int bsd_to_linux_domain __P((int));
int linux_to_bsd_sopt_level __P((int));
int linux_to_bsd_so_sockopt __P((int));
int linux_to_bsd_ip_sockopt __P((int));
int linux_to_bsd_tcp_sockopt __P((int));
int linux_to_bsd_udp_sockopt __P((int));
int linux_getifhwaddr __P((struct proc *, register_t *, u_int, void *));
static int linux_sa_get __P((struct proc *, caddr_t *sgp, struct sockaddr **sap,
		const struct osockaddr *osa, int *osalen));
static int linux_sa_put __P((struct osockaddr *osa));

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

/*
 * Convert between Linux and BSD socket domain values
 */
static int
linux_to_bsd_domain(ldom)
	int ldom;
{
	if (ldom < 0 || ldom >= LINUX_AF_MAX)
		return (-1);

	return linux_to_bsd_domain_[ldom];
}

/*
 * Convert between BSD and Linux socket domain values
 */
static int
bsd_to_linux_domain(bdom)
	int bdom;
{
	if (bdom < 0 || bdom >= AF_MAX)
		return (-1);

	return bsd_to_linux_domain_[bdom];
}

int
linux_sys_socket(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_socket_args /* {
		syscallarg(int)	domain;
		syscallarg(int)	type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct sys_socket_args bsa;
	int error;

	SCARG(&bsa, protocol) = SCARG(uap, protocol);
	SCARG(&bsa, type) = SCARG(uap, type);
	SCARG(&bsa, domain) = linux_to_bsd_domain(SCARG(uap, domain));
	if (SCARG(&bsa, domain) == -1)
		return EINVAL;
	error = sys_socket(l, &bsa, retval);

#ifdef INET6
	/*
	 * Linux AF_INET6 socket has IPV6_V6ONLY setsockopt set to 0 by
	 * default and some apps depend on this. So, set V6ONLY to 0
	 * for Linux apps if the sysctl value is set to 1.
	 */
	if (!error && ip6_v6only && SCARG(&bsa, domain) == PF_INET6) {
		struct proc *p = l->l_proc;
		struct file *fp;

		if (getsock(p->p_fd, *retval, &fp) == 0) {
			struct mbuf *m;

			m = m_get(M_WAIT, MT_SOOPTS);
			m->m_len = sizeof(int);
			*mtod(m, int *) = 0;

			/* ignore error */
			(void) sosetopt((struct socket *)fp->f_data,
				IPPROTO_IPV6, IPV6_V6ONLY, m);

			FILE_UNUSE(fp, p);
		}
	}
#endif

	return (error);
}

int
linux_sys_socketpair(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_socketpair_args /* {
		syscallarg(int) domain;
		syscallarg(int) type;
		syscallarg(int) protocol;
		syscallarg(int *) rsv;
	} */ *uap = v;
	struct sys_socketpair_args bsa;

	SCARG(&bsa, domain) = linux_to_bsd_domain(SCARG(uap, domain));
	if (SCARG(&bsa, domain) == -1)
		return EINVAL;
	SCARG(&bsa, type) = SCARG(uap, type);
	SCARG(&bsa, protocol) = SCARG(uap, protocol);
	SCARG(&bsa, rsv) = SCARG(uap, rsv);

	return sys_socketpair(l, &bsa, retval);
}

int
linux_sys_sendto(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sendto_args /* {
		syscallarg(int)				s;
		syscallarg(void *)			msg;
		syscallarg(int)				len;
		syscallarg(int)				flags;
		syscallarg(struct osockaddr *)		to;
		syscallarg(int)				tolen;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct sys_sendto_args bsa;
	int tolen;

	SCARG(&bsa, s) = SCARG(uap, s);
	SCARG(&bsa, buf) = SCARG(uap, msg);
	SCARG(&bsa, len) = (size_t) SCARG(uap, len);
	SCARG(&bsa, flags) = SCARG(uap, flags);
	tolen = SCARG(uap, tolen);
	if (SCARG(uap, to)) {
		struct sockaddr *sa;
		int error;
		caddr_t sg = stackgap_init(p, 0);

		if ((error = linux_sa_get(p, &sg, &sa, SCARG(uap, to), &tolen)))
			return (error);

		SCARG(&bsa, to) = sa;
	} else
		SCARG(&bsa, to) = NULL;
	SCARG(&bsa, tolen) = tolen;

	return (sys_sendto(l, &bsa, retval));
}

int
linux_sys_sendmsg(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_sendmsg_args /* {
		syscallarg(int) s;
		syscallarg(struct msghdr *) msg;
		syscallarg(u_int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct msghdr	msg;
	int		error;
	struct sys_sendmsg_args bsa;
	struct msghdr *nmsg = (struct msghdr *)SCARG(uap, msg);

	error = copyin(SCARG(uap, msg), (caddr_t)&msg, sizeof(msg));
	if (error)
		return (error);

	if (msg.msg_name) {
		struct sockaddr *sa;
		caddr_t sg = stackgap_init(p, 0);

		nmsg = (struct msghdr *) stackgap_alloc(p, &sg,
		    sizeof(struct msghdr));
		if (!nmsg)
			return (ENOMEM);

		error = linux_sa_get(p, &sg, &sa,
		    (struct osockaddr *) msg.msg_name, &msg.msg_namelen);
		if (error)
			return (error);

		msg.msg_name = (struct sockaddr *) sa;
		if ((error = copyout(&msg, nmsg, sizeof(struct msghdr))))
			return (error);
	}

	/*
	 * XXX handle different alignment of cmsg data on architectures where
	 * the Linux alignment is different (powerpc, sparc, sparc64).
	 */

	SCARG(&bsa, s) = SCARG(uap, s);
	SCARG(&bsa, msg) = nmsg;
	SCARG(&bsa, flags) = SCARG(uap, flags);

	if ((error = sys_sendmsg(l, &bsa, retval)))
		return (error);

	return (0);
}


int
linux_sys_recvfrom(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(struct osockaddr *) from;
		syscallarg(int *) fromlenaddr;
	} */ *uap = v;
	int		error;
	struct sys_recvfrom_args bra;

	SCARG(&bra, s) = SCARG(uap, s);
	SCARG(&bra, buf) = SCARG(uap, buf);
	SCARG(&bra, len) = SCARG(uap, len);
	SCARG(&bra, flags) = SCARG(uap, flags);
	SCARG(&bra, from) = (struct sockaddr *) SCARG(uap, from);
	SCARG(&bra, fromlenaddr) = SCARG(uap, fromlenaddr);

	if ((error = sys_recvfrom(l, &bra, retval)))
		return (error);

	if (SCARG(uap, from) && (error = linux_sa_put(SCARG(uap, from))))
		return (error);

	return (0);
}

int
linux_sys_recvmsg(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_recvmsg_args /* {
		syscallarg(int) s;
		syscallarg(struct msghdr *) msg;
		syscallarg(u_int) flags;
	} */ *uap = v;
	struct msghdr	msg;
	int		error;

	if ((error = sys_recvmsg(l, v, retval)))
		return (error);

	error = copyin((caddr_t)SCARG(uap, msg), (caddr_t)&msg,
		       sizeof(msg));

	if (!error && msg.msg_name && msg.msg_namelen > 2)
		error = linux_sa_put(msg.msg_name);
		
	/*
	 * XXX handle different alignment of cmsg data on architectures where
	 * the Linux alignment is different (powerpc, sparc, sparc64).
	 */

	return (error);
}

/*
 * Convert socket option level from Linux to NetBSD value. Only SOL_SOCKET
 * is different, the rest matches IPPROTO_* on both systems.
 */
int
linux_to_bsd_sopt_level(llevel)
	int llevel;
{

	switch (llevel) {
	case LINUX_SOL_SOCKET:
		return SOL_SOCKET;
	case LINUX_SOL_IP:
		return IPPROTO_IP;
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
linux_to_bsd_so_sockopt(lopt)
	int lopt;
{

	switch (lopt) {
	case LINUX_SO_DEBUG:
		return SO_DEBUG;
	case LINUX_SO_REUSEADDR:
		/* 
		 * Linux does not implement SO_REUSEPORT, but allows reuse of a
		 * host:port pair through SO_REUSEADDR even if the address is not a
		 * multicast-address.  Effectively, this means that we should use
		 * SO_REUSEPORT to allow Linux applications to not exit with
		 * EADDRINUSE
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
	case LINUX_SO_LINGER:
		return SO_LINGER;
	case LINUX_SO_PRIORITY:
	case LINUX_SO_NO_CHECK:
	default:
		return -1;
	}
}

/*
 * Convert Linux IP level socket option number to NetBSD values.
 */
int
linux_to_bsd_ip_sockopt(lopt)
	int lopt;
{

	switch (lopt) {
	case LINUX_IP_TOS:
		return IP_TOS;
	case LINUX_IP_TTL:
		return IP_TTL;
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
	default:
		return -1;
	}
}

/*
 * Convert Linux TCP level socket option number to NetBSD values.
 */
int
linux_to_bsd_tcp_sockopt(lopt)
	int lopt;
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
linux_to_bsd_udp_sockopt(lopt)
	int lopt;
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
linux_sys_setsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_setsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(void *) optval;
		syscallarg(int) optlen;
	} */ *uap = v;
	struct sys_setsockopt_args bsa;
	int name;

	SCARG(&bsa, s) = SCARG(uap, s);
	SCARG(&bsa, level) = linux_to_bsd_sopt_level(SCARG(uap, level));
	SCARG(&bsa, val) = SCARG(uap, optval);
	SCARG(&bsa, valsize) = SCARG(uap, optlen);

	switch (SCARG(&bsa, level)) {
	case SOL_SOCKET:
		name = linux_to_bsd_so_sockopt(SCARG(uap, optname));
		break;
	case IPPROTO_IP:
		name = linux_to_bsd_ip_sockopt(SCARG(uap, optname));
		break;
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
	SCARG(&bsa, name) = name;

	return sys_setsockopt(l, &bsa, retval);
}

/*
 * getsockopt(2) is very much the same as setsockopt(2) (see above)
 */
int
linux_sys_getsockopt(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_getsockopt_args /* {
		syscallarg(int) s;
		syscallarg(int) level;
		syscallarg(int) optname;
		syscallarg(void *) optval;
		syscallarg(int *) optlen;
	} */ *uap = v;
	struct sys_getsockopt_args bga;
	int name;

	SCARG(&bga, s) = SCARG(uap, s);
	SCARG(&bga, level) = linux_to_bsd_sopt_level(SCARG(uap, level));
	SCARG(&bga, val) = SCARG(uap, optval);
	SCARG(&bga, avalsize) = SCARG(uap, optlen);

	switch (SCARG(&bga, level)) {
	case SOL_SOCKET:
		name = linux_to_bsd_so_sockopt(SCARG(uap, optname));
		break;
	case IPPROTO_IP:
		name = linux_to_bsd_ip_sockopt(SCARG(uap, optname));
		break;
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

#define IF_NAME_LEN 16

int
linux_getifhwaddr(p, retval, fd, data)
	struct proc *p;
	register_t *retval;
	u_int fd;
	void *data;
{
	/* Not the full structure, just enough to map what we do here */
	struct linux_ifreq {
		char if_name[IF_NAME_LEN];
		struct osockaddr hwaddr;
	} lreq;
	struct filedesc *fdp;
	struct file *fp;
	struct ifaddr *ifa;
	struct ifnet *ifp;
	struct sockaddr_dl *sadl;
	int error, found;
	int index, ifnum;

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

	fdp = p->p_fd;
	if ((fp = fd_getfile(fdp, fd)) == NULL)
		return (EBADF);

	FILE_USE(fp);
	if ((fp->f_flag & (FREAD | FWRITE)) == 0) {
		error = EBADF;
		goto out;
	}

	error = copyin(data, (caddr_t)&lreq, sizeof(lreq));
	if (error)
		goto out;
	lreq.if_name[IF_NAME_LEN-1] = '\0';		/* just in case */

	/*
	 * Try real interface name first, then fake "ethX"
	 */
	for (ifp = ifnet.tqh_first, found = 0;
	     ifp != 0 && !found;
	     ifp = ifp->if_list.tqe_next) {
		if (strcmp(lreq.if_name, ifp->if_xname))
			/* not this interface */
			continue;
		found=1;           
		if ((ifa = ifp->if_addrlist.tqh_first) != 0) {
			for (; ifa != 0; ifa = ifa->ifa_list.tqe_next) {
				sadl = (struct sockaddr_dl *)ifa->ifa_addr;
				/* only return ethernet addresses */
				/* XXX what about FDDI, etc. ? */
				if (sadl->sdl_family != AF_LINK ||
				    sadl->sdl_type != IFT_ETHER)
					continue;
				memcpy((caddr_t)&lreq.hwaddr.sa_data,
				       LLADDR(sadl),
				       MIN(sadl->sdl_alen,
					   sizeof(lreq.hwaddr.sa_data)));
				lreq.hwaddr.sa_family =
					sadl->sdl_family;
				error = copyout((caddr_t)&lreq, data,
						sizeof(lreq));
				goto out; 
			}
		} else {
			error = ENODEV;
			goto out;
		}
	}

	if (strncmp(lreq.if_name, "eth", 3) == 0) {
		for (ifnum = 0, index = 3;
		     lreq.if_name[index] != '\0' && index < IF_NAME_LEN;
		     index++) {
			ifnum *= 10;
			ifnum += lreq.if_name[index] - '0';
		}

		error = EINVAL;			/* in case we don't find one */
		for (ifp = ifnet.tqh_first, found = 0;
		     ifp != 0 && !found;
		     ifp = ifp->if_list.tqe_next) {
			memcpy(lreq.if_name, ifp->if_xname,
			       MIN(IF_NAME_LEN, IFNAMSIZ));
			if ((ifa = ifp->if_addrlist.tqh_first) == 0)
				/* no addresses on this interface */
				continue;
			else 
				for (; ifa != 0; ifa = ifa->ifa_list.tqe_next) {
					sadl = (struct sockaddr_dl *)ifa->ifa_addr;
					/* only return ethernet addresses */
					/* XXX what about FDDI, etc. ? */
					if (sadl->sdl_family != AF_LINK ||
					    sadl->sdl_type != IFT_ETHER)
						continue;
					if (ifnum--)
						/* not the reqested iface */
						continue;
					memcpy((caddr_t)&lreq.hwaddr.sa_data,
					       LLADDR(sadl),
					       MIN(sadl->sdl_alen,
						   sizeof(lreq.hwaddr.sa_data)));
					lreq.hwaddr.sa_family =
						sadl->sdl_family;
					error = copyout((caddr_t)&lreq, data,
							sizeof(lreq));
					found = 1;
					break;
				}
		}
	} else {
		/* unknown interface, not even an "eth*" name */
		error = ENODEV;
	}
    
out:
	FILE_UNUSE(fp, p);
	return error;
}
#undef IF_NAME_LEN

int
linux_ioctl_socket(p, uap, retval)
	struct proc *p;
	struct linux_sys_ioctl_args /* {
		syscallarg(int) fd;
		syscallarg(u_long) com;
		syscallarg(caddr_t) data;
	} */ *uap;
	register_t *retval;
{
	u_long com;
	int error = 0, isdev = 0, dosys = 1;
	struct sys_ioctl_args ia;
	struct file *fp;
	struct filedesc *fdp;
	struct vnode *vp;
	int (*ioctlf)(struct file *, u_long, void *, struct proc *);
	struct ioctl_pt pt;

        fdp = p->p_fd;
	if ((fp = fd_getfile(fdp, SCARG(uap, fd))) == NULL)
		return (EBADF);

	FILE_USE(fp);

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
		error = ioctlf(fp, PTIOCLINUX, (caddr_t)&pt, p);
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
	case LINUX_SIOCGIFCONF:
		SCARG(&ia, com) = OSIOCGIFCONF;
		break;
	case LINUX_SIOCGIFFLAGS:
		SCARG(&ia, com) = SIOCGIFFLAGS;
		break;
	case LINUX_SIOCSIFFLAGS:
		SCARG(&ia, com) = SIOCSIFFLAGS;
		break;
	case LINUX_SIOCGIFADDR:
		SCARG(&ia, com) = OSIOCGIFADDR;
		break;
	case LINUX_SIOCGIFDSTADDR:
		SCARG(&ia, com) = OSIOCGIFDSTADDR;
		break;
	case LINUX_SIOCGIFBRDADDR:
		SCARG(&ia, com) = OSIOCGIFBRDADDR;
		break;
	case LINUX_SIOCGIFNETMASK:
		SCARG(&ia, com) = OSIOCGIFNETMASK;
		break;
	case LINUX_SIOCADDMULTI:
		SCARG(&ia, com) = SIOCADDMULTI;
		break;
	case LINUX_SIOCDELMULTI:
		SCARG(&ia, com) = SIOCDELMULTI;
		break;
	case LINUX_SIOCGIFHWADDR:
	        error = linux_getifhwaddr(p, retval, SCARG(uap, fd),
					 SCARG(uap, data));
		dosys = 0;
		break;
	default:
		error = EINVAL;
	}

out:
	FILE_UNUSE(fp, p);

	if (error ==0 && dosys) {
		SCARG(&ia, fd) = SCARG(uap, fd);
		SCARG(&ia, data) = SCARG(uap, data);
		/* XXX NJWLWP */
		error = sys_ioctl(curlwp, &ia, retval);
	}

	return error;
}

int
linux_sys_connect(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_connect_args /* {
		syscallarg(int) s;
		syscallarg(const struct sockaddr *) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int		error;
	struct sockaddr *sa;
	struct sys_connect_args bca;
	caddr_t sg = stackgap_init(p, 0);
	int namlen;

	namlen = SCARG(uap, namelen);
	error = linux_sa_get(p, &sg, &sa, SCARG(uap, name), &namlen);
	if (error)
		return (error);
	
	SCARG(&bca, s) = SCARG(uap, s);
	SCARG(&bca, name) = sa;
	SCARG(&bca, namelen) = (unsigned int) namlen;

	error = sys_connect(l, &bca, retval);

	if (error == EISCONN) {
		struct file *fp;
		struct socket *so;
		int s, state, prflags;
		
		/* getsock() will use the descriptor for us */
	    	if (getsock(p->p_fd, SCARG(uap, s), &fp) != 0)
		    	return EISCONN;

		s = splsoftnet();
		so = (struct socket *)fp->f_data;
		state = so->so_state;
		prflags = so->so_proto->pr_flags;
		splx(s);
		FILE_UNUSE(fp, p);
		/*
		 * We should only let this call succeed once per
		 * non-blocking connect; however we don't have
		 * a convenient place to keep that state..
		 */
		if ((state & SS_NBIO) && (state & SS_ISCONNECTED) &&
		    (prflags & PR_CONNREQUIRED))
			return 0;
	}

	return (error);
}

int
linux_sys_bind(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_bind_args /* {
		syscallarg(int) s;
		syscallarg(const struct osockaddr *) name;
		syscallarg(int) namelen;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	int		error, namlen;
	struct sys_bind_args bsa;

	namlen = SCARG(uap, namelen);
	SCARG(&bsa, s) = SCARG(uap, s);
	if (SCARG(uap, name)) {
		struct sockaddr *sa;
		caddr_t sg = stackgap_init(p, 0);

		error = linux_sa_get(p, &sg, &sa, SCARG(uap, name), &namlen);
		if (error)
			return (error);

		SCARG(&bsa, name) = sa;
	} else
		SCARG(&bsa, name) = NULL;
	SCARG(&bsa, namelen) = namlen;

	return (sys_bind(l, &bsa, retval));
}

int
linux_sys_getsockname(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(caddr_t) asa;
		syscallarg(int *) alen;
	} */ *uap = v;
	int error;

	if ((error = sys_getsockname(l, uap, retval)) != 0)
		return (error);

	if ((error = linux_sa_put((struct osockaddr *)SCARG(uap, asa))))
		return (error);

	return (0);
}

int
linux_sys_getpeername(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct sys_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(caddr_t) asa;
		syscallarg(int *) alen;
	} */ *uap = v;
	int error;

	if ((error = sys_getpeername(l, uap, retval)) != 0)
		return (error);

	if ((error = linux_sa_put((struct osockaddr *)SCARG(uap, asa))))
		return (error);

	return (0);
}

/*
 * Copy the osockaddr structure pointed to by osa to kernel, adjust
 * family and convert to sockaddr, allocate stackgap and put the
 * the converted structure there, address on stackgap returned in sap.
 */
static int
linux_sa_get(p, sgp, sap, osa, osalen)
	struct proc *p;
	caddr_t *sgp;
	struct sockaddr **sap;
	const struct osockaddr *osa;
	int *osalen;
{
	int error=0, bdom;
	struct sockaddr *sa, *usa;
	struct osockaddr *kosa = (struct osockaddr *) &sa;
	int alloclen;
#ifdef INET6
	int oldv6size;
	struct sockaddr_in6 *sin6;
#endif

	if (*osalen < 2 || *osalen > UCHAR_MAX || !osa) {
		DPRINTF(("bad osa=%p osalen=%d\n", osa, *osalen));
		return (EINVAL);
	}

	alloclen = *osalen;
#ifdef INET6
	oldv6size = 0;
	/*
	 * Check for old (pre-RFC2553) sockaddr_in6. We may accept it
	 * if it's a v4-mapped address, so reserve the proper space
	 * for it.
	 */
	if (alloclen == sizeof (struct sockaddr_in6) - sizeof (u_int32_t)) {
		alloclen = sizeof (struct sockaddr_in6);
		oldv6size = 1;
	}
#endif

	kosa = (struct osockaddr *) malloc(alloclen, M_TEMP, M_WAITOK);

	if ((error = copyin(osa, (caddr_t) kosa, *osalen))) {
		DPRINTF(("error copying osa %d\n", error));
		goto out;
	}

	bdom = linux_to_bsd_domain(kosa->sa_family);
	if (bdom == -1) {
		DPRINTF(("bad linux family=%d\n", kosa->sa_family));
		error = EINVAL;
		goto out;
	}

#ifdef INET6
	/*
	 * Older Linux IPv6 code uses obsolete RFC2133 struct sockaddr_in6,
	 * which lacks the scope id compared with RFC2553 one. If we detect
	 * the situation, reject the address and write a message to system log.
	 *
	 * Still accept addresses for which the scope id is not used.
	 */
	if (oldv6size && bdom == AF_INET6) {
		sin6 = (struct sockaddr_in6 *)kosa;
		if (IN6_IS_ADDR_V4MAPPED(&sin6->sin6_addr) ||
		    (!IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr) &&
		     !IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr) &&
		     !IN6_IS_ADDR_V4COMPAT(&sin6->sin6_addr) &&
		     !IN6_IS_ADDR_UNSPECIFIED(&sin6->sin6_addr) &&
		     !IN6_IS_ADDR_MULTICAST(&sin6->sin6_addr))) {
			sin6->sin6_scope_id = 0;
		} else {
			struct proc *p = curproc;	/* XXX */
			int uid = p->p_cred && p->p_ucred ? 
					p->p_ucred->cr_uid : -1;

			log(LOG_DEBUG,
			    "pid %d (%s), uid %d: obsolete pre-RFC2553 "
			    "sockaddr_in6 rejected",
			    p->p_pid, p->p_comm, uid);
			error = EINVAL;
			goto out;
		}
	} else
#endif 
	if (bdom == AF_INET) {
		alloclen = sizeof(struct sockaddr_in);
	}

	sa = (struct sockaddr *) kosa;
	sa->sa_family = bdom;
	sa->sa_len = alloclen;
#ifdef DEBUG_LINUX
	DPRINTF(("family %d, len = %d [ ", sa->sa_family, sa->sa_len));
	for (bdom = 0; bdom < sizeof(sa->sa_data); bdom++)
	    DPRINTF(("%02x ", sa->sa_data[bdom]));
	DPRINTF(("\n"));
#endif

	usa = (struct sockaddr *) stackgap_alloc(p, sgp, alloclen);
	if (!usa) {
		error = ENOMEM;
		goto out;
	}

	if ((error = copyout(sa, usa, alloclen))) {
		DPRINTF(("error copying out socket %d\n", error));
		goto out;
	}

	*sap = usa;

    out:
	*osalen = alloclen;
	free(kosa, M_TEMP);
	return (error);
}

static int
linux_sa_put(osa)
	struct osockaddr *osa;
{
	struct sockaddr sa;
	struct osockaddr *kosa;
	int error, bdom, len;

	/*
	 * Only read/write the sockaddr family and length part, the rest is
	 * not changed.
	 */
	len = sizeof(sa.sa_len) + sizeof(sa.sa_family);

	error = copyin((caddr_t) osa, (caddr_t) &sa, len);
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

int
linux_sys_recv(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_recv_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
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
linux_sys_send(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_send_args /* {
		syscallarg(int) s;
		syscallarg(caddr_t) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
	} */ *uap = v;
	struct sys_sendto_args bsa;

	SCARG(&bsa, s)		= SCARG(uap, s);
	SCARG(&bsa, buf)	= SCARG(uap, buf);
	SCARG(&bsa, len)	= SCARG(uap, len);
	SCARG(&bsa, flags)	= SCARG(uap, flags);
	SCARG(&bsa, to)		= NULL;
	SCARG(&bsa, tolen)	= 0;

	return (sys_sendto(l, &bsa, retval));
}

int
linux_sys_accept(l, v, retval)
	struct lwp *l;
	void *v;
	register_t *retval;
{
	struct linux_sys_accept_args /* {
		syscallarg(int) s;
		syscallarg(struct osockaddr *) name;
		syscallarg(int *) anamelen;
	} */ *uap = v;
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
