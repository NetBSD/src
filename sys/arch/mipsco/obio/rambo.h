/*	$NetBSD: rambo.h,v 1.4 2001/09/16 16:34:33 wiz Exp $	*/
/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Wayne Knowles
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
 *  RAMBO DMA controller/timer asic used on the Mips 3230 (Pizazz) 
 */

#ifndef _MACHINE_RAMBO_H
#define _MACHINE_RAMBO_H 1

/* Register laytout of a single RAMBO DMA channel */
struct	rambo_ch {
	u_long	dma_laddr;	/* DMA load address reg	32b R/W */
	u_long  __0[63];
	u_long  dma_diag;	/* DMA Diagnostic reg	32b R   */
	u_long	__1[63];
	u_short __2;
	u_short	dma_fifo;	/* FIFO Buffer 16 bits	16b R/W */
	u_long	__3[63];
	u_long	dma_mode;	/* DMA Mode Register	32b R/W */
	u_long	__4[63];
	u_short __5;
	u_short	dma_block;	/* DMA Block Count	16b R/W */
	u_long	__6[63];
	u_long	dma_caddr;	/* DMA Current Address	32b R   */
	u_long	__7[63];
};

#define RAMBO_LADDR	0x0000
#define RAMBO_DIAG	0x0100
#define	RAMBO_FIFO	0x0202
#define	RAMBO_MODE	0x0300
#define	RAMBO_BLKCNT	0x0402
#define	RAMBO_CADDR	0x0500

/* DMA mode register (dma_mode) (R/W) */

#define	RB_CLRFIFO	0x80000000 /* Clear DMA FIFO */
#define	RB_DMA_ENABLE	0x40000000 /* Enable DMA Transfer */
#define	RB_AUTORELOAD	0x20000000 /* Auto restart DMA */
#define	RB_INT_ENABLE  	0X10000000 /* INterrupt on terminal count */

#define RB_DMA_WR	0x08000000 /* Xfer into memory */
#define RB_DMA_RD	0x00000000 /* Xfer from memory */

#define	RB_CLRERROR	0x04000000 /* Clear DMA Error register */

/* status bits of mode register (R) */

#define	RB_FIFO_FULL	0x00000800 /* FIFO Buffer is full */
#define	RB_FIFO_EMPTY	0x00000400 /* FIFO Buffer is empty */
#define	RB_DMA_ERROR	0x00000200 /* Error has occurred */
#define	RB_INTR_PEND	0x00000100 /* Interrupt is pending */

#define	RB_CNT_MASK	0x000000ff /* half-words left in FIFO */

/* Offsets to other registers in the RAMBO asic */
#define	RB_TCOUNT	0x0c00
#define	RB_TBREAK	0x0d00
#define RB_ERRREG	0x0e00
#define	RB_CTLREG	0x0f00

/* Hardware Register */

#define	RB_BUZZ0	0x00	/* 1524 Hz */
#define	RB_BUZZ1	0x10	/* 762 Hz */
#define	RB_BUZZ2	0x20	/* 381 Hz */
#define	RB_BUZZ3	0x30	/* 190 Hz */
#define	RB_BUZZOFF	0x08	/* Buzzer Enable - Active Low  */
#define	RB_PARITY_EN	0x04	/* Enable Parity - Active High */
#define	RB_CLR_PAR	0x02	/* Clear SysParErr - Active High */
#define	RB_CLR_IOERR	0x01	/* Clear ErrIntB - Active Low */

#define	RB_BLK_SHIFT	6
#define	RB_BLK_CNT	32	/* half-word byte count */
#define	RB_BLK_MASK	0x3f	/* Alignment mask */
#define	RB_BLK_SIZE	64	/* Bytes in a DMA Block */

/* DMA cannot cross 512k boundry (2^19 == 512k) */
#define RB_BSIZE	19
#define RB_BMASK	((1<<RB_BSIZE)-1)
#define	RB_BOUNDRY	(1<<RB_BSIZE)

/* Rambo cycle counter is fed by 25MHz clock then divided by 4 */
#define	HZ_TO_TICKS(hz)		(6250000L/(hz))
#define TICKS_TO_USECS(t)	(((t)*4)/25)
#endif
