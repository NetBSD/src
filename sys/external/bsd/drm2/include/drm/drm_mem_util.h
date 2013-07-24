/*	$NetBSD: drm_mem_util.h,v 1.1.2.1 2013/07/24 01:56:19 riastradh Exp $	*/

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

#ifndef _DRM_MEM_UTIL_H_
#define _DRM_MEM_UTIL_H_

#include <sys/types.h>
#include <sys/kmem.h>
#include <sys/systm.h>

static inline void *
drm_calloc_large(size_t n, size_t size)
{

#if 1
	KASSERT(size != 0);	/* XXX Let's see whether this ever happens.  */
#else
	if (size == 0)
		return NULL;	/* XXX OK?  */
#endif

	if (n > (SIZE_MAX / size))
		return NULL;

	return kmem_zalloc((n * size), KM_SLEEP);
}

static inline void *
drm_malloc_ab(size_t n, size_t size)
{

#if 1
	KASSERT(size != 0);	/* XXX Let's see whether this ever happens.  */
#else
	if (size == 0)
		return NULL;	/* XXX OK?  */
#endif

	return kmem_alloc((n * size), KM_SLEEP);
}

static inline void
drm_free_large(void *ptr, size_t n, size_t size)
{

#if 0				/* XXX */
	if (ptr != NULL)
#endif
	{
		KASSERT(size != 0);
		KASSERT(n <= (SIZE_MAX / size));
		kmem_free(ptr, (n * size));
	}
}

#endif  /* _DRM_MEM_UTIL_H_ */
