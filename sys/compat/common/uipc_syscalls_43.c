/*	$NetBSD: uipc_syscalls_43.c,v 1.29 2007/05/29 21:32:27 christos Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1989, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)uipc_syscalls.c	8.4 (Berkeley) 2/21/94
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: uipc_syscalls_43.c,v 1.29 2007/05/29 21:32:27 christos Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/filedesc.h>
#include <sys/kernel.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/fcntl.h>
#include <sys/malloc.h>
#include <sys/syslog.h>
#include <sys/unistd.h>
#include <sys/resourcevar.h>
#include <sys/mbuf.h>		/* for MLEN */
#include <sys/protosw.h>

#include <sys/mount.h>
#include <sys/syscallargs.h>

#include <net/if.h>
#include <compat/sys/socket.h>
#include <compat/sys/sockio.h>

#include <compat/common/compat_util.h>

#include <uvm/uvm_extern.h>

/*
 * Following 4.3 syscalls were not versioned, even through they should
 * have been:
 * connect(2), bind(2), sendto(2)
 */

static int compat_43_sa_put(void *);

int
compat_43_sys_accept(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_accept_args /* {
		syscallarg(int) s;
		syscallarg(void *) name;
		syscallarg(int *) anamelen;
	} */ *uap = v;
	int error;

	if ((error = sys_accept(l, v, retval)) != 0)
		return error;

	if (SCARG(uap, name)
	    && (error = compat_43_sa_put(SCARG(uap, name))))
		return (error);

	return 0;
}

int
compat_43_sys_getpeername(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_getpeername_args /* {
		syscallarg(int) fdes;
		syscallarg(void *) asa;
		syscallarg(int *) alen;
	} */ *uap = v;

	int error;

	if ((error = sys_getpeername(l, v, retval)) != 0)
		return error;

	if ((error = compat_43_sa_put(SCARG(uap, asa))))
		return (error);

	return 0;
}

int
compat_43_sys_getsockname(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_getsockname_args /* {
		syscallarg(int) fdes;
		syscallarg(void *) asa;
		syscallarg(int *) alen;
	} */ *uap = v;
	int error;

	if ((error = sys_getsockname(l, v, retval)) != 0)
		return error;

	if ((error = compat_43_sa_put(SCARG(uap, asa))))
		return (error);

	return 0;
}

int
compat_43_sys_recv(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_recv_args /* {
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
compat_43_sys_recvfrom(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_recvfrom_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
		syscallarg(size_t) len;
		syscallarg(int) flags;
		syscallarg(void *) from;
		syscallarg(int *) fromlenaddr;
	} */ *uap = v;
	int error;

	if ((error = sys_recvfrom(l, v, retval)))
		return (error);

	if (SCARG(uap, from) && (error = compat_43_sa_put(SCARG(uap, from))))
		return (error);

	return (0);
}

/*
 * Old recvmsg. Arrange necessary structures, calls generic code and
 * adjusts results accordingly.
 */
