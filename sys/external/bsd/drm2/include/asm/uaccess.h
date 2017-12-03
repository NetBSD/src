/*	$NetBSD: uaccess.h,v 1.2.10.3 2017/12/03 11:37:59 jdolecek Exp $	*/

/*-
 * Copyright (c) 2013 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef _ASM_UACCESS_H_
#define _ASM_UACCESS_H_

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/systm.h>

/* XXX This is a cop-out.  */
#define	VERIFY_READ	0
#define	VERIFY_WRITE	1
static inline bool
access_ok(int verify_op __unused, const void *uaddr __unused,
    size_t nbytes __unused)
{
	return true;
}

#define	__copy_from_user	copy_from_user
#define	__copy_to_user		copy_to_user

static inline int
copy_from_user(void *kernel_addr, const void *user_addr, size_t len)
{
	/* XXX errno NetBSD->Linux */
	return -copyin(user_addr, kernel_addr, len);
}

static inline int
copy_to_user(void *user_addr, const void *kernel_addr, size_t len)
{
	/* XXX errno NetBSD->Linux */
	return -copyout(kernel_addr, user_addr, len);
}

#define	get_user(KERNEL_LOC, USER_ADDR)					\
	copy_from_user(&(KERNEL_LOC), (USER_ADDR), sizeof(KERNEL_LOC))

#define	put_user(KERNEL_LOC, USER_ADDR)					\
	copy_to_user((USER_ADDR), &(KERNEL_LOC), sizeof(KERNEL_LOC))

#if 0
/*
 * XXX These `inatomic' versions are a cop out, but they should do for
 * now -- they are used only in fast paths which can't fault but which
 * can fall back to slower paths that arrange things so faulting is OK.
 */

static inline int
__copy_from_user_inatomic(void *kernel_addr __unused,
    const void *user_addr __unused, size_t len __unused)
{
	return -EFAULT;
}

static inline int
__copy_to_user_inatomic(void *user_addr __unused,
    const void *kernel_addr __unused, size_t len __unused)
{
	return -EFAULT;
}
#endif	/* 0 */

static inline int
__copy_from_user_inatomic_nocache(void *kernel_addr __unused,
    const void *user_addr __unused, size_t len __unused)
{
	return -EFAULT;
}

#endif  /* _ASM_UACCESS_H_ */
