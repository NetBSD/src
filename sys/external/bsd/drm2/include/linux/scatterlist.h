/*	$NetBSD: scatterlist.h,v 1.8 2021/12/19 12:13:23 riastradh Exp $	*/

/*-
 * Copyright (c) 2018 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Taylor R. Campbell.
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

#ifndef	_LINUX_SCATTERLIST_H_
#define	_LINUX_SCATTERLIST_H_

#include <sys/bus.h>

#include <asm/io.h>

#include <linux/mm.h>
#include <linux/mm_types.h>
#include <linux/gfp.h>
#include <linux/types.h>

/* namespace */
#define	__sg_alloc_table_from_pages	linux___sg_alloc_table_from_pages
#define	dma_map_sg			linux_dma_map_sg
#define	dma_map_sg_attrs		linux_dma_map_sg_attrs
#define	dma_unmap_sg			linux_dma_unmap_sg
#define	dma_unmap_sg_attrs		linux_dma_unmap_sg_attrs
#define	sg_alloc_table			linux_sg_alloc_table
#define	sg_alloc_table_from_bus_dmamem	linux_sg_alloc_table_from_bus_dmamem
#define	sg_alloc_table_from_pages	linux_sg_alloc_table_from_pages
#define	sg_dma_address			linux_sg_dma_address
#define	sg_dma_len			linux_sg_dma_len
#define	sg_free_table			linux_sg_free_table

struct page;

struct sg_table {
	struct scatterlist {
		struct page	**sg_pgs;
		unsigned	sg_npgs;
		bus_dma_tag_t	sg_dmat;
		bus_dmamap_t	sg_dmamap;
	} sgl[1];
	unsigned	nents;
};

int sg_alloc_table(struct sg_table *, unsigned, gfp_t);
int __sg_alloc_table_from_pages(struct sg_table *, struct page **, unsigned,
    bus_size_t, bus_size_t, unsigned, gfp_t);
int sg_alloc_table_from_pages(struct sg_table *, struct page **, unsigned,
    bus_size_t, bus_size_t, gfp_t);
int sg_alloc_table_from_bus_dmamem(struct sg_table *, bus_dma_tag_t,
    const bus_dma_segment_t *, int, gfp_t);
void sg_free_table(struct sg_table *);

bus_addr_t sg_dma_address(const struct scatterlist *);
bus_size_t sg_dma_len(const struct scatterlist *);

int dma_map_sg(bus_dma_tag_t, struct scatterlist *, int, int);
int dma_map_sg_attrs(bus_dma_tag_t, struct scatterlist *, int, int, int);
void dma_unmap_sg(bus_dma_tag_t, struct scatterlist *, int, int);
void dma_unmap_sg_attrs(bus_dma_tag_t, struct scatterlist *, int, int, int);

#endif	/* _LINUX_SCATTERLIST_H_ */
