/*	$NetBSD: netbsd32_socket.c,v 1.34.4.1 2009/07/23 23:31:43 jym Exp $	*/

/*
 * Copyright (c) 1998, 2001 Matthew R. Green
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_socket.c,v 1.34.4.1 2009/07/23 23:31:43 jym Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#define msg __msg /* Don't ask me! */
#include <sys/malloc.h>
#include <sys/mount.h>
#include <sys/socket.h>
#include <sys/sockio.h>
#include <sys/socketvar.h>
#include <sys/mbuf.h>
#include <sys/ktrace.h>
#include <sys/file.h>
#include <sys/filedesc.h>
#include <sys/syscallargs.h>
#include <sys/proc.h>
#include <sys/dirent.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

/* note that the netbsd32_msghdr's iov really points to a struct iovec, not a netbsd32_iovec. */
static int recvit32(struct lwp *, int, struct netbsd32_msghdr *, struct iovec *, void *,
			 register_t *);

int
netbsd32_recvmsg(struct lwp *l, const struct netbsd32_recvmsg_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */
	struct netbsd32_msghdr msg;
	struct iovec aiov[UIO_SMALLIOV], *iov;
	struct netbsd32_iovec *iov32;
	int error;

	error = copyin(SCARG_P32(uap, msg), &msg, sizeof(msg));
		/* netbsd32_msghdr needs the iov pre-allocated */
	if (error)
		return (error);
	if ((u_int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = (struct iovec *)malloc(
		       sizeof(struct iovec) * (u_int)msg.msg_iovlen, M_IOV,
		       M_WAITOK);
	} else 
		iov = aiov;
	msg.msg_flags = SCARG(uap, flags);
	iov32 = NETBSD32PTR64(msg.msg_iov);
	error = netbsd32_to_iovecin(iov32, iov, msg.msg_iovlen);
	if (error)
		goto done;
	if ((error = recvit32(l, SCARG(uap, s), &msg, iov, (void *)0,
	    retval)) == 0) {
		error = copyout(&msg, SCARG_P32(uap, msg), sizeof(msg));
	}
done:
	if (iov != aiov)
		free(iov, M_IOV);
	return (error);
}

int
recvit32(struct lwp *l, int s, struct netbsd32_msghdr *mp, struct iovec *iov, void *namelenp, register_t *retsize)
{
	struct uio auio;
	int i, len, error, iovlen;
	struct mbuf *from = 0, *control = 0;
	struct socket *so;
	struct proc *p;
	struct iovec *ktriov = NULL;
	p = l->l_proc;

	/* fd_getsock() will use the descriptor for us */
	if ((error = fd_getsock(s, &so)) != 0)
		return (error);
	auio.uio_iov = iov;
	auio.uio_iovcnt = mp->msg_iovlen;
	auio.uio_rw = UIO_READ;
	auio.uio_vmspace = l->l_proc->p_vmspace;
	auio.uio_offset = 0;			/* XXX */
	auio.uio_resid = 0;
	for (i = 0; i < mp->msg_iovlen; i++, iov++) {
#if 0
		/* cannot happen iov_len is unsigned */
		if (iov->iov_len < 0) {
			error = EINVAL;
			goto out1;
		}
#endif
		/*
		 * Reads return ssize_t because -1 is returned on error.
		 * Therefore we must restrict the length to SSIZE_MAX to
		 * avoid garbage return values.
		 */
		auio.uio_resid += iov->iov_len;
		if (iov->iov_len > SSIZE_MAX || auio.uio_resid > SSIZE_MAX) {
			error = EINVAL;
			goto out1;
		}
	}

	if (ktrpoint(KTR_GENIO)) {
		iovlen = auio.uio_iovcnt * sizeof(struct iovec);
		ktriov = (struct iovec *)malloc(iovlen, M_TEMP, M_WAITOK);
		memcpy((void *)ktriov, (void *)auio.uio_iov, iovlen);
	}

	len = auio.uio_resid;
	error = (*so->so_receive)(so, &from, &auio, NULL,
			  NETBSD32PTR64(mp->msg_control) ? &control : NULL,
			  &mp->msg_flags);
	if (error) {
		if (auio.uio_resid != len && (error == ERESTART ||
		    error == EINTR || error == EWOULDBLOCK))
			error = 0;
	}

	if (ktriov != NULL) {
		ktrgeniov(s, UIO_READ, ktriov, len - auio.uio_resid, error);
		free(ktriov, M_TEMP);
	}

	if (error)
		goto out;
	*retsize = len - auio.uio_resid;
	if (NETBSD32PTR64(mp->msg_name)) {
		len = mp->msg_namelen;
		if (len <= 0 || from == 0)
			len = 0;
		else {
			if (len > from->m_len)
				len = from->m_len;
			/* else if len < from->m_len ??? */
			error = copyout(mtod(from, void *),
			    (void *)NETBSD32PTR64(mp->msg_name),
			    (unsigned)len);
			if (error)
				goto out;
		}
		mp->msg_namelen = len;
		if (namelenp &&
		    (error = copyout((void *)&len, namelenp, sizeof(int))))
			goto out;
	}
	if (NETBSD32PTR64(mp->msg_control)) {
		len = mp->msg_controllen;
		if (len <= 0 || control == 0)
			len = 0;
		else {
			struct mbuf *m = control;
			void *cp = (void *)NETBSD32PTR64(mp->msg_control);

			do {
				i = m->m_len;
				if (len < i) {
					mp->msg_flags |= MSG_CTRUNC;
					i = len;
				}
				error = copyout(mtod(m, void *), cp,
				    (unsigned)i);
				if (m->m_next)
					i = ALIGN(i);
				cp = (char *)cp + i;
				len -= i;
				if (error != 0 || len <= 0)
					break;
			} while ((m = m->m_next) != NULL);
			len = (char *)cp - (char *)NETBSD32PTR64(mp->msg_control);
		}
		mp->msg_controllen = len;
	}
 out:
	if (from)
		m_freem(from);
	if (control)
		m_freem(control);
 out1:
	fd_putfile(s);
	return (error);
}

int
netbsd32_sendmsg(struct lwp *l, const struct netbsd32_sendmsg_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(const netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */
	struct msghdr msg;
	struct netbsd32_msghdr msg32;
	struct iovec aiov[UIO_SMALLIOV], *iov;
	struct netbsd32_iovec *iov32;
	int error;

	error = copyin(SCARG_P32(uap, msg), &msg32, sizeof(msg32));
	if (error)
		return (error);
	netbsd32_to_msghdr(&msg32, &msg);

	if ((u_int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = (struct iovec *)malloc(
		       sizeof(struct iovec) * (u_int)msg.msg_iovlen, M_IOV,
		       M_WAITOK);
	} else
		iov = aiov;

	iov32 = NETBSD32PTR64(msg32.msg_iov);
	error = netbsd32_to_iovecin(iov32, iov, msg.msg_iovlen);
	if (error)
		goto done;
	msg.msg_iov = iov;
	msg.msg_flags = 0;

	/* Luckily we can use this directly */
	/* XXX: dsl (June'07) The cmsg alignment rules differ ! */
	error = do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
done:
	if (iov != aiov)
		free(iov, M_IOV);
	return (error);
}

int
netbsd32_recvfrom(struct lwp *l, const struct netbsd32_recvfrom_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(netbsd32_sockaddrp_t) from;
		syscallarg(netbsd32_intp) fromlenaddr;
	} */
	struct netbsd32_msghdr msg;
	struct iovec aiov;
	int error;

	if (SCARG_P32(uap, fromlenaddr)) {
		error = copyin(SCARG_P32(uap, fromlenaddr),
		    &msg.msg_namelen, sizeof(msg.msg_namelen));
		if (error)
			return (error);
	} else
		msg.msg_namelen = 0;
	msg.msg_name = SCARG(uap, from);
	NETBSD32PTR32(msg.msg_iov, 0); /* ignored in recvit32(), uses iov */
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG_P32(uap, buf);
	aiov.iov_len = (u_long)SCARG(uap, len);
	NETBSD32PTR32(msg.msg_control, 0);
	msg.msg_flags = SCARG(uap, flags);
	return (recvit32(l, SCARG(uap, s), &msg, &aiov,
	    SCARG_P32(uap, fromlenaddr), retval));
}

int
netbsd32_sendto(struct lwp *l, const struct netbsd32_sendto_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(const netbsd32_voidp) buf;
		syscallarg(netbsd32_size_t) len;
		syscallarg(int) flags;
		syscallarg(const netbsd32_sockaddrp_t) to;
		syscallarg(int) tolen;
	} */
	struct msghdr msg;
	struct iovec aiov;

	msg.msg_name = SCARG_P32(uap, to); /* XXX kills const */
	msg.msg_namelen = SCARG(uap, tolen);
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	msg.msg_control = 0;
	aiov.iov_base = SCARG_P32(uap, buf);	/* XXX kills const */
	aiov.iov_len = SCARG(uap, len);
	msg.msg_flags = 0;
	return do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
}
