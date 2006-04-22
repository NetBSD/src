/*	$NetBSD: gtidmareg.h,v 1.4.6.1 2006/04/22 11:39:09 simonb Exp $	*/

/*
 * Copyright (c) 2002 Allegro Networks, Inc., Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Allegro Networks, Inc., and Wasabi Systems, Inc.
 * 4. The name of Allegro Networks, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 * 5. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY ALLEGRO NETWORKS, INC. AND
 * WASABI SYSTEMS, INC. ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL EITHER ALLEGRO NETWORKS, INC. OR WASABI SYSTEMS, INC.
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * idmareg.h - register & descriptor definitions for GT-64260 IDMA
 *
 * creation	Wed Aug 15 08:58:37 PDT 2001	cliff
 */

#ifndef _IDMAREG_H
#define _IDMAREG_H

#define NIDMA_CHANS	8
#define IDMA_BOUNDARY	1024*1024         /* boundary for bus_dmamap_create */

#define IDMA_BIT(bitno)   (1U << (bitno))
#define IDMA_BITS(hi, lo) ((unsigned int)(~((~0ULL)<<((hi)+1)))&((~0)<<(lo)))

#define IDMA_WORD_SHIFT		2	/* log2(sizeof(u_int32_t) */


static volatile __inline unsigned int
idma_desc_read(u_int32_t *ip)
{
	u_int32_t rv;

	__asm volatile ("lwbrx %0,0,%1; eieio;"
		: "=r"(rv) : "r"(ip));
	return rv;
}

static volatile __inline void
idma_desc_write(u_int32_t *ip, u_int32_t val)
{
	__asm volatile ("stwbrx %0,0,%1; eieio;"
		:: "r"(val), "r"(ip));
}

typedef struct idma_desc {
	u_int32_t idd_ctl;
	u_int32_t idd_src_addr;
	u_int32_t idd_dst_addr;
	u_int32_t idd_next;
	u_int32_t idd_pad[4];	/* pad to CACHELINESIZE */
} idma_desc_t __attribute__ ((aligned(CACHELINESIZE)));

#define IDMA_DESC_CTL_CNT	IDMA_BITS(23,0)
#define IDMA_DESC_CTL_RES	IDMA_BITS(29,24)
#define IDMA_DESC_CTL_TERM	IDMA_BIT(30)
#define IDMA_DESC_CTL_OWN	IDMA_BIT(31)


#define IDMA_CH0_REG_OFF		0
#define IDMA_CH4_REG_OFF		0x100
#define IDMA_REG(chan, base) \
		((chan < 4) ? \
			((base) + ((chan) << IDMA_WORD_SHIFT)) : \
			(((base) + IDMA_CH4_REG_OFF) \
				 + (((chan) - 4) << IDMA_WORD_SHIFT)))

#define IDMA_2X4_REG(chan, base) \
		(((chan) < 4) ?  \
			(base) : \
			((base) + IDMA_CH4_REG_OFF))

/*
 * IDMA Descriptor Registers
 */
#define IDMA_CNT_REG_BASE	0x800
#define IDMA_CNT_REG(chan)	IDMA_REG(chan, IDMA_CNT_REG_BASE)
#define IDMA_SRC_REG_BASE	0x810
#define IDMA_SRC_REG(chan) 	IDMA_REG(chan, IDMA_SRC_REG_BASE)
#define IDMA_DST_REG_BASE	0x820
#define IDMA_DST_REG(chan) 	IDMA_REG(chan, IDMA_DST_REG_BASE)
#define IDMA_NXT_REG_BASE	0x830
#define IDMA_NXT_REG(chan) 	IDMA_REG(chan, IDMA_NXT_REG_BASE)
#define IDMA_CUR_REG_BASE	0x870
#define IDMA_CUR_REG(chan) 	IDMA_REG(chan, IDMA_CUR_REG_BASE)
#define IDMA_SRC_HIPCI_REG_BASE	0x890
#define IDMA_SRC_HIPCI_REG(chan) \
			 	IDMA_REG(chan, IDMA_SRC_HIPCI_REG_BASE)
#define IDMA_DST_HIPCI_REG_BASE	0x8a0
#define IDMA_DST_HIPCI_REG(chan) \
			 	IDMA_REG(chan, IDMA_DST_HIPCI_REG_BASE)
#define IDMA_NXT_HIPCI_REG_BASE	0x8b0
#define IDMA_NXT_HIPCI_REG(chan) \
			 	IDMA_REG(chan, IDMA_NXT_HIPCI_REG_BASE)

/*
 * IDMA Control Registers
 */
