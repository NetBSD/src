/*	$NetBSD: bus.h,v 1.25 2012/10/13 06:44:24 tsutsui Exp $	*/

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

	int	(*x68k_bus_space_map)(
				bus_space_tag_t,
				bus_addr_t,
				bus_size_t,
				int,			/* flags */
				bus_space_handle_t *);
	void	(*x68k_bus_space_unmap)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t);
	int	(*x68k_bus_space_subregion)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/* offset */
				bus_size_t,		/* size */
				bus_space_handle_t *);

	int	(*x68k_bus_space_alloc)(
				bus_space_tag_t,
				bus_addr_t,		/* reg_start */
				bus_addr_t,		/* reg_end */
				bus_size_t,
				bus_size_t,		/* alignment */
				bus_size_t,		/* boundary */
				int,			/* flags */
				bus_addr_t *,
				bus_space_handle_t *);
	void	(*x68k_bus_space_free)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t);

#if 0
	void	(*x68k_bus_space_barrier)(
				bus_space_tag_t,
				bus_space_handle_t,
				bus_size_t,		/* offset */
				bus_size_t,		/* length */
				int);			/* flags */
#endif

	device_t x68k_bus_device;
};

int x68k_bus_space_alloc(bus_space_tag_t, bus_addr_t, bus_addr_t, bus_size_t,
    bus_size_t, bus_size_t, int, bus_addr_t *, bus_space_handle_t *);
void x68k_bus_space_free(bus_space_tag_t, bus_space_handle_t, bus_size_t);

/*
 * bus_space(9) interface
 */

#define bus_space_map(t, a, s, f, h) \
		((*((t)->x68k_bus_space_map)) ((t), (a), (s), (f), (h)))
#define bus_space_unmap(t, h, s) \
		((*((t)->x68k_bus_space_unmap)) ((t), (h), (s)))
#define bus_space_subregion(t, h, o, s, p) \
		((*((t)->x68k_bus_space_subregion)) ((t), (h), (o), (s), (p)))
#define BUS_SPACE_MAP_CACHEABLE		0x0001
#define BUS_SPACE_MAP_LINEAR		0x0002
#define BUS_SPACE_MAP_PREFETCHABLE	0x0004
/*
 * For simpler hardware, many x68k devices are mapped with shifted address
 * i.e. only on even or odd addresses.
 */
#define BUS_SPACE_MAP_SHIFTED_MASK	0x1001
#define BUS_SPACE_MAP_SHIFTED_ODD	0x1001
#define BUS_SPACE_MAP_SHIFTED_EVEN	0x1000
#define BUS_SPACE_MAP_SHIFTED		BUS_SPACE_MAP_SHIFTED_ODD

#define bus_space_alloc(t, rs, re, s, a, b, f, r, h)			\
		((*((t)->x68k_bus_space_alloc)) ((t),			\
		    (rs), (re), (s), (a), (b), (f), (r), (h)))
#define bus_space_free(t, h, s) \
		((*((t)->x68k_bus_space_free)) ((t), (h), (s)))

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

static __inline uint8_t _bus_space_read_1
	(bus_space_tag_t, bus_space_handle_t bsh, bus_size_t offset);
static __inline uint16_t _bus_space_read_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t);
static __inline uint32_t _bus_space_read_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t);

static __inline void _bus_space_read_multi_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint8_t *, bus_size_t);
static __inline void _bus_space_read_multi_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint16_t *, bus_size_t);
static __inline void _bus_space_read_multi_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint32_t *, bus_size_t);

static __inline void _bus_space_read_region_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint8_t *, bus_size_t);
static __inline void _bus_space_read_region_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint16_t *, bus_size_t);
static __inline void _bus_space_read_region_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint32_t *, bus_size_t);

static __inline void _bus_space_write_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint8_t);
static __inline void _bus_space_write_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint16_t);
static __inline void _bus_space_write_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t, uint32_t);

static __inline void _bus_space_write_multi_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     const uint8_t *, bus_size_t);
static __inline void _bus_space_write_multi_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     const uint16_t *, bus_size_t);
static __inline void _bus_space_write_multi_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     const uint32_t *, bus_size_t);

static __inline void _bus_space_write_region_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     const uint8_t *, bus_size_t);
static __inline void _bus_space_write_region_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     const uint16_t *, bus_size_t);
static __inline void _bus_space_write_region_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     const uint32_t *, bus_size_t);

static __inline void _bus_space_set_region_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint8_t, bus_size_t);
static __inline void _bus_space_set_region_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint16_t, bus_size_t);
static __inline void _bus_space_set_region_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     uint32_t, bus_size_t);

