/* $NetBSD: osf1_socket.c,v 1.22.14.1 2018/05/21 04:36:04 pgoyette Exp $ */

/*
 * Copyright (c) 1999 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Copyright (c) 1994, 1995 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: osf1_socket.c,v 1.22.14.1 2018/05/21 04:36:04 pgoyette Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/namei.h>
#include <sys/proc.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/syscallargs.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/exec.h>

#include <compat/osf1/osf1.h>
#include <compat/osf1/osf1_syscallargs.h>
#include <compat/common/compat_util.h>
#include <compat/osf1/osf1_cvt.h>

int
osf1_sys_recvmsg_xopen(struct lwp *l, const struct osf1_sys_recvmsg_xopen_args *uap, register_t *retval)
{

	/* XXX */
	return (EINVAL);
}

int
osf1_sys_sendmsg_xopen(struct lwp *l, const struct osf1_sys_sendmsg_xopen_args *uap, register_t *retval)
{
	struct osf1_msghdr_xopen osf_msghdr;
	struct osf1_iovec_xopen osf_iovec, *osf_iovec_ptr;
	struct msghdr bsd_msghdr;
	struct iovec *bsd_iovec;
	unsigned long leftovers;
	int flags;
	unsigned int i, iov_len;
	int error;

	/*
	 * translate msghdr structure
	 */
	if ((error = copyin(SCARG(uap, msg), &osf_msghdr,
	    sizeof osf_msghdr)) != 0)
		return (error);

	error = osf1_cvt_msghdr_xopen_to_native(&osf_msghdr, &bsd_msghdr);
	if (error != 0)
		return (error);

	/*
	 * translate flags
	 */
	flags = emul_flags_translate(osf1_sendrecv_msg_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	iov_len = bsd_msghdr.msg_iovlen;
	if ((iov_len > IOV_MAX) || (iov_len == 0))
		return EMSGSIZE;
	bsd_iovec = kmem_alloc(iov_len * sizeof(struct iovec), KM_SLEEP);
	bsd_msghdr.msg_iov = bsd_iovec;

	osf_iovec_ptr = osf_msghdr.msg_iov;
	for (i = 0; i < iov_len; i++) {
		error = copyin(&osf_iovec_ptr[i], &osf_iovec, sizeof osf_iovec);
		if (error != 0)
			goto err;
                bsd_iovec[i].iov_base = osf_iovec.iov_base;
                bsd_iovec[i].iov_len = osf_iovec.iov_len;
	}

	error = do_sys_sendmsg(l, SCARG(uap, s), &bsd_msghdr, flags, retval);
err:
	kmem_free(bsd_iovec, iov_len * sizeof(struct iovec));
	return error;
}

int
osf1_sys_sendto(struct lwp *l, const struct osf1_sys_sendto_args *uap, register_t *retval)
{
	struct sys_sendto_args a;
	unsigned long leftovers;

	SCARG(&a, s) = SCARG(uap, s);
	SCARG(&a, buf) = SCARG(uap, buf);
	SCARG(&a, len) = SCARG(uap, len);
	SCARG(&a, to) = SCARG(uap, to);
	SCARG(&a, tolen) = SCARG(uap, tolen);

	/* translate flags */
	SCARG(&a, flags) = emul_flags_translate(osf1_sendrecv_msg_flags_xtab,
	    SCARG(uap, flags), &leftovers);
	if (leftovers != 0)
		return (EINVAL);

	return sys_sendto(l, &a, retval);
}

int
osf1_sys_socket(struct lwp *l, const struct osf1_sys_socket_args *uap, register_t *retval)
{
	struct compat_30_sys_socket_args a;

	/* XXX TRANSLATE */

	if (SCARG(uap, domain) > AF_LINK)
		return (EINVAL);	/* XXX After AF_LINK, divergence. */

	SCARG(&a, domain) = SCARG(uap, domain);
	SCARG(&a, type) = SCARG(uap, type);
	SCARG(&a, protocol) = SCARG(uap, protocol);

	return compat_30_sys_socket(l, &a, retval);
}

int
osf1_sys_socketpair(struct lwp *l, const struct osf1_sys_socketpair_args *uap, register_t *retval)
{
	struct sys_socketpair_args a;

	/* XXX TRANSLATE */

	if (SCARG(uap, domain) > AF_LINK)
		return (EINVAL);	/* XXX After AF_LINK, divergence. */

	SCARG(&a, domain) = SCARG(uap, domain);
	SCARG(&a, type) = SCARG(uap, type);
	SCARG(&a, protocol) = SCARG(uap, protocol);
	SCARG(&a, rsv) = SCARG(uap, rsv);

	return sys_socketpair(l, &a, retval);
}
