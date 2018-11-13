/*	$NetBSD: netbsd32_socket.c,v 1.44.8.1 2018/11/13 16:55:22 snj Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_socket.c,v 1.44.8.1 2018/11/13 16:55:22 snj Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#define msg __msg /* Don't ask me! */
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

/*
 * XXX Assumes that struct sockaddr is compatible.
 */

#define	CMSG32_ALIGN(n)	(((n) + ALIGNBYTES32) & ~ALIGNBYTES32)
#define	CMSG32_DATA(cmsg) \
	((u_char *)(void *)(cmsg) + CMSG32_ALIGN(sizeof(struct cmsghdr)))

#define	CMSG32_NXTHDR(mhdr, cmsg)	\
	(((char *)(cmsg) + CMSG32_ALIGN((cmsg)->cmsg_len) + \
			    CMSG32_ALIGN(sizeof(struct cmsghdr)) > \
	    (((char *)(mhdr)->msg_control) + (mhdr)->msg_controllen)) ? \
	    (struct cmsghdr *)0 : \
	    (struct cmsghdr *)((char *)(cmsg) + \
	        CMSG32_ALIGN((cmsg)->cmsg_len)))
#define	CMSG32_FIRSTHDR(mhdr) \
	((mhdr)->msg_controllen >= sizeof(struct cmsghdr) ? \
	 (struct cmsghdr *)(mhdr)->msg_control : \
	 (struct cmsghdr *)0)

#define CMSG32_SPACE(l)	(CMSG32_ALIGN(sizeof(struct cmsghdr)) + CMSG32_ALIGN(l))
#define CMSG32_LEN(l)	(CMSG32_ALIGN(sizeof(struct cmsghdr)) + (l))

static int
copyout32_msg_control_mbuf(struct lwp *l, struct msghdr *mp, int *len, struct mbuf *m, char **q, bool *truncated)
{
	struct cmsghdr *cmsg, cmsg32;
	int i, j, error;

	*truncated = false;
	cmsg = mtod(m, struct cmsghdr *);
	do {
		if ((char *)cmsg == mtod(m, char *) + m->m_len)
			break;
		if ((char *)cmsg > mtod(m, char *) + m->m_len - sizeof(*cmsg))
			return EINVAL;
		cmsg32 = *cmsg;
		j = cmsg->cmsg_len - CMSG_LEN(0);
		i = cmsg32.cmsg_len = CMSG32_LEN(j);
		if (i > *len) {
			mp->msg_flags |= MSG_CTRUNC;
			if (cmsg->cmsg_level == SOL_SOCKET
			    && cmsg->cmsg_type == SCM_RIGHTS) {
				*truncated = true;
				return 0;
			}
			j -= i - *len;
			i = *len;
		}

		ktrkuser("msgcontrol", cmsg, cmsg->cmsg_len);
		error = copyout(&cmsg32, *q, MIN(i, sizeof(cmsg32)));
		if (error)
			return (error);
		if (i > CMSG32_LEN(0)) {
			error = copyout(CMSG_DATA(cmsg), *q + CMSG32_LEN(0), i - CMSG32_LEN(0));
			if (error)
				return (error);
		}
		j = CMSG32_SPACE(cmsg->cmsg_len - CMSG_LEN(0));
		if (*len >= j) {
			*len -= j;
			*q += j;
		} else {
			*q += i;
			*len = 0;
		}
		cmsg = (void *)((char *)cmsg + CMSG_ALIGN(cmsg->cmsg_len));
	} while (*len > 0);

	return 0;
}

static int
copyout32_msg_control(struct lwp *l, struct msghdr *mp, struct mbuf *control)
{
	int len, error = 0;
	struct mbuf *m;
	char *q;
	bool truncated;

	len = mp->msg_controllen;
	if (len <= 0 || control == 0) {
		mp->msg_controllen = 0;
		free_control_mbuf(l, control, control);
		return 0;
	}

	q = (char *)mp->msg_control;

	for (m = control; len > 0 && m != NULL; m = m->m_next) {
		error = copyout32_msg_control_mbuf(l, mp, &len, m, &q, &truncated);
		if (truncated) {
			m = control;
			break;
		}
		if (error)
			break;
	}

	free_control_mbuf(l, control, m);

	mp->msg_controllen = q - (char *)mp->msg_control;
	return error;
}

