/*	$NetBSD: dmareg.h,v 1.6 2007/02/03 05:17:30 tsutsui Exp $ */

/*
 * Copyright (c) 1994 Peter Galbavy.  All rights reserved.
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
 *	This product includes software developed by Peter Galbavy.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

#define DMACSRBITS "\020\01INT\02ERR\03DR1\04DR2\05IEN\011WRITE\016ENCNT\017TC\032DMAON"

#define DMAREG_SIZE		0x10

#define DMA_REG_CSR		0x00
#define DMA_REG_ADDR		0x04
#define DMA_REG_BCNT		0x08
#define DMA_REG_TEST		0x0c

struct dma_regs {
	uint32_t	csr;		/* DMA CSR */
	/* bits common to all revs. */
#define  D_INT_PEND	0x00000001	/* interrupt pending */
#define  D_ERR_PEND	0x00000002	/* error pending */
#define  D_PACKCNT	0x0000000c	/* byte pack count */
#define  D_INT_EN	0x00000010	/* interrupt enable */
#define  D_FLUSH	0x00000020	/* invalidate fifo */
#define  D_DRAIN	0x00000040	/* drain fifo */
#define  D_RESET	0x00000080	/* reset scsi */
#define  D_WRITE	0x00000100	/* device -> mem */
#define  D_EN_DMA	0x00000200	/* enable DMA requests */
#define  D_R_PEND	0x00000400	/* REV1,ESC: request pending */
#define  D_BYTEADR	0x00001800	/* REV1: next byte address */
#define  D_EN_CNT	0x00002000	/* REV1: enable byte counter */
#define  D_TC		0x00004000	/* REV1,2: terminal count */

#define  D_BURST_SIZE	0x000c0000	/* read/write burst size */
#define   D_BURST_16	0x00000000	/*   16-byte bursts */
#define   D_BURST_32    0x00040000	/*   32-byte bursts */
#define   D_BURST_0	0x00080000	/*   no bursts (SCSI-only) */

#define  D_TWO_CYCLE	0x00200000	/* REV3: 2 clocks per transfer */
#define  D_FASTER	0x00400000	/* 3 clocks per transfer */
#define  D_TCI_DIS	0x00800000	/* disable intr on D_TC */

#define  D_EN_NEXT	0x01000000	/* enable auto next address */
#define  D_DMA_ON	0x02000000	/* enable dma from scsi */
#define  D_A_LOADED	0x04000000	/* address loaded */
#define  D_NA_LOADED	0x08000000	/* next address loaded */

#define  D_DEV_ID	0xf0000000	/* device ID */
#define   DMAREV_0	0x00000000	/* Sunray DMA */
#define   DMAREV_ESC	0x40000000	/*  DMA ESC array */
#define   DMAREV_1	0x80000000	/* 'DMA' */
#define   DMAREV_PLUS	0x90000000	/* 'DMA+' */
#define   DMAREV_2	0xa0000000	/* 'DMA2' */

	uint32_t	addr;
#define DMA_D_ADDR		0x01		/* DMA ADDR (in longs) */

	uint32_t	bcnt;		/* DMA COUNT (in longs) */
#define  D_BCNT_MASK		0x00ffffff	/* only 24 bits */

	uint32_t	test;		/* DMA TEST (in longs) */
#define en_testcsr	addr			/* enet registers overlap */
#define en_cachev	bcnt
#define en_bar		test
};
