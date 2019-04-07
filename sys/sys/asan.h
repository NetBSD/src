/*	$NetBSD: asan.h,v 1.10 2019/04/07 09:20:04 maxv Exp $	*/

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

#ifdef _KERNEL_OPT
#include "opt_kasan.h"
#endif

#include <sys/types.h>

/* Stack redzone values. Part of the compiler ABI. */
#define KASAN_STACK_LEFT	0xF1
#define KASAN_STACK_MID		0xF2
#define KASAN_STACK_RIGHT	0xF3
#define KASAN_STACK_PARTIAL	0xF4
#define KASAN_USE_AFTER_SCOPE	0xF8

/* Our redzone values. */
#define KASAN_GENERIC_REDZONE	0xFA
#define KASAN_MALLOC_REDZONE	0xFB
#define KASAN_KMEM_REDZONE	0xFC
#define KASAN_POOL_REDZONE	0xFD
#define KASAN_POOL_FREED	0xFE

#ifdef KASAN
void kasan_shadow_map(void *, size_t);
void kasan_early_init(void *);
void kasan_init(void);
void kasan_softint(struct lwp *);

void kasan_add_redzone(size_t *);
void kasan_mark(const void *, size_t, size_t, uint8_t);
#else
#define kasan_add_redzone(s)	__nothing
#define kasan_mark(p, s, l, c)	__nothing
#endif

#endif /* !_SYS_ASAN_H_ */
