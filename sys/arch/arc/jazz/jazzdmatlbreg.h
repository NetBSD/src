/*	$NetBSD: jazzdmatlbreg.h,v 1.2 2000/06/10 12:56:46 soda Exp $	*/
/*	$OpenBSD: dma.h,v 1.3 1997/04/19 17:19:51 pefo Exp $	*/

/*
 * Copyright (c) 1996 Per Fogelstrom
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Per Fogelstrom.
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

/*
 *	The R4030 system has four DMA channels capable of scatter/gather
 *	and full memory addressing. The maximum transfer length is 1Mb.
 *	DMA snopes the L2 cache so no precaution is required. However
 *	if L1 cache is cached 'write back' the processor is responible
 *	for flushing/invalidating it.
 *
 *	The DMA mapper has up to 4096 page descriptors.
 */

/* XXX */
#define	PICA_TL_BASE		0xa0180000	/* Base of tl register area */
#define JAZZ_DMATLB_SIZE	0x00008000	/* Size of tl register area */

#define JAZZ_DMATLBREG_MAP	0x00		/* DMA transl. table base */
#define JAZZ_DMATLBREG_LIMIT	0x08		/* DMA transl. table limit */
#define JAZZ_DMATLBREG_IVALID	0x10		/* DMA transl. cache inval */
#define JAZZ_DMATLB_REGSIZE	0x18		/* size of bus_space region */

#define JAZZ_DMA_PAGE_SIZE	0x00001000		/* Address page size */
#define JAZZ_DMA_PAGE_OFFS	(JAZZ_DMA_PAGE_SIZE-1)	/* page offset */
#define JAZZ_DMA_PAGE_NUM	(~JAZZ_DMA_PAGE_OFFS)	/* page number */

#define jazz_dma_page_offs(x)	\
	((int)(x) & JAZZ_DMA_PAGE_OFFS)
#define jazz_dma_page_round(x)	\
	(((int)(x) + JAZZ_DMA_PAGE_OFFS) & JAZZ_DMA_PAGE_NUM)

/*
 *  DMA TLB entry
 */

typedef struct jazz_dma_pte {
	u_int32_t	lo_addr;	/* Low part of translation addr */
	u_int32_t	hi_addr;	/* High part of translation addr */
} jazz_dma_pte_t;
