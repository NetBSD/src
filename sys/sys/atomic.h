/*	$NetBSD: atomic.h,v 1.13.22.1 2019/12/08 14:26:38 martin Exp $	*/

/*-
 * Copyright (c) 2007, 2008 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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

#ifndef _SYS_ATOMIC_H_
#define	_SYS_ATOMIC_H_

#include <sys/types.h>
#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <stdint.h>
#endif

__BEGIN_DECLS
/*
 * Atomic ADD
 */
void		atomic_add_32(volatile uint32_t *, int32_t);
void		atomic_add_int(volatile unsigned int *, int);
void		atomic_add_long(volatile unsigned long *, long);
void		atomic_add_ptr(volatile void *, ssize_t);
void		atomic_add_64(volatile uint64_t *, int64_t);

uint32_t	atomic_add_32_nv(volatile uint32_t *, int32_t);
unsigned int	atomic_add_int_nv(volatile unsigned int *, int);
unsigned long	atomic_add_long_nv(volatile unsigned long *, long);
void *		atomic_add_ptr_nv(volatile void *, ssize_t);
uint64_t	atomic_add_64_nv(volatile uint64_t *, int64_t);

/*
 * Atomic AND
 */
void		atomic_and_32(volatile uint32_t *, uint32_t);
void		atomic_and_uint(volatile unsigned int *, unsigned int);
void		atomic_and_ulong(volatile unsigned long *, unsigned long);
void		atomic_and_64(volatile uint64_t *, uint64_t);

uint32_t	atomic_and_32_nv(volatile uint32_t *, uint32_t);
unsigned int	atomic_and_uint_nv(volatile unsigned int *, unsigned int);
unsigned long	atomic_and_ulong_nv(volatile unsigned long *, unsigned long);
uint64_t	atomic_and_64_nv(volatile uint64_t *, uint64_t);

/*
 * Atomic OR
 */
void		atomic_or_32(volatile uint32_t *, uint32_t);
void		atomic_or_uint(volatile unsigned int *, unsigned int);
void		atomic_or_ulong(volatile unsigned long *, unsigned long);
void		atomic_or_64(volatile uint64_t *, uint64_t);

uint32_t	atomic_or_32_nv(volatile uint32_t *, uint32_t);
unsigned int	atomic_or_uint_nv(volatile unsigned int *, unsigned int);
unsigned long	atomic_or_ulong_nv(volatile unsigned long *, unsigned long);
uint64_t	atomic_or_64_nv(volatile uint64_t *, uint64_t);

/*
 * Atomic COMPARE-AND-SWAP
 */
uint32_t	atomic_cas_32(volatile uint32_t *, uint32_t, uint32_t);
unsigned int	atomic_cas_uint(volatile unsigned int *, unsigned int,
				unsigned int);
unsigned long	atomic_cas_ulong(volatile unsigned long *, unsigned long,
				 unsigned long);
void *		atomic_cas_ptr(volatile void *, void *, void *);
uint64_t	atomic_cas_64(volatile uint64_t *, uint64_t, uint64_t);

/*
 * This operations will be provided for userland, but may not be
 * implemented efficiently.
 */
uint16_t	atomic_cas_16(volatile uint16_t *, uint16_t, uint16_t);
uint8_t 	atomic_cas_8(volatile uint8_t *, uint8_t, uint8_t);

/*
 * Non-interlocked atomic COMPARE-AND-SWAP.
 */
uint32_t	atomic_cas_32_ni(volatile uint32_t *, uint32_t, uint32_t);
unsigned int	atomic_cas_uint_ni(volatile unsigned int *, unsigned int,
				   unsigned int);
unsigned long	atomic_cas_ulong_ni(volatile unsigned long *, unsigned long,
				    unsigned long);
void *		atomic_cas_ptr_ni(volatile void *, void *, void *);
uint64_t	atomic_cas_64_ni(volatile uint64_t *, uint64_t, uint64_t);

/*
 * Atomic SWAP
 */
uint32_t	atomic_swap_32(volatile uint32_t *, uint32_t);
unsigned int	atomic_swap_uint(volatile unsigned int *, unsigned int);
unsigned long	atomic_swap_ulong(volatile unsigned long *, unsigned long);
void *		atomic_swap_ptr(volatile void *, void *);
uint64_t	atomic_swap_64(volatile uint64_t *, uint64_t);

/*
 * Atomic DECREMENT
 */
void		atomic_dec_32(volatile uint32_t *);
void		atomic_dec_uint(volatile unsigned int *);
void		atomic_dec_ulong(volatile unsigned long *);
void		atomic_dec_ptr(volatile void *);
void		atomic_dec_64(volatile uint64_t *);

uint32_t	atomic_dec_32_nv(volatile uint32_t *);
unsigned int	atomic_dec_uint_nv(volatile unsigned int *);
unsigned long	atomic_dec_ulong_nv(volatile unsigned long *);
void *		atomic_dec_ptr_nv(volatile void *);
uint64_t	atomic_dec_64_nv(volatile uint64_t *);

