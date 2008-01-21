/*	$NetBSD: mach_errno.c,v 1.15.4.1 2008/01/21 09:41:39 yamt Exp $ */

/*-
 * Copyright (c) 2002 The NetBSD Foundation, Inc.
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
__KERNEL_RCSID(0, "$NetBSD: mach_errno.c,v 1.15.4.1 2008/01/21 09:41:39 yamt Exp $");

#include <sys/types.h>
#include <sys/systm.h>
#include <sys/null.h>
#include <sys/queue.h>
#include <sys/errno.h>

#include <compat/mach/mach_types.h>
#include <compat/mach/mach_message.h>
#include <compat/mach/mach_errno.h>

int native_to_mach_errno[] = {
	MACH_KERN_SUCCESS,					/* 0 */
	MACH_KERN_PROTECTION_FAILURE, 		/* EPERM */
	MACH_KERN_FAILURE, 			/* ENOENT */
	MACH_KERN_FAILURE, 			/* ESRCH */
	MACH_KERN_FAILURE, 			/* EINTR */
	MACH_KERN_FAILURE, 			/* EIO */	/* 5 */
	MACH_KERN_FAILURE,			/* ENXIO */
	MACH_KERN_FAILURE,			/* E2BIG */
	MACH_KERN_FAILURE,			/* ENOEXEC */
	MACH_KERN_FAILURE,			/* EBADF */
	MACH_KERN_FAILURE,			/* ECHILD */	/* 10 */
	MACH_KERN_FAILURE,			/* EDEADLK */
	MACH_KERN_NO_SPACE,			/* ENOMEM */
	MACH_KERN_FAILURE,			/* EACCES */
	MACH_KERN_INVALID_ADDRESS,		/* EFAULT */
	MACH_KERN_FAILURE,			/* ENOTBLK */	/* 15 */
	MACH_KERN_FAILURE,			/* EBUSY */
	MACH_KERN_FAILURE,			/* EEXIST */
	MACH_KERN_FAILURE,			/* EXDEV */
	MACH_KERN_FAILURE,			/* ENODEV */
	MACH_KERN_FAILURE,			/* ENOTDIR */	/* 20 */
	MACH_KERN_FAILURE,			/* EISDIR */
	MACH_KERN_INVALID_ARGUMENT,		/* EINVAL */
	MACH_KERN_FAILURE,			/* ENFILE */
	MACH_KERN_FAILURE,			/* EMFILE */
	MACH_KERN_FAILURE,			/* ENOTTY */	/* 25 */
	MACH_KERN_FAILURE,			/* ETXTBSY */
	MACH_KERN_FAILURE,			/* EFBIG */
	MACH_KERN_FAILURE,			/* ENOSPC */
	MACH_KERN_FAILURE,			/* ESPIPE */
	MACH_KERN_FAILURE,			/* EROFS */	/* 30 */
	MACH_KERN_FAILURE,			/* EMLINK */
	MACH_KERN_FAILURE,			/* EPIPE */
	MACH_KERN_FAILURE,			/* EDOM */
	MACH_KERN_FAILURE,			/* ERANGE */
	MACH_KERN_FAILURE,			/* EAGAIN */	/* 35 */
	MACH_KERN_FAILURE,			/* EWOULDBLOCK */
	MACH_KERN_FAILURE,			/* EINPROGRESS */
	MACH_KERN_FAILURE,			/* EALREADY */
	MACH_KERN_FAILURE,			/* ENOTSOCK */
	MACH_KERN_FAILURE,			/* EDESTADDRREQ */ /* 40 */
	MACH_KERN_FAILURE,			/* EMSGSIZE */
	MACH_KERN_FAILURE,			/* EPROTOTYPE */
	MACH_KERN_FAILURE,			/* ENOPROTOOPT */
	MACH_KERN_FAILURE,			/* ESOCKTNOSUPPORT */
	MACH_KERN_FAILURE,			/* EOPNOTSUPP */ /* 45 */
	MACH_KERN_FAILURE,			/* EPFNOSUPPORT */
	MACH_KERN_FAILURE,			/* EAFNOSUPPORT */
	MACH_KERN_FAILURE,			/* EADDRINUSE */
	MACH_KERN_FAILURE,			/* EADDRNOTAVAIL */
	MACH_KERN_FAILURE,			/* ENETDOWN */	/* 50 */
	MACH_KERN_FAILURE,			/* ENETUNREACH */
	MACH_KERN_FAILURE,			/* ENETRESET */
	MACH_KERN_FAILURE,			/* ECONNABORTED */
	MACH_KERN_FAILURE,			/* ECONNRESET */
	MACH_KERN_FAILURE,			/* ENOBUFS */	/* 55 */
	MACH_KERN_FAILURE,			/* EISCONN */
	MACH_KERN_FAILURE,			/* ENOTCONN */
	MACH_KERN_FAILURE,			/* ESHUTDOWN */
	MACH_KERN_FAILURE,			/* ETOOMANYREFS */
	MACH_KERN_FAILURE,			/* ETIMEDOUT */	/* 60 */
	MACH_KERN_FAILURE,			/* ECONNREFUSED */
	MACH_KERN_FAILURE,			/* ELOOP */
	MACH_KERN_FAILURE,			/* ENAMETOOLONG */
	MACH_KERN_FAILURE,			/* EHOSTDOWN */
	MACH_KERN_FAILURE,			/* EHOSTUNREACH */ /* 65 */
	MACH_KERN_FAILURE,			/* ENOTEMPTY */
	MACH_KERN_FAILURE,			/* EPROCLIM */
	MACH_KERN_FAILURE,			/* EUSERS */
	MACH_KERN_FAILURE,			/* EDQUOT */
	MACH_KERN_FAILURE,			/* ESTALE */	/* 70 */
	MACH_KERN_FAILURE,			/* EREMOTE */
	MACH_KERN_FAILURE,			/* EBADRPC */
	MACH_KERN_FAILURE,			/* ERPCMISMATCH */
	MACH_KERN_FAILURE,			/* EPROGUNAVAIL */
	MACH_KERN_FAILURE,			/* EPROGMISMATCH */ /* 75 */
	MACH_KERN_FAILURE,			/* EPROCUNAVAIL */
	MACH_KERN_FAILURE,			/* ENOLCK */
	MACH_KERN_FAILURE,			/* ENOSYS */
	MACH_KERN_FAILURE,			/* EFTYPE */
	MACH_KERN_FAILURE,			/* EAUTH */	/* 80 */
	MACH_KERN_FAILURE,			/* ENEEDAUTH */
	MACH_KERN_FAILURE,			/* EIDRM */
	MACH_KERN_FAILURE,			/* ENOMSG */
	MACH_KERN_FAILURE,			/* EOVERFLOW */
	MACH_KERN_FAILURE,			/* EILSEQ */	/* 85 */
};

int
mach_msg_error(struct mach_trap_args *args, int error)
{
	mach_msg_header_t *req = args->smsg;
	mach_error_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = native_to_mach_errno[error];

	mach_set_trailer(rep, *msglen);

#ifdef DEBUG_MACH
	if (error != 0)
		printf("failure in kernel service %d (err %x, native %d)\n",
		    req->msgh_id, (int)rep->rep_retval, error);
#endif
	return 0;
}

int
mach_iokit_error(struct mach_trap_args *args, int error)
{
	mach_msg_header_t *req = args->smsg;
	mach_error_reply_t *rep = args->rmsg;
	size_t *msglen = args->rsize;

	*msglen = sizeof(*rep);
	mach_set_header(rep, req, *msglen);

	rep->rep_retval = error;

#ifdef DEBUG_MACH
	if (error != 0)
		printf("failure in kernel service %d (Mach err %x)\n",
		    req->msgh_id, error);
#endif
	mach_set_trailer(rep, *msglen);

	return 0;
}
