/*	$NetBSD: bus.h,v 1.1.4.2 2002/04/01 07:41:51 nathanw Exp $	*/

/*-
 * Copyright (c) 1997, 1998, 2000, 2001 The NetBSD Foundation, Inc.
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

/*
 * Copyright (c) 1996 Carnegie-Mellon University.
 * All rights reserved.
 *
 * Author: Chris G. Demetriou
 *
 * Permission to use, copy, modify and distribute this software and
 * its documentation is hereby granted, provided that both the copyright
 * notice and this permission notice appear in all copies of the
 * software, derivative works or modified versions, and any portions
 * thereof, and that both notices appear in supporting documentation.
 *
 * CARNEGIE MELLON ALLOWS FREE USE OF THIS SOFTWARE IN ITS "AS IS"
 * CONDITION.  CARNEGIE MELLON DISCLAIMS ANY LIABILITY OF ANY KIND
 * FOR ANY DAMAGES WHATSOEVER RESULTING FROM THE USE OF THIS SOFTWARE.
 *
 * Carnegie Mellon requests users of this software to return to
 *
 *  Software Distribution Coordinator  or  Software.Distribution@CS.CMU.EDU
 *  School of Computer Science
 *  Carnegie Mellon University
 *  Pittsburgh PA 15213-3890
 *
 * any improvements or extensions that they make and grant Carnegie the
 * rights to redistribute these changes.
 */

#ifndef _PLAYSTATION2_BUS_H_
#define	_PLAYSTATION2_BUS_H_

#include <sys/types.h>

#ifdef _KERNEL
/*
 * Turn on BUS_SPACE_DEBUG if the global DEBUG option is enabled.
 */
#if defined(DEBUG) && !defined(BUS_SPACE_DEBUG)
#define	BUS_SPACE_DEBUG
#endif

#ifdef BUS_SPACE_DEBUG
#include <sys/systm.h> /* for printf() prototype */
/*
 * Macros for checking the aligned-ness of pointers passed to bus
 * space ops.  Strict alignment is required by the MIPS architecture,
 * and a trap will occur if unaligned access is performed.  These
 * may aid in the debugging of a broken device driver by displaying
 * useful information about the problem.
 */
#define	__BUS_SPACE_ALIGNED_ADDRESS(p, t)				\
	((((u_int32_t)(p)) & (sizeof(t)-1)) == 0)

#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)				\
({									\
	if (__BUS_SPACE_ALIGNED_ADDRESS((p), t) == 0) {			\
		printf("%s 0x%x not aligned to %u bytes %s:%d\n",	\
		    d, (u_int32_t)(p), (u_int32_t)sizeof(t), __FILE__,	\
		    __LINE__);						\
	}								\
	(void) 0;							\
})

#define BUS_SPACE_ALIGNED_POINTER(p, t) __BUS_SPACE_ALIGNED_ADDRESS(p, t)
#else
#define	__BUS_SPACE_ADDRESS_SANITY(p, t, d)	(void) 0
#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)
#endif /* BUS_SPACE_DEBUG */
#endif /* _KERNEL */

/*
 * Addresses (in bus space).
 */
typedef long bus_addr_t;
typedef long bus_size_t;

/*
 * Access methods for bus space.
 */
typedef const struct playstation2_bus_space *bus_space_tag_t;
typedef bus_addr_t bus_space_handle_t;

struct extent; /* forward declaration */

struct playstation2_bus_space {
	struct extent	*pbs_extent;
	bus_addr_t	pbs_base_addr;

	/* cookie */
	void		*pbs_cookie;

	/* mapping/unmapping */
	int		(*pbs_map)(void *, bus_addr_t, bus_size_t,
			    int, bus_space_handle_t *);
	void		(*pbs_unmap)(void *, bus_space_handle_t,
			    bus_size_t);
	int		(*pbs_subregion)(void *, bus_space_handle_t,
			    bus_size_t, bus_size_t, bus_space_handle_t *);

