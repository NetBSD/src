/*	$NetBSD: dmareg.h,v 1.3 2002/09/11 01:46:36 mycroft Exp $	*/
/*
 * Copyright (c) 1997 Rolf Grossmann
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
 *      This product includes software developed by Rolf Grossmann.
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

#define MAX_DMASIZE 4096

/* from nextdev/dma.h */

#if 0
#define	DMA_BEGINALIGNMENT	4	/* initial buffer must be on long */
#else
#define	DMA_BEGINALIGNMENT	16	/* initial buffer must be on long */
#endif

#define	DMA_ENDALIGNMENT	16	/* DMA must end on quad longword */
#define ENDMA_ENDALIGNMENT	32	/* Ethernet DMA is very special */

#define	DMA_ALIGN(type, addr)	\
	((type)(((unsigned)(addr)+DMA_BEGINALIGNMENT-1) \
		&~(DMA_BEGINALIGNMENT-1)))

#define	DMA_ENDALIGN(type, addr)	\
	((type)(((unsigned)(addr)+DMA_ENDALIGNMENT-1) \
		&~(DMA_ENDALIGNMENT-1)))

#define	ENDMA_ENDALIGN(type, addr)	\
	((type)((((unsigned)(addr)+ENDMA_ENDALIGNMENT-1) \
		 &~(DMA_ENDALIGNMENT-1))|0x80000000))

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

/*
 * bits in dd_csr
 */
/* read bits */
#define	DMACSR_ENABLE		0x01000000	/* enable dma transfer */
#define	DMACSR_SUPDATE		0x02000000	/* single update */
#define	DMACSR_COMPLETE		0x08000000	/* current dma has completed */
#define	DMACSR_BUSEXC		0x10000000	/* bus exception occurred */
/* write bits */
#define	DMACSR_SETENABLE	0x00010000 	/* set enable */
#define	DMACSR_SETSUPDATE	0x00020000	/* set single update */
#define	DMACSR_READ		0x00040000	/* dma from dev to mem */
#define	DMACSR_WRITE		0x00000000	/* dma from mem to dev */
#define	DMACSR_CLRCOMPLETE	0x00080000	/* clear complete conditional */
#define	DMACSR_RESET		0x00100000	/* clr cmplt, sup, enable */
#define	DMACSR_INITBUF		0x00200000	/* initialize DMA buffers */
#define DMACSR_INITBUFTURBO	0x00800000
