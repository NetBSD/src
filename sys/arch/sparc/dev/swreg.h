/*	$NetBSD: swreg.h,v 1.1 2000/06/26 19:54:09 pk Exp $	*/

/*-
 * Copyright (c) 2000 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Paul Kranenburg.
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
 * Register map for the Sun3 SCSI Interface (si)
 * The first part of this register map is an NCR5380
 * SCSI Bus Interface Controller (SBIC).  The rest is a
 * DMA controller and custom logic for the OBIO interface (3/50,3/60,4/110)
 *
 * Modified for Sun 4 systems by Jason R. Thorpe <thorpej@NetBSD.ORG>.
 */

/*
 * Note that the obio version on the 4/1xx (the so-called "SCSI Weird", or
 * "sw" controller) is laid out a bit differently, and hence the evilness
 * with unions.  Also, the "sw" doesn't appear to have a FIFO.
 */

#if __for_reference_only__
struct sw_regs {
	/*
	 * Am5380 Register map (no padding). See dev/ic/ncr5380reg.h
	 */
	struct ncr5380regs {
		u_char r[8];
	} sci;

	/* DMA controller registers on OBIO */
	u_int	dma_addr;	/* dma address */
	u_int	dma_count;	/* dma count */
	u_int	pad0;		/* no-existent register */
	u_int	sw_csr;		/* sw control/status */
	u_int	bpr;		/* sw byte pack */
};
#endif

/*
 * Size of NCR5380 registers located at the bottom of the register bank.
 */
#define NCR5380REGS_SZ	8

/*
 * Register definition for the `sw' OBIO controller
 */
#define SWREG_DMA_ADDR	(NCR5380REGS_SZ + 0)
#define SWREG_DMA_CNT	(NCR5380REGS_SZ + 4)
#define SWREG_CSR	(NCR5380REGS_SZ + 12)
#define SWREG_BPR	(NCR5380REGS_SZ + 16)
#define SWREG_BANK_SZ	(NCR5380REGS_SZ + 20)

/*
 * Status Register.
 * Note:
 *	(r)	indicates bit is read only.
 *	(rw)	indicates bit is read or write.
 *	(v)	vme host adaptor interface only.
 *	(o)	sun3/50 onboard host adaptor interface only.
 *	(b)	both vme and sun3/50 host adaptor interfaces.
 */
#define SW_CSR_DMA_ACTIVE	0x8000	/* (r,o) dma transfer active */
#define SW_CSR_DMA_CONFLICT	0x4000	/* (r,b) reg accessed while dmaing */
#define SW_CSR_DMA_BUS_ERR	0x2000	/* (r,b) bus error during dma */
#define SW_CSR_ID		0x1000	/* (r,b) 0 for 3/50, 1 for SCSI-3, */
					/* 0 if SCSI-3 unmodified */
#define SW_CSR_FIFO_FULL	0x0800	/* (r,b) fifo full */
#define SW_CSR_FIFO_EMPTY	0x0400	/* (r,b) fifo empty */
#define SW_CSR_SBC_IP		0x0200	/* (r,b) sbc interrupt pending */
#define SW_CSR_DMA_IP		0x0100	/* (r,b) dma interrupt pending */
#define SW_CSR_DMA_EN		0x0010	/* (rw,v) dma/interrupt enable */
#define SW_CSR_SEND		0x0008	/* (rw,b) dma dir, 1=to device */
#define SW_CSR_INTR_EN		0x0004	/* (rw,b) interrupts enable */
#define SW_CSR_FIFO_RES		0x0002	/* (rw,b) inits fifo, 0=reset */
#define SW_CSR_SCSI_RES		0x0001	/* (rw,b) reset sbc and udc, 0=reset */