	/* allocation/deallocation */
	int		(*pbs_alloc)(void *, bus_addr_t, bus_addr_t,
			    bus_size_t, bus_size_t, bus_size_t, int,
			    bus_addr_t *, bus_space_handle_t *);
	void		(*pbs_free)(void *, bus_space_handle_t,
			    bus_size_t);

	/* get kernel virtual address */
	void *		(*pbs_vaddr)(void *, bus_space_handle_t);

	/* read (single) */
	u_int8_t	(*pbs_r_1)(void *, bus_space_handle_t,
			    bus_size_t);
	u_int16_t	(*pbs_r_2)(void *, bus_space_handle_t,
			    bus_size_t);
	u_int32_t	(*pbs_r_4)(void *, bus_space_handle_t,
			    bus_size_t);
	u_int64_t	(*pbs_r_8)(void *, bus_space_handle_t,
			    bus_size_t);

	/* read multiple */
	void		(*pbs_rm_1)(void *, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t);
	void		(*pbs_rm_2)(void *, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t);
	void		(*pbs_rm_4)(void *, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t);
	void		(*pbs_rm_8)(void *, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t);
					
	/* read region */
	void		(*pbs_rr_1)(void *, bus_space_handle_t,
			    bus_size_t, u_int8_t *, bus_size_t);
	void		(*pbs_rr_2)(void *, bus_space_handle_t,
			    bus_size_t, u_int16_t *, bus_size_t);
	void		(*pbs_rr_4)(void *, bus_space_handle_t,
			    bus_size_t, u_int32_t *, bus_size_t);
	void		(*pbs_rr_8)(void *, bus_space_handle_t,
			    bus_size_t, u_int64_t *, bus_size_t);
					
	/* write (single) */
	void		(*pbs_w_1)(void *, bus_space_handle_t,
			    bus_size_t, u_int8_t);
	void		(*pbs_w_2)(void *, bus_space_handle_t,
			    bus_size_t, u_int16_t);
	void		(*pbs_w_4)(void *, bus_space_handle_t,
			    bus_size_t, u_int32_t);
	void		(*pbs_w_8)(void *, bus_space_handle_t,
			    bus_size_t, u_int64_t);

	/* write multiple */
	void		(*pbs_wm_1)(void *, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t);
	void		(*pbs_wm_2)(void *, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t);
	void		(*pbs_wm_4)(void *, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t);
	void		(*pbs_wm_8)(void *, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t);
					
	/* write region */
	void		(*pbs_wr_1)(void *, bus_space_handle_t,
			    bus_size_t, const u_int8_t *, bus_size_t);
	void		(*pbs_wr_2)(void *, bus_space_handle_t,
			    bus_size_t, const u_int16_t *, bus_size_t);
	void		(*pbs_wr_4)(void *, bus_space_handle_t,
			    bus_size_t, const u_int32_t *, bus_size_t);
	void		(*pbs_wr_8)(void *, bus_space_handle_t,
			    bus_size_t, const u_int64_t *, bus_size_t);

	/* set multiple */
	void		(*pbs_sm_1)(void *, bus_space_handle_t,
			    bus_size_t, u_int8_t, bus_size_t);
	void		(*pbs_sm_2)(void *, bus_space_handle_t,
			    bus_size_t, u_int16_t, bus_size_t);
	void		(*pbs_sm_4)(void *, bus_space_handle_t,
			    bus_size_t, u_int32_t, bus_size_t);
	void		(*pbs_sm_8)(void *, bus_space_handle_t,
			    bus_size_t, u_int64_t, bus_size_t);

	/* set region */
	void		(*pbs_sr_1)(void *, bus_space_handle_t,
			    bus_size_t, u_int8_t, bus_size_t);
	void		(*pbs_sr_2)(void *, bus_space_handle_t,
			    bus_size_t, u_int16_t, bus_size_t);
	void		(*pbs_sr_4)(void *, bus_space_handle_t,
			    bus_size_t, u_int32_t, bus_size_t);
	void		(*pbs_sr_8)(void *, bus_space_handle_t,
			    bus_size_t, u_int64_t, bus_size_t);

