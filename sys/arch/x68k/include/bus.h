/*	$NetBSD: bus.h,v 1.10 2003/01/28 01:08:08 kent Exp $	*/

/*-
 * Copyright (c) 1998, 2001 The NetBSD Foundation, Inc.
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
 * bus_space(9) and bus_dma(9) interface for NetBSD/x68k.
 */

#ifndef _X68K_BUS_H_
#define _X68K_BUS_H_

#ifndef X68K_BUS_PERFORMANCE_HACK
#if defined(__GNUC__) && defined(__STDC__)
#define X68K_BUS_PERFORMANCE_HACK	1
#else
#define X68K_BUS_PERFORMANCE_HACK	0
#endif
#endif

/*
 * Bus address and size types
 */
typedef u_long	bus_addr_t;
typedef u_long	bus_size_t;
typedef	u_long	bus_space_handle_t;

/*
 * Bus space descripter
 */
typedef struct x68k_bus_space *bus_space_tag_t;

struct x68k_bus_space {
#if 0
	enum {
		X68K_INTIO_BUS,
		X68K_PCI_BUS,
		X68K_NEPTUNE_BUS
	}	x68k_bus_type;
#endif

	int	(*x68k_bus_space_map) __P((
				bus_space_tag_t,
				bus_addr_t,
				bus_size_t,
				int,			/* flags */
				bus_space_handle_t *));
	void	(*x68k_bus_space_unmap) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t));
	int	(*x68k_bus_space_subregion) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/* offset */
				bus_size_t,		/* size */
				bus_space_handle_t *));

	int	(*x68k_bus_space_alloc) __P((
				bus_space_tag_t,
				bus_addr_t,		/* reg_start */
				bus_addr_t,		/* reg_end */
				bus_size_t,
				bus_size_t,		/* alignment */
				bus_size_t,		/* boundary */
				int,			/* flags */
				bus_addr_t *,
				bus_space_handle_t *));
	void	(*x68k_bus_space_free) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t));

#if 0
	void	(*x68k_bus_space_barrier) __P((
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/* offset */
				bus_size_t,		/* length */
				int));			/* flags */
#endif

	struct device *x68k_bus_device;
};

int x68k_bus_space_alloc __P((bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t, bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *));
void x68k_bus_space_free __P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

/*
 * bus_space(9) interface
 */

#define bus_space_map(t,a,s,f,h) \
		((*((t)->x68k_bus_space_map)) ((t),(a),(s),(f),(h)))
#define bus_space_unmap(t,h,s) \
		((*((t)->x68k_bus_space_unmap)) ((t),(h),(s)))
#define bus_space_subregion(t,h,o,s,p) \
		((*((t)->x68k_bus_space_subregion)) ((t),(h),(o),(s),(p)))
#define BUS_SPACE_MAP_CACHEABLE		0x0001
#define BUS_SPACE_MAP_LINEAR		0x0002
#define BUS_SPACE_MAP_PREFETCHABLE	0x0004
/*
 * For simpler hadware, many x68k devices are mapped with shifted address
 * i.e. only on even or odd addresses.
 */
#define BUS_SPACE_MAP_SHIFTED_MASK	0x1001
#define BUS_SPACE_MAP_SHIFTED_ODD	0x1001
#define BUS_SPACE_MAP_SHIFTED_EVEN	0x1000
#define BUS_SPACE_MAP_SHIFTED		BUS_SPACE_MAP_SHIFTED_ODD

#define bus_space_alloc(t,rs,re,s,a,b,f,r,h) \
		((*((t)->x68k_bus_space_alloc)) ((t),(rs),(re),(s),(a),(b),(f),(r),(h)))
#define bus_space_free(t,h,s) \
		((*((t)->x68k_bus_space_free)) ((t),(h),(s)))

/*
 * Note: the 680x0 does not currently require barriers, but we must
 * provide the flags to MI code.
 */
#define	bus_space_barrier(t, h, o, l, f)	\
	((void)((void)(t), (void)(h), (void)(o), (void)(l), (void)(f)))
#define	BUS_SPACE_BARRIER_READ	0x01		/* force read barrier */
#define	BUS_SPACE_BARRIER_WRITE	0x02		/* force write barrier */

#define bus_space_read_1(t,h,o) _bus_space_read_1(t,h,o)
#define bus_space_read_2(t,h,o) _bus_space_read_2(t,h,o)
#define bus_space_read_4(t,h,o) _bus_space_read_4(t,h,o)