static __inline void _bus_space_copy_region_1
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     bus_space_handle_t, bus_size_t, bus_size_t);
static __inline void _bus_space_copy_region_2
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     bus_space_handle_t, bus_size_t, bus_size_t);
static __inline void _bus_space_copy_region_4
	(bus_space_tag_t, bus_space_handle_t, bus_size_t,
	     bus_space_handle_t, bus_size_t, bus_size_t);


#define __X68K_BUS_ADDR(tag, handle, offset)	\
	(((long)(handle) < 0 ? (offset) * 2 : (offset))	\
		+ ((handle) & 0x7fffffff))

static __inline uint8_t
_bus_space_read_1(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{

	return *((volatile uint8_t *) __X68K_BUS_ADDR(t, bsh, offset));
}

static __inline uint16_t
_bus_space_read_2(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{

	return *((volatile uint16_t *) __X68K_BUS_ADDR(t, bsh, offset));
}

static __inline uint32_t
_bus_space_read_4(bus_space_tag_t t, bus_space_handle_t bsh, bus_size_t offset)
{

	return *((volatile uint32_t *) __X68K_BUS_ADDR(t, bsh, offset));
}

static __inline void
_bus_space_read_multi_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *datap, bus_size_t count)
{
	volatile uint8_t *regadr;

	regadr = (volatile uint8_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*datap++ = *regadr;
}

static __inline void
_bus_space_read_multi_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *datap, bus_size_t count)
{
	volatile uint16_t *regadr;

	regadr = (volatile uint16_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*datap++ = *regadr;
}

static __inline void
_bus_space_read_multi_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *datap, bus_size_t count)
{
	volatile uint32_t *regadr;

	regadr = (volatile uint32_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*datap++ = *regadr;
}

static __inline void
_bus_space_read_region_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t *datap, bus_size_t count)
{
	volatile uint8_t *addr;

	addr = (volatile uint8_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*datap++ = *addr++;
}

static __inline void
_bus_space_read_region_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t *datap, bus_size_t count)
{
	volatile uint16_t *addr;

	addr = (volatile uint16_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*datap++ = *addr++;
}

static __inline void
_bus_space_read_region_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t *datap, bus_size_t count)
{
	volatile uint32_t *addr;

	addr = (volatile uint32_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*datap++ = *addr++;
}

static __inline void
_bus_space_write_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value)
{

	*(volatile uint8_t *) __X68K_BUS_ADDR(t, bsh, offset) = value;
}

static __inline void
_bus_space_write_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value)
{

	*(volatile uint16_t *) __X68K_BUS_ADDR(t, bsh, offset) = value;
}

static __inline void
_bus_space_write_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value)
{

	*(volatile uint32_t *) __X68K_BUS_ADDR(t, bsh, offset) = value;
}

