/* $NetBSD: drm_atomic.h,v 1.2.16.2 2008/06/04 02:05:10 yamt Exp $ */

/*-
 * Copyright (c) 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Andrew Doran, Blair Sadewitz, et. al.
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

/*
 * XXX	This code is essentially a frob of the atomic operations provided by
 * 	the Linux kernel at the time the DRM was ported.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: drm_atomic.h,v 1.2.16.2 2008/06/04 02:05:10 yamt Exp $");

#include <sys/atomic.h>

typedef uint32_t atomic_t;

#define	atomic_set(p, v)		(*((volatile uint32_t *)p) = (v))
#define	atomic_read(p)			(*((volatile uint32_t *)p))
#define	atomic_inc(p)			atomic_inc_uint(p)
#define	atomic_dec(p)			atomic_dec_uint(p)
#define	atomic_add(v, p)		atomic_add_int(p, v)
#define	atomic_sub(v, p)		atomic_add_int(p, -(v))
#define	atomic_set_int(p, bits)		atomic_or_uint(p, bits)
#define	atomic_clear_int(p, bits)	atomic_and_uint(p, ~(bits))

#define	atomic_cmpset_int(p, o, n)					\
			((old == atomic_cas_uint(p, o, n)) ? 1 : 0)

#define	set_bit(b, p)							\
	atomic_set_int(((volatile uint32_t *)(void *)p) + (b >> 5),	\
			(1 << (b & 0x1f)))

#define	clear_bit(b, p)							\
	atomic_clear_int(((volatile uint32_t *)(void *)p) + (b >> 5), 	\
			(1 << (b & 0x1f)))

#define	test_bit(b, p)							\
	(((volatile uint32_t *)(void *)p)[bit >> 5] & (1 << (bit & 0x1f)))

static inline uint32_t 
test_and_set_bit(int b, volatile void *p)
{
	volatile uint32_t *val;
	uint32_t mask, old;

	val = (volatile uint32_t *)p;
	mask = 1 << b;

	do {
		old = *val;
		if ((old & mask) != 0)
			break;
	} while (atomic_cas_uint(val, old, old | mask) != old);

	return old & mask;
}

static inline int
find_first_zero_bit(volatile void *p, int maxbit)
{
	int b;
	volatile int *ptr = (volatile int *)p;

	for (b = 0; b < maxbit; b += 32) {
		if (ptr[b >> 5] != ~0) {
			for (;;) {
				if ((ptr[b >> 5] & (1 << (b & 0x1f))) == 0)
					return b;
				b++;
			}
		}
	}
	return maxbit;
}
