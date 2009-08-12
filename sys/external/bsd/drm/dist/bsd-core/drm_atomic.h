/**
 * \file drm_atomic.h
 * Atomic operations used in the DRM which may or may not be provided by the OS.
 * 
 * \author Eric Anholt <anholt@FreeBSD.org>
 */

/*-
 * Copyright 2004 Eric Anholt
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEMS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/* Many of these implementations are rather fake, but good enough. */

typedef u_int32_t atomic_t;

#ifdef __FreeBSD__
#define atomic_set(p, v)	(*(p) = (v))
#define atomic_read(p)		(*(p))
#define atomic_inc(p)		atomic_add_int(p, 1)
#define atomic_dec(p)		atomic_subtract_int(p, 1)
#define atomic_add(n, p)	atomic_add_int(p, n)
#define atomic_sub(n, p)	atomic_subtract_int(p, n)
#else /* __FreeBSD__ */
#ifndef __NetBSD__
/* FIXME */
#define atomic_set(p, v)	(*(p) = (v))
#define atomic_read(p)		(*(p))
#define atomic_inc(p)		(*(p) += 1)
#define atomic_dec(p)		(*(p) -= 1)
#define atomic_add(n, p)	(*(p) += (n))
#define atomic_sub(n, p)	(*(p) -= (n))
/* FIXME */
#define atomic_add_int(p, v)      *(p) += v
#define atomic_subtract_int(p, v) *(p) -= v
#define atomic_set_int(p, bits)   *(p) |= (bits)
#define atomic_clear_int(p, bits) *(p) &= ~(bits)
#endif /* !__NetBSD__ */
#endif /* !__FreeBSD__ */

#if defined(__NetBSD__)

#include <sys/atomic.h>

#define	atomic_set(p, v)		(*((volatile uint32_t *)p) = (v))
					/* XXXMRG */
#define	atomic_read(p)			(*(/*(volatile uint32_t *)*/p))
#define	atomic_inc(p)			atomic_inc_uint(p)
#define	atomic_dec(p)			atomic_dec_uint(p)
#define	atomic_add(v, p)		atomic_add_int(p, v)
#define	atomic_sub(v, p)		atomic_add_int(p, -(v))
#define atomic_add_acq_int(p, v)	atomic_add(v, p)
#define atomic_subtract_acq_int(p, v)	atomic_sub(v, p)
#define	atomic_set_int(p, bits)		atomic_or_uint(p, bits)
#define	atomic_clear_int(p, bits)	atomic_and_uint(p, ~(bits))

#define	atomic_cmpset_int(p, o, n)					\
			((old == atomic_cas_uint(p, o, n)) ? 1 : 0)

#define	set_bit(b, p)							\
	atomic_set_int(((volatile uint32_t *)(volatile void *)p) + (b >> 5),\
			(1 << (b & 0x1f)))

#define	clear_bit(b, p)							\
	atomic_clear_int(((volatile uint32_t *)(volatile void *)p) + (b >> 5), \
			(1 << (b & 0x1f)))

#define	test_bit(b, p)							\
	(((volatile uint32_t *)(volatile void *)p)[b >> 5] & (1 << (b & 0x1f)))

static __inline uint32_t 
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

#else
#if !defined(__FreeBSD_version) || (__FreeBSD_version < 500000)
#if defined(__i386__)
/* The extra atomic functions from 5.0 haven't been merged to 4.x */
static __inline int
atomic_cmpset_int(volatile u_int *dst, u_int exp, u_int src)
{
	int res = exp;

	__asm __volatile (
	"	lock ;			"
	"	cmpxchgl %1,%2 ;	"
	"       setz	%%al ;		"
	"	movzbl	%%al,%0 ;	"
	"1:				"
	"# atomic_cmpset_int"
	: "+a" (res)			/* 0 (result) */
	: "r" (src),			/* 1 */
	  "m" (*(dst))			/* 2 */
	: "memory");				 

	return (res);
}
#else /* __i386__ */
static __inline int
atomic_cmpset_int(__volatile__ int *dst, int old, int new)
{
	int s = splhigh();
	if (*dst==old) {
		*dst = new;
		splx(s);
		return 1;
	}
	splx(s);
	return 0;
}
#endif /* !__i386__ */
#endif /* !__FreeBSD_version || __FreeBSD_version < 500000 */

static __inline atomic_t
test_and_set_bit(int b, volatile void *p)
{
	int s = splhigh();
	unsigned int m = 1<<b;
	unsigned int r = *(volatile int *)p & m;
	*(volatile int *)p |= m;
	splx(s);
	return r;
}

static __inline void
clear_bit(int b, volatile void *p)
{
	atomic_clear_int(((volatile int *)p) + (b >> 5), 1 << (b & 0x1f));
}

static __inline void
set_bit(int b, volatile void *p)
{
	atomic_set_int(((volatile int *)p) + (b >> 5), 1 << (b & 0x1f));
}

static __inline int
test_bit(int b, volatile void *p)
{
	return ((volatile int *)p)[b >> 5] & (1 << (b & 0x1f));
}

#endif /* !__NetBSD__ */
static __inline int
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
