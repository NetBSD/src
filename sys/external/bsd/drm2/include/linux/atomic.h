/*	$NetBSD: atomic.h,v 1.1.2.7 2013/07/24 02:27:52 riastradh Exp $	*/

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

#ifndef _LINUX_ATOMIC_H_
#define _LINUX_ATOMIC_H_

#include <sys/atomic.h>

struct atomic {
	union {
		int au_int;
		unsigned int au_uint;
	} a_u;
};

typedef struct atomic atomic_t;

static inline int
atomic_read(atomic_t *atomic)
{
	return *(volatile int *)&atomic->a_u.au_int;
}

static inline void
atomic_set(atomic_t *atomic, int value)
{
	atomic->a_u.au_int = value;
}

static inline void
atomic_add(int addend, atomic_t *atomic)
{
	atomic_add_int(&atomic->a_u.au_uint, addend);
}

static inline int
atomic_add_return(int addend, atomic_t *atomic)
{
	return (int)atomic_add_int_nv(&atomic->a_u.au_uint, addend);
}

static inline void
atomic_inc(atomic_t *atomic)
{
	atomic_inc_uint(&atomic->a_u.au_uint);
}

static inline void
atomic_dec(atomic_t *atomic)
{
	atomic_dec_uint(&atomic->a_u.au_uint);
}

static inline int
atomic_dec_and_test(atomic_t *atomic)
{
	return (-1 == (int)atomic_dec_uint_nv(&atomic->a_u.au_uint));
}

static inline void
set_bit(unsigned long bit, volatile unsigned long *ptr)
{
	atomic_or_ulong(ptr, (1 << bit));
}

static inline void
clear_bit(unsigned long bit, volatile unsigned long *ptr)
{
	atomic_and_ulong(ptr, ~(1 << bit));
}

static inline void
change_bit(unsigned long bit, volatile unsigned long *ptr)
{
	unsigned long v;

	do v = *ptr; while (atomic_cas_ulong(ptr, v, v ^ (1 << bit)) != v);
}

static inline unsigned long
test_and_set_bit(unsigned long bit, volatile unsigned long *ptr)
{
	unsigned long v;

	do v = *ptr; while (atomic_cas_ulong(ptr, v, v | (1 << bit)) != v);

	return (v & (1 << bit));
}

static inline unsigned long
test_and_clear_bit(unsigned long bit, volatile unsigned long *ptr)
{
	unsigned long v;

	do v = *ptr; while (atomic_cas_ulong(ptr, v, v &~ (1 << bit)) != v);

	return (v & (1 << bit));
}

static inline unsigned long
test_and_change_bit(unsigned long bit, volatile unsigned long *ptr)
{
	unsigned long v;

	do v = *ptr; while (atomic_cas_ulong(ptr, v, v ^ (1 << bit)) != v);

	return (v & (1 << bit));
}

#endif  /* _LINUX_ATOMIC_H_ */