int
netbsd32_recvmsg(struct lwp *l, const struct netbsd32_recvmsg_args *uap, register_t *retval)
{
	/* {
		syscallarg(int) s;
		syscallarg(netbsd32_msghdrp_t) msg;
		syscallarg(int) flags;
	} */
	struct netbsd32_msghdr	msg32;
	struct iovec aiov[UIO_SMALLIOV], *iov;
	struct msghdr	msg;
	int		error;
	struct mbuf	*from, *control;
	size_t iovsz;

	error = copyin(SCARG_P32(uap, msg), &msg32, sizeof(msg32));
	if (error)
		return (error);

	iovsz = msg32.msg_iovlen * sizeof(struct iovec);
	if (msg32.msg_iovlen > UIO_SMALLIOV) {
		if (msg32.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = kmem_alloc(iovsz, KM_SLEEP);
	} else 
		iov = aiov;
	error = netbsd32_to_iovecin(NETBSD32PTR64(msg32.msg_iov), iov,
	    msg32.msg_iovlen);
	if (error)
		goto done;

	msg.msg_flags = SCARG(uap, flags) & MSG_USERFLAGS;
	msg.msg_name = NETBSD32PTR64(msg32.msg_name);
	msg.msg_namelen = msg32.msg_namelen;
	msg.msg_control = NETBSD32PTR64(msg32.msg_control);
	msg.msg_controllen = msg32.msg_controllen;
	msg.msg_iov = iov;
	msg.msg_iovlen = msg32.msg_iovlen;

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, &msg32, sizeof(msg32),
	    &from, msg.msg_control != NULL ? &control : NULL, retval);
	if (error != 0)
		goto done;

	if (msg.msg_control != NULL)
		error = copyout32_msg_control(l, &msg, control);

	if (error == 0)
		error = copyout_sockname(msg.msg_name, &msg.msg_namelen, 0,
			from);
	if (from != NULL)
		m_free(from);
	if (error == 0) {
		msg32.msg_namelen = msg.msg_namelen;
		msg32.msg_controllen = msg.msg_controllen;
		msg32.msg_flags = msg.msg_flags;
		ktrkuser("msghdr", &msg32, sizeof msg32);
		error = copyout(&msg32, SCARG_P32(uap, msg), sizeof(msg32));
	}

 done:
	if (iov != aiov)
		kmem_free(iov, iovsz);
	return (error);
}