#define bus_space_read_multi_1(t,h,o,p,c) _bus_space_read_multi_1(t,h,o,p,c)
#define bus_space_read_multi_2(t,h,o,p,c) _bus_space_read_multi_2(t,h,o,p,c)
#define bus_space_read_multi_4(t,h,o,p,c) _bus_space_read_multi_4(t,h,o,p,c)

#define bus_space_read_region_1(t,h,o,p,c) _bus_space_read_region_1(t,h,o,p,c)
#define bus_space_read_region_2(t,h,o,p,c) _bus_space_read_region_2(t,h,o,p,c)
#define bus_space_read_region_4(t,h,o,p,c) _bus_space_read_region_4(t,h,o,p,c)

#define bus_space_write_1(t,h,o,v) _bus_space_write_1(t,h,o,v)
#define bus_space_write_2(t,h,o,v) _bus_space_write_2(t,h,o,v)
#define bus_space_write_4(t,h,o,v) _bus_space_write_4(t,h,o,v)

#define bus_space_write_multi_1(t,h,o,p,c) _bus_space_write_multi_1(t,h,o,p,c)
#define bus_space_write_multi_2(t,h,o,p,c) _bus_space_write_multi_2(t,h,o,p,c)
#define bus_space_write_multi_4(t,h,o,p,c) _bus_space_write_multi_4(t,h,o,p,c)

#define bus_space_write_region_1(t,h,o,p,c) \
		_bus_space_write_region_1(t,h,o,p,c)
#define bus_space_write_region_2(t,h,o,p,c) \
		_bus_space_write_region_2(t,h,o,p,c)
#define bus_space_write_region_4(t,h,o,p,c) \
		_bus_space_write_region_4(t,h,o,p,c)

#define bus_space_set_region_1(t,h,o,v,c) _bus_space_set_region_1(t,h,o,v,c)
#define bus_space_set_region_2(t,h,o,v,c) _bus_space_set_region_2(t,h,o,v,c)
#define bus_space_set_region_4(t,h,o,v,c) _bus_space_set_region_4(t,h,o,v,c)

#define bus_space_copy_region_1(t,sh,so,dh,do,c) \
		_bus_space_copy_region_1(t,sh,so,dh,do,c)
#define bus_space_copy_region_2(t,sh,so,dh,do,c) \
		_bus_space_copy_region_2(t,sh,so,dh,do,c)
#define bus_space_copy_region_4(t,sh,so,dh,do,c) \
		_bus_space_copy_region_4(t,sh,so,dh,do,c)

static inline u_int8_t _bus_space_read_1
	__P((bus_space_tag_t, bus_space_handle_t bsh, bus_size_t offset));
static inline u_int16_t _bus_space_read_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));
static inline u_int32_t _bus_space_read_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t));

static inline void _bus_space_read_multi_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int8_t *, bus_size_t));
static inline void _bus_space_read_multi_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int16_t *, bus_size_t));
static inline void _bus_space_read_multi_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int32_t *, bus_size_t));

static inline void _bus_space_read_region_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int8_t *, bus_size_t));
static inline void _bus_space_read_region_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int16_t *, bus_size_t));
static inline void _bus_space_read_region_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int32_t *, bus_size_t));

static inline void _bus_space_write_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int8_t));
static inline void _bus_space_write_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int16_t));
static inline void _bus_space_write_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t, u_int32_t));

static inline void _bus_space_write_multi_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int8_t *, bus_size_t));
static inline void _bus_space_write_multi_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int16_t *, bus_size_t));
static inline void _bus_space_write_multi_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int32_t *, bus_size_t));

static inline void _bus_space_write_region_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int8_t *, bus_size_t));
static inline void _bus_space_write_region_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int16_t *, bus_size_t));
static inline void _bus_space_write_region_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int32_t *, bus_size_t));

static inline void _bus_space_set_region_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int8_t, bus_size_t));
static inline void _bus_space_set_region_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int16_t, bus_size_t));
static inline void _bus_space_set_region_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     u_int32_t, bus_size_t));

static inline void _bus_space_copy_region_1
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     bus_space_handle_t, bus_size_t, bus_size_t));
static inline void _bus_space_copy_region_2
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     bus_space_handle_t, bus_size_t, bus_size_t));
static inline void _bus_space_copy_region_4
	__P((bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     bus_space_handle_t, bus_size_t, bus_size_t));


#define __X68K_BUS_ADDR(tag, handle, offset)	\
	(((long)(handle) < 0 ? (offset) * 2 : (offset))	\
		+ ((handle) & 0x7fffffff))

static inline u_int8_t
_bus_space_read_1(t, bsh, offset)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*((volatile u_int8_t *) __X68K_BUS_ADDR(t, bsh, offset)));
}

