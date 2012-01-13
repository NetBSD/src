/*	$NetBSD: netbsd32_socket.c,v 1.38 2012/01/13 21:02:03 joerg Exp $	*/

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
__KERNEL_RCSID(0, "$NetBSD: netbsd32_socket.c,v 1.38 2012/01/13 21:02:03 joerg Exp $");

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
 * XXX Assumes that sockaddr is compatible.
 * XXX Assumes that copyout_msg_control uses identical alignment.
 */
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

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, &from,
	    msg.msg_control != NULL ? &control : NULL, retval);
	if (error != 0)
		goto done;

	if (msg.msg_control != NULL)
		error = copyout_msg_control(l, &msg, control);

	if (error == 0)
		error = copyout_sockname(msg.msg_name, &msg.msg_namelen, 0,
			from);
	if (from != NULL)
		m_free(from);
	if (error == 0) {
		ktrkuser("msghdr", &msg, sizeof msg);
		msg32.msg_namelen = msg.msg_namelen;
		msg32.msg_controllen = msg.msg_controllen;
		msg32.msg_flags = msg.msg_flags;
		error = copyout(&msg32, SCARG_P32(uap, msg), sizeof(msg32));
	}

 done:
	if (iov != aiov)
		kmem_free(iov, iovsz);
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
	size_t iovsz;
	int error;

	error = copyin(SCARG_P32(uap, msg), &msg32, sizeof(msg32));
	if (error)
		return (error);
	netbsd32_to_msghdr(&msg32, &msg);

	iovsz = msg.msg_iovlen * sizeof(struct iovec);
	if ((u_int)msg.msg_iovlen > UIO_SMALLIOV) {
		if ((u_int)msg.msg_iovlen > IOV_MAX)
			return (EMSGSIZE);
		iov = kmem_alloc(iovsz, KM_SLEEP);
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
		kmem_free(iov, iovsz);
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

	error = do_sys_recvmsg(l, SCARG(uap, s), &msg, &from, NULL, retval);
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
	return do_sys_sendmsg(l, SCARG(uap, s), &msg, SCARG(uap, flags), retval);
}