	/* copy */
	void		(*pbs_c_1)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*pbs_c_2)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*pbs_c_4)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
	void		(*pbs_c_8)(void *, bus_space_handle_t, bus_size_t,
			    bus_space_handle_t, bus_size_t, bus_size_t);
};

#ifdef _KERNEL
#define _wbflush()	__asm__ __volatile__("sync.l")
#ifdef _PLAYSTATION2_BUS_SPACE_PRIVATE

#ifndef __read_1
#define	__read_1(a)	(*(__volatile__ u_int8_t *)(a))
#endif
#ifndef __read_2
#define	__read_2(a)	(*(__volatile__ u_int16_t *)(a))
#endif
#ifndef __read_4
#define	__read_4(a)	(*(__volatile__ u_int32_t *)(a))
#endif
#ifndef __read_8
#define __read_8(a)							\
({									\
	u_int32_t lo, hi;						\
	__asm__ __volatile__(						\
		".set noreorder;"					\
		".set push;"						\
		".set mips3;"						\
		"ld	$8, (%2);"					\
		"dsra	%1, $8, 32;"					\
		"dsll	%0, $8, 32;"					\
		"dsra	%0, %0, 32;"					\
		".set pop;"						\
		".set reorder;"						\
		: "=r"(lo), "=r"(hi) : "r"(a) : "$8");			\
	((u_int64_t)hi << 32) | lo;					\
})
#endif
#define __read_16(a)	"error. not yet"

#ifndef __write_1
#define	__write_1(a, v) {						\
	*(__volatile__ u_int8_t *)(a) = (v);				\
	_wbflush();							\
}
#endif
#ifndef __write_2
#define	__write_2(a, v)	{						\
	*(__volatile__ u_int16_t *)(a) = (v);				\
	_wbflush();							\
}
#endif
#ifndef __write_4
#define	__write_4(a, v)	{						\
	*(__volatile__ u_int32_t *)(a) = (v);				\
	_wbflush();							\
}
#endif
#ifdef EE_GCC
#ifndef __write_8
#define	__write_8(a, v)	(*(volatile u_int64_t *)(a) = (v)) {		\
	_wbflush();							\
}
#endif
#ifndef __write_16
#define __write_16(a, v)	(*(volatile u_int128_t *)(a) = (v)) {	\
	_wbflush();							\
}
#endif
#else /* EE_GCC */
#ifdef __write_8
#error "can't override __write_8"
#endif
static __inline__ void
__write_8(bus_addr_t a, u_int64_t v)
{
	__asm__ __volatile__(
		".set noreorder;"
		".set push;"
		".set r5900;"
		"pextlw	$8, %0, %1;"
		"sd	$8, 0(%2);"
		"sync.l;"
		".set pop;"
		".set reorder;"
		: : "r"((u_int32_t)((u_int64_t)(v) >> 32)),
		"r"((u_int32_t)(v)), "r"((u_int32_t)(a)) : "$8");
}
#define _write_16(a)	"error. not yet"
#endif /* EE_GCC */

#define	__TYPENAME(BITS)	u_int##BITS##_t

#define _BUS_SPACE_READ(PREFIX, BYTES, BITS)				\
static __TYPENAME(BITS)							\
PREFIX##_read_##BYTES(void *, bus_space_handle_t,  bus_size_t);		\
static __TYPENAME(BITS)							\
PREFIX##_read_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset)							\
{									\
	return __read_##BYTES##(VADDR(bsh, offset));			\
}

#define _BUS_SPACE_READ_MULTI(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_read_multi_##BYTES(void *, bus_space_handle_t,	bus_size_t,	\
    __TYPENAME(BITS) *,	bus_size_t);					\
static void								\
PREFIX##_read_multi_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset, __TYPENAME(BITS) *addr, bus_size_t count)	\
{									\
	bus_addr_t a = VADDR(bsh, offset);				\
	while (count--)							\
		*addr++ = __read_##BYTES##(a);				\
}

