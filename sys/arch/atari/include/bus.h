/*	$NetBSD: bus.h,v 1.11 1998/03/10 11:43:10 leo Exp $	*/

/*
 * Copyright (c) 1996 Leo Weppelman.  All rights reserved.
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
 *      This product includes software developed by Leo Weppelman for the
 *	NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
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

#ifndef _ATARI_BUS_H_
#define _ATARI_BUS_H_

/*
 * I/O addresses (in bus space)
 */
typedef u_long bus_io_addr_t;
typedef u_long bus_io_size_t;

/*
 * Memory addresses (in bus space)
 */
typedef u_long	bus_addr_t;
typedef u_long	bus_size_t;

/*
 * Access methods for bus resources and address space.
 */
typedef u_long	bus_space_tag_t;
typedef caddr_t	bus_space_handle_t;

#define	BUS_SPACE_MAP_CACHEABLE	0x01
#define	BUS_SPACE_MAP_LINEAR	0x02

int	bus_space_map __P((bus_space_tag_t, bus_addr_t, bus_size_t,
				int, bus_space_handle_t *));
void	bus_space_unmap __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t));
int	bus_space_subregion __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, bus_size_t, bus_space_handle_t *));
void	bus_space_read_multi_1 __P((bus_space_tag_t, bus_space_handle_t,
				int, caddr_t, int));
void	bus_space_read_multi_2 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int16_t *, int));
void	bus_space_read_multi_4 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int32_t *, int));
void	bus_space_read_multi_8 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int64_t *, int));
void	bus_space_write_multi_1 __P((bus_space_tag_t, bus_space_handle_t,
				int, caddr_t, int));
void	bus_space_write_multi_2 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int16_t *, int));
void	bus_space_write_multi_4 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int32_t *, int));
void	bus_space_write_multi_8 __P((bus_space_tag_t, bus_space_handle_t,
				int, u_int64_t *, int));
void	bus_space_read_region_1 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int8_t *, size_t));
void	bus_space_read_region_2 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int16_t *, size_t));
void	bus_space_read_region_4 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int32_t *, size_t));
void	bus_space_read_region_8 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int64_t *, size_t));
void	bus_space_write_region_1 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int8_t *, size_t));
void	bus_space_write_region_2 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int16_t *, size_t));
void	bus_space_write_region_4 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int32_t *, size_t));
void	bus_space_write_region_8 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, const u_int64_t *, size_t));
void	bus_space_set_multi_1 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int8_t, size_t));
void	bus_space_set_multi_2 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int16_t, size_t));
void	bus_space_set_multi_4 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int32_t, size_t));
void	bus_space_set_multi_8 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int64_t, size_t));
void	bus_space_set_region_1 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int8_t, size_t));
void	bus_space_set_region_2 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int16_t, size_t));
void	bus_space_set_region_4 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int32_t, size_t));
void	bus_space_set_region_8 __P((bus_space_tag_t, bus_space_handle_t,
				bus_size_t, u_int64_t, size_t));
/*
 * Check accesibility of the location for various sized bus accesses
 */
#define bus_space_peek_1(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 1))
#define bus_space_peek_2(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 2))
#define bus_space_peek_4(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 4))
#define bus_space_peek_8(t, h, o)					\
	(!badbaddr((caddr_t)((h) + (o)), 8))

#define	bus_space_read_1(t, h, o)					\
    ((void) t, (*(volatile u_int8_t *)((h) + (o))))
#define	bus_space_read_2(t, h, o)					\
    ((void) t, (*(volatile u_int16_t *)((h) + (o))))
#define	bus_space_read_4(t, h, o)					\
    ((void) t, (*(volatile u_int32_t *)((h) + (o))))
#define	bus_space_read_8(t, h, o)					\
    ((void) t, (*(volatile u_int64_t *)((h) + (o))))
#define	bus_space_write_1(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int8_t *)((h) + (o)) = (v))))
#define	bus_space_write_2(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int16_t *)((h) + (o)) = (v))))
#define	bus_space_write_4(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int32_t *)((h) + (o)) = (v))))
#define	bus_space_write_8(t, h, o, v)					\
    ((void) t, ((void)(*(volatile u_int64_t *)((h) + (o)) = (v))))

