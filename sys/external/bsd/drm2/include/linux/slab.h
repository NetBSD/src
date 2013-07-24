/*	$NetBSD: slab.h,v 1.1.2.7 2013/07/24 04:00:19 riastradh Exp $	*/

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

#ifndef _LINUX_SLAB_H_
#define _LINUX_SLAB_H_

#include <sys/malloc.h>

#include <machine/limits.h>

#include <uvm/uvm_extern.h>	/* For PAGE_SIZE.  */

#include <linux/gfp.h>

/* XXX Should use kmem, but Linux kfree doesn't take the size.  */

static inline int
linux_gfp_to_malloc(gfp_t gfp)
{
	int flags = 0;

	/* XXX Handle other cases as they arise.  */
	KASSERT((gfp == GFP_ATOMIC) || (gfp == GFP_KERNEL));

	if (ISSET(gfp, __GFP_WAIT))
		flags |= M_WAITOK;
	else
		flags |= M_NOWAIT;

	if (ISSET(gfp, __GFP_ZERO))
		flags |= M_ZERO;

	return flags;
}

static inline void *
kmalloc(size_t size, gfp_t gfp)
{
	return malloc(size, M_TEMP, linux_gfp_to_malloc(gfp));
}

static inline void *
kzalloc(size_t size, gfp_t gfp)
{
	return malloc(size, M_TEMP, (linux_gfp_to_malloc(gfp) | M_ZERO));
}

static inline void *
kcalloc(size_t n, size_t size, gfp_t gfp)
{
	KASSERT(size <= (SIZE_MAX / n));
	return malloc((n * size), M_TEMP, (linux_gfp_to_malloc(gfp) | M_ZERO));
}

static inline void *
krealloc(void *ptr, size_t size, gfp_t gfp)
{
	return realloc(ptr, size, M_TEMP, linux_gfp_to_malloc(gfp));
}

static inline void
kfree(void *ptr)
{
	free(ptr, M_TEMP);
}

#endif  /* _LINUX_SLAB_H_ */
