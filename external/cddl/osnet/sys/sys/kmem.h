
/*	$NetBSD: kmem.h,v 1.11 2019/05/23 08:32:31 hannken Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran.
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

#ifndef _OPENSOLARIS_SYS_KMEM_H_
#define	_OPENSOLARIS_SYS_KMEM_H_

#include_next <sys/kmem.h>
#include <sys/pool.h>
#include <sys/vmem.h>

#define	KM_PUSHPAGE	KM_SLEEP
#define	KM_NORMALPRI	0
#define	KM_NODEBUG	0

#define	KMC_NOTOUCH	0x00010000
#define	KMC_NODEBUG	0x00020000

struct kmem_cache;

typedef struct kmem_cache kmem_cache_t;

#define	POINTER_IS_VALID(p)	(!((uintptr_t)(p) & 0x3))
#define	POINTER_INVALIDATE(pp)	(*(pp) = (void *)((uintptr_t)(*(pp)) | 0x1))

kmem_cache_t *kmem_cache_create(char *, size_t, size_t,
    int (*)(void *, void *, int), void (*)(void *, void *),
    void (*)(void *), void *, vmem_t *, int);
void kmem_cache_destroy(kmem_cache_t *);
void *kmem_cache_alloc(kmem_cache_t *, int);
void kmem_cache_free(kmem_cache_t *, void *);
void kmem_cache_reap_now(kmem_cache_t *);
#define	kmem_cache_set_move(cache, movefunc)	do { } while (0)

#define	heap_arena			kmem_arena

#define kmem_alloc solaris_kmem_alloc
#define kmem_zalloc solaris_kmem_zalloc
#define kmem_free solaris_kmem_free
#define kmem_size() ((uint64_t)physmem * PAGE_SIZE)

void *solaris_kmem_alloc(size_t, int);
void *solaris_kmem_zalloc(size_t, int);
void solaris_kmem_free(void *, size_t);

#endif	/* _OPENSOLARIS_SYS_KMEM_H_ */