int
compat_43_sys_recvmsg(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_recvmsg_args /* {
		syscallarg(int) s;
		syscallarg(struct omsghdr *) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct omsghdr omsg;
	struct msghdr msg;
	struct iovec aiov[UIO_SMALLIOV], *iov;
	int error;

	error = copyin((void *)SCARG(uap, msg), (void *)&omsg,
	    sizeof (struct omsghdr));
	if (error)
		return (error);
	if ((u_int)omsg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)omsg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = malloc(sizeof(struct iovec) * omsg.msg_iovlen,
		    M_IOV, M_WAITOK);
	} else
		iov = aiov;

	error = copyin((void *)omsg.msg_iov, (void *)iov,
	    (unsigned)(omsg.msg_iovlen * sizeof (struct iovec)));
	if (error)
		goto done;

	msg.msg_name	= omsg.msg_name;
	msg.msg_namelen = omsg.msg_namelen;
	msg.msg_iovlen	= omsg.msg_iovlen;
	msg.msg_iov	= iov;
	msg.msg_flags	= SCARG(uap, flags);

	/*
	 * If caller passes accrights, arrange things for generic code to
	 * DTRT.
	 */
	if (omsg.msg_accrights && omsg.msg_accrightslen) {
		void *sg = stackgap_init(p, 0);
		struct cmsg *ucmsg;

		/* it was this way in 4.4BSD */
		if ((u_int) omsg.msg_accrightslen > MLEN)
			return (EINVAL);

		ucmsg = stackgap_alloc(p, &sg, CMSG_SPACE(omsg.msg_accrightslen));
		if (ucmsg == NULL)
			return (EMSGSIZE);

		msg.msg_control = ucmsg;
		msg.msg_controllen = CMSG_SPACE(omsg.msg_accrightslen);
	} else {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
	}

	error = recvit(l, SCARG(uap, s), &msg,
	    (void *)&SCARG(uap, msg)->msg_namelen, retval);

	/*
	 * If there is any control information and it's SCM_RIGHTS,
	 * pass it back to the program.
	 */
	if (!error && omsg.msg_accrights && msg.msg_controllen > 0) {
		struct cmsghdr *cmsg;

		/* safe - msg.msg_controllen set by kernel */
		cmsg = (struct cmsghdr *) malloc(msg.msg_controllen,
		    M_TEMP, M_WAITOK);

		error = copyin(msg.msg_control, cmsg, msg.msg_controllen);
		if (error) {
			free(cmsg, M_TEMP);
			return (error);
		}

		if (cmsg->cmsg_level != SOL_SOCKET
		    || cmsg->cmsg_type != SCM_RIGHTS
		    || copyout(CMSG_DATA(cmsg), omsg.msg_accrights,
			    cmsg->cmsg_len)) {
			omsg.msg_accrightslen = 0;
		}

		free(cmsg, M_TEMP);

		if (!error) {
			error = copyout(&cmsg->cmsg_len,
			    &SCARG(uap, msg)->msg_accrightslen, sizeof(int));
		}
	}

	if (!error && omsg.msg_name) {
		int namelen;

		if ((error = copyin(&SCARG(uap, msg)->msg_namelen, &namelen, sizeof(int)) == 0)
		    && namelen > 0)
			error = compat_43_sa_put(omsg.msg_name);
	}

done:
	if (iov != aiov)
		free(iov, M_IOV);
	return (error);
}