#define IDMA_CTLLO_REG_BASE	0x840
#define IDMA_CTLLO_REG(chan)	IDMA_REG(chan, IDMA_CTLLO_REG_BASE)
#define IDMA_CTLHI_REG_BASE	0x880
#define IDMA_CTLHI_REG(chan)	IDMA_REG(chan, IDMA_CTLHI_REG_BASE)
#define IDMA_ARB_REG_BASE	0x860
#define IDMA_ARB_REG(chan)	IDMA_2X4_REG(chan, IDMA_ARB_REG_BASE)
#define IDMA_XTO_REG_BASE	0x8d0
#define IDMA_XTO_REG(chan)	IDMA_2X4_REG(chan, IDMA_XTO_REG_BASE)
#define IDMA_CAUSE_REG_BASE	0x8c0
#define IDMA_CAUSE_REG(chan)	IDMA_2X4_REG(chan, IDMA_CAUSE_REG_BASE)
#define IDMA_MASK_REG_BASE	0x8c4
#define IDMA_MASK_REG(chan)	IDMA_2X4_REG(chan, IDMA_MASK_REG_BASE)
#define IDMA_EADDR_REG_BASE	0x8c8
#define IDMA_EADDR_REG(chan)	IDMA_2X4_REG(chan, IDMA_EADDR_REG_BASE)
#define IDMA_ESEL_REG_BASE	0x8cc
#define IDMA_ESEL_REG(chan)	IDMA_2X4_REG(chan, IDMA_ESEL_REG_BASE)

/*
 * IDMA Channel Control Lo bits
 */
#define IDMA_CTLLO_RESA		IDMA_BITS(2,0)
#define IDMA_CTLLO_SRCHOLD	IDMA_BIT(3)
#define IDMA_CTLLO_RESB		IDMA_BIT(4)
#define IDMA_CTLLO_DSTHOLD	IDMA_BIT(5)
#define IDMA_CTLLO_BURSTLIM	IDMA_BITS(8,6)
#define IDMA_CTLLO_BURST16	IDMA_BIT(6)
#define IDMA_CTLLO_BURST32	IDMA_BITS(7,6)
#define IDMA_CTLLO_BURST64	IDMA_BITS(8,6)
#define IDMA_CTLLO_NOCHAIN	IDMA_BIT(9)
#define IDMA_CTLLO_INTR		IDMA_BIT(10)
#define IDMA_CTLLO_BLKMODE	IDMA_BIT(11)
#define IDMA_CTLLO_ENB		IDMA_BIT(12)
#define IDMA_CTLLO_FETCHND	IDMA_BIT(13)
#define IDMA_CTLLO_ACTIVE	IDMA_BIT(14)
#define IDMA_CTLLO_REQDIR	IDMA_BIT(15)
#define IDMA_CTLLO_REQMODE	IDMA_BIT(16)
#define IDMA_CTLLO_CDEN		IDMA_BIT(17)
#define IDMA_CTLLO_EOTEN	IDMA_BIT(18)
#define IDMA_CTLLO_EOTMODE	IDMA_BIT(19)
#define IDMA_CTLLO_ABORT	IDMA_BIT(20)
#define IDMA_CTLLO_SADDROVR	IDMA_BITS(22,21)
#define IDMA_CTLLO_DADDROVR	IDMA_BITS(24,23)
#define IDMA_CTLLO_NADDROVR	IDMA_BITS(26,25)
#define IDMA_CTLLO_ACKMODE	IDMA_BIT(27)
#define IDMA_CTLLO_TREQ		IDMA_BIT(28)
#define IDMA_CTLLO_ACKDIR	IDMA_BITS(30,29)
#define IDMA_CTLLO_DESCMODE	IDMA_BIT(31)
#define IDMA_CTLLO_RES \
		(IDMA_CTLHI_RESA|IDMA_CHN_CTLHI_RESB)

#define IDMA_CTLL0_BURSTCODE(sz) (((sz) == 64) ? IDMA_CTLLO_BURST64 \
			       : (((sz) == 32) ? IDMA_CTLLO_BURST32 \
			       : (((sz) == 16) ? IDMA_CTLLO_BURST16 \
			       : ~0)))

/*
 * IDMA Channel Control Hi bits
 */
#define IDMA_CTLHI_RESA			IDMA_BITS(3,0)
#define IDMA_CTLHI_SRCPCISWAP		IDMA_BITS(5,4)
#define IDMA_CTLHI_SRCPCISWAP_NONE	IDMA_BIT(4)
#define IDMA_CTLHI_SRCSNOOP		IDMA_BIT(6)
#define IDMA_CTLHI_SRCPCI64		IDMA_BIT(7)
#define IDMA_CTLHI_RESB			IDMA_BITS(11,8)
#define IDMA_CTLHI_DSTPCISWAP		IDMA_BITS(13,12)
#define IDMA_CTLHI_DSTPCISWAP_NONE	IDMA_BIT(12)
#define IDMA_CTLHI_DSTSNOOP		IDMA_BIT(14)
#define IDMA_CTLHI_DSTPCI64		IDMA_BIT(15)
#define IDMA_CTLHI_RESC			IDMA_BITS(19,16)
#define IDMA_CTLHI_NXTPCISWAP		IDMA_BITS(21,20)
#define IDMA_CTLHI_NXTPCISWAP_NONE	IDMA_BIT(20)
#define IDMA_CTLHI_NXTSNOOP		IDMA_BIT(22)
#define IDMA_CTLHI_NXTPCI64		IDMA_BIT(23)
#define IDMA_CTLHI_RESD			IDMA_BITS(31,24)
#define IDMA_CTLHI_RES \
		(IDMA_CTLHI_RESA|IDMA_CHN_CTLHI_RESB \
		|IDMA_CTLHI_RESC|IDMA_CHN_CTLHI_RESD)