static inline u_int16_t
_bus_space_read_2(t, bsh, offset)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*((volatile u_int16_t *) __X68K_BUS_ADDR(t, bsh, offset)));
}

static inline u_int32_t
_bus_space_read_4(t, bsh, offset)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
{
	return (*((volatile u_int32_t *) __X68K_BUS_ADDR(t, bsh, offset)));
}

static inline void
_bus_space_read_multi_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int8_t *regadr = (u_int8_t *) __X68K_BUS_ADDR(t, bsh, offset);
	for (; count; count--) {
		__asm("| avoid optim. _bus_space_read_multi_1" : : : "memory");
		*datap++ = *regadr;
	}
#else
	while (count-- > 0) {
		*datap++ = *(volatile u_int8_t *)
				__X68K_BUS_ADDR(t, bsh, offset);
	}
#endif
}

static inline void
_bus_space_read_multi_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int16_t *regadr = (u_int16_t *) __X68K_BUS_ADDR(t, bsh, offset);
	for (; count; count--) {
		__asm("| avoid optim. _bus_space_read_multi_2" : : : "memory");
		*datap++ = *regadr;
	}
#else
	while (count-- > 0) {
		*datap++ = *(volatile u_int16_t *)
				__X68K_BUS_ADDR(t, bsh, offset);
	}
#endif
}

static inline void
_bus_space_read_multi_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int32_t *regadr = (u_int32_t *) __X68K_BUS_ADDR(t, bsh, offset);
	for (; count; count--) {
		__asm("| avoid optim. _bus_space_read_multi_4" : : : "memory");
		*datap++ = *regadr;
	}
#else
	while (count-- > 0) {
		*datap++ = *(volatile u_int32_t *)
				__X68K_BUS_ADDR(t, bsh, offset);
	}
#endif
}

static inline void
_bus_space_read_region_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int8_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_read_region_1" : : : "memory");
		*datap++ = *addr++;
	}
#else
	volatile u_int8_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*datap++ = *addr++;
	}
#endif
}

static inline void
_bus_space_read_region_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int16_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_read_region_2" : : : "memory");
		*datap++ = *addr++;
	}
#else
	volatile u_int16_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*datap++ = *addr++;
	}
#endif
}

static inline void
_bus_space_read_region_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int32_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_read_region_4" : : : "memory");
		*datap++ = *addr++;
	}
#else
	volatile u_int32_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*datap++ = *addr++;
	}
#endif
}

static inline void
_bus_space_write_1(t, bsh, offset, value)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t value;
{
	*(volatile u_int8_t *) __X68K_BUS_ADDR(t, bsh, offset) = value;
}

static inline void
_bus_space_write_2(t, bsh, offset, value)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
{
	*(volatile u_int16_t *) __X68K_BUS_ADDR(t, bsh, offset) = value;
}

static inline void
_bus_space_write_4(t, bsh, offset, value)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
{
	*(volatile u_int32_t *) __X68K_BUS_ADDR(t, bsh, offset) = value;
}

static inline void
_bus_space_write_multi_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int8_t *regadr = (u_int8_t *) __X68K_BUS_ADDR(t, bsh, offset);
	for (; count; count--) {
		__asm("| avoid optim. _bus_space_write_multi_1" : : : "memory");
		*regadr = *datap++;
	}
#else
	while (count-- > 0) {
		*(volatile u_int8_t *) __X68K_BUS_ADDR(t, bsh, offset)
		    = *datap++;
	}
#endif
}

static inline void
_bus_space_write_multi_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int16_t *regadr = (u_int16_t *) __X68K_BUS_ADDR(t, bsh, offset);
	for (; count; count--) {
		__asm("| avoid optim. _bus_space_write_multi_2" : : : "memory");
		*regadr = *datap++;
	}
#else
	while (count-- > 0) {
		*(volatile u_int16_t *) __X68K_BUS_ADDR(t, bsh, offset)
		    = *datap++;
	}
#endif
}

static inline void
_bus_space_write_multi_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int32_t *regadr = (u_int32_t *) __X68K_BUS_ADDR(t, bsh, offset);
	for (; count; count--) {
		__asm("| avoid optim. _bus_space_write_multi_4" : : : "memory");
		*regadr = *datap++;
	}
#else
	while (count-- > 0) {
		*(volatile u_int32_t *) __X68K_BUS_ADDR(t, bsh, offset)
		    = *datap++;
	}
#endif
}

static inline void
_bus_space_write_region_1(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int8_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_write_region_1": : : "memory");
		*addr++ = *datap++;
	}
