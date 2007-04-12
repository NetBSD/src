/*	$NetBSD: atomic.h,v 1.1.2.1 2007/04/12 15:15:59 thorpej Exp $	*/

/*-
 * Copyright (c) 2007 The NetBSD Foundation, Inc.
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
#if !defined(_KERNEL)
#include <stdint.h>
#endif

/*
 * Atomic ADD
 */
void		atomic_add_8(volatile uint8_t *, int8_t);
void		atomic_add_char(volatile unsigned char *, signed char);
void		atomic_add_16(volatile uint16_t *, int16_t);
void		atomic_add_short(volatile unsigned short *, short);
void		atomic_add_32(volatile uint32_t *, int32_t);
void		atomic_add_int(volatile unsigned int *, int);
void		atomic_add_long(volatile unsigned long *, long);
void		atomic_add_ptr(volatile void *, ssize_t);
#if defined(__HAVE_ATOMIC64_OPS)
void		atomic_add_64(volatile uint64_t *, int64_t);
#endif

uint8_t		atomic_add_8_nv(volatile uint8_t *, int8_t);
unsigned char	atomic_add_char_nv(volatile unsigned char *, signed char);
uint16_t	atomic_add_16_nv(volatile uint16_t *, int16_t);
unsigned short	atomic_add_short_nv(volatile unsigned short *, short);
uint32_t	atomic_add_32_nv(volatile uint32_t *, int32_t);
unsigned int	atomic_add_int_nv(volatile unsigned int *, int);
unsigned long	atomic_add_long_nv(volatile unsigned long *, long);
void *		atomic_add_ptr_nv(volatile void *, ssize_t);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_add_64_nv(volatile uint64_t *, int64_t);
#endif

/*
 * Atomic AND
 */
void		atomic_and_8(volatile uint8_t *, uint8_t);
void		atomic_and_uchar(volatile unsigned char *, unsigned char);
void		atomic_and_16(volatile uint16_t *, uint16_t);
void		atomic_and_ushort(volatile unsigned short *, unsigned short);
void		atomic_and_32(volatile uint32_t *, uint32_t);
void		atomic_and_uint(volatile unsigned int *, unsigned int);
void		atomic_and_ulong(volatile unsigned long *, unsigned long);
#if defined(__HAVE_ATOMIC64_OPS)
void		atomic_and_64(volatile uint64_t *, uint64_t);
#endif

uint8_t		atomic_and_8_nv(volatile uint8_t *, uint8_t);
unsigned char	atomic_and_uchar_nv(volatile unsigned char *, unsigned char);
uint16_t	atomic_and_16_nv(volatile uint16_t *, uint16_t);
unsigned short	atomic_and_ushort_nv(volatile unsigned short *, unsigned short);
uint32_t	atomic_and_32_nv(volatile uint32_t *, uint32_t);
unsigned int	atomic_and_uint_nv(volatile unsigned int *, unsigned int);
unsigned long	atomic_and_ulong_nv(volatile unsigned long *, unsigned long);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_and_64_nv(volatile uint64_t *, uint64_t);
#endif

/*
 * Atomic OR
 */
void		atomic_or_8(volatile uint8_t *, uint8_t);
void		atomic_or_uchar(volatile unsigned char *, unsigned char);
void		atomic_or_16(volatile uint16_t *, uint16_t);
void		atomic_or_ushort(volatile unsigned short *, unsigned short);
void		atomic_or_32(volatile uint32_t *, uint32_t);
void		atomic_or_uint(volatile unsigned int *, unsigned int);
void		atomic_or_ulong(volatile unsigned long *, unsigned long);
#if defined(__HAVE_ATOMIC64_OPS)
void		atomic_or_64(volatile uint64_t *, uint64_t);
#endif

uint8_t		atomic_or_8_nv(volatile uint8_t *, uint8_t);
unsigned char	atomic_or_uchar_nv(volatile unsigned char *, unsigned char);
uint16_t	atomic_or_16_nv(volatile uint16_t *, uint16_t);
unsigned short	atomic_or_ushort_nv(volatile unsigned short *, unsigned short);
uint32_t	atomic_or_32_nv(volatile uint32_t *, uint32_t);
unsigned int	atomic_or_uint_nv(volatile unsigned int *, unsigned int);
unsigned long	atomic_or_ulong_nv(volatile unsigned long *, unsigned long);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_or_64_nv(volatile uint64_t *, uint64_t);
#endif

/*
 * Atomic COMPARE-AND-SWAP
 */