static __inline void
_bus_space_write_multi_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *datap, bus_size_t count)
{
	volatile uint8_t *regadr;

	regadr = (volatile uint8_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*regadr = *datap++;
}

static __inline void
_bus_space_write_multi_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *datap, bus_size_t count)
{
	volatile uint16_t *regadr;

	regadr = (volatile uint16_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*regadr = *datap++;
}

static __inline void
_bus_space_write_multi_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *datap, bus_size_t count)
{
	volatile uint32_t *regadr;

	regadr = (volatile uint32_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*regadr = *datap++;
}

static __inline void
_bus_space_write_region_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const uint8_t *datap, bus_size_t count)
{
	volatile uint8_t *addr;

	addr = (volatile uint8_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*addr++ = *datap++;
}

static __inline void
_bus_space_write_region_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const uint16_t *datap, bus_size_t count)
{
	volatile uint16_t *addr;

	addr = (volatile uint16_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*addr++ = *datap++;
}

static __inline void
_bus_space_write_region_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, const uint32_t *datap, bus_size_t count)
{
	volatile uint32_t *addr;

	addr = (volatile uint32_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*addr++ = *datap++;
}

static __inline void
_bus_space_set_region_1(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint8_t value, bus_size_t count)
{
	volatile uint8_t *addr;

	addr = (volatile uint8_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*addr++ = value;
}

static __inline void
_bus_space_set_region_2(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint16_t value, bus_size_t count)
{
	volatile uint16_t *addr;

	addr = (volatile uint16_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*addr++ = value;
}

static __inline void
_bus_space_set_region_4(bus_space_tag_t t, bus_space_handle_t bsh,
    bus_size_t offset, uint32_t value, bus_size_t count)
{
	volatile uint32_t *addr;

	addr = (volatile uint32_t *)__X68K_BUS_ADDR(t, bsh, offset);

	for (; count; count--)
		*addr++ = value;
}

static __inline void
_bus_space_copy_region_1(bus_space_tag_t t,
    bus_space_handle_t sbsh, bus_size_t soffset,
    bus_space_handle_t dbsh, bus_size_t doffset,
    bus_size_t count)
{
	volatile uint8_t *saddr = (void *) (sbsh + soffset);
	volatile uint8_t *daddr = (void *) (dbsh + doffset);

	if ((uint32_t) saddr >= (uint32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

static __inline void
_bus_space_copy_region_2(bus_space_tag_t t,
    bus_space_handle_t sbsh, bus_size_t soffset,
    bus_space_handle_t dbsh, bus_size_t doffset,
    bus_size_t count)
{
	volatile uint16_t *saddr = (void *) (sbsh + soffset);
	volatile uint16_t *daddr = (void *) (dbsh + doffset);

	if ((uint32_t) saddr >= (uint32_t) daddr)
		while (count-- > 0)
			*daddr++ = *saddr++;
	else {
		saddr += count;
		daddr += count;
		while (count-- > 0)
			*--daddr = *--saddr;
	}
}

static __inline void
_bus_space_copy_region_4(bus_space_tag_t t,
    bus_space_handle_t sbsh, bus_size_t soffset,
    bus_space_handle_t dbsh, bus_size_t doffset,
    bus_size_t count)
{
	volatile uint32_t *saddr = (void *) (sbsh + soffset);
	volatile uint32_t *daddr = (void *) (dbsh + doffset);

	if ((uint32_t) saddr >= (uint32_t) daddr)
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

#define BUS_DMA_TAG_VALID(t)    ((t) != (bus_dma_tag_t)0)

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
	int	(*x68k_dmamap_create)(bus_dma_tag_t, bus_size_t, int,
		    bus_size_t, bus_size_t, int, bus_dmamap_t *);
	void	(*x68k_dmamap_destroy)(bus_dma_tag_t, bus_dmamap_t);
	int	(*x68k_dmamap_load)(bus_dma_tag_t, bus_dmamap_t, void *,
		    bus_size_t, struct proc *, int);
	int	(*x68k_dmamap_load_mbuf)(bus_dma_tag_t, bus_dmamap_t,
		    struct mbuf *, int);
	int	(*x68k_dmamap_load_uio)(bus_dma_tag_t, bus_dmamap_t,
		    struct uio *, int);
	int	(*x68k_dmamap_load_raw)(bus_dma_tag_t, bus_dmamap_t,
		    bus_dma_segment_t *, int, bus_size_t, int);
	void	(*x68k_dmamap_unload)(bus_dma_tag_t, bus_dmamap_t);
	void	(*x68k_dmamap_sync)(bus_dma_tag_t, bus_dmamap_t,
		    bus_addr_t, bus_size_t, int);

	/*
	 * DMA memory utility functions.
	 */
	int	(*x68k_dmamem_alloc)(bus_dma_tag_t, bus_size_t, bus_size_t,
		    bus_size_t, bus_dma_segment_t *, int, int *, int);
	void	(*x68k_dmamem_free)(bus_dma_tag_t,
		    bus_dma_segment_t *, int);
	int	(*x68k_dmamem_map)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, size_t, void **, int);
	void	(*x68k_dmamem_unmap)(bus_dma_tag_t, void *, size_t);
	paddr_t	(*x68k_dmamem_mmap)(bus_dma_tag_t, bus_dma_segment_t *,
		    int, off_t, int, int);
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
	bus_size_t	x68k_dm_maxmaxsegsz; /* fixed largest possible segment*/
	bus_size_t	x68k_dm_boundary; /* don't cross this */
	bus_addr_t	x68k_dm_bounce_thresh; /* bounce threshold */
	int		x68k_dm_flags;	/* misc. flags */

	void		*x68k_dm_cookie; /* cookie for bus-specific functions */

	/*
	 * PUBLIC MEMBERS: these are used by machine-independent code.
	 */
	bus_size_t	dm_maxsegsz;	/* largest possible segment */
	bus_size_t	dm_mapsize;	/* size of the mapping */
	int		dm_nsegs;	/* # valid segments in mapping */
	bus_dma_segment_t dm_segs[1];	/* segments; variable length */
};

int	x68k_bus_dmamap_create(bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *);
void	x68k_bus_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	x68k_bus_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	x68k_bus_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	x68k_bus_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	x68k_bus_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	x68k_bus_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	x68k_bus_dmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int);

int	x68k_bus_dmamem_alloc(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags);
void	x68k_bus_dmamem_free(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs);
int	x68k_bus_dmamem_map(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, void **kvap, int flags);
void	x68k_bus_dmamem_unmap(bus_dma_tag_t tag, void *kva,
	    size_t size);
paddr_t	x68k_bus_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

int	x68k_bus_dmamap_load_buffer(bus_dmamap_t, void *,
	    bus_size_t buflen, struct proc *, int, paddr_t *, int *, int);
int	x68k_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    paddr_t low, paddr_t high);

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

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

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