#else
	volatile u_int8_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*addr++ = *datap++;
	}
#endif
}

static inline void
_bus_space_write_region_2(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int16_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_write_region_2": : : "memory");
		*addr++ = *datap++;
	}
#else
	volatile u_int16_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*addr++ = *datap++;
	}
#endif
}

static inline void
_bus_space_write_region_4(t, bsh, offset, datap, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t *datap;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int32_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_write_region_4": : : "memory");
		*addr++ = *datap++;
	}
#else
	volatile u_int32_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*addr++ = *datap++;
	}
#endif
}

static inline void
_bus_space_set_region_1(t, bsh, offset, value, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int8_t value;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int8_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_set_region_1" : : : "memory");
		*addr++ = value;
	}
#else
	volatile u_int8_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*addr++ = value;
	}
#endif
}

static inline void
_bus_space_set_region_2(t, bsh, offset, value, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int16_t value;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int16_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_set_region_2" : : : "memory");
		*addr++ = value;
	}
#else
	volatile u_int16_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*addr++ = value;
	}
#endif
}

static inline void
_bus_space_set_region_4(t, bsh, offset, value, count)
	bus_space_tag_t t;
	bus_space_handle_t bsh;
	bus_size_t offset;
	u_int32_t value;
	bus_size_t count;
{
#if X68K_BUS_PERFORMANCE_HACK
	u_int32_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--) {
		__asm("| avoid optim. _bus_space_set_region_4" : : : "memory");
		*addr++ = value;
	}
#else
	volatile u_int32_t *addr = (void *) __X68K_BUS_ADDR(t, bsh, offset);

	while (count-- > 0) {
		*addr++ = value;
	}
#endif
}

