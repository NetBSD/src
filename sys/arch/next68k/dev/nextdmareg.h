/*	$NetBSD: nextdmareg.h,v 1.7 2001/03/31 06:56:54 dbj Exp $	*/
/*
 * Copyright (c) 1998 Darrin B. Jewell
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
 *      This product includes software developed by Darrin B. Jewell
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

/* I think the chip can handle 64k per chain, but I don't
 * know how much per segment for sure.  We might try
 * experimenting with this value.  Can we cross page boundaries?
 */
#define MAX_DMASIZE 8192

/* from nextdev/dma.h */

#if 0
#define	DMA_BEGINALIGNMENT	4		/* initial buffer must be on long */
#else
/* But to make cache handling easier, we put it on a cache line anyway. */
#define	DMA_BEGINALIGNMENT 16
#endif
#define	DMA_ENDALIGNMENT	16		/* DMA must end on quad longword */

#define	DMA_ALIGN(type, addr)	\
	((type)(((unsigned)(addr)+DMA_BEGINALIGNMENT-1) \
		&~(DMA_BEGINALIGNMENT-1)))

#define	DMA_ENDALIGN(type, addr)	\
	((type)(((unsigned)(addr)+DMA_ENDALIGNMENT-1) \
		&~(DMA_ENDALIGNMENT-1)))

#define	DMA_BEGINALIGNED(addr)	(((unsigned)(addr)&(DMA_BEGINALIGNMENT-1))==0)
#define	DMA_ENDALIGNED(addr)	(((unsigned)(addr)&(DMA_ENDALIGNMENT-1))==0)

#if 0
struct dma_dev {		/* format of dma device registers */
	int dd_csr;		/* control & status register */
	char dd_pad[0x3fec];	/* csr not contiguous with next */
	char *dd_saved_next;	/* saved pointers for HW restart */
	char *dd_saved_limit;
	char *dd_saved_start;
	char *dd_saved_stop;
	char *dd_next;		/* next word to dma */
	char *dd_limit;		/* dma complete when next == limit */
	char *dd_start;		/* start of 2nd buf to dma */
	char *dd_stop;		/* end of 2nd buf to dma */
	char dd_pad2[0x1f0];
	char *dd_next_initbuf;	/* next register that inits dma buffering */
};
#endif

#define DD_CSR          0
#define DD_SAVED_NEXT   (DD_CSR         +sizeof(int) + 0x3fec)
#define DD_SAVED_LIMIT  (DD_SAVED_NEXT  +sizeof(char *))
#define DD_SAVED_START  (DD_SAVED_LIMIT +sizeof(char *))
#define DD_SAVED_STOP   (DD_SAVED_START +sizeof(char *))
#define DD_NEXT         (DD_SAVED_STOP  +sizeof(char *))
#define DD_LIMIT        (DD_NEXT        +sizeof(char *))
#define DD_START        (DD_LIMIT       +sizeof(char *))
#define DD_STOP         (DD_START       +sizeof(char *))
#define DD_NEXT_INITBUF (DD_STOP        +sizeof(char *) + 0x1f0)

#define DD_SIZE         (DD_NEXT_INITBUF+sizeof(char *))
/*
 * bits in dd_csr
 */
/* read bits */
#define	DMACSR_ENABLE		0x01000000	/* enable dma transfer */
#define	DMACSR_SUPDATE		0x02000000	/* single update */
#define	DMACSR_READ		0x04000000	/* dma is ina read operation */
#define	DMACSR_COMPLETE		0x08000000	/* current dma has completed */
#define	DMACSR_BUSEXC		0x10000000	/* bus exception occurred */
/* write bits */
#define	DMACSR_SETENABLE	0x00010000 	/* set enable */
#define	DMACSR_SETSUPDATE	0x00020000	/* set single update */
#define	DMACSR_SETREAD		0x00040000	/* dma from dev to mem */
#define	DMACSR_SETWRITE		0x00000000	/* dma from mem to dev */
#define	DMACSR_CLRCOMPLETE	0x00080000	/* clear complete conditional */
#define	DMACSR_RESET		0x00100000	/* clr cmplt, sup, enable */
#define	DMACSR_INITBUF		0x00200000	/* initialize DMA buffers */

#define DMACSR_BITS \
"\20\35BUSEXC\34COMPLETE\33READ\32SUPDATE\31ENABLE\26INITBUF\25RESET\24CLRCOMPLETE\23SETREAD\22SETSUPDATE\21SETENABLE"
