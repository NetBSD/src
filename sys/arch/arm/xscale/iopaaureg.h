/*	$NetBSD: iopaaureg.h,v 1.3 2002/08/03 21:58:56 thorpej Exp $	*/

/*
 * Copyright (c) 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _XSCALE_IOPAAUREG_H_
#define	_XSCALE_IOPAAUREG_H_

/*
 * The AAU can transfer a maximum of 16MB.
 *
 * XXX The DMA maps are > 32K if we allow the maximum number
 * XXX of DMA segments (16MB / 4K + 1) -- fix this bus_dma
 * XXX problem.
 */
#define	AAU_MAX_XFER	(16U * 1024 * 1024)
#if 0
#define	AAU_MAX_SEGS	((AAU_MAX_XFER / PAGE_SIZE) + 1)
#else
#define	AAU_MAX_SEGS	1024
#endif

/*
 * AAU I/O descriptor for operations with various numbers of inputs.
 * Note that all descriptors must be 8-word (32-byte) aligned.
 */
struct aau_desc_4 {
	struct aau_desc_4 *d_next;	/* pointer to next (va) */
	uint32_t d_pa;			/* our physical address */

	/* Hardware portion -- must be 32-byte aligned. */
	uint32_t	d_nda;		/* next descriptor address */
	uint32_t	d_sar[4];	/* source address */
	uint32_t	d_dar;		/* destination address */
	uint32_t	d_bc;		/* byte count */
	uint32_t	d_dc;		/* descriptor control */
} __attribute__((__packed__));

struct aau_desc_8 {
	struct aau_desc_8 *d_next;	/* pointer to next (va) */
	uint32_t d_pa;			/* our physical address */

	/* Hardware portion -- must be 32-byte aligned. */
	uint32_t	d_nda;		/* next descriptor address */
	uint32_t	d_sar[4];	/* source address */
	uint32_t	d_dar;		/* destination address */
	uint32_t	d_bc;		/* byte count */
	uint32_t	d_dc;		/* descriptor control */
	/* Mini Descriptor */
	uint32_t	d_sar5_8[4];	/* source address */
} __attribute__((__packed__));

struct aau_desc_16 {
	struct aau_desc_16 *d_next;	/* pointer to next (va) */
	uint32_t d_pa;			/* our physical address */

	/* Hardware portion -- must be 32-byte aligned. */
	uint32_t	d_nda;		/* next descriptor address */
	uint32_t	d_sar[4];	/* source address */
	uint32_t	d_dar;		/* destination address */
	uint32_t	d_bc;		/* byte count */
	uint32_t	d_dc;		/* descriptor control */
	/* Mini Descriptor */
	uint32_t	d_sar5_8[4];	/* source address */
	/* Extended Descriptor 0 */
	uint32_t	d_edc0;		/* ext. descriptor control */
	uint32_t	d_sar9_16[8];	/* source address */
} __attribute__((__packed__));

struct aau_desc_32 {
	struct aau_desc_32 *d_next;	/* pointer to next (va) */
	uint32_t d_pa;			/* our physical address */

	/* Hardware portion -- must be 32-byte aligned. */
	uint32_t	d_nda;		/* next descriptor address */
	uint32_t	d_sar[4];	/* source address */
	uint32_t	d_dar;		/* destination address */
	uint32_t	d_bc;		/* byte count */
	uint32_t	d_dc;		/* descriptor control */
	/* Mini Descriptor */
	uint32_t	d_sar5_8[4];	/* source address */
	/* Extended Descriptor 0 */
	uint32_t	d_edc0;		/* ext. descriptor control */
	uint32_t	d_sar9_16[8];	/* source address */
	/* Extended Descriptor 1 */
	uint32_t	d_edc1;		/* ext. descriptor control */
	uint32_t	d_sar17_24[8];	/* source address */
	/* Extended Descriptor 2 */
	uint32_t	d_edc2;		/* ext. descriptor control */
	uint32_t	d_sar25_32[8];	/* source address */
} __attribute__((__packed__));

#define	AAU_DESC_SIZE(ninputs)						\
	((ninputs > 16) ? sizeof(struct aau_desc_32) :			\
	 (ninputs > 8) ? sizeof(struct aau_desc_16) :			\
	 (ninputs > 4) ? sizeof(struct aau_desc_8) :			\
	 sizeof(struct aau_desc_4))