static int
copyin32_msg_control(struct lwp *l, struct msghdr *mp)
{
	/*
	 * Handle cmsg if there is any.
	 */
	struct cmsghdr *cmsg, cmsg32, *cc;
	struct mbuf *ctl_mbuf;
	ssize_t resid = mp->msg_controllen;
	size_t clen, cidx = 0, cspace;
	u_int8_t *control;
	int error;

	ctl_mbuf = m_get(M_WAIT, MT_CONTROL);
	clen = MLEN;
	control = mtod(ctl_mbuf, void *);
	memset(control, 0, clen);

	cc = CMSG32_FIRSTHDR(mp);
	do {
		error = copyin(cc, &cmsg32, sizeof(cmsg32));
		if (error)
			goto failure;

		/*
		 * Sanity check the control message length.
		 */
		if (cmsg32.cmsg_len > resid ||
		    cmsg32.cmsg_len < sizeof(cmsg32)) {
			error = EINVAL;
			goto failure;
		}

		cspace = CMSG_SPACE(cmsg32.cmsg_len - CMSG32_LEN(0));

		/* Check the buffer is big enough */
		if (__predict_false(cidx + cspace > clen)) {
			u_int8_t *nc;
			size_t nclen;

			nclen = cidx + cspace;
			if (nclen >= PAGE_SIZE) {
				error = EINVAL;
				goto failure;
			}
			nc = realloc(clen <= MLEN ? NULL : control,
				     nclen, M_TEMP, M_WAITOK);
			if (!nc) {
				error = ENOMEM;
				goto failure;
			}
			if (cidx <= MLEN) {
				/* Old buffer was in mbuf... */
				memcpy(nc, control, cidx);
				memset(nc + cidx, 0, nclen - cidx);
			} else {
				memset(nc + nclen, 0, nclen - clen);
			}
			control = nc;
			clen = nclen;
		}

		/* Copy header */
		cmsg = (void *)&control[cidx];
		cmsg->cmsg_len = CMSG_LEN(cmsg32.cmsg_len - CMSG32_LEN(0));
		cmsg->cmsg_level = cmsg32.cmsg_level;
		cmsg->cmsg_type = cmsg32.cmsg_type;

		/* Copyin the data */
		error = copyin(CMSG32_DATA(cc), CMSG_DATA(cmsg),
		    cmsg32.cmsg_len - CMSG32_LEN(0));
		if (error)
			goto failure;

		resid -= CMSG32_ALIGN(cmsg32.cmsg_len);
		cidx += cmsg->cmsg_len;
	} while (resid > 0 && (cc = CMSG32_NXTHDR(mp, &cmsg32)));

	/* If we allocated a buffer, attach to mbuf */
	if (cidx > MLEN) {
		MEXTADD(ctl_mbuf, control, clen, M_MBUF, NULL, NULL);
		ctl_mbuf->m_flags |= M_EXT_RW;
	}
	control = NULL;
	mp->msg_controllen = ctl_mbuf->m_len = CMSG_ALIGN(cidx);

	mp->msg_control = ctl_mbuf;
	mp->msg_flags |= MSG_CONTROLMBUF;

	ktrkuser("msgcontrol", mtod(ctl_mbuf, void *),
	    mp->msg_controllen);

	return 0;

failure:
	if (control != mtod(ctl_mbuf, void *))
		free(control, M_MBUF);
	m_free(ctl_mbuf);
	return error;
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
	struct iovec aiov[UIO_SMALLIOV], *iov = aiov;
	struct netbsd32_iovec *iov32;
	size_t iovsz;
	int error;

	error = copyin(SCARG_P32(uap, msg), &msg32, sizeof(msg32));
	if (error)
		return (error);
	netbsd32_to_msghdr(&msg32, &msg);
	msg.msg_flags = 0;

	if (CMSG32_FIRSTHDR(&msg)) {
		error = copyin32_msg_control(l, &msg);
		if (error)
			return (error);
		/* From here on, msg.msg_control is allocated */
	} else {
		msg.msg_control = NULL;
		msg.msg_controllen = 0;
	}

	iovsz = msg.msg_iovlen * sizeof(struct iovec);
	if ((u_int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen > IOV_MAX) {
			error = EMSGSIZE;
			goto out;
		}
		iov = kmem_alloc(iovsz, KM_SLEEP);
	}

	iov32 = NETBSD32PTR64(msg32.msg_iov);
	error = netbsd32_to_iovecin(iov32, iov, msg.msg_iovlen);
	if (error)
		goto out;
	msg.msg_iov = iov;

	error = do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags),
	    &msg32, sizeof(msg32), retval);
	/* msg.msg_control freed by do_sys_sendmsg() */

	if (iov != aiov)
		kmem_free(iov, iovsz);
	return (error);

out:
	if (iov != aiov)
		kmem_free(iov, iovsz);
	if (msg.msg_control)
		m_free(msg.msg_control);
	return error;
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
	struct msghdr	msg;
	struct iovec	aiov;
	int		error;
	struct mbuf	*from;

	msg.msg_name = NULL;
	msg.msg_iov = &aiov;
	msg.msg_iovlen = 1;
	aiov.iov_base = SCARG_P32(uap, buf);
	aiov.iov_len = SCARG(uap, len);
	msg.msg_control = NULL;
	msg.msg_flags = SCARG(uap, flags) & MSG_USERFLAGS;

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, NULL, ~0U,
	    &from, NULL, retval);
	if (error != 0)
		return error;

	error = copyout_sockname(SCARG_P32(uap, from), SCARG_P32(uap, fromlenaddr),
	    MSG_LENUSRSPACE, from);
	if (from != NULL)
		m_free(from);
	return error;
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
	return do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags),
	    NULL, ~0U, retval);
}