int
compat_43_sys_send(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_send_args /* {
		syscallarg(int) s;
		syscallarg(void *) buf;
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

/*
 * Old sendmsg. Arrange necessary structures, call generic code and
 * adjust the results accordingly for old code.
 */
int
compat_43_sys_sendmsg(struct lwp *l, void *v, register_t *retval)
{
	struct compat_43_sys_sendmsg_args /* {
		syscallarg(int) s;
		syscallarg(void *) msg;
		syscallarg(int) flags;
	} */ *uap = v;
	struct proc *p = l->l_proc;
	struct omsghdr omsg;
	struct msghdr msg;
	struct iovec aiov[UIO_SMALLIOV], *iov;
	int error;
	void *sg = stackgap_init(p, 0);

	error = copyin(SCARG(uap, msg), (void *)&omsg,
	    sizeof (struct omsghdr));
	if (error)
		return (error);
	if ((u_int)omsg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)omsg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = malloc(sizeof(struct iovec) * omsg.msg_iovlen,
		    M_IOV, M_WAITOK);
	} else
		iov = aiov;
	error = copyin((void *)omsg.msg_iov, (void *)iov,
	    (unsigned)(omsg.msg_iovlen * sizeof (struct iovec)));
	if (error)
		goto done;

	if (omsg.msg_name) {
		struct osockaddr *osa;
		struct sockaddr *sa, *usa;

		if ((u_int) omsg.msg_namelen > UCHAR_MAX)
			return (EINVAL);

		osa = malloc(omsg.msg_namelen, M_TEMP, M_WAITOK);

		if ((error = copyin(omsg.msg_name, osa, omsg.msg_namelen))) {
			free(osa, M_TEMP);
			return (error);
		}

		sa = (struct sockaddr *) osa;
		sa->sa_family = osa->sa_family;
		sa->sa_len = omsg.msg_namelen;

		usa = stackgap_alloc(p, &sg, omsg.msg_namelen);
		if (!usa) {
			free(osa, M_TEMP);
			return (ENOMEM);
		}

		(void) copyout(sa, usa, omsg.msg_namelen);
		free(osa, M_TEMP);

		msg.msg_name = usa;
		msg.msg_namelen = omsg.msg_namelen;
	} else {
		msg.msg_name = NULL;
		msg.msg_namelen = 0;
	}
	msg.msg_iovlen	= omsg.msg_iovlen;
	msg.msg_iov	= iov;
	msg.msg_flags	= 0;

	if (omsg.msg_accrights && omsg.msg_accrightslen != 0) {
		struct cmsghdr *cmsg, *ucmsg;

		/* it was this way in 4.4BSD */
		if ((u_int) omsg.msg_accrightslen > MLEN)
			return (EINVAL);

		cmsg = malloc(CMSG_SPACE(omsg.msg_accrightslen), M_TEMP,
		    M_WAITOK);
		cmsg->cmsg_len		= CMSG_SPACE(omsg.msg_accrightslen);
		cmsg->cmsg_level	= SOL_SOCKET;
		cmsg->cmsg_type 	= SCM_RIGHTS;

		error = copyin(omsg.msg_accrights, CMSG_DATA(cmsg),
		    omsg.msg_accrightslen);
		if (error) {
			free(cmsg, M_TEMP);
			return (error);
		}

		ucmsg = stackgap_alloc(p, &sg, CMSG_SPACE(omsg.msg_accrightslen));
		if (!ucmsg) {
			free(cmsg, M_TEMP);
			return (EMSGSIZE);
		}

		(void) copyout(cmsg, ucmsg, CMSG_SPACE(omsg.msg_accrightslen));
		free(cmsg, M_TEMP);

		msg.msg_control = ucmsg;
		msg.msg_controllen = CMSG_SPACE(omsg.msg_accrightslen);
	} else {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
	}

	error = sendit(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
done:
	if (iov != aiov)
		FREE(iov, M_IOV);
	return (error);
}

static int
compat_43_sa_put(from)
	void *from;
{
	struct osockaddr *osa = (struct osockaddr *) from;
	struct sockaddr sa;
	struct osockaddr *kosa;
	int error, len;

	/*
	 * Only read/write the sockaddr family and length, the rest is
	 * not changed.
	 */
	len = sizeof(sa.sa_len) + sizeof(sa.sa_family);

	error = copyin((void *) osa, (void *) &sa, len);
	if (error)
		return (error);

	/* Note: we convert from sockaddr sa_family to osockaddr one here */
	kosa = (struct osockaddr *) &sa;
	kosa->sa_family = sa.sa_family;
	error = copyout(kosa, osa, len);
	if (error)
		return (error);

	return (0);
}

int
compat_ifioctl(struct socket *so, u_long ocmd, u_long cmd, void *data,
    struct lwp *l)
{
	int error;
	struct ifreq *ifr = data;
	struct ifnet *ifp = ifunit(ifr->ifr_name);
	struct sockaddr *sa;

	if (ifp == NULL)
		return ENXIO;

	switch (ocmd) {
	case OSIOCSIFADDR:
	case OSIOCSIFDSTADDR:
	case OSIOCSIFBRDADDR:
	case OSIOCSIFNETMASK:
		sa = &ifr->ifr_addr;
#if BYTE_ORDER != BIG_ENDIAN
		if (sa->sa_family == 0 && sa->sa_len < 16) {
			sa->sa_family = sa->sa_len;
			sa->sa_len = 16;
		}
#else
		if (sa->sa_len == 0)
			sa->sa_len = 16;
#endif
		break;

	case OOSIOCGIFADDR:
		cmd = SIOCGIFADDR;
		break;

	case OOSIOCGIFDSTADDR:
		cmd = SIOCGIFDSTADDR;
		break;

	case OOSIOCGIFBRDADDR:
		cmd = SIOCGIFBRDADDR;
		break;

	case OOSIOCGIFNETMASK:
		cmd = SIOCGIFNETMASK;
	}

	error = (*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
	    (struct mbuf *)cmd, (struct mbuf *)ifr, (struct mbuf *)ifp, l);

	switch (ocmd) {
	case OOSIOCGIFADDR:
	case OOSIOCGIFDSTADDR:
	case OOSIOCGIFBRDADDR:
	case OOSIOCGIFNETMASK:
		*(u_int16_t *)&ifr->ifr_addr = 
		    ((struct sockaddr *)&ifr->ifr_addr)->sa_family;
	}
	return error;
}
