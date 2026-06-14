/*	$NetBSD: dwcdmacreg.h,v 1.1 2026/06/14 00:02:35 rkujawa Exp $	*/

/*
 * Copyright (c) 2024, 2026 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Radoslaw Kujawa.
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
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * Synopsys DesignWare AHB Central DMA Controller (dw_dmac).
 *
 * Register layout is generic to the core; this copy lives here for the
 * 460EX integration, where one dedicated instance sits on the AHB just
 * below the DWC SATA core and feeds its DMADR FIFO window through the
 * hardware handshake interface.
 *
 * This should probably go to sys/dev/ic/, when someone else needs it.
 */

#ifndef _IBM4XX_DWCDMACREG_H_
#define	_IBM4XX_DWCDMACREG_H_

#define	DWCDMAC_SIZE		0x400
/* offset of the DMAC below the SATA core on the 460EX AHB */
#define	DWCDMAC_OFFSET		0x800

/* per-channel register block, ch = 0..7 */
#define	DWCDMAC_CHAN_SIZE	0x58
#define	DWCDMAC_CHAN(ch)	((ch) * DWCDMAC_CHAN_SIZE)
#define	DWCDMAC_SAR(ch)		(DWCDMAC_CHAN(ch) + 0x00) /* source addr */
#define	DWCDMAC_DAR(ch)		(DWCDMAC_CHAN(ch) + 0x08) /* dest addr */
#define	DWCDMAC_LLP(ch)		(DWCDMAC_CHAN(ch) + 0x10) /* list pointer */
#define	DWCDMAC_CTL(ch)		(DWCDMAC_CHAN(ch) + 0x18) /* control, low */
#define	DWCDMAC_CTL_HI(ch)	(DWCDMAC_CHAN(ch) + 0x1c) /* control, high */
#define	DWCDMAC_SSTAT(ch)	(DWCDMAC_CHAN(ch) + 0x20)
#define	DWCDMAC_DSTAT(ch)	(DWCDMAC_CHAN(ch) + 0x28)
#define	DWCDMAC_SSTATAR(ch)	(DWCDMAC_CHAN(ch) + 0x30)
#define	DWCDMAC_DSTATAR(ch)	(DWCDMAC_CHAN(ch) + 0x38)
#define	DWCDMAC_CFG(ch)		(DWCDMAC_CHAN(ch) + 0x40) /* config, low */
#define	DWCDMAC_CFG_HI(ch)	(DWCDMAC_CHAN(ch) + 0x44) /* config, high */
#define	DWCDMAC_SGR(ch)		(DWCDMAC_CHAN(ch) + 0x48)
#define	DWCDMAC_DSR(ch)		(DWCDMAC_CHAN(ch) + 0x50)

/*
 * Interrupt registers, one channel bit each.  RAW latches regardless of
 * MASK_*; STATUS_* is RAW & MASK; CLEAR_* is write-1-to-clear.
 */
#define	DWCDMAC_RAW_TFR		0x2c0	/* DMA (chain) transfer complete */
#define	DWCDMAC_RAW_BLOCK	0x2c8	/* block transfer complete */
#define	DWCDMAC_RAW_SRCTRAN	0x2d0
#define	DWCDMAC_RAW_DSTTRAN	0x2d8
#define	DWCDMAC_RAW_ERR		0x2e0	/* AHB error response */
#define	DWCDMAC_STATUS_TFR	0x2e8
#define	DWCDMAC_STATUS_BLOCK	0x2f0
#define	DWCDMAC_STATUS_SRCTRAN	0x2f8
#define	DWCDMAC_STATUS_DSTTRAN	0x300
#define	DWCDMAC_STATUS_ERR	0x308
#define	DWCDMAC_MASK_TFR	0x310	/* write-enable scheme, see below */
#define	DWCDMAC_MASK_BLOCK	0x318
#define	DWCDMAC_MASK_SRCTRAN	0x320
#define	DWCDMAC_MASK_DSTTRAN	0x328
#define	DWCDMAC_MASK_ERR	0x330
#define	DWCDMAC_CLEAR_TFR	0x338
#define	DWCDMAC_CLEAR_BLOCK	0x340
#define	DWCDMAC_CLEAR_SRCTRAN	0x348
#define	DWCDMAC_CLEAR_DSTTRAN	0x350
#define	DWCDMAC_CLEAR_ERR	0x358
#define	DWCDMAC_STATUSINT	0x360	/* combined status, read-only */

/* software handshake request registers; unused (hardware handshake) */
#define	DWCDMAC_REQ_SRC		0x368
#define	DWCDMAC_REQ_DST		0x370
#define	DWCDMAC_SGL_REQ_SRC	0x378
#define	DWCDMAC_SGL_REQ_DST	0x380
#define	DWCDMAC_LST_SRC		0x388
#define	DWCDMAC_LST_DST		0x390

