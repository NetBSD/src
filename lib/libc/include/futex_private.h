/*	$NetBSD: futex_private.h,v 1.2 2020/05/16 16:16:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef __LIBC_FUTEX_PRIVATE
#define __LIBC_FUTEX_PRIVATE

#include <sys/cdefs.h>
#include <sys/syscall.h>
#include <sys/futex.h>
#include <unistd.h>

/* XXX Avoid pulling in namespace.h. */
extern int _syscall(int, ...);

static inline int __unused
__futex(volatile int *uaddr, int op, int val, const struct timespec *timeout,
	volatile int *uaddr2, int val2, int val3)
{
	return _syscall(SYS___futex, uaddr, op, val, timeout, uaddr2,
			val2, val3);
}

static inline int __unused
__futex_set_robust_list(void *head, size_t len)
{
	return _syscall(SYS___futex_set_robust_list, head, len);
}

static inline int __unused
__futex_get_robust_list(lwpid_t lwpid, void **headp, size_t *lenp)
{
	return _syscall(SYS___futex_get_robust_list, lwpid, headp, lenp);
}

#endif /* __LIBC_FUTEX_PRIVATE */