#define _BUS_SPACE_READ_REGION(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_read_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
    __TYPENAME(BITS) *, bus_size_t);					\
static void								\
PREFIX##_read_region_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset, __TYPENAME(BITS) *addr, bus_size_t count)	\
{									\
	while (count--) {						\
		*addr++ = __read_##BYTES##(VADDR(bsh, offset));		\
		offset += BYTES;					\
	}								\
}

#define _BUS_SPACE_WRITE(PREFIX, BYTES, BITS)				\
static void								\
PREFIX##_write_##BYTES(void *, bus_space_handle_t, bus_size_t,		\
    __TYPENAME(BITS));							\
static void								\
PREFIX##_write_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset, __TYPENAME(BITS) value)				\
{									\
	__write_##BYTES##(VADDR(bsh, offset), value);			\
}

#define _BUS_SPACE_WRITE_MULTI(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_write_multi_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
    const __TYPENAME(BITS) *, bus_size_t);				\
static void								\
PREFIX##_write_multi_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset, const __TYPENAME(BITS) *addr, bus_size_t count)	\
{									\
	bus_addr_t a = VADDR(bsh, offset);				\
	while (count--) {						\
		__write_##BYTES##(a, *addr++);				\
	}								\
}

#define _BUS_SPACE_WRITE_REGION(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_write_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
    const __TYPENAME(BITS) *, bus_size_t);				\
static void								\
PREFIX##_write_region_##BYTES(void *tag, bus_space_handle_t bsh,	\
    bus_size_t offset, const __TYPENAME(BITS) *addr, bus_size_t count)	\
{									\
	while (count--) {						\
		__write_##BYTES##(VADDR(bsh, offset), *addr++);		\
		offset += BYTES;					\
	}								\
}

#define _BUS_SPACE_SET_MULTI(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_set_multi_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
    __TYPENAME(BITS), bus_size_t);					\
static void								\
PREFIX##_set_multi_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset, __TYPENAME(BITS) value, bus_size_t count)	\
{									\
	bus_addr_t a = VADDR(bsh, offset);				\
	while (count--) {						\
		__write_##BYTES##(a, value);				\
	}								\
}

#define _BUS_SPACE_SET_REGION(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_set_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
    __TYPENAME(BITS), bus_size_t);					\
static void								\
PREFIX##_set_region_##BYTES(void *tag, bus_space_handle_t bsh,		\
    bus_size_t offset, __TYPENAME(BITS) value, bus_size_t count)	\
{									\
	while (count--) {						\
		__write_##BYTES##(VADDR(bsh, offset), value);		\
		offset += BYTES;					\
	}								\
}

#define _BUS_SPACE_COPY_REGION(PREFIX, BYTES, BITS)			\
static void								\
PREFIX##_copy_region_##BYTES(void *, bus_space_handle_t, bus_size_t,	\
    bus_space_handle_t, bus_size_t, bus_size_t);			\
static void								\
PREFIX##_copy_region_##BYTES(void *t, bus_space_handle_t h1,		\
    bus_size_t o1, bus_space_handle_t h2, bus_size_t o2, bus_size_t c)	\
{									\
	bus_size_t o;							\
	if ((h1 + o1) >= (h2 + o2)) {					\
		/* src after dest: copy forward */			\
		for (o = 0; c != 0; c--, o += BYTES)			\
			__write_##BYTES##(VADDR(h2, o2 + o),		\
			    __read_##BYTES##(VADDR(h1, o1 + o)));	\
	} else {							\
		/* dest after src: copy backwards */			\
		for (o = (c - 1) * BYTES; c != 0; c--, o -= BYTES)	\
			__write_##BYTES##(VADDR(h2, o2 + o),		\
			    __read_##BYTES##(VADDR(h1, o1 + o)));	\
	}								\
}

#define _BUS_SPACE_NO_MAP						\
	(int (*)(void *, bus_addr_t, bus_size_t, int,			\
	bus_space_handle_t *))_bus_space_invalid_access
#define _BUS_SPACE_NO_UNMAP						\
	(void (*)(void *, bus_space_handle_t, bus_size_t))		\
	_bus_space_invalid_access