#define	DWCDMAC_DMACFG		0x398	/* global configuration */
#define	DWCDMAC_DMACFG_EN	0x00000001
#define	DWCDMAC_CHEN		0x3a0	/* channel enable */
#define	DWCDMAC_ID		0x3a8
#define	DWCDMAC_TEST		0x3b0
/* 0x3c8-0x3f7: component parameter registers, unused */

/*
 * CHEN and MASK_* use a write-enable scheme: bits 15:8 select which of
 * the channel bits 7:0 a write actually affects.  A channel's CHEN bit
 * self-clears when its transfer (chain) completes.
 */
#define	DWCDMAC_CHANBIT(ch)	(1U << (ch))
#define	DWCDMAC_CH_ENABLE(ch)	(DWCDMAC_CHANBIT(ch) | (DWCDMAC_CHANBIT(ch) << 8))
#define	DWCDMAC_CH_DISABLE(ch)	(DWCDMAC_CHANBIT(ch) << 8)

/* CTL, low word */
#define	DWCDMAC_CTL_INT_EN		0x00000001
#define	DWCDMAC_CTL_DST_TRWID(w)	(((w) & 0x7) << 1)  /* 2 = 32-bit */
#define	DWCDMAC_CTL_SRC_TRWID(w)	(((w) & 0x7) << 4)
#define	DWCDMAC_CTL_DINC_INC		0x00000000
#define	DWCDMAC_CTL_DINC_DEC		0x00000080
#define	DWCDMAC_CTL_DINC_NOCHANGE	0x00000100
#define	DWCDMAC_CTL_SINC_INC		0x00000000
#define	DWCDMAC_CTL_SINC_DEC		0x00000200
#define	DWCDMAC_CTL_SINC_NOCHANGE	0x00000400
#define	DWCDMAC_CTL_DST_MSIZE(m)	(((m) & 0x7) << 11) /* 3 = 16 items */
#define	DWCDMAC_CTL_SRC_MSIZE(m)	(((m) & 0x7) << 14)
#define	DWCDMAC_CTL_TTFC(t)		(((t) & 0x7) << 20)
#define	DWCDMAC_TTFC_P2M_DMAC		0x2 /* dev->mem, DMAC flow control */
#define	DWCDMAC_TTFC_M2P_DMAC		0x1 /* mem->dev, DMAC flow control */
#define	DWCDMAC_TTFC_M2P_PER		0x3 /* mem->dev, periph flow control */
#define	DWCDMAC_CTL_DMS(m)		(((m) & 0x3) << 23) /* dst master */
#define	DWCDMAC_CTL_SMS(m)		(((m) & 0x3) << 25) /* src master */
#define	DWCDMAC_CTL_LLP_DST_EN		0x08000000 /* block chaining, dst */
#define	DWCDMAC_CTL_LLP_SRC_EN		0x10000000 /* block chaining, src */
/* CTL, high word */
#define	DWCDMAC_CTL_BLOCK_TS(items)	((items) & 0xfff)
#define	DWCDMAC_MAX_BLOCK_ITEMS		0x800	/* x4 bytes = 8KB per block */
#define	DWCDMAC_CTL_DONE	0x00001000 /* block done; written back to the
					      LLI ctl_hi in memory */

/* CFG, low word */
#define	DWCDMAC_CFG_CH_PRIOR(p)	(((p) & 0x7) << 5)
#define	DWCDMAC_CFG_CH_SUSP	0x00000100
#define	DWCDMAC_CFG_FIFO_EMPTY	0x00000200
#define	DWCDMAC_CFG_HS_SEL_DST	0x00000400 /* 1 = software handshake */
#define	DWCDMAC_CFG_HS_SEL_SRC	0x00000800
/* CFG, high word */
#define	DWCDMAC_CFG_FCMODE_REQ	0x00000001 /* prefetch only on request */
#define	DWCDMAC_CFG_FIFO_MODE	0x00000002 /* burst only when FIFO ready */
#define	DWCDMAC_CFG_PROTCTL	0x0000000c /* AHB hprot, per Linux */
#define	DWCDMAC_CFG_HS_SRC(i)	(((i) & 0xf) << 7)  /* hw handshake if */
#define	DWCDMAC_CFG_HS_DST(i)	(((i) & 0xf) << 11)

/*
 * Master select encodes 
 */
#define	DWCDMAC_MS_PERIPH	0
#define	DWCDMAC_MS_MEM		1

/*
 * In-memory linked list item, fetched by the DMAC through the master
 * named in the LMS bits.
 */
struct dwcdmac_lli {
	uint32_t sar;
	uint32_t dar;
	uint32_t llp;		/* phys addr of next item | LMS; 0 = last */
	uint32_t ctl_lo;
	uint32_t ctl_hi;
	uint32_t dstat_lo;
	uint32_t dstat_hi;
};

#endif	/* _IBM4XX_DWCDMACREG_H_ */
