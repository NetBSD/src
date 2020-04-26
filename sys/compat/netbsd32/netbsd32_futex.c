/*	$NetBSD: netbsd32_futex.c,v 1.1 2020/04/26 18:53:33 thorpej Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell and Jason R. Thorpe.
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

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: netbsd32_futex.c,v 1.1 2020/04/26 18:53:33 thorpej Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/lwp.h>
#include <sys/syscallargs.h>
#include <sys/futex.h>

#include <compat/netbsd32/netbsd32.h>
#include <compat/netbsd32/netbsd32_syscallargs.h>
#include <compat/netbsd32/netbsd32_conv.h>

/* Sycalls conversion */

int
netbsd32___futex(struct lwp *l, const struct netbsd32___futex_args *uap,
    register_t *retval)
{
	/* {
		syscallarg(netbsd32_intp) uaddr;
		syscallarg(int) op;
		syscallarg(int) val;
		syscallarg(netbsd32_timespecp_t) timeout;
		syscallarg(netbsd32_intp) uaddr2;
		syscallarg(int) val2;
		syscallarg(int) val3;
	} */
	struct netbsd32_timespec ts32;
	struct timespec ts, *tsp;
	int error;

	/*
	 * Copy in the timeout argument, if specified.
	 */
	if (SCARG_P32(uap, timeout)) {
		error = copyin(SCARG_P32(uap, timeout), &ts32, sizeof(ts32));
		if (error)
			return error;
		netbsd32_to_timespec(&ts32, &ts);
		tsp = &ts;
	} else {
		tsp = NULL;
	}

	return do_futex(SCARG_P32(uap, uaddr), SCARG(uap, op),
	    SCARG(uap, val), tsp, SCARG_P32(uap, uaddr2), SCARG(uap, val2),
	    SCARG(uap, val3), retval);
}

int
netbsd32___futex_set_robust_list(struct lwp *l,
    const struct netbsd32___futex_set_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(netbsd32_voidp) head;
		syscallarg(netbsd32_size_t) len;
	} */
	void *head = SCARG_P32(uap, head);

	if (SCARG(uap, len) != _FUTEX_ROBUST_HEAD_SIZE32)
		return EINVAL;
	if ((uintptr_t)head % sizeof(uint32_t))
		return EINVAL;
	
	l->l_robust_head = (uintptr_t)head;

	return 0;
}

int
netbsd32___futex_get_robust_list(struct lwp *l,
    const struct netbsd32___futex_get_robust_list_args *uap, register_t *retval)
{
	/* {
		syscallarg(lwpid_t) lwpid;
		syscallarg(netbsd32_voidp) headp;
		syscallarg(netbsd32_size_tp) lenp;
	} */
	void *head;
	const netbsd32_size_t len = _FUTEX_ROBUST_HEAD_SIZE32;
	netbsd32_voidp head32;
	int error;

	error = futex_robust_head_lookup(l, SCARG(uap, lwpid), &head);
	if (error)
		return error;

	head32.i32 = (uintptr_t)head;
	if (NETBSD32PTR64(head32) != head)
		return EFAULT;

	/* Copy out the head pointer and the head structure length. */
	/* (N.B.: "headp" is actually a "void **". */
	error = copyout(&head32, SCARG_P32(uap, headp), sizeof(head32));
	if (__predict_true(error == 0)) {
		error = copyout(&len, SCARG_P32(uap, lenp), sizeof(len));
	}

	return error;
}
