/*	$NetBSD: bus_funcs.h,v 1.1 2011/07/01 17:10:01 dyoung Exp $	*/

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

#ifndef _SPARC_BUS_FUNCS_H_
#define _SPARC_BUS_FUNCS_H_

bus_space_tag_t bus_space_tag_alloc(bus_space_tag_t, void *);
int		bus_space_translate_address_generic(struct openprom_range *,
						    int, bus_addr_t *);

/*
 * Bus space function prototypes.
 * In bus_space_map2(), supply a special virtual address only if you
 * get it from ../sparc/vaddrs.h.
 */
int	bus_space_map2(
			bus_space_tag_t,
			bus_addr_t,
			bus_size_t,
			int,			/*flags*/
			vaddr_t,		/*preferred vaddr*/
			bus_space_handle_t *);
void	*bus_intr_establish(
			bus_space_tag_t,
			int,			/*bus-specific intr*/
			int,			/*device class level,
						  see machine/intr.h*/
			int (*)(void *),	/*handler*/
			void *);		/*handler arg*/
void	*bus_intr_establish2(
			bus_space_tag_t,
			int,			/*bus-specific intr*/
			int,			/*device class level,
						  see machine/intr.h*/
			int (*)(void *),	/*handler*/
			void *,			/*handler arg*/
			void (*)(void));	/*optional fast vector*/

#define	bus_space_vaddr(t, h)	((void)(t), (void *)(h))

/*
 * Device space probe assistant.
 * The optional callback function's arguments are:
 *	the temporary virtual address
 *	the passed `arg' argument
 */
int bus_space_probe(
		bus_space_tag_t,
		bus_addr_t,
		bus_size_t,			/* probe size */
		size_t,				/* offset */
		int,				/* flags */
		int (*)(void *, void *),	/* callback function */
		void *);			/* callback arg */

/*
 *	u_intN_t bus_space_read_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset);
 *
 * Read a 1, 2, 4, or 8 byte quantity from bus space
 * described by tag/handle/offset.
 */

#define	bus_space_read_1_real(t, h, o)					\
	    ((void)(t), *(volatile uint8_t *)((h) + (o)))

#define	bus_space_read_2_real(t, h, o)					\
	    ((void)(t), *(volatile uint16_t *)((h) + (o)))

#define	bus_space_read_4_real(t, h, o)					\
	    ((void)(t), *(volatile uint32_t *)((h) + (o)))

#define	bus_space_read_8_real(t, h, o)					\
	    ((void)(t), *(volatile uint64_t *)((h) + (o)))

#define bus_space_read_stream_1 bus_space_read_1_real
#define bus_space_read_stream_2 bus_space_read_2_real
#define bus_space_read_stream_4 bus_space_read_4_real
#define bus_space_read_stream_8 bus_space_read_8_real


/*
 *	void bus_space_write_N(bus_space_tag_t tag,
 *	    bus_space_handle_t bsh, bus_size_t offset,
 *	    u_intN_t value);
 *
 * Write the 1, 2, 4, or 8 byte value `value' to bus space
 * described by tag/handle/offset.
 */

#define	bus_space_write_1_real(t, h, o, v)	do {			\
	((void)(t), (void)(*(volatile uint8_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_2_real(t, h, o, v)	do {			\
	((void)(t), (void)(*(volatile uint16_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_4_real(t, h, o, v)	do {			\
	((void)(t), (void)(*(volatile uint32_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define	bus_space_write_8_real(t, h, o, v)	do {			\
	((void)(t), (void)(*(volatile uint64_t *)((h) + (o)) = (v)));	\
} while (/* CONSTCOND */ 0)

#define bus_space_write_stream_1 bus_space_write_1_real
#define bus_space_write_stream_2 bus_space_write_2_real
#define bus_space_write_stream_4 bus_space_write_4_real
#define bus_space_write_stream_8 bus_space_write_8_real

#define bus_space_read_multi_stream_1 bus_space_read_multi_1
#define bus_space_write_multi_stream_1 bus_space_write_multi_1

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
void	_bus_dmamem_unmap(bus_dma_tag_t tag, void *kva,
	    size_t size);
paddr_t	_bus_dmamem_mmap(bus_dma_tag_t tag, bus_dma_segment_t *segs,
	    int nsegs, off_t off, int prot, int flags);

int	_bus_dmamem_alloc_range(bus_dma_tag_t tag, bus_size_t size,
	    bus_size_t alignment, bus_size_t boundary,
	    bus_dma_segment_t *segs, int nsegs, int *rsegs, int flags,
	    vaddr_t low, vaddr_t high);

vaddr_t	_bus_dma_valloc_skewed(size_t, u_long, u_long, u_long);
#endif /* _SPARC_BUS_DMA_PRIVATE */

#endif /* _SPARC_BUS_FUNCS_H_ */
