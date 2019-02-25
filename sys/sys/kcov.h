/*      $NetBSD: kcov.h,v 1.3 2019/02/25 13:19:14 kamil Exp $        */

/*
 * Copyright (c) 2019 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Siddharth Muralee.
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

#ifndef _SYS_KCOV_H_
#define _SYS_KCOV_H_

#include <sys/param.h>
#include <sys/types.h>
#include <sys/atomic.h>

#define KCOV_IOC_SETBUFSIZE	_IOW('K', 1, uint64_t)
#define KCOV_IOC_ENABLE		_IO('K', 2)
#define KCOV_IOC_DISABLE	_IO('K', 3)

typedef volatile uint64_t kcov_int_t;
#define KCOV_ENTRY_SIZE sizeof(kcov_int_t)

/*
 * Always prefer 64-bit atomic operations whenever accessible.
 *
 * As a fallback keep regular volatile move operation that it's not known
 * to have negative effect in KCOV even if interrupted in the middle of
 * operation.
 */
#ifdef __HAVE_ATOMIC64_OPS
#define KCOV_STORE(x,v)	__atomic_store_n(&(x), (v), __ATOMIC_RELAXED)
#define KCOV_LOAD(x)	__atomic_load_n(&(x), __ATOMIC_RELAXED)
#else
#define KCOV_STORE(x,v)	(x) = (v)
#define KCOV_LOAD(x)	(x)
#endif

#endif /* !_SYS_KCOV_H_ */