/*
 * IDMA Channel Arbiter Control bits
 */
#define IDMA_ARB_ARB_SHIFT	2
#define IDMA_ARB_ARB_MASK(arb) \
		(BITS(1,0) << ((arb) << IDMA_ARB_ARB_SHIFT))


/*
 * common IDMA Interrupt Register bits
 */
#define IDMA_INTR_COMP		IDMA_BIT(0)	/* completion */
#define IDMA_INTR_MISS		IDMA_BIT(1)	/* failed addr decode */
#define IDMA_INTR_ACCESS	IDMA_BIT(2)	/* access violation */
#define IDMA_INTR_WPROT		IDMA_BIT(3)	/* write protect */
#define IDMA_INTR_OWN		IDMA_BIT(4)	/* desc owner violation */
#define IDMA_INTR_EOT		IDMA_BIT(5)	/* end of xfer */
#define IDMA_INTR_RESX		IDMA_BITS(7,6)

#define IDMA_INTR_SHIFT	8
#define IDMA_MASK_BITS	IDMA_BITS(7,0)
#define IDMA_INTR_BITS	IDMA_BITS(5,0)
#define IDMA_INTR_ERRS	IDMA_BITS(5,1)
#define IDMA_INTR_ALL_ERRS	(IDMA_INTR_ERRS \
				|(IDMA_INTR_ERRS <<  IDMA_INTR_SHIFT) \
				|(IDMA_INTR_ERRS << (IDMA_INTR_SHIFT*2)) \
				|(IDMA_INTR_ERRS << (IDMA_INTR_SHIFT*3)))

/*
 * IDMA Interrupt Cause Register bits
 */
#define IDMA_CAUSE_RES	( IDMA_INTR_RESX \
			|(IDMA_INTR_RESX << IDMA_INTR_SHIFT) \
			|(IDMA_INTR_RESX << (IDMA_INTR_SHIFT * 2)) \
			|(IDMA_INTR_RESX << (IDMA_INTR_SHIFT * 3)))

/*
 * IDMA Interrupt Mask Register bits
 */
#define IDMA_MASK_SHIFT(chan) \
		(IDMA_INTR_SHIFT * ((chan < 4) ? chan : (chan - 4)))
#define IDMA_MASK(chan, mask) \
		(((mask) & IDMA_MASK_BITS) << IDMA_MASK_SHIFT(chan))


#define IDMA_MASK_RES	(IDMA_MASK_RESX(0) \
			|IDMA_MASK_RESX(1) \
			|IDMA_MASK_RESX(2) \
			|IDMA_MASK_RESX(3))

/*
 * IDMA Error Select Register bits
 */
#define IDMA_ESEL_MASK	IDMA_BITS(4,0)
#define IDMA_ESEL_RES	IDMA_BITS(31,5)


/*
 * IDMA Debug Registers
 */
#define IDMA_DBG_XADDR_REG_BASE	0x8e0
#define IDMA_DBG_XADDR(xno)	IDMA_2X4_REG(xno, IDMA_DBG_XADDR_REG_BASE)
#define IDMA_DBG_XCMD_REG_BASE	0x8e4
#define IDMA_DBG_XCMD(xno)	IDMA_2X4_REG(xno, IDMA_DBG_XCMD_REG_BASE)
#define IDMA_DBG_WDATLO_REG_BASE	0x8e8
#define IDMA_DBG_WDATLO(xno)	IDMA_2X4_REG(xno, IDMA_DBG_WDATLO_REG_BASE)
#define IDMA_DBG_WDATHI_REG_BASE	0x8ec
#define IDMA_DBG_WDATHI(xno)	IDMA_2X4_REG(xno, IDMA_DBG_WDATHI_REG_BASE)
#define IDMA_DBG_RDATLO_REG_BASE	0x8f0
#define IDMA_DBG_RDATLO(xno)	IDMA_2X4_REG(xno, IDMA_DBG_RDATLO_REG_BASE)
#define IDMA_DBG_RDATHI_REG_BASE	0x8f4
#define IDMA_DBG_RDATHI(xno)	IDMA_2X4_REG(xno, IDMA_DBG_RDATHI_REG_BASE)
#define IDMA_DBG_RID_REG_BASE	0x8fc
#define IDMA_DBG_RID(xno)	IDMA_2X4_REG(xno, IDMA_DBG_RID_REG_BASE)


#endif	/* _IDMAREG_H */
