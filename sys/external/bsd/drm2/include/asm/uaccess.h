/*	$NetBSD: uaccess.h,v 1.9 2021/12/19 11:24:14 riastradh Exp $	*/

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

#include <linux/compiler.h>

/* XXX This is a cop-out.  */
#define	VERIFY_READ	0
#define	VERIFY_WRITE	1
static inline bool
access_ok(const void *uaddr __unused, size_t nbytes __unused)
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

#define	get_user(KERNEL_LVAL, USER_PTR)					      \
	copy_from_user(&(KERNEL_LVAL), (USER_PTR), sizeof(*(USER_PTR)) +      \
	    0*sizeof(&(KERNEL_LVAL) - (USER_PTR)))

#define	put_user(KERNEL_RVAL, USER_PTR)	({				      \
	const typeof(*(USER_PTR)) __put_user_tmp = (KERNEL_RVAL);	      \
	copy_to_user((USER_PTR), &__put_user_tmp, sizeof(__put_user_tmp));    \
})

#define	__get_user	get_user
#define	__put_user	put_user

#define	user_access_begin()	__nothing
#define	user_access_end()	__nothing

#define	unsafe_put_user(KERNEL_RVAL, USER_PTR, LABEL)	do {		      \
	if (__put_user(KERNEL_RVAL, USER_PTR))				      \
		goto LABEL;						      \
} while (0)

static inline size_t
clear_user(void __user *user_ptr, size_t size)
{
	char __user *p = user_ptr;
	size_t n = size;

	/*
	 * This loop which sets up a fault handler on every iteration
	 * is not going to win any speed records, but it'll do to copy
	 * out an int.
	 */
	while (n --> 0) {
		if (ustore_char(p, 0) != 0)
			return ++n;
	}

	return 0;
}

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
