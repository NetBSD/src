/*	$NetBSD: linux_socket.c,v 1.26 2000/12/22 23:41:16 fvdl Exp $	*/

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
 */

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

#include <sys/syscallargs.h>

#include <compat/linux/common/linux_types.h>
#include <compat/linux/common/linux_util.h>
#include <compat/linux/common/linux_signal.h>
#include <compat/linux/common/linux_ioctl.h>
#include <compat/linux/common/linux_socket.h>
#include <compat/linux/common/linux_socketcall.h>
#include <compat/linux/common/linux_sockio.h>

#include <compat/linux/linux_syscallargs.h>

#ifndef MIN
#define	MIN(a,b) (((a)<(b))?(a):(b))
#endif

/*
 * The calls in this file are entered either via the linux_socketcall()
 * interface or, on the Alpha, as individual syscalls.  The
 * linux_socketcall function does any massaging of arguments so that all
 * the calls in here need not think that they are anything other
 * than a normal syscall.
 */

int linux_to_bsd_domain __P((int));
int linux_to_bsd_sopt_level __P((int));
int linux_to_bsd_so_sockopt __P((int));
int linux_to_bsd_ip_sockopt __P((int));
int linux_to_bsd_tcp_sockopt __P((int));
int linux_to_bsd_udp_sockopt __P((int));
int linux_getifhwaddr __P((struct proc *, register_t *, u_int, void *));

/*
 * Convert between Linux and BSD socket domain values
 */
int
linux_to_bsd_domain(ldom)
	int ldom;
{

	switch (ldom) {
	case LINUX_AF_UNSPEC:
		return AF_UNSPEC;
	case LINUX_AF_UNIX:
		return AF_LOCAL;
	case LINUX_AF_INET:
		return AF_INET;
	case LINUX_AF_AX25:
		return AF_CCITT;
	case LINUX_AF_IPX:
		return AF_IPX;
	case LINUX_AF_APPLETALK:
		return AF_APPLETALK;
	case LINUX_AF_X25:
		return AF_CCITT;
	case LINUX_AF_INET6:
		return AF_INET6;
	case LINUX_AF_DECnet:
		return AF_DECnet;
	case LINUX_AF_NETLINK:
		return AF_ROUTE;
	/* NETROM, BRIDGE, ATMPVC, ROSE, NETBEUI, SECURITY, */
	/* pseudo_AF_KEY, PACKET, ASH, ECONET, ATMSVC, SNA */
	default:
		return -1;
	}
}

int
linux_sys_socket(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_socket_args /* {
		syscallarg(int)	domain;
		syscallarg(int)	type;
		syscallarg(int) protocol;
	} */ *uap = v;
	struct sys_socket_args bsa;

	SCARG(&bsa, protocol) = SCARG(uap, protocol);
	SCARG(&bsa, type) = SCARG(uap, type);
	SCARG(&bsa, domain) = linux_to_bsd_domain(SCARG(uap, domain));
	if (SCARG(&bsa, domain) == -1)
		return EINVAL;
	return sys_socket(p, &bsa, retval);
}

int
linux_sys_socketpair(p, v, retval)
	struct proc *p;
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

	return sys_socketpair(p, &bsa, retval);
}

int
linux_sys_sendto(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_sendto_args /* {
		syscallarg(int) s;
		syscallarg(void *) msg;
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(sockaddr *) to;
		syscallarg(int) tolen;
	} */ *uap = v;
	struct sys_sendto_args bsa;

	SCARG(&bsa, s) = SCARG(uap, s);
	SCARG(&bsa, buf) = SCARG(uap, msg);
	SCARG(&bsa, len) = SCARG(uap, len);
	SCARG(&bsa, flags) = SCARG(uap, flags);
	SCARG(&bsa, to) = (void *) SCARG(uap, to);
	SCARG(&bsa, tolen) = SCARG(uap, tolen);

	return sys_sendto(p, &bsa, retval);
}

int
linux_sys_recvfrom(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	struct linux_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(int) len;
		syscallarg(int) flags;
		syscallarg(struct sockaddr *) from;
		syscallarg(int *) fromlen;
	} */ *uap = v;
	struct compat_43_sys_recvfrom_args bra;

	SCARG(&bra, s) = SCARG(uap, s);
	SCARG(&bra, buf) = SCARG(uap, buf);
	SCARG(&bra, len) = SCARG(uap, len);
	SCARG(&bra, flags) = SCARG(uap, flags);
	SCARG(&bra, from) = (caddr_t) SCARG(uap, from);
	SCARG(&bra, fromlenaddr) = SCARG(uap, fromlen);

	return compat_43_sys_recvfrom(p, &bra, retval);
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
		return SO_REUSEADDR;
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
linux_sys_setsockopt(p, v, retval)
	struct proc *p;
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

	return sys_setsockopt(p, &bsa, retval);
}

/*
 * getsockopt(2) is very much the same as setsockopt(2) (see above)
 */
int
linux_sys_getsockopt(p, v, retval)
	struct proc *p;
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

	return sys_getsockopt(p, &bga, retval);
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
	if (fd >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[fd]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
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

	if (lreq.if_name[0] == 'e' &&
	    lreq.if_name[1] == 't' &&
	    lreq.if_name[2] == 'h') {
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
	int (*ioctlf) __P((struct file *, u_long, caddr_t, struct proc *));
	struct ioctl_pt pt;

        fdp = p->p_fd;
	if ((u_int)SCARG(uap, fd) >= fdp->fd_nfiles ||
	    (fp = fdp->fd_ofiles[SCARG(uap, fd)]) == NULL ||
	    (fp->f_iflags & FIF_WANTCLOSE) != 0)
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
		break;
	default:
		error = EINVAL;
	}

out:
	FILE_UNUSE(fp, p);

	if (error ==0 && dosys) {
		SCARG(&ia, fd) = SCARG(uap, fd);
		SCARG(&ia, data) = SCARG(uap, data);
		error = sys_ioctl(p, &ia, retval);
	}

	return error;
}

int
linux_sys_connect(p, v, retval)
	struct proc *p;
	void *v;
	register_t *retval;
{
	int error;

	struct sys_connect_args /* {
		syscallarg(int) s;
		syscallarg(const struct sockaddr *) name;
		syscallarg(unsigned int) namelen;
	} */ *uap = v;
	
	error = sys_connect (p, v, retval);

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

	return error;
}
