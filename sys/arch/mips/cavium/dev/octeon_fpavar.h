/*	$NetBSD: octeon_fpavar.h,v 1.7 2020/06/23 05:14:18 simonb Exp $	*/

/*
 * Copyright (c) 2007 Internet Initiative Japan, Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _OCTEON_FPAVAR_H_
#define _OCTEON_FPAVAR_H_

#include <mips/cache_octeon.h>

#include <mips/cavium/octeonreg.h>
#include <mips/cavium/dev/octeon_fpareg.h>

struct octfpa_buf {
	int		fb_poolno;	/* pool # */

	size_t		fb_size;	/* element size */
	size_t		fb_nelems;	/* # of elements */

	paddr_t		fb_paddr;	/* physical address */
	vaddr_t		fb_addr;	/* virtual address */
	size_t		fb_len;		/* total length */

	bus_dma_tag_t	fb_dmat;
	bus_dmamap_t	fb_dmah;
	bus_dma_segment_t
			*fb_dma_segs;
	int		fb_dma_nsegs;
};

uint64_t	octfpa_int_summary(void);
int		octfpa_buf_init(int, size_t, size_t, struct octfpa_buf **);
void		*octfpa_buf_get(struct octfpa_buf *);
uint64_t	octfpa_query(int);
int		octfpa_available_fpa_pool(int *available, int pool_no);

/* Pool sizes in bytes, must be multiple of a cache line */
#define FPA_POOL_0_SIZE (16 * OCTEON_CACHELINE_SIZE)
#define FPA_POOL_1_SIZE (1 * OCTEON_CACHELINE_SIZE)
#define FPA_POOL_2_SIZE (8 * OCTEON_CACHELINE_SIZE)
#define FPA_POOL_3_SIZE (4 * OCTEON_CACHELINE_SIZE)

#define FPA_POOL_4_SIZE (16 * OCTEON_CACHELINE_SIZE)
#define FPA_POOL_5_SIZE (16 * OCTEON_CACHELINE_SIZE)
#define FPA_POOL_6_SIZE (16 * OCTEON_CACHELINE_SIZE)
#define FPA_POOL_7_SIZE (16 * OCTEON_CACHELINE_SIZE)

/* Pools in use */
#define FPA_RECV_PKT_POOL		(0)	/* Receive Packet buffers */
#define FPA_RECV_PKT_POOL_SIZE		FPA_POOL_0_SIZE
#define FPA_RECV_PKT_POOL_LINE		16
#define FPA_WQE_POOL			(1)	/* Work queue entrys */
#define FPA_WQE_POOL_SIZE		FPA_POOL_1_SIZE
#define FPA_WQE_POOL_LINE		1
#define FPA_COMMAND_BUFFER_POOL		(2)	/* PKO queue command buffers */
#define FPA_COMMAND_BUFFER_POOL_SIZE	FPA_POOL_2_SIZE
#define FPA_COMMAND_BUFFER_POOL_LINE	8
#define FPA_GATHER_BUFFER_POOL		(3)	/* PKO gather list buffers */
#define FPA_GATHER_BUFFER_POOL_SIZE	FPA_POOL_3_SIZE
#define FPA_GATHER_BUFFER_POOL_LINE	4

#ifndef FPA_OUTPUT_BUFFER_POOL
#define FPA_OUTPUT_BUFFER_POOL		FPA_COMMAND_BUFFER_POOL
#define FPA_OUTPUT_BUFFER_POOL_SIZE	FPA_COMMAND_BUFFER_POOL_SIZE
#endif

/*
 * operations
 */

static __inline uint64_t
octfpa_load(uint64_t fpapool)
{
	/* for FPA operations, subdid == pool number */
	uint64_t addr = OCTEON_ADDR_IO_DID(FPA_MAJOR_DID, fpapool);

	return octeon_xkphys_read_8(addr);
}

#ifdef notyet
static __inline uint64_t
octfpa_iobdma(struct octfpa_softc *sc, int srcaddr, int len)
{

	/* XXX */
	return 0ULL;
}
#endif

static __inline void
octfpa_store(uint64_t addr, uint64_t fpapool, uint64_t dwbcount)
{
	/* for FPA operations, subdid == pool number */
	uint64_t ptr =
	    OCTEON_ADDR_IO_DID(FPA_MAJOR_DID, fpapool) |
	    __SHIFTIN(addr, FPA_OPS_STORE_ADDR);

	OCTEON_SYNCWS;
	octeon_xkphys_write_8(ptr,
	    __SHIFTIN(dwbcount, FPA_OPS_STORE_DATA_DWBCOUNT));
}

static __inline paddr_t
octfpa_buf_get_paddr(struct octfpa_buf *fb)
{

	return octfpa_load(fb->fb_poolno);
}

static __inline void
octfpa_buf_put_paddr(struct octfpa_buf *fb, paddr_t paddr)
{

	KASSERT(paddr >= fb->fb_paddr);
	KASSERT(paddr < fb->fb_paddr + fb->fb_len);
	octfpa_store(paddr, fb->fb_poolno, fb->fb_size / 128);
}

static __inline void
octfpa_buf_put(struct octfpa_buf *fb, void *addr)
{
	paddr_t paddr;

	KASSERT((vaddr_t)addr >= fb->fb_addr);
	KASSERT((vaddr_t)addr < fb->fb_addr + fb->fb_len);
	paddr = fb->fb_paddr + (paddr_t/* XXX */)((vaddr_t)addr - fb->fb_addr);
	octfpa_buf_put_paddr(fb, paddr);
}

#endif /* _OCTEON_FPAVAR_H_ */