#define	SYNC_DESC_4_OFFSET	offsetof(struct aau_desc_4, d_nda)
#define	SYNC_DESC_4_SIZE	(sizeof(struct aau_desc_4) - SYNC_DESC_4_OFFSET)

#define	SYNC_DESC(d, size)						\
	cpu_dcache_wbinv_range(((vaddr_t)(d)) + SYNC_DESC_4_OFFSET, (size))

/* Descriptor control */
#define	AAU_DC_IE		(1U << 0)	/* interrupt enable */
#define	AAU_DC_B1_CC(x)		((x) << 1)	/* block command/control */
#define	AAU_DC_B2_CC(x)		((x) << 4)
#define	AAU_DC_B3_CC(x)		((x) << 7)
#define	AAU_DC_B4_CC(x)		((x) << 10)
#define	AAU_DC_B5_CC(x)		((x) << 13)
#define	AAU_DC_B6_CC(x)		((x) << 16)
#define	AAU_DC_B7_CC(x)		((x) << 19)
#define	AAU_DC_B8_CC(x)		((x) << 22)
#define	AAU_DC_SBCI_5_8		(1U << 25)	/* SAR5-SAR8 valid */
#define	AAU_DC_SBCI_5_16	(2U << 25)	/* SAR5-SAR16 valid */
#define	AAU_DC_SBCI_5_32	(3U << 25)	/* SAR5-SAR32 valid */
#define	AAU_DC_TC		(1U << 28)	/* transfer complete */
#define	AAU_DC_PERR		(1U << 29)	/* parity check error */
#define	AAU_DC_PENB		(1U << 30)	/* parity check enable */
#define	AAU_DC_DWE		(1U << 31)	/* destination write enable */

#define	AAU_DC_CC_NULL		0		/* null command */
#define	AAU_DC_CC_XOR		1U		/* XOR command */
#define	AAU_DC_CC_FILL		2U		/* fill command */
#define	AAU_DC_CC_DIRECT_FILL	7U		/* direct fill (copy) */

/* Extended descriptor control */
#define	AAU_EDC_B1_CC(x)	((x) << 1)	/* block command/control */
#define	AAU_EDC_B2_CC(x)	((x) << 4)	/* block command/control */
#define	AAU_EDC_B3_CC(x)	((x) << 7)	/* block command/control */
#define	AAU_EDC_B4_CC(x)	((x) << 10)	/* block command/control */
#define	AAU_EDC_B5_CC(x)	((x) << 13)	/* block command/control */
#define	AAU_EDC_B6_CC(x)	((x) << 16)	/* block command/control */
#define	AAU_EDC_B7_CC(x)	((x) << 19)	/* block command/control */
#define	AAU_EDC_B8_CC(x)	((x) << 22)	/* block command/control */

/* Hardware registers */
#define	AAU_ACR		0x00		/* accelerator control */
#define	AAU_ASR		0x04		/* accelerator status */
#define	AAU_ADAR	0x08		/* descriptor address */
#define	AAU_ANDAR	0x0c		/* next descriptor address */
#define	AAU_DAR		0x20		/* destination address */
#define	AAU_ABCR	0x24		/* byte count */
#define	AAU_ADCR	0x28		/* descriptor control */
/* i80321 only */
#define	AAU_EDCR0	0x3c		/* extended descriptor control 0 */
#define	AAU_EDCR1	0x60		/* extended descriptor control 1 */
#define	AAU_EDCR2	0x84		/* extended descriptor control 2 */

#define	AAU_ACR_AAE	(1U << 0)	/* accelerator enable */
#define	AAU_ACR_CR	(1U << 1)	/* chain resume */
#define	AAU_ACR_512	(1U << 2)	/* 512-byte buffer enable */

#define	AAU_ASR_MA	(1U << 5)	/* master abort */
#define	AAU_ASR_ECIF	(1U << 8)	/* end of chain interrupt */
#define	AAU_ASR_ETIF	(1U << 9)	/* end of transfer interrupt */
#define	AAU_ASR_AAF	(1U << 10)	/* acellerator active */

#define	AAU_ABCR_MASK	0x00ffffff	/* 24-bit count */

#endif /* _XSCALE_IOPAAUREG_H_ */
