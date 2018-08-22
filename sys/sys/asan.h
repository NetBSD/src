/*	$NetBSD: asan.h,v 1.5 2018/08/22 12:14:29 kre Exp $	*/

/*
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Maxime Villard.
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

#ifndef _SYS_ASAN_H_
#define _SYS_ASAN_H_

#include <sys/types.h>

#ifdef KASAN
void kasan_add_redzone(size_t *);
void kasan_alloc(const void *, size_t, size_t);
void kasan_free(const void *, size_t);
#else
#if 0
/* there are type issues - kmem_alloc() takes a u_long size not size_t */
static void __inline __unused
kasan_add_redzone(size_t *size __unused)
{
	/* nothing */
}

static void __inline __unused
kasan_alloc(const void *addr __unused, size_t size __unused,
    size_t sz_with_redz __unused)
{
	/* nothing */
}

static void __inline __unused
kasan_free(const void *addr __unused, size_t sz_with_redz __unused)
{
	/* nothing */
}
#else
#define	kasan_add_redzone(SP)	__nothing
#define	kasan_alloc(P, S1, S2)	__nothing
#define	kasan_free(P, S)	__nothing
#endif
#endif

#endif /* !_SYS_ASAN_H_ */
