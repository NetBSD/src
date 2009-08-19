/* $NetBSD: isa_machdep.h,v 1.8.108.2 2009/08/19 18:45:54 yamt Exp $ */

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
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

#include <dev/isa/isadmavar.h>

/*
 * Types provided to machine-independent ISA code.
 */
typedef struct alpha_isa_chipset *isa_chipset_tag_t;

struct alpha_isa_chipset {
	void	*ic_v;

	struct isa_dma_state ic_dmastate;

	void	(*ic_attach_hook)(struct device *, struct device *,
		    struct isabus_attach_args *);
	const struct evcnt *(*ic_intr_evcnt)(void *, int);
	void	*(*ic_intr_establish)(void *, int, int, int,
		    int (*)(void *), void *);
	void	(*ic_intr_disestablish)(void *, void *);
	int	(*ic_intr_alloc)(void *, int, int, int *);
	void	(*ic_detach_hook)(isa_chipset_tag_t, device_t);
};


/*
 * Functions provided to machine-independent ISA code.
 */
#define	isa_attach_hook(p, s, a)					\
    (*(a)->iba_ic->ic_attach_hook)((p), (s), (a))
#define	isa_detach_hook(c, s)						\
    (*(c)->ic_detach_hook)((c), (s))
#define	isa_intr_evcnt(c, i)					\
    (*(c)->ic_intr_evcnt)((c)->ic_v, (i))
#define	isa_intr_establish(c, i, t, l, f, a)				\
    (*(c)->ic_intr_establish)((c)->ic_v, (i), (t), (l), (f), (a))
#define	isa_intr_disestablish(c, h)					\
    (*(c)->ic_intr_disestablish)((c)->ic_v, (h))
#define	isa_intr_alloc(c, m, t, i)					\
    (*(c)->ic_intr_alloc)((c)->ic_v, (m), (t), (i))

#define	isa_dmainit(ic, bst, dmat, d)					\
	_isa_dmainit(&(ic)->ic_dmastate, (bst), (dmat), (d))
#define	isa_dmadestroy(ic)						\
	_isa_dmadestroy(&(ic)->ic_dmastate)
#define	isa_dmacascade(ic, c)						\
	_isa_dmacascade(&(ic)->ic_dmastate, (c))
#define	isa_dmamaxsize(ic, c)						\
	_isa_dmamaxsize(&(ic)->ic_dmastate, (c))
#define	isa_dmamap_create(ic, c, s, f)					\
	_isa_dmamap_create(&(ic)->ic_dmastate, (c), (s), (f))
#define	isa_dmamap_destroy(ic, c)					\
	_isa_dmamap_destroy(&(ic)->ic_dmastate, (c))
#define	isa_dmastart(ic, c, a, n, p, f, bf)				\
	_isa_dmastart(&(ic)->ic_dmastate, (c), (a), (n), (p), (f), (bf))
#define	isa_dmaabort(ic, c)						\
	_isa_dmaabort(&(ic)->ic_dmastate, (c))
#define	isa_dmacount(ic, c)						\
	_isa_dmacount(&(ic)->ic_dmastate, (c))
#define	isa_dmafinished(ic, c)						\
	_isa_dmafinished(&(ic)->ic_dmastate, (c))
#define	isa_dmadone(ic, c)						\
	_isa_dmadone(&(ic)->ic_dmastate, (c))
#define	isa_dmafreeze(ic)						\
	_isa_dmafreeze(&(ic)->ic_dmastate)
#define	isa_dmathaw(ic)							\
	_isa_dmathaw(&(ic)->ic_dmastate)
#define	isa_dmamem_alloc(ic, c, s, ap, f)				\
	_isa_dmamem_alloc(&(ic)->ic_dmastate, (c), (s), (ap), (f))
#define	isa_dmamem_free(ic, c, a, s)					\
	_isa_dmamem_free(&(ic)->ic_dmastate, (c), (a), (s))
#define	isa_dmamem_map(ic, c, a, s, kp, f)				\
	_isa_dmamem_map(&(ic)->ic_dmastate, (c), (a), (s), (kp), (f))
#define	isa_dmamem_unmap(ic, c, k, s)					\
	_isa_dmamem_unmap(&(ic)->ic_dmastate, (c), (k), (s))
#define	isa_dmamem_mmap(ic, c, a, s, o, p, f)				\
	_isa_dmamem_mmap(&(ic)->ic_dmastate, (c), (a), (s), (o), (p), (f))
#define isa_drq_alloc(ic, c)						\
	_isa_drq_alloc(&(ic)->ic_dmastate, c)
#define isa_drq_free(ic, c)						\
	_isa_drq_free(&(ic)->ic_dmastate, c)
#define	isa_drq_isfree(ic, c)						\
	_isa_drq_isfree(&(ic)->ic_dmastate, (c))
#define	isa_malloc(ic, c, s, p, f)					\
	_isa_malloc(&(ic)->ic_dmastate, (c), (s), (p), (f))
#define	isa_free(a, p)							\
	_isa_free((a), (p))
#define	isa_mappage(m, o, p)						\
	_isa_mappage((m), (o), (p))

/*
 * alpha-specific ISA functions.
 * NOT TO BE USED DIRECTLY BY MACHINE INDEPENDENT CODE.
 */ 
int	isa_display_console(bus_space_tag_t, bus_space_tag_t);
void	isabeep(int, int);

#ifdef _ALPHA_BUS_DMA_PRIVATE
int	isadma_bounce_dmamap_create(bus_dma_tag_t, bus_size_t, int,
	    bus_size_t, bus_size_t, int, bus_dmamap_t *);
void	isadma_bounce_dmamap_destroy(bus_dma_tag_t, bus_dmamap_t);
int	isadma_bounce_dmamap_load(bus_dma_tag_t, bus_dmamap_t, void *,
	    bus_size_t, struct proc *, int);
int	isadma_bounce_dmamap_load_mbuf(bus_dma_tag_t, bus_dmamap_t,
	    struct mbuf *, int);
int	isadma_bounce_dmamap_load_uio(bus_dma_tag_t, bus_dmamap_t,
	    struct uio *, int);
int	isadma_bounce_dmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
	    bus_dma_segment_t *, int, bus_size_t, int);
void	isadma_bounce_dmamap_unload(bus_dma_tag_t, bus_dmamap_t);
void	isadma_bounce_dmamap_sync(bus_dma_tag_t, bus_dmamap_t,
	    bus_addr_t, bus_size_t, int);

int	isadma_bounce_dmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t,
	    bus_size_t, bus_dma_segment_t *, int, int *, int);
#endif /* _ALPHA_BUS_DMA_PRIVATE */