/*
 * Atomic INCREMENT
 */
void		atomic_inc_32(volatile uint32_t *);
void		atomic_inc_uint(volatile unsigned int *);
void		atomic_inc_ulong(volatile unsigned long *);
void		atomic_inc_ptr(volatile void *);
void		atomic_inc_64(volatile uint64_t *);

uint32_t	atomic_inc_32_nv(volatile uint32_t *);
unsigned int	atomic_inc_uint_nv(volatile unsigned int *);
unsigned long	atomic_inc_ulong_nv(volatile unsigned long *);
void *		atomic_inc_ptr_nv(volatile void *);
uint64_t	atomic_inc_64_nv(volatile uint64_t *);

/*
 * Memory barrier operations
 */
void		membar_enter(void);
void		membar_exit(void);
void		membar_producer(void);
void		membar_consumer(void);
void		membar_sync(void);

#ifdef	__HAVE_MEMBAR_DATADEP_CONSUMER
void		membar_datadep_consumer(void);
#else
#define	membar_datadep_consumer()	((void)0)
#endif

#ifdef _KERNEL

#if 1 // XXX: __STDC_VERSION__ < 201112L

/* Pre-C11 definitions */

#include <sys/cdefs.h>
#include <sys/bitops.h>

#ifdef _LP64
#define	__HAVE_ATOMIC64_LOADSTORE	1
#define	__ATOMIC_SIZE_MAX		8
#else
#define	__ATOMIC_SIZE_MAX		4
#endif

/*
 * We assume that access to an aligned pointer to a volatile object of
 * at most __ATOMIC_SIZE_MAX bytes is guaranteed to be atomic.  This is
 * an assumption that may be wrong, but we hope it won't be wrong
 * before we just adopt the C11 atomic API.
 */
#define	__ATOMIC_PTR_CHECK(p) do					      \
{									      \
	CTASSERT(sizeof(*(p)) <= __ATOMIC_SIZE_MAX);			      \
	KASSERT(((uintptr_t)(p) & ilog2(sizeof(*(p)))) == 0);		      \
} while (0)

#define	atomic_load_relaxed(p)						      \
({									      \
	const volatile __typeof__(*(p)) *__al_ptr = (p);		      \
	__ATOMIC_PTR_CHECK(__al_ptr);					      \
	*__al_ptr;							      \
})

#define	atomic_load_consume(p)						      \
({									      \
	const volatile __typeof__(*(p)) *__al_ptr = (p);		      \
	__ATOMIC_PTR_CHECK(__al_ptr);					      \
	__typeof__(*(p)) __al_val = *__al_ptr;				      \
	membar_datadep_consumer();					      \
	__al_val;							      \
})

/*
 * We want {loads}-before-{loads,stores}.  It is tempting to use
 * membar_enter, but that provides {stores}-before-{loads,stores},
 * which may not help.  So we must use membar_sync, which does the
 * slightly stronger {loads,stores}-before-{loads,stores}.
 */
#define	atomic_load_acquire(p)						      \
({									      \
	const volatile __typeof__(*(p)) *__al_ptr = (p);		      \
	__ATOMIC_PTR_CHECK(__al_ptr);					      \
	__typeof__(*(p)) __al_val = *__al_ptr;				      \
	membar_sync();							      \
	__al_val;							      \
})

#define	atomic_store_relaxed(p,v)					      \
({									      \
	volatile __typeof__(*(p)) *__as_ptr = (p);			      \
	__ATOMIC_PTR_CHECK(__as_ptr);					      \
	*__as_ptr = (v);						      \
})

#define	atomic_store_release(p,v)					      \
({									      \
	volatile __typeof__(*(p)) *__as_ptr = (p);			      \
	__typeof__(*(p)) __as_val = (v);				      \
	__ATOMIC_PTR_CHECK(__as_ptr);					      \
	membar_exit();							      \
	*__as_ptr = __as_val;						      \
})

#else  /* __STDC_VERSION__ >= 201112L */

/* C11 definitions, not yet available */

#include <stdatomic.h>

#define	atomic_load_relaxed(p)						      \
	atomic_load_explicit((p), memory_order_relaxed)
#if 0				/* memory_order_consume is not there yet */
#define	atomic_load_consume(p)						      \
	atomic_load_explicit((p), memory_order_consume)
#else
#define	atomic_load_consume(p)						      \
({									      \
	const __typeof__(*(p)) __al_val = atomic_load_relaxed(p);	      \
	membar_datadep_consumer();					      \
	__al_val;							      \
})
#endif
#define	atomic_load_acquire(p)						      \
	atomic_load_explicit((p), memory_order_acquire)
#define	atomic_store_relaxed(p, v)					      \
	atomic_store_explicit((p), (v), memory_order_relaxed)
#define	atomic_store_release(p, v)					      \
	atomic_store_explicit((p), (v), memory_order_release)

#endif	/* __STDC_VERSION__ */

#endif	/* _KERNEL */

__END_DECLS

#endif /* ! _SYS_ATOMIC_H_ */
