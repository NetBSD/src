/*	$NetBSD: bus_space.h,v 1.15.34.1 2012/04/17 00:06:44 yamt Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe of the Numerical Aerospace Simulation Facility,
 * NASA Ames Research Center.
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
 * Copyright (C) 1997 Scott Reynolds.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _NEXT68K_BUS_SPACE_H_
#define	_NEXT68K_BUS_SPACE_H_
/*
 * Addresses (in bus space).
 */
typedef u_long bus_addr_t;
typedef u_long bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef volatile char *  bus_space_tag_t;
typedef u_long	bus_space_handle_t;

/*
 * Value for the next68k bus space tag, not to be used directly by MI code.
 */
#define NEXT68K_INTIO_BUS_SPACE		((bus_space_tag_t)intiobase)

/*
 * Values for the next68k video bus space tags, not to be used directly
 * by MI code.
 */
#define NEXT68K_MONO_VIDEO_BUS_SPACE	((bus_space_tag_t)monobase)
#define NEXT68K_COLOR_VIDEO_BUS_SPACE	((bus_space_tag_t)colorbase)

/*
 * Mapping and unmapping operations.
 */
#define	bus_space_map(t, a, s, f, hp)					\
    ((((a)>=INTIOBASE)&&((a)+(s)<INTIOTOP)) ?				\
     ((*(hp)=(bus_space_handle_t)((t)+((a)-INTIOBASE))),0) :            \
     ((((a)>=MONOBASE)&&((a)+(s)<MONOTOP)) ?                          \
      ((*(hp)=(bus_space_handle_t)((t)+((a)-MONOBASE))),0) :           \
      ((((a)>=COLORBASE)&&((a)+(s)<COLORTOP)) ?                         \
       ((*(hp)=(bus_space_handle_t)((t)+((a)-COLORBASE))),0) : (-1))))

#define	bus_space_unmap(t, h, s)
	
#define	bus_space_subregion(t, h, o, s, hp)				\
     (*(hp)=(h)+(o))

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02

/*
 * Allocation and deallocation operations.
 */
#define	bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)  		\
     (-1)

#define	bus_space_free(t, h, s)

/*
 *	paddr_t bus_space_mmap(bus_space_tag_t t, bus_addr_t base,
 *	    off_t offset, int prot, int flags);
 *
 * Mmap an area of bus space.
 */

#define bus_space_mmap(t, a, s, prot, flags)				\
	((((a)>=INTIOBASE)&&((a)+(s)<INTIOTOP)) ?			\
		m68k_btop((t)+((a)-INTIOBASE)) :			\
	 ((((a)>=MONOBASE)&&((a)+(s)<MONOTOP)) ?			\
		m68k_btop((t)+((a)-MONOBASE)) :				\
	  ((((a)>=COLORBASE)&&((a)+(s)<COLORTOP)) ?			\
		m68k_btop((t)+((a)-COLORBASE)) : (-1))))

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1(t, h, o)					\
    ((void) t, (*(volatile u_int8_t *)((h) + (o))))

#define	bus_space_read_2(t, h, o)					\
    ((void) t, (*(volatile u_int16_t *)((h) + (o))))

#define	bus_space_read_4(t, h, o)					\
    ((void) t, (*(volatile u_int32_t *)((h) + (o))))

#if 0	/* Cause a link error for bus_space_read_8 */
#define	bus_space_read_8(t, h, o)	!!! bus_space_read_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle/offset and copy into buffer provided.
 */

#define	bus_space_read_multi_1(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movb	%%a0@,%%a1@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_read_multi_2(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movw	%%a0@,%%a1@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_read_multi_4(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movl	%%a0@,%%a1@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#if 0	/* Cause a link error for bus_space_read_multi_8 */
#define	bus_space_read_multi_8	!!! bus_space_read_multi_8 unimplemented !!!
#endif

/*
 *	void bus_space_read_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t *addr, size_t count);
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */

#define	bus_space_read_region_1(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movb	%%a0@+,%%a1@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_read_region_2(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movw	%%a0@+,%%a1@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_read_region_4(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movl	%%a0@+,%%a1@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#if 0	/* Cause a link error for bus_space_read_region_8 */
#define	bus_space_read_region_8	!!! bus_space_read_region_8 unimplemented !!!
#endif

/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int8_t *)((h) + (o)) = (v))))

#define	bus_space_write_2(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int16_t *)((h) + (o)) = (v))))

#define	bus_space_write_4(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int32_t *)((h) + (o)) = (v))))

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_8	!!! bus_space_write_8 not implemented !!!
#endif

/*
 *	void bus_space_write_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer
 * provided to bus space described by tag/handle/offset.
 */

#define	bus_space_write_multi_1(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movb	%%a1@+,%%a0@				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_write_multi_2(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movw	%%a1@+,%%a0@				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_write_multi_4(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movl	%%a1@+,%%a0@				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#if 0	/* Cause a link error for bus_space_write_8 */
#define	bus_space_write_multi_8(t, h, o, a, c)				\
			!!! bus_space_write_multi_8 unimplimented !!!
