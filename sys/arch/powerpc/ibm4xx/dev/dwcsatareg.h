/*	$NetBSD: dwcsatareg.h,v 1.1 2026/06/14 00:02:35 rkujawa Exp $	*/

/*
 * Copyright (c) 2026 The NetBSD Foundation, Inc.
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
 * Synopsys DesignWare Cores SATA-II AHB host as integrated in the
 * AMCC/AppliedMicro PPC460EX
 */

#ifndef _IBM4XX_DWCSATAREG_H_
#define	_IBM4XX_DWCSATAREG_H_

#define	DWCSATA_SIZE		0x800	/* whole block, incl. DMADR window */

/*
 * ATA taskfile shadow registers (SATA0_CDR0-CDR7), 4-byte stride, in
 * the standard wd_* order: data, error/features, seccnt, sector,
 * cyl_lo, cyl_hi, sdh, command/status.
 */
#define	DWCSATA_CDR_BASE	0x00
#define	DWCSATA_CDR_STRIDE	4

#define	DWCSATA_CLR0		0x20	/* altstatus / device control */

/* SATA status and control registers (SATA0_SCR0-SCR4) */
#define	DWCSATA_SSTATUS		0x24
#define	DWCSATA_SERROR		0x28	/* write 1s to clear */
#define	DWCSATA_SCONTROL	0x2c
#define	DWCSATA_SACTIVE		0x30
#define	DWCSATA_SNOTIFICATION	0x34

/*
 * DWC-specific control block. (NCQ) DMA registers and
 * DMACR/DBTSR only matter on the DMA data path.
 */
#define	DWCSATA_FPTAGR		0x64	/* first-party DMA tag */
#define	DWCSATA_FPBOR		0x68	/* first-party DMA buffer offset */
#define	DWCSATA_FPTCR		0x6c	/* first-party DMA transfer count */

#define	DWCSATA_DMACR		0x70	/* DMA channel control */
#define	DWCSATA_DMACR_TXCHEN	0x00000001 /* TX channel enable */
#define	DWCSATA_DMACR_RXCHEN	0x00000002 /* RX channel enable */
#define	DWCSATA_DMACR_TXMODE	0x00000004 /* close TX FIS on TXCHEN clear */
/* clears both channel enables; TXMODE is what U-Boot/Linux run with */
#define	DWCSATA_DMACR_TXRXCH_CLEAR DWCSATA_DMACR_TXMODE
/* TXMODE must accompany every DMACR write, including the RMW clears */
#define	DWCSATA_DMACR_TX_START	(DWCSATA_DMACR_TXCHEN | DWCSATA_DMACR_TXMODE)
#define	DWCSATA_DMACR_RX_START	(DWCSATA_DMACR_RXCHEN | DWCSATA_DMACR_TXMODE)
#define	DWCSATA_DMACR_TX_CLEAR(v) \
	(((v) & ~DWCSATA_DMACR_TXCHEN) | DWCSATA_DMACR_TXMODE)
#define	DWCSATA_DMACR_RX_CLEAR(v) \
	(((v) & ~DWCSATA_DMACR_RXCHEN) | DWCSATA_DMACR_TXMODE)

#define	DWCSATA_DBTSR		0x74	/* DMA burst transaction size */
#define	DWCSATA_DBTSR_MWR(bytes) (((bytes) / 4) & 0x01ff)
#define	DWCSATA_DBTSR_MRD(bytes) ((((bytes) / 4) & 0x01ff) << 16)

#define	DWCSATA_INTPR		0x78	/* interrupt pending, write 1 clears */
#define	DWCSATA_INTPR_DMAT	0x00000001 /* DMA transfer done */
#define	DWCSATA_INTPR_NEWFP	0x00000002 /* new first-party DMA FIS */
#define	DWCSATA_INTPR_PMABRT	0x00000004 /* power mgmt request aborted */
#define	DWCSATA_INTPR_ERR	0x00000008 /* SError & ERRMR nonzero */
#define	DWCSATA_INTPR_NEWBIST	0x00000010 /* BIST activate FIS received */
#define	DWCSATA_INTPR_PRIMERR	0x00000020 /* link layer primitive error */
#define	DWCSATA_INTPR_CMDABORT	0x00000040 /* Reg/SDB FIS w/ Status.ERR=1 */
#define	DWCSATA_INTPR_CMDGOOD	0x00000080 /* Reg/SDB FIS w/ Status.ERR=0 */
#define	DWCSATA_INTPR_ERRADDR	0x07ff0000 /* AHB slave bad-access addr bits:
					      haddr[21:31] of the offending
					      access (0x3ff = dma_finish_tx
					      with nothing in DMADR) */
#define	DWCSATA_INTPR_ERRADDR_GET(v) (((v) & DWCSATA_INTPR_ERRADDR) >> 16)
#define	DWCSATA_INTPR_IPF	0x10000000 /* ATA interrupt flag (D2H intrq) */

#define	DWCSATA_INTMR		0x7c	/* interrupt mask, 1 = enabled */
#define	DWCSATA_INTMR_DMATM	0x00000001
#define	DWCSATA_INTMR_NEWFPM	0x00000002
#define	DWCSATA_INTMR_PMABRTM	0x00000004
#define	DWCSATA_INTMR_ERRM	0x00000008
#define	DWCSATA_INTMR_NEWBISTM	0x00000010

#define	DWCSATA_ERRMR		0x80	/* SError bits allowed into INTPR_ERR */
#define	DWCSATA_ERRMR_ERR_BITS	0x0fff0f03 /* all error (non-DIAG) bits */

#define	DWCSATA_LLCR		0x84	/* link layer control */
#define	DWCSATA_LLCR_SCRAMEN	0x00000001 /* scrambler enable */
#define	DWCSATA_LLCR_DESCRAMEN	0x00000002 /* descrambler enable */
#define	DWCSATA_LLCR_RPDEN	0x00000004 /* random pattern (BIST) enable */

/* BIST registers (0x88-0xb4 area per the Synopsys layout) not listed */

#define	DWCSATA_TESTR		0xf4	/* test mode */
#define	DWCSATA_VERSIONR	0xf8	/* core version, 4 ASCII chars */
#define	DWCSATA_IDR		0xfc	/* core ID; reset value 0 on 460EX! */

/*
 * DMA data FIFO window
 */
#define	DWCSATA_DMADR		0x400
#define	DWCSATA_DMADR_PHYS(base) ((uint32_t)(base) + DWCSATA_DMADR)

#endif	/* _IBM4XX_DWCSATAREG_H_ */
