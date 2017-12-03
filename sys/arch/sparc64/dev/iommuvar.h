/*	$NetBSD: iommuvar.h,v 1.21.2.1 2017/12/03 11:36:44 jdolecek Exp $	*/

/*
 * Copyright (c) 1999 Matthew R. Green
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _SPARC64_DEV_IOMMUVAR_H_
#define _SPARC64_DEV_IOMMUVAR_H_

/*
 * Streaming buffer control
 *
 * It's easy to deal w/sysio since it has a single streaming buffer, and
 * flushes are done on 8-byte boundaries.  But psycho is a pain since it
 * has two streaming buffers and the streaming buffer flush dumps 64 bytes
 * of data.
 */
struct strbuf_ctl {
	struct iommu_state	*sb_is;		/* Pointer to our iommu */
	bus_space_handle_t	sb_sb;		/* Handle for our regs */
	paddr_t			sb_flushpa;	/* to flush streaming buffers */
	volatile int64_t	*sb_flush;
};

/*
 * per-IOMMU state
 */
struct iommu_state {
	paddr_t			is_ptsb;	/* TSB physical address */
	int64_t			*is_tsb;	/* TSB virtual address */
	int			is_tsbsize;	/* 0 = 8K, ... */
	u_int			is_dvmabase;
	u_int			is_dvmaend;
	int64_t			is_cr;		/* IOMMU control regiter value */
	struct extent		*is_dvmamap;	/* DVMA map for this instance */
	kmutex_t		is_lock;	/* lock for DVMA map */
	int			is_flags;
#define IOMMU_FLUSH_CACHE	0x00000001
#define IOMMU_TSBSIZE_IN_PTSB	0x00000002	/* PCIe */

	struct strbuf_ctl	*is_sb[2];	/* Streaming buffers if any */

	/* copies of our parents state, to allow us to be self contained */
	bus_space_tag_t		is_bustag;	/* our bus tag */
	bus_space_handle_t	is_iommu;	/* IOMMU registers */
	uint64_t		is_devhandle;   /* for sun4v hypervisor calls */
};

/* interfaces for PCI/SBUS code */
void	iommu_init(char *, struct iommu_state *, int, uint32_t);
void	iommu_reset(struct iommu_state *);
void    iommu_enter(struct strbuf_ctl *, vaddr_t, int64_t, int);
void    iommu_remove(struct iommu_state *, vaddr_t, size_t);
paddr_t iommu_extract(struct iommu_state *, vaddr_t);

int	iommu_dvmamap_load(bus_dma_tag_t, bus_dmamap_t, void *, bus_size_t,
		struct proc *, int);
void	iommu_dvmamap_unload(bus_dma_tag_t, bus_dmamap_t);
int	iommu_dvmamap_load_raw(bus_dma_tag_t, bus_dmamap_t,
		bus_dma_segment_t *, int, bus_size_t, int);
void	iommu_dvmamap_sync(bus_dma_tag_t, bus_dmamap_t, bus_addr_t, bus_size_t,
		int);
int	iommu_dvmamem_alloc(bus_dma_tag_t, bus_size_t, bus_size_t, bus_size_t,
		bus_dma_segment_t *, int, int *, int);
void	iommu_dvmamem_free(bus_dma_tag_t, bus_dma_segment_t *, int);
int	iommu_dvmamem_map(bus_dma_tag_t, bus_dma_segment_t *, int, size_t,
		void **, int);
void	iommu_dvmamem_unmap(bus_dma_tag_t, void *, size_t);

#endif /* _SPARC64_DEV_IOMMUVAR_H_ */