#endif

/*
 *	void bus_space_write_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    const u_intN_t *addr, size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * to bus space described by tag/handle starting at `offset'.
 */

#define	bus_space_write_region_1(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movb	%%a1@+,%%a0@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_write_region_2(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movw	%%a1@+,%%a0@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#define	bus_space_write_region_4(t, h, o, a, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%a1					;	\
		movl	%2,%%d0					;	\
	1:	movl	%%a1@+,%%a0@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (a), "g" (c)		:	\
		    "a0","a1","d0");					\
} while (0);

#if 0	/* Cause a link error for bus_space_write_region_8 */
#define	bus_space_write_region_8					\
			!!! bus_space_write_region_8 unimplemented !!!
#endif

/*
 *	void bus_space_set_multi_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

#define	bus_space_set_multi_1(t, h, o, val, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%d1					;	\
		movl	%2,%%d0					;	\
	1:	movb	%%d1,%%a0@				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (val), "g" (c)		:	\
		    "a0","d0","d1");					\
} while (0);

#define	bus_space_set_multi_2(t, h, o, val, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%d1					;	\
		movl	%2,%%d0					;	\
	1:	movw	%%d1,%%a0@				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (val), "g" (c)		:	\
		    "a0","d0","d1");					\
} while (0);

#define	bus_space_set_multi_4(t, h, o, val, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%d1					;	\
		movl	%2,%%d0					;	\
	1:	movl	%%d1,%%a0@				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (val), "g" (c)		:	\
		    "a0","d0","d1");					\
} while (0);

#if 0	/* Cause a link error for bus_space_set_multi_8 */
#define	bus_space_set_multi_8						\
			!!! bus_space_set_multi_8 unimplemented !!!
#endif

/*
 *	void bus_space_set_region_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *	    size_t count);
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */

#define	bus_space_set_region_1(t, h, o, val, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%d1					;	\
		movl	%2,%%d0					;	\
	1:	movb	%%d1,%%a0@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (val), "g" (c)		:	\
		    "a0","d0","d1");					\
} while (0);

#define	bus_space_set_region_2(t, h, o, val, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%d1					;	\
		movl	%2,%%d0					;	\
	1:	movw	%%d1,%%a0@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (val), "g" (c)		:	\
		    "a0","d0","d1");					\
} while (0);

#define	bus_space_set_region_4(t, h, o, val, c) do {			\
	(void) t;							\
	__asm volatile ("						\
		movl	%0,%%a0					;	\
		movl	%1,%%d1					;	\
		movl	%2,%%d0					;	\
	1:	movl	%%d1,%%a0@+				;	\
		subql	#1,%%d0					;	\
		jne	1b"					:	\
								:	\
		    "r" ((h) + (o)), "g" (val), "g" (c)		:	\
		    "a0","d0","d1");					\
} while (0);

#if 0	/* Cause a link error for bus_space_set_region_8 */
#define	bus_space_set_region_8						\
			!!! bus_space_set_region_8 unimplemented !!!
#endif

/*
 *	void bus_space_copy_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh1, bus_size_t off1,
 *	    bus_space_handle_t bsh2, bus_size_t off2,
 *	    size_t count);
 *
 * Copy `count' 1, 2, 4, or 8 byte values from bus space starting
 * at tag/bsh1/off1 to bus space starting at tag/bsh2/off2.
 */

#define	__NEXT68K_copy_region_N(BYTES)					\
static __inline void __CONCAT(bus_space_copy_region_,BYTES)		\
	(bus_space_tag_t,						\
	 bus_space_handle_t, bus_size_t,				\
	 bus_space_handle_t, bus_size_t,				\
	 bus_size_t);							\
									\
static __inline void							\
__CONCAT(bus_space_copy_region_,BYTES)(					\
	bus_space_tag_t t,						\
	bus_space_handle_t h1,						\
	bus_size_t o1,							\
	bus_space_handle_t h2,						\
	bus_size_t o2,							\
	bus_size_t c)							\
{									\
	bus_size_t o;							\
									\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__CONCAT(bus_space_write_,BYTES)(t, h2, o2 + o,	\
			    __CONCAT(bus_space_read_,BYTES)(t, h1, o1 + o)); \
	}								\
}
__NEXT68K_copy_region_N(1)
__NEXT68K_copy_region_N(2)
__NEXT68K_copy_region_N(4)
#if 0	/* Cause a link error for bus_space_copy_8 */
#define	bus_space_copy_8						\
			!!! bus_space_copy_8 unimplemented !!!
#endif

#undef __NEXT68K_copy_region_N

/*
 * Bus read/write barrier methods.
 *
 *	void bus_space_barrier(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    bus_size_t len, int flags);
 *
 * Note: the 680x0 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

#endif /* _NEXT68K_BUS_SPACE_H_ */
