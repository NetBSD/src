/*	$NetBSD: bus_funcs.h,v 1.2.12.1 2014/08/20 00:03:25 tls Exp $	*/

/*-
 * Copyright (c) 1996, 1997, 1998, 2001 The NetBSD Foundation, Inc.
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
 * Copyright (c) 1997-1999, 2001 Eduardo E. Horvath. All rights reserved.
 * Copyright (c) 1996 Charles M. Hannum.  All rights reserved.
 * Copyright (c) 1996 Christopher G. Demetriou.  All rights reserved.
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
 *      This product includes software developed by Christopher G. Demetriou
 *	for the NetBSD Project.
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

#ifndef _SPARC64_BUS_FUNCS_H_
#define _SPARC64_BUS_FUNCS_H_

/*
 * Debug hooks
 */

extern int bus_space_debug;

bus_space_tag_t bus_space_tag_alloc(bus_space_tag_t, void *);
int		bus_space_translate_address_generic(struct openprom_range *,
						    int, bus_addr_t *);

#if 0
/*
 * The following macro could be used to generate the bus_space*() functions
 * but it uses a gcc extension and is ANSI-only.
#define PROTO_bus_space_xxx		(bus_space_tag_t t, ...)
#define RETURNTYPE_bus_space_xxx	void *
#define BUSFUN(name, returntype, t, args...)			\
	static __inline RETURNTYPE_##name			\
	bus_##name PROTO_##name					\
	{							\
		while (t->sparc_##name == NULL)			\
			t = t->parent;				\
		return (*(t)->sparc_##name)(t, args);		\
	}
 */
#endif

/*
 * Bus space function prototypes.
 */
static void	*bus_intr_establish(
				bus_space_tag_t,
				int,			/*bus-specific intr*/
				int,			/*device class level,
							  see machine/intr.h*/
				int (*)(void *),	/*handler*/
				void *);		/*handler arg*/


/* This macro finds the first "upstream" implementation of method `f' */
#define _BS_CALL(t,f)			\
	while (t->f == NULL)		\
		t = t->parent;		\
	return (*(t)->f)

#define _BS_VOID_CALL(t,f)			\
	while (t->f == NULL)		\
		t = t->parent;		\
	(*(t)->f)

static __inline void *
bus_intr_establish(bus_space_tag_t t, int p, int l, int	(*h)(void *), void *a)
{
	_BS_CALL(t, sparc_intr_establish)(t, p, l, h, a, NULL);
}

/* XXXX Things get complicated if we use unmapped register accesses. */
#define	bus_space_vaddr(t, h)	(PHYS_ASI((h)._asi) ? \
			NULL : (void *)(vaddr_t)((h)._ptr))

#define bus_space_barrier(t, h, o, s, f)	\
	sparc_bus_space_barrier((t), (h), (o), (s), (f))

static __inline void
sparc_bus_space_barrier(bus_space_tag_t t, bus_space_handle_t h,
    bus_size_t o, bus_size_t s, int f)
{
	/*
	 * We have a bit of a problem with the bus_space_barrier()
	 * interface.  It defines a read barrier and a write barrier
	 * which really don't map to the 7 different types of memory
	 * barriers in the SPARC v9 instruction set.
	 */
	if (f == BUS_SPACE_BARRIER_READ)
		/* A load followed by a load to the same location? */
		__asm volatile("membar #Lookaside");
	else if (f == BUS_SPACE_BARRIER_WRITE)
		/* A store followed by a store? */
		__asm volatile("membar #StoreStore");
	else 
		/* A store followed by a load? */
		__asm volatile("membar #StoreLoad|#MemIssue|#Lookaside");
}

/*
 *	uintN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */
#ifndef BUS_SPACE_DEBUG
#define	bus_space_read_1(t, h, o)					\
	    (0 ? (t)->type : lduba((h)._ptr + (o), (h)._asi))

#define	bus_space_read_2(t, h, o)					\
	    (0 ? (t)->type : lduha((h)._ptr + (o), (h)._asi))

#define	bus_space_read_4(t, h, o)					\
	    (0 ? (t)->type : lda((h)._ptr + (o), (h)._asi))

#define	bus_space_read_8(t, h, o)					\
	    (0 ? (t)->type : ldxa((h)._ptr + (o), (h)._asi))
#else
#define	bus_space_read_1(t, h, o) ({					\
	uint8_t __bv =							\
	    lduba((h)._ptr + (o), (h)._asi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr1(%llx + %llx, %x) -> %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (uint32_t) __bv);				\
	__bv; })

#define	bus_space_read_2(t, h, o) ({					\
	uint16_t __bv =							\
	    lduha((h)._ptr + (o), (h)._asi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr2(%llx + %llx, %x) -> %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (uint32_t)__bv);				\
	__bv; })

#define	bus_space_read_4(t, h, o) ({					\
	uint32_t __bv =							\
	    lda((h)._ptr + (o), (h)._asi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr4(%llx + %llx, %x) -> %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, __bv);					\
	__bv; })

#define	bus_space_read_8(t, h, o) ({					\
	uint64_t __bv =							\
	    ldxa((h)._ptr + (o), (h)._asi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr8(%llx + %llx, %x) -> %llx\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (long long)__bv);				\
	__bv; })
#endif
/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */
#ifndef BUS_SPACE_DEBUG
#define	bus_space_write_1(t, h, o, v)					\
	(0 ? (t)->type : ((void)(stba((h)._ptr + (o), (h)._asi, (v)))))

#define	bus_space_write_2(t, h, o, v)					\
	(0 ? (t)->type : ((void)(stha((h)._ptr + (o), (h)._asi, (v)))))