extern __inline__ void
bus_space_read_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	caddr_t			a;
{
	for (; c; a++, c--)
		*(u_int8_t *)a = bus_space_read_1(t, h, o);
}

extern __inline__ void
bus_space_read_multi_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int16_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_2(t, h, o);
}

extern __inline__ void
bus_space_read_multi_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int32_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_4(t, h, o);
}

extern __inline__ void
bus_space_read_multi_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int64_t		*a;
{
	for (; c; a++, c--)
		*a = bus_space_read_8(t, h, o);
}

extern __inline__ void
bus_space_write_multi_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	caddr_t			a;
{
	for (; c; a++, c--)
		bus_space_write_1(t, h, o, *(u_int8_t *)a);
}

extern __inline__ void
bus_space_write_multi_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int16_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_2(t, h, o, *a);
}

extern __inline__ void
bus_space_write_multi_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int32_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_4(t, h, o, *a);
}

extern __inline__ void
bus_space_write_multi_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	int			o, c;
	u_int64_t		*a;
{
	for (; c; a++, c--)
		bus_space_write_8(t, h, o, *a);
}

/*
 *	void bus_space_read_region_N __P((bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		u_intN_t *addr, size_t count));
 *
 * Read `count' 1, 2, 4, or 8 byte quantities from bus space
 * described by tag/handle and starting at `offset' and copy into
 * buffer provided.
 */
extern __inline__ void
bus_space_read_region_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int8_t		*a;
	size_t			c;
{
	for (; c; a++, o++, c--)
		*a = bus_space_read_1(t, h, o);
}

extern __inline__ void
bus_space_read_region_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int16_t		*a;
	size_t			c;
{
	for (; c; a++, o += 2, c--)
		*a = bus_space_read_2(t, h, o);
}

extern __inline__ void
bus_space_read_region_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int32_t		*a;
	size_t			c;
{
	for (; c; a++, o += 4, c--)
		*a = bus_space_read_4(t, h, o);
}

extern __inline__ void
bus_space_read_region_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int64_t		*a;
	size_t			c;
{
	for (; c; a++, o += 8, c--)
		*a = bus_space_read_8(t, h, o);
}

/*
 *	void bus_space_write_region_N __P((bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset,
 *		u_intN_t *addr, size_t count));
 *
 * Copy `count' 1, 2, 4, or 8 byte quantities from the buffer provided
 * into the bus space described by tag/handle and starting at `offset'.
 */