static inline void
_bus_space_copy_region_1(t, sbsh, soffset, dbsh, doffset, count)
	bus_space_tag_t t;
	bus_space_handle_t sbsh;
	bus_size_t soffset;
	bus_space_handle_t dbsh;
	bus_size_t doffset;
	bus_size_t count;
{
	volatile u_int8_t *saddr = (void *) (sbsh + soffset);
	volatile u_int8_t *daddr = (void *) (dbsh + doffset);

	if ((u_int32_t) saddr >= (u_int32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

static inline void
_bus_space_copy_region_2(t, sbsh, soffset, dbsh, doffset, count)
	bus_space_tag_t t;
	bus_space_handle_t sbsh;
	bus_size_t soffset;
	bus_space_handle_t dbsh;
	bus_size_t doffset;
	bus_size_t count;
{
	volatile u_int16_t *saddr = (void *) (sbsh + soffset);
	volatile u_int16_t *daddr = (void *) (dbsh + doffset);

	if ((u_int32_t) saddr >= (u_int32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

static inline void
_bus_space_copy_region_4(t, sbsh, soffset, dbsh, doffset, count)
	bus_space_tag_t t;
	bus_space_handle_t sbsh;
	bus_size_t soffset;
	bus_space_handle_t dbsh;
	bus_size_t doffset;
	bus_size_t count;
{
	volatile u_int32_t *saddr = (void *) (sbsh + soffset);
	volatile u_int32_t *daddr = (void *) (dbsh + doffset);

	if ((u_int32_t) saddr >= (u_int32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

#define BUS_SPACE_ALIGNED_POINTER(p, t) ALIGNED_POINTER(p, t)

/*
 * DMA segment
 */
struct x68k_bus_dma_segment {
	bus_addr_t	ds_addr;
	bus_size_t	ds_len;
};
typedef struct x68k_bus_dma_segment	bus_dma_segment_t;

/*
 * DMA descriptor
 */
/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

typedef struct x68k_bus_dma		*bus_dma_tag_t;
typedef struct x68k_bus_dmamap		*bus_dmamap_t;
struct x68k_bus_dma {
	/*
	 * The `bounce threshold' is checked while we are loading
	 * the DMA map.  If the physical address of the segment
	 * exceeds the threshold, an error will be returned.  The
	 * caller can then take whatever action is necessary to
	 * bounce the transfer.  If this value is 0, it will be
	 * ignored.
	 */
	bus_addr_t _bounce_thresh;

	/*
	 * DMA mapping methods.
	 */
	int	(*x68k_dmamap_create) __P((bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *));
	void	(*x68k_dmamap_destroy) __P((bus_dma_tag_t, bus_dmamap_t));
	int	(*x68k_dmamap_load) __P((bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int));
	int	(*x68k_dmamap_load_mbuf) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int));
	int	(*x68k_dmamap_load_uio) __P((bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int));
	int	(*x68k_dmamap_load_raw) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int));
	void	(*x68k_dmamap_unload) __P((bus_dma_tag_t, bus_dmamap_t));
	void	(*x68k_dmamap_sync) __P((bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int));

	/*
	 * DMA memory utility functions.
	 */
	int	(*x68k_dmamem_alloc) __P((bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int));
	void	(*x68k_dmamem_free) __P((bus_dma_tag_t,
		    bus_dma_segment_t *, int));
	int	(*x68k_dmamem_map) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, caddr_t *, int));
	void	(*x68k_dmamem_unmap) __P((bus_dma_tag_t, caddr_t, size_t));
	paddr_t	(*x68k_dmamem_mmap) __P((bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int));
};

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct x68k_bus_dmamap {
	/*
	 * PRIVATE MEMBERS: not for use my machine-independent code.
	 */
	bus_size_t	x68k_dm_size;	/* largest DMA transfer mappable */
	int		x68k_dm_segcnt;	/* number of segs this map can map */
	bus_size_t	x68k_dm_maxsegsz; /* largest possible segment */
	bus_size_t	x68k_dm_boundary; /* don't cross this */
	bus_addr_t	x68k_dm_bounce_thresh; /* bounce threshold */
	int		x68k_dm_flags;	/* misc. flags */

	void		*x68k_dm_cookie; /* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

int	x68k_bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
void	x68k_bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
int	x68k_bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	x68k_bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	x68k_bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	x68k_bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	x68k_bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	x68k_bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	x68k_bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void	x68k_bus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
int	x68k_bus_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags));
void	x68k_bus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
	    size_t size));
paddr_t	x68k_bus_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags));

int	x68k_bus_dmamap_load_buffer __P((bus_dmamap_t, void *,
	    bus_size_t buflen, struct proc *, int, paddr_t *, int *, int));
int	x68k_bus_dmamem_alloc_range __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high));

#define	bus_dmamap_create(t,s,n,m,b,f,p) \
	((*((t)->x68k_dmamap_create)) ((t),(s),(n),(m),(b),(f),(p)))
#define	bus_dmamap_destroy(t,p) \
	((*((t)->x68k_dmamap_destroy)) ((t),(p)))
#define	bus_dmamap_load(t,m,b,s,p,f) \
	((*((t)->x68k_dmamap_load)) ((t),(m),(b),(s),(p),(f)))
#define	bus_dmamap_load_mbuf(t,m,b,f) \
	((*((t)->x68k_dmamap_load_mbuf)) ((t),(m),(b),(f)))
#define	bus_dmamap_load_uio(t,m,u,f) \
	((*((t)->x68k_dmamap_load_uio)) ((t),(m),(u),(f)))
#define	bus_dmamap_load_raw(t,m,sg,n,s,f) \
	((*((t)->x68k_dmamap_load_raw)) ((t),(m),(sg),(n),(s),(f)))
#define	bus_dmamap_unload(t,p) \
	((*((t)->x68k_dmamap_unload)) ((t),(p)))
#define	bus_dmamap_sync(t,p,o,l,ops) \
	((*((t)->x68k_dmamap_sync)) ((t),(p),(o),(l),(ops)))

#define	bus_dmamem_alloc(t,s,a,b,sg,n,r,f) \
	((*((t)->x68k_dmamem_alloc)) ((t),(s),(a),(b),(sg),(n),(r),(f)))
#define	bus_dmamem_free(t,sg,n) \
	((*((t)->x68k_dmamem_free)) ((t),(sg),(n)))
#define	bus_dmamem_map(t,sg,n,s,k,f) \
	((*((t)->x68k_dmamem_map)) ((t),(sg),(n),(s),(k),(f)))
#define	bus_dmamem_unmap(t,k,s) \
	((*((t)->x68k_dmamem_unmap)) ((t),(k),(s)))
#define	bus_dmamem_mmap(t,sg,n,o,p,f) \
	((*((t)->x68k_dmamem_mmap)) ((t),(sg),(n),(o),(p),(f)))

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
#define	BUS_DMA_NOCACHE		0x400	/* hint: map non-cached memory */

/*
 * Operations performed by bus_dmamap_sync().
 */
#define	BUS_DMASYNC_PREREAD	0x01	/* pre-read synchronization */
#define	BUS_DMASYNC_POSTREAD	0x02	/* post-read synchronization */
#define	BUS_DMASYNC_PREWRITE	0x04	/* pre-write synchronization */
#define	BUS_DMASYNC_POSTWRITE	0x08	/* post-write synchronization */

#endif /* _X68K_BUS_H_ */