uint8_t		atomic_cas_8(volatile uint8_t *, uint8_t, uint8_t);
unsigned char	atomic_cas_uchar(volatile unsigned char *, unsigned char,
				 unsigned char);
uint16_t	atomic_cas_16(volatile uint16_t *, uint16_t, uint16_t);
unsigned short	atomic_cas_ushort(volatile unsinged short *, unsigned short,
				  unsigned short);
uint32_t	atomic_cas_32(volatile uint32_t *, uint32_t, uint32_t);
unsigned int	atomic_cas_uint(volatile unsigned int *, unsigned int,
				unsigned int);
unsigned long	atomic_cas_ulong(volatile unsigned long *, unsigned long,
				 unsigned long);
void *		atomic_cas_ptr(volatile void *, void *, void *);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_cas_64(volatile uint64_t *, uint64_t, uint64_t);
#endif

/*
 * Atomic SWAP
 */
uint8_t		atomic_swap_8(volatile uint8_t *, uint8_t);
unsigned char	atomic_swap_uchar(volatile unsigned char *, unsigned char);
uint16_t	atomic_swap_16(volatile uint16_t *, uint16_t);
unsigned short	atomic_swap_ushort(volatile unsigned short *, unsigned short);
uint32_t	atomic_swap_32(volatile uint32_t *, uint32_t);
unsigned int	atomic_swap_uint(volatile unsigned int *, unsigned int);
unsigned long	atomic_swap_ulong(volatile unsigned long *, unsigned long);
void *		atomic_swap_ptr(volatile void *, void *);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_swap_64(volatile uint64_t *, uint64_t);
#endif

/*
 * Atomic DECREMENT
 */
void		atomic_dec_8(volatile uint8_t *);
void		atomic_dec_uchar(volatile unsigned char *);
void		atomic_dec_16(volatile uint16_t *);
void		atomic_dec_ushort(volatile unsigned short *);
void		atomic_dec_32(volatile uint32_t *);
void		atomic_dec_uint(volatile unsigned int *);
void		atomic_dec_ulong(volatile unsigned long *);
void		atomic_dec_ptr(volatile void *);
#if defined(__HAVE_ATOMIC64_OPS)
void		atomic_dec_64(volatile uint64_t *);
#endif

uint8_t		atomic_dec_8_nv(volatile uint8_t *);
unsigned char	atomic_dec_uchar_nv(volatile unsigned char *);
uint16_t	atomic_dec_16_nv(volatile uint16_t *);
unsigned short	atomic_dec_ushort_nv(volatile unsigned short *);
uint32_t	atomic_dec_32_nv(volatile uint32_t *);
unsigned int	atomic_dec_uint_nv(volatile unsigned int *);
unsigned long	atomic_dec_ulong_nv(volatile unsigned long *);
void *		atomic_dec_ptr_nv(volatile void *);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_dec_64_nv(volatile uint64_t *);
#endif

/*
 * Atomic INCREMENT
 */
void		atomic_inc_8(volatile uint8_t *);
void		atomic_inc_uchar(volatile unsigned char *);
void		atomic_inc_16(volatile uint16_t *);
void		atomic_inc_ushort(volatile unsigned short *);
void		atomic_inc_32(volatile uint32_t *);
void		atomic_inc_uint(volatile unsigned int *);
void		atomic_inc_ulong(volatile unsigned long *);
void		atomic_inc_ptr(volatile void *);
#if defined(__HAVE_ATOMIC64_OPS)
void		atomic_inc_64(volatile uint64_t *);
#endif

uint8_t		atomic_inc_8_nv(volatile uint8_t *);
unsigned char	atomic_inc_uchar_nv(volatile unsigned char *);
uint16_t	atomic_inc_16_nv(volatile uint16_t *);
unsigned short	atomic_inc_ushort_nv(volatile unsigned short *);
uint32_t	atomic_inc_32_nv(volatile uint32_t *);
unsigned int	atomic_inc_uint_nv(volatile unsigned int *);
unsigned long	atomic_inc_ulong_nv(volatile unsigned long *);
void *		atomic_inc_ptr_nv(volatile void *);
#if defined(__HAVE_ATOMIC64_OPS)
uint64_t	atomic_inc_64_nv(volatile uint64_t *);
#endif

/*
 * Memory barrier operations
 */
void		membar_enter(void);
void		membar_exit(void);
void		membar_producer(void);
void		membar_consumer(void);
void		membar_sync(void);
void		membar_write(void);
void		membar_read(void);

#endif /* ! _SYS_ATOMIC_H_ */