extern __inline__ void
bus_space_write_region_1(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	const u_int8_t		*a;
	size_t			c;
{
	for (; c; a++, o++, c--)
		bus_space_write_1(t, h, o, *a);
}

extern __inline__ void
bus_space_write_region_2(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	const u_int16_t		*a;
	size_t			c;
{
	for (; c; a++, o += 2, c--)
		bus_space_write_2(t, h, o, *a);
}

extern __inline__ void
bus_space_write_region_4(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	const u_int32_t		*a;
	size_t			c;
{
	for (; c; a++, o += 4, c--)
		bus_space_write_4(t, h, o, *a);
}

extern __inline__ void
bus_space_write_region_8(t, h, o, a, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	const u_int64_t		*a;
	size_t			c;
{
	for (; c; a++, o += 8, c--)
		bus_space_write_8(t, h, o, *a);
}

/*
 *	void bus_space_set_multi_N __P((bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *		size_t count));
 *
 * Write the 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle/offset `count' times.
 */

extern __inline__ void
bus_space_set_multi_1(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int8_t		v;
	size_t			c;
{
	for (; c; c--)
		bus_space_write_1(t, h, o, v);
}

extern __inline__ void
bus_space_set_multi_2(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int16_t		v;
	size_t			c;
{
	for (; c; c--)
		bus_space_write_2(t, h, o, v);
}

extern __inline__ void
bus_space_set_multi_4(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int32_t		v;
	size_t			c;
{
	for (; c; c--)
		bus_space_write_4(t, h, o, v);
}

extern __inline__ void
bus_space_set_multi_8(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int64_t		v;
	size_t			c;
{
	for (; c; c--)
		bus_space_write_8(t, h, o, v);
}

/*
 *	void bus_space_set_region_N __P((bus_space_tag_t tag,
 *		bus_space_handle_t bsh, bus_size_t offset, u_intN_t val,
 *		size_t count));
 *
 * Write `count' 1, 2, 4, or 8 byte value `val' to bus space described
 * by tag/handle starting at `offset'.
 */
extern __inline__ void
bus_space_set_region_1(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int8_t		v;
	size_t			c;
{
	for (; c; o++, c--)
		bus_space_write_1(t, h, o, v);
}

extern __inline__ void
bus_space_set_region_2(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int16_t		v;
	size_t			c;
{
	for (; c; o += 2, c--)
		bus_space_write_2(t, h, o, v);
}

extern __inline__ void
bus_space_set_region_4(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int32_t		v;
	size_t			c;
{
	for (; c; o += 4, c--)
		bus_space_write_4(t, h, o, v);
}

extern __inline__ void
bus_space_set_region_8(t, h, o, v, c)
	bus_space_tag_t		t;
	bus_space_handle_t	h;
	bus_size_t		o;
	u_int64_t		v;
	size_t			c;
{
	for (; c; o += 8, c--)
		bus_space_write_8(t, h, o, v);
}

/*
 * Flags used in various bus DMA methods.
 */
#define	BUS_DMA_WAITOK		0x00	/* safe to sleep (pseudo-flag) */
#define	BUS_DMA_NOWAIT		0x01	/* not safe to sleep */
#define	BUS_DMA_ALLOCNOW	0x02	/* perform resource allocation now */
#define	BUS_DMA_COHERENT	0x04	/* hint: map memory DMA coherent */
#define	BUS_DMA_BUS1		0x10	/* placeholders for bus functions... */
#define	BUS_DMA_BUS2		0x20
#define	BUS_DMA_BUS3		0x40
#define	BUS_DMA_BUS4		0x80

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

/*
 * Tag values:
 */
#define	BUS_ISA_DMA_TAG		0x00
#define	BUS_PCI_DMA_TAG		0x01

typedef int				bus_dma_tag_t;
typedef struct atari_bus_dmamap		*bus_dmamap_t;

/*
 *	bus_dma_segment_t
 *
 *	Describes a single contiguous DMA transaction.  Values
 *	are suitable for programming into DMA registers.
 */
struct atari_bus_dma_segment {
	bus_addr_t	ds_addr;	/* DMA address */
	bus_size_t	ds_len;		/* length of transfer */
};
typedef struct atari_bus_dma_segment	bus_dma_segment_t;

/*
 *	bus_dmamap_t
 *
 *	Describes a DMA mapping.
 */
struct atari_bus_dmamap {
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

int	bus_dmamap_create __P((bus_dma_tag_t, bus_size_t, int, bus_size_t,
	    bus_size_t, int, bus_dmamap_t *));
void	bus_dmamap_destroy __P((bus_dma_tag_t, bus_dmamap_t));
int	bus_dmamap_load __P((bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int));
int	bus_dmamap_load_mbuf __P((bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int));
int	bus_dmamap_load_uio __P((bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int));
int	bus_dmamap_load_raw __P((bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int));
void	bus_dmamap_unload __P((bus_dma_tag_t, bus_dmamap_t));
void	bus_dmamap_sync __P((bus_dma_tag_t, bus_dmamap_t, bus_addr_t,
	    bus_size_t, int));

int	bus_dmamem_alloc __P((bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags));
void	bus_dmamem_free __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs));
int	bus_dmamem_map __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, size_t size, caddr_t *kvap, int flags));
void	bus_dmamem_unmap __P((bus_dma_tag_t tag, caddr_t kva,
	    size_t size));
int	bus_dmamem_mmap __P((bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, int off, int prot, int flags));

#endif /* _ATARI_BUS_H_ */