#define _BUS_SPACE_NO_SUBREGION						\
	(int (*)(void *, bus_space_handle_t, bus_size_t, bus_size_t,	\
	bus_space_handle_t *))_bus_space_invalid_access
#define _BUS_SPACE_NO_ALLOC						\
	(int (*)(void *, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t,\
	 bus_size_t, int, bus_addr_t *,	bus_space_handle_t *))		\
	_bus_space_invalid_access
#define _BUS_SPACE_NO_FREE						\
	(void (*)(void *, bus_space_handle_t, bus_size_t))		\
	_bus_space_invalid_access
#define _BUS_SPACE_NO_VADDR						\
	(void *(*)(void *, bus_space_handle_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_READ(BYTES, BITS)					\
	(u_int##BITS##_t (*)(void *, bus_space_handle_t, bus_size_t))	\
	_bus_space_invalid_access
#define _BUS_SPACE_NO_READ_MULTI(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	u_int##BITS##_t *, bus_size_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_READ_REGION(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	u_int##BITS##_t *, bus_size_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_WRITE(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	u_int##BITS##_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_WRITE_MULTI(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	const u_int##BITS##_t *, bus_size_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_WRITE_REGION(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	const u_int##BITS##_t *, bus_size_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_SET_MULTI(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	u_int##BITS##_t, bus_size_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_SET_REGION(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	u_int##BITS##_t, bus_size_t))_bus_space_invalid_access
#define _BUS_SPACE_NO_COPY_REGION(BYTES, BITS)				\
	(void (*)(void *, bus_space_handle_t, bus_size_t,		\
	bus_space_handle_t, bus_size_t, bus_size_t))_bus_space_invalid_access

void _bus_space_invalid_access(void);
#endif /* _PLAYSTATION2_BUS_SPACE_PRIVATE */

#define	__pbs_c(a,b)		__CONCAT(a,b)
#define	__pbs_opname(op,size)	__pbs_c(__pbs_c(__pbs_c(pbs_,op),_),size)

#define	__pbs_rs(sz, tn, t, h, o)					\
	(__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr"),		\
	 (*(t)->__pbs_opname(r,sz))((t)->pbs_cookie, h, o))

#define	__pbs_ws(sz, tn, t, h, o, v)					\
({									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__pbs_opname(w,sz))((t)->pbs_cookie, h, o, v);		\
})

#define	__pbs_nonsingle(type, sz, tn, t, h, o, a, c)			\
({									\
	__BUS_SPACE_ADDRESS_SANITY((a), tn, "buffer");			\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__pbs_opname(type,sz))((t)->pbs_cookie, h, o, a, c);	\
})

#define	__pbs_set(type, sz, tn, t, h, o, v, c)				\
({									\
	__BUS_SPACE_ADDRESS_SANITY((h) + (o), tn, "bus addr");		\
	(*(t)->__pbs_opname(type,sz))((t)->pbs_cookie, h, o, v, c);	\
})

#define	__pbs_copy(sz, tn, t, h1, o1, h2, o2, cnt)			\
({									\
	__BUS_SPACE_ADDRESS_SANITY((h1) + (o1), tn, "bus addr 1");	\
	__BUS_SPACE_ADDRESS_SANITY((h2) + (o2), tn, "bus addr 2");	\
	(*(t)->__pbs_opname(c,sz))((t)->pbs_cookie, h1, o1, h2, o2, cnt); \
})

/*
 * Create default bus_space tag.
 */
bus_space_tag_t bus_space_create(bus_space_tag_t, const char *, bus_addr_t,
    bus_size_t);
void bus_space_destroy(bus_space_tag_t);

/*
 * Mapping and unmapping operations.
 */
#define	bus_space_map(t, a, s, f, hp)					\
	(*(t)->pbs_map)((t)->pbs_cookie, (a), (s), (f), (hp))
#define	bus_space_unmap(t, h, s)					\
	(*(t)->pbs_unmap)((t)->pbs_cookie, (h), (s))
#define	bus_space_subregion(t, h, o, s, hp)				\
	(*(t)->pbs_subregion)((t)->pbs_cookie, (h), (o), (s), (hp))

#endif /* _KERNEL */

#define	BUS_SPACE_MAP_CACHEABLE		0x01
#define	BUS_SPACE_MAP_LINEAR		0x02
#define	BUS_SPACE_MAP_PREFETCHABLE     	0x04

#ifdef _KERNEL
/*
 * Allocation and deallocation operations.
 */
#define	bus_space_alloc(t, rs, re, s, a, b, f, ap, hp)			\
	(*(t)->pbs_alloc)((t)->pbs_cookie, (rs), (re), (s), (a), (b),	\
	    (f), (ap), (hp))
#define	bus_space_free(t, h, s)						\
	(*(t)->pbs_free)((t)->pbs_cookie, (h), (s))

/*
 * Get kernel virtual address for ranges mapped BUS_SPACE_MAP_LINEAR.
 */
#define bus_space_vaddr(t, h)						\
	(*(t)->pbs_vaddr)((t)->pbs_cookie, (h))

/*
 * Bus barrier operations.  The playstation2 does not currently require
 * barriers, but we must provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)				\
({									\
	_wbflush();							\
})


#define	BUS_SPACE_BARRIER_READ	0x01
#define	BUS_SPACE_BARRIER_WRITE	0x02


/*
 * Bus read (single) operations.
 */
#define	bus_space_read_1(t, h, o)	__pbs_rs(1,u_int8_t,(t),(h),(o))
#define	bus_space_read_2(t, h, o)	__pbs_rs(2,u_int16_t,(t),(h),(o))
#define	bus_space_read_4(t, h, o)	__pbs_rs(4,u_int32_t,(t),(h),(o))
#define	bus_space_read_8(t, h, o)	__pbs_rs(8,u_int64_t,(t),(h),(o))


/*
 * Bus read multiple operations.
 */
#define	bus_space_read_multi_1(t, h, o, a, c)				\
	__pbs_nonsingle(rm,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_2(t, h, o, a, c)				\
	__pbs_nonsingle(rm,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_4(t, h, o, a, c)				\
	__pbs_nonsingle(rm,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_multi_8(t, h, o, a, c)				\
	__pbs_nonsingle(rm,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Bus read region operations.
 */
#define	bus_space_read_region_1(t, h, o, a, c)				\
	__pbs_nonsingle(rr,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_2(t, h, o, a, c)				\
	__pbs_nonsingle(rr,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_4(t, h, o, a, c)				\
	__pbs_nonsingle(rr,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_read_region_8(t, h, o, a, c)				\
	__pbs_nonsingle(rr,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Bus write (single) operations.
 */
#define	bus_space_write_1(t, h, o, v)	__pbs_ws(1,u_int8_t,(t),(h),(o),(v))
#define	bus_space_write_2(t, h, o, v)	__pbs_ws(2,u_int16_t,(t),(h),(o),(v))
#define	bus_space_write_4(t, h, o, v)	__pbs_ws(4,u_int32_t,(t),(h),(o),(v))
#define	bus_space_write_8(t, h, o, v)	__pbs_ws(8,u_int64_t,(t),(h),(o),(v))


/*
 * Bus write multiple operations.
 */
#define	bus_space_write_multi_1(t, h, o, a, c)				\
	__pbs_nonsingle(wm,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_2(t, h, o, a, c)				\
	__pbs_nonsingle(wm,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_4(t, h, o, a, c)				\
	__pbs_nonsingle(wm,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_multi_8(t, h, o, a, c)				\
	__pbs_nonsingle(wm,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Bus write region operations.
 */
#define	bus_space_write_region_1(t, h, o, a, c)				\
	__pbs_nonsingle(wr,1,u_int8_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_2(t, h, o, a, c)				\
	__pbs_nonsingle(wr,2,u_int16_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_4(t, h, o, a, c)				\
	__pbs_nonsingle(wr,4,u_int32_t,(t),(h),(o),(a),(c))
#define	bus_space_write_region_8(t, h, o, a, c)				\
	__pbs_nonsingle(wr,8,u_int64_t,(t),(h),(o),(a),(c))


/*
 * Set multiple operations.
 */
#define	bus_space_set_multi_1(t, h, o, v, c)				\
	__pbs_set(sm,1,u_int8_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_2(t, h, o, v, c)				\
	__pbs_set(sm,2,u_int16_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_4(t, h, o, v, c)				\
	__pbs_set(sm,4,u_int32_t,(t),(h),(o),(v),(c))
#define	bus_space_set_multi_8(t, h, o, v, c)				\
	__pbs_set(sm,8,u_int64_t,(t),(h),(o),(v),(c))


/*
 * Set region operations.
 */
#define	bus_space_set_region_1(t, h, o, v, c)				\
	__pbs_set(sr,1,u_int8_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_2(t, h, o, v, c)				\
	__pbs_set(sr,2,u_int16_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_4(t, h, o, v, c)				\
	__pbs_set(sr,4,u_int32_t,(t),(h),(o),(v),(c))
#define	bus_space_set_region_8(t, h, o, v, c)				\
	__pbs_set(sr,8,u_int64_t,(t),(h),(o),(v),(c))


/*
 * Copy region operations.
 */
#define	bus_space_copy_region_1(t, h1, o1, h2, o2, c)			\
	__pbs_copy(1, u_int8_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_2(t, h1, o1, h2, o2, c)			\
	__pbs_copy(2, u_int16_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_4(t, h1, o1, h2, o2, c)			\
	__pbs_copy(4, u_int32_t, (t), (h1), (o1), (h2), (o2), (c))
#define	bus_space_copy_region_8(t, h1, o1, h2, o2, c)			\
	__pbs_copy(8, u_int64_t, (t), (h1), (o1), (h2), (o2), (c))

/*
 * Bus stream operations--defined in terms of non-stream counterparts
 */
#define __BUS_SPACE_HAS_STREAM_METHODS 1
#define bus_space_read_stream_1 bus_space_read_1
#define bus_space_read_stream_2 bus_space_read_2
#define bus_space_read_stream_4 bus_space_read_4
#define	bus_space_read_stream_8 bus_space_read_8
#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_read_multi_stream_2 bus_space_read_multi_2
#define bus_space_read_multi_stream_4 bus_space_read_multi_4
#define	bus_space_read_multi_stream_8 bus_space_read_multi_8
#define bus_space_read_region_stream_1 bus_space_read_region_1
#define bus_space_read_region_stream_2 bus_space_read_region_2
#define bus_space_read_region_stream_4 bus_space_read_region_4
#define	bus_space_read_region_stream_8 bus_space_read_region_8
#define bus_space_write_stream_1 bus_space_write_1
#define bus_space_write_stream_2 bus_space_write_2
#define bus_space_write_stream_4 bus_space_write_4
#define	bus_space_write_stream_8 bus_space_write_8
#define bus_space_write_multi_stream_1 bus_space_write_multi_1
#define bus_space_write_multi_stream_2 bus_space_write_multi_2
#define bus_space_write_multi_stream_4 bus_space_write_multi_4
#define	bus_space_write_multi_stream_8 bus_space_write_multi_8
#define bus_space_write_region_stream_1 bus_space_write_region_1
#define bus_space_write_region_stream_2 bus_space_write_region_2
#define bus_space_write_region_stream_4 bus_space_write_region_4
#define	bus_space_write_region_stream_8	bus_space_write_region_8

#endif /* _KERNEL */

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x000	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x001	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x002	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x004	/* hint: map memory DMA coherent */
#define	BUS_DMA_STREAMING	0x008	/* hint: sequential, unidirectional */
#define	BUS_DMA_BUS1		0x010	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x020
#define	BUS_DMA_BUS3		0x040
#define	BUS_DMA_BUS4		0x080
#define	BUS_DMA_READ		0x100	/* mapping is device -> memory only */
#define	BUS_DMA_WRITE		0x200	/* mapping is memory -> device only */

#define	PLAYSTATION2_DMAMAP_COHERENT	0x10000	/* no cache flush necessary on sync */

/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

typedef struct playstation2_bus_dma_tag		*bus_dma_tag_t;
typedef struct playstation2_bus_dmamap		*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct playstation2_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
	bus_addr_t	_ds_vaddr;	/* virtual address, 0 if invalid */
};
typedef struct playstation2_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dma_tag_t
 *
 *	A machine-dependent opaque type describing the implementation of
 *	DMA for a given bus.
 */

struct playstation2_bus_dma_tag {
	/*
	 * DMA mapping methods.
	 */
	int	(*_dmamap_create)(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*_dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*_dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*_dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*_dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*_dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*_dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*_dmamap_sync)(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*_dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*_dmamem_free)(bus_dma_tag_t,
		    bus_dma_segment_t *, int);
	int	(*_dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int);
	void	(*_dmamem_unmap)(bus_dma_tag_t, caddr_t, size_t);
	paddr_t	(*_dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);

	/* 
	 * DMA controller private.
	 */
	void	*_dmachip_cookie;
};

#define	bus_dmamap_create(t, s, n, m, b, f, p)			\
	(*(t)->_dmamap_create)((t), (s), (n), (m), (b), (f), (p))
#define	bus_dmamap_destroy(t, p)				\
	(*(t)->_dmamap_destroy)((t), (p))
#define	bus_dmamap_load(t, m, b, s, p, f)			\
	(*(t)->_dmamap_load)((t), (m), (b), (s), (p), (f))
#define	bus_dmamap_load_mbuf(t, m, b, f)			\
	(*(t)->_dmamap_load_mbuf)((t), (m), (b), (f))
#define	bus_dmamap_load_uio(t, m, u, f)				\
	(*(t)->_dmamap_load_uio)((t), (m), (u), (f))
#define	bus_dmamap_load_raw(t, m, sg, n, s, f)			\
	(*(t)->_dmamap_load_raw)((t), (m), (sg), (n), (s), (f))
#define	bus_dmamap_unload(t, p)					\
	(*(t)->_dmamap_unload)((t), (p))
#define	bus_dmamap_sync(t, p, o, l, ops)			\
	(*(t)->_dmamap_sync)((t), (p), (o), (l), (ops))

#define	bus_dmamem_alloc(t, s, a, b, sg, n, r, f)		\
	(*(t)->_dmamem_alloc)((t), (s), (a), (b), (sg), (n), (r), (f))
#define	bus_dmamem_free(t, sg, n)				\
	(*(t)->_dmamem_free)((t), (sg), (n))
#define	bus_dmamem_map(t, sg, n, s, k, f)			\
	(*(t)->_dmamem_map)((t), (sg), (n), (s), (k), (f))
#define	bus_dmamem_unmap(t, k, s)				\
	(*(t)->_dmamem_unmap)((t), (k), (s))
#define	bus_dmamem_mmap(t, sg, n, o, p, f)			\
	(*(t)->_dmamem_mmap)((t), (sg), (n), (o), (p), (f))

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct playstation2_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	_dm_size;	/* largest DMA transfer mappable */
	int		_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	_dm_maxsegsz;	/* largest possible segment */
	bus_size_t	_dm_boundary;	/* don't cross this */
	int		_dm_flags;	/* misc. flags */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

#ifdef _PLAYSTATION2_BUS_DMA_PRIVATE
int	_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);

int	_bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags);
void	_bus_dmamem_free(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs);
int	_bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags);
void	_bus_dmamem_unmap(bus_dma_tag_t tag, caddr_t kva,
	    size_t size);
paddr_t	_bus_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

int	_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    vaddr_t low, vaddr_t high);

extern struct playstation2_bus_dma_tag playstation2_default_bus_dma_tag;
#endif /* _PLAYSTATION2_BUS_DMA_PRIVATE */

#endif /* _PLAYSTATION2_BUS_H_ */