#define	bus_space_write_4(t, h, o, v)					\
	(0 ? (t)->type : ((void)(sta((h)._ptr + (o), (h)._asi, (v)))))

#define	bus_space_write_8(t, h, o, v)					\
	(0 ? (t)->type : ((void)(stxa((h)._ptr + (o), (h)._asi, (v)))))
#else
#define	bus_space_write_1(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw1(%llx + %llx, %x) <- %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (uint32_t) v);				\
	((void)(stba((h)._ptr + (o), (h)._asi, (v)))); })

#define	bus_space_write_2(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw2(%llx + %llx, %x) <- %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (uint32_t) v);				\
	((void)(stha((h)._ptr + (o), (h)._asi, (v)))); })

#define	bus_space_write_4(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw4(%llx + %llx, %x) <- %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (uint32_t) v);				\
	((void)(sta((h)._ptr + (o), (h)._asi, (v)))); })

#define	bus_space_write_8(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw8(%llx + %llx, %x) <- %llx\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._asi, (long long) v);				\
	((void)(stxa((h)._ptr + (o), (h)._asi, (v)))); })
#endif
/*
 *	uintN_t bus_space_read_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */
#ifndef BUS_SPACE_DEBUG
#define	bus_space_read_stream_1(t, h, o)				\
	    (0 ? (t)->type : lduba((h)._ptr + (o), (h)._sasi))

#define	bus_space_read_stream_2(t, h, o)				\
	    (0 ? (t)->type : lduha((h)._ptr + (o), (h)._sasi))

#define	bus_space_read_stream_4(t, h, o)				\
	    (0 ? (t)->type : lda((h)._ptr + (o), (h)._sasi))

#define	bus_space_read_stream_8(t, h, o)				\
	    (0 ? (t)->type : ldxa((h)._ptr + (o), (h)._sasi))
#else
#define	bus_space_read_stream_1(t, h, o) ({				\
	uint8_t __bv =							\
	    lduba((h)._ptr + (o), (h)._sasi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr1(%llx + %llx, %x) -> %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (uint32_t) __bv);				\
	__bv; })

#define	bus_space_read_stream_2(t, h, o) ({				\
	uint16_t __bv =							\
	    lduha((h)._ptr + (o), (h)._sasi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr2(%llx + %llx, %x) -> %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (uint32_t)__bv);				\
	__bv; })

#define	bus_space_read_stream_4(t, h, o) ({				\
	uint32_t __bv =							\
	    lda((h)._ptr + (o), (h)._sasi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr4(%llx + %llx, %x) -> %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, __bv);					\
	__bv; })

#define	bus_space_read_stream_8(t, h, o) ({				\
	uint64_t __bv =							\
	    ldxa((h)._ptr + (o), (h)._sasi);				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsr8(%llx + %llx, %x) -> %llx\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (long long)__bv);				\
	__bv; })
#endif
/*
 *	void bus_space_write_stream_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    uintN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */
#ifndef BUS_SPACE_DEBUG
#define	bus_space_write_stream_1(t, h, o, v)				\
	(0 ? (t)->type : ((void)(stba((h)._ptr + (o), (h)._sasi, (v)))))

#define	bus_space_write_stream_2(t, h, o, v)				\
	(0 ? (t)->type : ((void)(stha((h)._ptr + (o), (h)._sasi, (v)))))

#define	bus_space_write_stream_4(t, h, o, v)				\
	(0 ? (t)->type : ((void)(sta((h)._ptr + (o), (h)._sasi, (v)))))

#define	bus_space_write_stream_8(t, h, o, v)				\
	(0 ? (t)->type : ((void)(stxa((h)._ptr + (o), (h)._sasi, (v)))))
#else
#define	bus_space_write_stream_1(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw1(%llx + %llx, %x) <- %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (uint32_t) v);				\
	((void)(stba((h)._ptr + (o), (h)._sasi, (v)))); })

#define	bus_space_write_stream_2(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw2(%llx + %llx, %x) <- %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (uint32_t) v);				\
	((void)(stha((h)._ptr + (o), (h)._sasi, (v)))); })

#define	bus_space_write_stream_4(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw4(%llx + %llx, %x) <- %x\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (uint32_t) v);				\
	((void)(sta((h)._ptr + (o), (h)._sasi, (v)))); })

#define	bus_space_write_stream_8(t, h, o, v) ({				\
	if (bus_space_debug & BSDB_ACCESS)				\
	printf("bsw8(%llx + %llx, %x) <- %llx\n", (long long)(h)._ptr,	\
		(long long)(o),						\
		(h)._sasi, (long long) v);				\
	((void)(stxa((h)._ptr + (o), (h)._sasi, (v)))); })
#endif
/* Forwards needed by prototypes below. */
struct mbuf;
struct uio;

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
	(void)((t)->_dmamap_sync ?				\
	    (*(t)->_dmamap_sync)((t), (p), (o), (l), (ops)) : (void)0)

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

#define bus_dmatag_subregion(t, mna, mxa, nt, f) EOPNOTSUPP
#define bus_dmatag_destroy(t)

#ifdef _SPARC_BUS_DMA_PRIVATE
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
	    int nsegs, size_t size, void **kvap, int flags);
void	_bus_dmamem_unmap(bus_dma_tag_t tag, void *kva,
	    size_t size);
paddr_t	_bus_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

int	_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    vaddr_t low, vaddr_t high);
#endif /* _SPARC_BUS_DMA_PRIVATE */

#endif /* _SPARC64_BUS_FUNCS_H_ */
