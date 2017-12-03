/*	$NetBSD: etsecreg.h,v 1.5.2.1 2017/12/03 11:36:37 jdolecek Exp $	*/
/*-
 * Copyright (c) 2010, 2011 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Raytheon BBN Technologies Corp and Defense Advanced Research Projects
 * Agency and which was developed by Matt Thomas of 3am Software Foundry.
 *
 * This material is based upon work supported by the Defense Advanced Research
 * Projects Agency and Space and Naval Warfare Systems Center, Pacific, under
 * Contract No. N66001-09-C-2073.
 * Approved for Public Release, Distribution Unlimited
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

#ifndef _POWERPC_BOOKE_ETSEC_REG_H_
#define _POWERPC_BOOKE_ETSEC_REG_H_

#define	TXBD_R		0x8000		/* Ready (HW Owned) */
#define	TXBD_PADCRC	0x4000		/* B: Pad+CRC */
#define	TXBD_W		0x2000		/* B: Wrap (End of Ring) */
#define	TXBD_I		0x1000		/* B: Interrupt (IEVENT[TXB|TXF]) */
#define	TXBD_L		0x0800		/* B: Last */
#define	TXBD_TC		0x0400		/* B: Tx CRC. (Add CRC) */
#define	TXBD_PRE	0x0200		/* B: custom preamble */
#define	TXBD_DEF	TXBD_PRE	/* A: transmit deferred */
#define	TXBD_HFE	0x0080		/* B: Huge Frame Enable */
#define	TXBD_LC		TXBD_HFE	/* A: Late Coll */
#define	TXBD_CF		0x0040		/* B: Control Frame */
#define	TXBD_RL		TXBD_CF		/* A: Tx Limit */
#define	TXBD_RC		0x003c		/* A: Retry Count */
#define	TXBD_TOE	0x0002		/* B: TOE frame */
#define	TXBD_UN		TXBD_TOE	/* A: Underrun */
#define	TXBD_TR		0x0001		/* A: Truncation */
#define	TXBD_ERRORS	(TXBD_LC|TXBD_RL|TXBD_UN|TXBD_TR)

struct txbd {
	uint16_t txbd_flags;
	uint16_t txbd_len;
	uint32_t txbd_bufptr;
};

#define	TXFCB_VLN	0x8000	/* VLaN control word valid */
#define	TXFCB_IP	0x4000	/* Layer 3 header is an IP header */
#define	TXFCB_IP6	0x2000	/* IP Header is IPv6 */
#define	TXFCB_TUP	0x1000	/* Layer 4 is TCP or UDP */
#define	TXFCB_UDP	0x0800	/* UDP at layer 4 */
#define	TXFCB_CIP	0x0400	/* Checksum IP header enable */
#define	TXFCB_CTU	0x0200	/* Checksum TCP or UCP header enable */
#define	TXFCB_NPH	0x0100	/* No std Pseudo-Header checksm, use phcs */

struct txfcb {
	uint16_t txfcb_flags;
	uint8_t txfcb_l4os;	/* layer 4 hdr from start of layer 3 header */
	uint8_t txfcb_l3os;	/* layer 3 hdr from start of layer 2 header */
	uint16_t txfcb_phcs;	/* pseudo-header checksum for NPH */
	uint16_t txfcb_vlctl;	/* vlan control word for insertion for VLN */
};

#define	RXBD_E		0x8000		/* Empty (1 = Owned by ETSEC) */
#define	RXBD_RO1	0x4000		/* S/W Ownership */
#define	RXBD_W		0x2000		/* Wrap ring. */
#define	RXBD_I		0x1000		/* Interrupt IEVENT[RXB|RXF] */
#define	RXBD_L		0x0800		/* Last in frame */
#define	RXBD_F		0x0400		/* First in frame */
#define	RXBD_M		0x0100		/* Miss (promiscious match) */
#define	RXBD_BC		0x0080		/* BroadCast match */
#define	RXBD_MC		0x0040		/* MultiCast match */
#define	RXBD_LG		0x0020		/* rx LarGe frame error */
#define	RXBD_NO		0x0010		/* Non-octect aligned frame error */
#define	RXBD_SH		0x0008		/* SHort frame */
#define	RXBD_CR		0x0004		/* rx CRc error */
#define	RXBD_OV		0x0002		/* OVerrun error */
#define	RXBD_TR		0x0001		/* TRuncation error */
#define	RXBD_ERRORS	0x003f

struct rxbd {
	uint16_t rxbd_flags;
	uint16_t rxbd_len;
	uint32_t rxbd_bufptr;
};

#define	RXFCB_VLN	0x8000	/* VLaN tag recognized */
#define	RXFCB_IP	0x4000	/* IP header found at layer 3 */
#define	RXFCB_IP6	0x2000	/* IPv6 header found */
#define	RXFCB_TUP	0x1000	/* TCP or UDP header found */
#define	RXFCB_CIP	0x0800	/* IPv4 checksum performed */
#define	RXFCB_CTU	0x0400	/* TCP or UDP checksum checked */
#define	RXFCB_EIP	0x0200	/* IPv4 header checksum error */
#define	RXFCB_ETU	0x0100	/* TCP or UDP header checksum error */
#define	RXFCB_HASH_VAL	0x0010	/* FLR_HASH value is valid */
#define	RXFCB_PERR	0x000c	/* Parse Error */
#define	RXFCB_PERR_L3	0x0008	/* L3 Parse Error */

struct rxfcb {
	uint16_t rxfcb_flags;
	uint8_t rxfcb_rq;		/* receive queue index */
	uint8_t rxfcb_pro;		/* IP Protocol received */
	uint16_t rxfcb_flr_hash;	/* filer hash value */
	uint16_t rxfcb_vlctl;		/* VLAN control field */
};

/* 0x000-0x0ff eTSEC general control/status registers */
#define	TSEC_ID		0x000	/* Controller ID register */
#define	TSEC_ID2	0x004	/* Controller ID register */
#define	IEVENT		0x010	/* Interrupt event register */
#define	IEVENT_BABR	__PPCBIT(0)	/* babbling receive error */
#define	IEVENT_RXC	__PPCBIT(1)	/* receive control interrupt */
#define	IEVENT_BSY	__PPCBIT(2)	/* busy condition interrupt */
#define	IEVENT_EBERR	__PPCBIT(3)	/* internal bus error */
#define	IEVENT_MSR0	__PPCBIT(5)	/* MIB counter overflow */
#define	IEVENT_GTSC	__PPCBIT(6)	/* graceful transmit stop complete */
#define	IEVENT_BABT	__PPCBIT(7)	/* babbing transmit error */
#define	IEVENT_TXC	__PPCBIT(8)	/* transmit control interrupt */
#define	IEVENT_TXE	__PPCBIT(9)	/* transmit error */
#define	IEVENT_TXB	__PPCBIT(10)	/* transmit buffer */
#define	IEVENT_TXF	__PPCBIT(11)	/* transmit frame interrupt */
#define	IEVENT_LC	__PPCBIT(13)	/* late collision */
#define	IEVENT_CRL	__PPCBIT(14)	/* collision retry limit */
#define	IEVENT_XFUN	__PPCBIT(15)	/* transmit fifo underrun */
#define	IEVENT_RXB	__PPCBIT(16)	/* receive buffer */
#define	IEVENT_TWK	__PPCBIT(19)	/* timer wakeup */
#define	IEVENT_MAG	__PPCBIT(20)	/* magic packet detected */
#define	IEVENT_MMRD	__PPCBIT(21)	/* MMI manangement read complete */
#define	IEVENT_MMWR	__PPCBIT(22)	/* MMI manangement write complete */
#define	IEVENT_GRSC	__PPCBIT(23)	/* graceful receive stop complete */
#define	IEVENT_RXF	__PPCBIT(24)	/* receive frame interrupt */
#define	IEVENT_FGPI	__PPCBIT(27)	/* filer generated general purpose interrupt */
#define	IEVENT_FIR	__PPCBIT(28)	/* receive queue filer is invalid */
#define	IEVENT_FIQ	__PPCBIT(29)	/* filed frame to invalid receive queue */
#define	IEVENT_DPE	__PPCBIT(30)	/* internal data parity error */
#define	IEVENT_PERR	__PPCBIT(31)	/* Receive parse error for TOE */
#define	IMASK		0x014	/* Interrupt mask register */
#define	EDIS		0x018	/* error disabled register */
#define	EMAPG		0x01c	/* group eror mapping register */
#define	ECNTRL		0x020	/* ethernet control register */
#define	ECNTRL_FIFM	__PPCBIT(16)	/* FIFO mode enable */
#define	ECNTRL_CLRCNT	__PPCBIT(17)	/* Clear all MIB counters */
#define	ECNTRL_AUTOZ	__PPCBIT(18)	/* Auto zero MIB counter on read */
#define	ECNTRL_STEN	__PPCBIT(19)	/* MIB Statistics Enabled */
#define	ECNTRL_GMIIM	__PPCBIT(25)	/* GMII Interface Mode */
#define	ECNTRL_TBIM	__PPCBIT(26)	/* Ten-Bit Interface Mode */
#define	ECNTRL_RPM	__PPCBIT(27)	/* Reduced Pin Mode */
#define	ECNTRL_R100M	__PPCBIT(28)	/* RGMII/RMII 100 Mode */
#define	ECNTRL_RMM	__PPCBIT(29)	/* Reduced Pin Mode for 10/100 */
#define	ECNTRL_SGMIIM	__PPCBIT(30)	/* SGMII Interface Mode */
#define ECNTRL_DEFAULT	ECNTRL_STEN

#define	PTV		0x028	/* Pause time value register */
#define	DMACTRL		0x02c	/* DMA control register */
#define	DMACTRL_LE	__PPCBIT(16)	/* Little Endian Descriptor Mode */
#define	DMACTRL_TDSEN	__PPCBIT(24)	/* TX Data Snoop enable */
#define	DMACTRL_TBDSEN	__PPCBIT(25)	/* TxBD Data Snoop enable */
#define	DMACTRL_GRS	__PPCBIT(27)	/* graceful receive stop */
#define	DMACTRL_GTS	__PPCBIT(28)	/* graceful transmit stop */
#define	DMACTRL_TOD	__PPCBIT(29)	/* Transmi On Demand for TxBD ring 0 */
#define	DMACTRL_WWR	__PPCBIT(30)	/* Write With Response */
#define	DMACTRL_WOP	__PPCBIT(31)	/* Wait or pool for TxBD ring 0 */
#define	DMACTRL_DEFAULT	(DMACTRL_WOP|DMACTRL_WWR|DMACTRL_TDSEN|DMACTRL_TBDSEN)

#define	TBIPA		0x030	/* TBI phy address register */

/* 0x100-0x2ff eTSEC transmit control/status registers */

#define TCTRL		0x100 /* Transmit control register */
#define TCTRL_IPCSEN	__PPCBIT(17) /* IP header checksum generation enable */
#define TCTRL_TUCSEN	__PPCBIT(18) /* TCP/UDP header checksum generation enable */
#define TCTRL_VLINS	__PPCBIT(19) /* VLAN tag insertion */
#define TCTRL_THDF	__PPCBIT(29) /* Transmit half duplex */
#define TCTRL_RFC_PAUSE	__PPCBIT(27) /* receive flow control pause frame */
#define TCTRL_TFC_PAUSE	__PPCBIT(28) /* transmit flow control pause frame */
#define TCTRL_TXSCHED	__PPCBITS(29,30) /* transmit ring scheduling algorithm */
#define TCTRL_TXSCHED_SINGLE	__SHIFTIN(0,TCTRL_TXSCHED)
#define TCTRL_TXSCHED_PRIO	__SHIFTIN(1,TCTRL_TXSCHED)
#define TCTRL_TXSCHED_MWRR	__SHIFTIN(2,TCTRL_TXSCHED)

#define TSTAT		0x104 /* Transmit status register */
#define	TSTAT_THLT0	__PPCBIT(0) /* transmit halt of ring 0 */
#define	TSTAT_THLT1	__PPCBIT(1)
#define	TSTAT_THLT2	__PPCBIT(2)
#define	TSTAT_THLT3	__PPCBIT(3)
#define	TSTAT_THLT4	__PPCBIT(4)
#define	TSTAT_THLT5	__PPCBIT(5)
#define	TSTAT_THLT6	__PPCBIT(6)
#define	TSTAT_THLT7	__PPCBIT(7)
#define	TSTAT_THLTn(n)	(TSTAT_THLT0 >> (n))
#define	TSTAT_THLT	__PPCBITS(0,7)
#define	TSTAT_TXF0	__PPCBIT(16) /* transmit frame event occurred on ring 0 */
#define	TSTAT_TXF1	__PPCBIT(17)
#define	TSTAT_TXF2	__PPCBIT(18)
#define	TSTAT_TXF3	__PPCBIT(19)
#define	TSTAT_TXF4	__PPCBIT(20)
#define	TSTAT_TXF5	__PPCBIT(21)
#define	TSTAT_TXF6	__PPCBIT(22)
#define	TSTAT_TXF7	__PPCBIT(23)
#define	TSTAT_TXF	__PPCBITS(16,23)
#define	TSTAT_TXFn(n)	(TSTAT_TXF0 >> (n))
#define DFVLAN		0x108 /* Default VLAN control word [TSEC3] */
#define TXIC		0x110 /* Transmit interrupt coalescing register */
#define	TXIC_ICEN	__PPCBIT(0) /* Interrupt coalescing enable */
#define	TXIC_ICCS	__PPCBIT(1) /* Interrupt coalescing timer clock source */
#define	TXIC_ICCS_ETSEC		0         /* eTSEC Tx interface clocks */
#define	TXIC_ICCS_SYSTEM	TXIC_ICCS /* system clocks */
#define	TXIC_ICFT	__PPCBITS(3,10)
#define	TXIC_ICFT_SET(n)	__SHIFTIN((n),TXIC_ICFT)
#define	TXIC_ICTT	__PPCBITS(16,31)
#define	TXIC_ICTT_SET(n)	__SHIFTIN((n),TXIC_ICTT)
#define TQUEUE		0x114 /* Transmit queue control register [TSEC3] */
#define	TQUEUE_EN0	__PPCBIT(16) /* transmit ring enabled */
#define	TQUEUE_EN1	__PPCBIT(17)
#define	TQUEUE_EN2	__PPCBIT(18)
#define	TQUEUE_EN3	__PPCBIT(19)
#define	TQUEUE_EN4	__PPCBIT(20)
#define	TQUEUE_EN5	__PPCBIT(21)
#define	TQUEUE_EN6	__PPCBIT(22)
#define	TQUEUE_EN7	__PPCBIT(23)
#define	TQUEUE_EN	__PPCBITS(16,23)
#define	TQUEUE_ENn(n)	(TQUEUE_EN0 >> (n))
#define TR03WT		0x140 /* TxBD Rings 0-3 round-robin weightings [TSEC3] */
#define TR47WT		0x144 /* TxBD Rings 4-7 round-robin weightings [TSEC3] */
#define TBDBPH		0x180 /* Tx data buffer pointer high bits [TSEC3] */
#define TBPTR0		0x184 /* TxBD pointer for ring 0 */
#define TBPTR1		0x18C /* TxBD pointer for ring 1 [TSEC3] */
#define TBPTR2		0x194 /* TxBD pointer for ring 2 [TSEC3] */
#define TBPTR3		0x19C /* TxBD pointer for ring 3 [TSEC3] */
#define TBPTR4		0x1A4 /* TxBD pointer for ring 4 [TSEC3] */
#define TBPTR5		0x1AC /* TxBD pointer for ring 5 [TSEC3] */
#define TBPTR6		0x1B4 /* TxBD pointer for ring 6 [TSEC3] */
#define TBPTR7		0x1BC /* TxBD pointer for ring 7 [TSEC3] */
#define	TBPTRn(n)	(TBPTR0 + 8*(n))
#define TBASEH		0x200 /* TxBD base address high bits [TSEC3] */
#define TBASE0		0x204 /* TxBD base address of ring 0 */
#define TBASE1		0x20C /* TxBD base address of ring 1 [TSEC3] */
#define TBASE2		0x214 /* TxBD base address of ring 2 [TSEC3] */
#define TBASE3		0x21C /* TxBD base address of ring 3 [TSEC3] */
#define TBASE4		0x224 /* TxBD base address of ring 4 [TSEC3] */
#define TBASE5		0x22C /* TxBD base address of ring 5 [TSEC3] */
#define TBASE6		0x234 /* TxBD base address of ring 6 [TSEC3] */
#define TBASE7		0x23C /* TxBD base address of ring 7 [TSEC3] */
#define	TBASEn(n)	(TBASE0 + 8*(n))
#define TMR_TXTS1_ID	0x280 /* Tx time stamp identification tag (set 1) [TSEC3] */
#define TMR_TXTS2_ID	0x284 /* Tx time stamp identification tag (set 2) [TSEC3] */
#define TMR_TXTS1_H	0x2C0 /* Tx time stamp high (set 1) [TSEC3] */
#define TMR_TXTS1_L	0x2C4 /* Tx time stamp high (set 1) [TSEC3] */
#define TMR_TXTS2_H	0x2C8 /* Tx time stamp high (set 2) [TSEC3] */
#define TMR_TXTS2_L	0x2CC /* Tx time stamp high (set 2) [TSEC3] */

/* 0x300-0x4ff eTSEC receive control/status registers */

#define RCTRL		0x300 /* Receive control register */
#define	RCTRL_L2OFF	__PPCBITS(0,6)
#define	RCTRL_L2OFF_SET(n)	__SHIFTIN((n),RCTRL_L2OFF)
#define	RCTRL_TS	__PPCBIT(7)
#define	RCTRL_RR	__PPCBIT(10)
#define	RCTRL_PAL	__PPCBITS(11,15)
#define	RCTRL_VLEX	__PPCBIT(18)
#define	RCTRL_FILREN	__PPCBIT(19)
#define	RCTRL_FSQEN	__PPCBIT(20)
#define	RCTRL_GHTX	__PPCBIT(21)
#define	RCTRL_IPCSEN	__PPCBIT(22)
#define	RCTRL_TUCSEN	__PPCBIT(23)
#define	RCTRL_PRSDEP	__PPCBITS(24,25)
#define	RCTRL_PRSDEP_L4		__SHIFTIN(3,RCTRL_PRSDEP)
#define	RCTRL_PRSDEP_L3		__SHIFTIN(2,RCTRL_PRSDEP)
#define	RCTRL_PRSDEP_L2		__SHIFTIN(1,RCTRL_PRSDEP)
#define	RCTRL_PRSDEP_OFF	__SHIFTIN(0,RCTRL_PRSDEP)
#define	RCTRL_BC_REJ	__PPCBIT(27)
#define	RCTRL_PROM	__PPCBIT(28)
#define	RCTRL_RSF	__PPCBIT(29)
#define	RCTRL_EMEN	__PPCBIT(30)
#define	RCTRL_DEFAULT	(__SHIFTIN(2, RCTRL_PAL)|RCTRL_EMEN)
#define RSTAT		0x304 /* Receive status register */
#define	RSTAT_QHLT0	__PPCBIT(8) /* receive halt of ring 0 */
#define	RSTAT_QHLT1	__PPCBIT(9)
#define	RSTAT_QHLT2	__PPCBIT(10)
#define	RSTAT_QHLT3	__PPCBIT(11)
#define	RSTAT_QHLT4	__PPCBIT(12)
#define	RSTAT_QHLT5	__PPCBIT(13)
#define	RSTAT_QHLT6	__PPCBIT(14)
#define	RSTAT_QHLT7	__PPCBIT(15)
#define	RSTAT_QHLTn(n)	(RSTAT_QHLT0 >> (n))
#define	RSTAT_QHLT	__PPCBITS(8,15)
#define	RSTAT_RXF0	__PPCBIT(24) /* receive frame event occurred on ring 0 */
#define	RSTAT_RXF1	__PPCBIT(25)
#define	RSTAT_RXF2	__PPCBIT(26)
#define	RSTAT_RXF3	__PPCBIT(27)
#define	RSTAT_RXF4	__PPCBIT(28)
#define	RSTAT_RXF5	__PPCBIT(29)
#define	RSTAT_RXF6	__PPCBIT(30)
#define	RSTAT_RXF7	__PPCBIT(31)
#define	RSTAT_RXF	__PPCBITS(24,31)
#define	RSTAT_RXFn(n)	(RSTAT_RXF0 >> (n))
#define RXIC		0x310 /* Receive interrupt coalescing register */
#define	RXIC_ICEN	__PPCBIT(0) /* Interrupt coalescing enable */
#define	RXIC_ICCS	__PPCBIT(1) /* Interrupt coalescing timer clock source */
#define	RXIC_ICCS_ETSEC		0         /* eTSEC Rx interface clocks */
#define	RXIC_ICCS_SYSTEM	TXIC_ICCS /* system clocks */
#define	RXIC_ICFT	__PPCBITS(3,10)
#define	RXIC_ICFT_SET(n)	__SHIFTIN((n),TXIC_ICFT)
#define	RXIC_ICTT	__PPCBITS(16,31)
#define	RXIC_ICTT_SET(n)	__SHIFTIN((n),TXIC_ICTT)
#define RQUEUE		0x314 /* Receive queue control register. [TSEC3] */
#define	RQUEUE_EX0	__PPCBIT(8) /* data transferred by DMA to ring extracted according to ATTR register */
#define	RQUEUE_EX1	__PPCBIT(9)
#define	RQUEUE_EX2	__PPCBIT(10)
#define	RQUEUE_EX3	__PPCBIT(11)
#define	RQUEUE_EX4	__PPCBIT(12)
#define	RQUEUE_EX5	__PPCBIT(13)
#define	RQUEUE_EX6	__PPCBIT(14)
#define	RQUEUE_EX7	__PPCBIT(15)
#define	RQUEUE_EXn(n)	(RQUEUE_EX0 >> (n))
#define	RQUEUE_EX	__PPCBITS(8,15)
#define	RQUEUE_EN0	__PPCBIT(24) /* ring is queried for reception */
#define	RQUEUE_EN1	__PPCBIT(25)
#define	RQUEUE_EN2	__PPCBIT(26)
#define	RQUEUE_EN3	__PPCBIT(27)
#define	RQUEUE_EN4	__PPCBIT(28)
#define	RQUEUE_EN5	__PPCBIT(29)
#define	RQUEUE_EN6	__PPCBIT(30)
#define	RQUEUE_EN7	__PPCBIT(31)
#define	RQUEUE_EN	__PPCBITS(24,31)
#define	RQUEUE_ENn(n)	(RQUEUE_EN0 >> (n))
#define	RIR0		0x318	/* Ring mapping register 0 */
#define	RIR1		0x31c	/* Ring mapping register 1 */
#define	RIR2		0x320	/* Ring mapping register 2 */
#define	RIR3		0x324	/* Ring mapping register 3 */
#define	RIRn(n)		(RIR0 + 4*(n))
#define RBIFX		0x330 /* Receive bit field extract control register [TSEC3] */
#define RQFAR		0x334 /* Receive queue filing table address register [TSEC3] */
#define RQFCR		0x338 /* Receive queue filing table control register [TSEC3] */
#define RQFCR_GPI	__PPCBIT(0) /* General purpose interrupt */
#define RQFCR_HASHTBL	__PPCBITS(12,14) /* Select between filer Q value and RIR fileds. */
#define RQFCR_HASHTBL_Q	__SHIFTIN(0,RQFCR_HASHTBL)
#define RQFCR_HASHTBL_0	__SHIFTIN(1,RQFCR_HASHTBL)
#define RQFCR_HASHTBL_1	__SHIFTIN(2,RQFCR_HASHTBL)
#define RQFCR_HASHTBL_2	__SHIFTIN(3,RQFCR_HASHTBL)
#define RQFCR_HASHTBL_3	__SHIFTIN(4,RQFCR_HASHTBL)
#define RQFCR_HASH	__PPCBIT(15) /* Include parser results in hash */
#define RQFCR_QUEUE	__PPCBITS(16,21) /* Receive queue index */
#define RQFCR_QUEUE_SET(n)	__SHIFTIN((n),RQFCR_QUEUE)
#define RQFCR_CLE	__PPCBIT(22) /* Cluster entry/exit */
#define RQFCR_REJ	__PPCBIT(23) /* Reject frame */
#define RQFCR_AND	__PPCBIT(24) /* AND, in combination with CLE, REJ, and PID match */
#define RQFCR_CMP	__PPCBITS(25,26) /* Comparison operation to perform on the RQPROP entry at this index when PID > 0 */
#define RQFCR_CMP_EXACT	__SHIFTIN(0,RQFCR_CMP)
#define RQFCR_CMP_MATCH	__SHIFTIN(1,RQFCR_CMP)
#define RQFCR_CMP_NOEXACT	__SHIFTIN(2,RQFCR_CMP)
#define RQFCR_CMP_NOMATCH	__SHIFTIN(3,RQFCR_CMP)
#define RQFCR_PID	__PPCBITS(28,31)
#define RQFCR_PID_MASK	0
#define RQFCR_PID_PARSE	1
#define RQFCR_PID_ARB	2
#define RQFCR_PID_DAH	3
#define RQFCR_PID_DAL	4
#define RQFCR_PID_SAH	5
#define RQFCR_PID_SAL	6
#define RQFCR_PID_ETY	7
#define RQFCR_PID_VID	8
#define RQFCR_PID_PRI	9
#define RQFCR_PID_TOS	10
#define RQFCR_PID_L4P	11
#define RQFCR_PID_DIA	12
#define RQFCR_PID_SIA	13
#define RQFCR_PID_DPT	14
#define RQFCR_PID_SPT	15
#define RQFPR		0x33C /* Receive queue filing table property register [TSEC3] */
#define RQFPR_PID1_AR	__PPCBIT(14) /* ARP response */
#define RQFPR_PID1_ARQ	__PPCBIT(15) /* ARP request */
#define RQFPR_PID1_EBC	__PPCBIT(16) /* destination Ethernet address is to the broadcast address */
#define RQFPR_PID1_VLN	__PPCBIT(17) /* VLAN tag */
#define RQFPR_PID1_CFI	__PPCBIT(18) /* Canonical Format Indicator */
#define RQFPR_PID1_JUM	__PPCBIT(19) /* Jumbo Ethernet frame */
#define RQFPR_PID1_IPF	__PPCBIT(20) /* fragmented IPv4 or IPv6 header */
#define RQFPR_PID1_IP4	__PPCBIT(22) /* IPv4 header */
#define RQFPR_PID1_IP6	__PPCBIT(23) /* IPv6 header */
#define RQFPR_PID1_ICC	__PPCBIT(24) /* IPv4 header checksum */
#define RQFPR_PID1_ICV	__PPCBIT(25) /* IPv4 header checksum was verifed correct */
#define RQFPR_PID1_TCP	__PPCBIT(26) /* TCP header */
#define RQFPR_PID1_UDP	__PPCBIT(27) /* UDP header */
#define RQFPR_PID1_PER	__PPCBIT(30) /* parse error */
#define RQFPR_PID1_EER	__PPCBIT(31) /* Ethernet framing error */
#define MRBLR		0x340 /* Maximum receive buffer length register */
#define RBDBPH		0x380 /* Rx data buffer pointer high bits [TSEC3] */
#define RBPTR0		0x384 /* RxBD pointer for ring 0 */
#define RBPTR1		0x38C /* RxBD pointer for ring 1 [TSEC3] */
#define RBPTR2		0x394 /* RxBD pointer for ring 2 [TSEC3] */
#define RBPTR3		0x39C /* RxBD pointer for ring 3 [TSEC3] */
#define RBPTR4		0x3A4 /* RxBD pointer for ring 4 [TSEC3] */
#define RBPTR5		0x3AC /* RxBD pointer for ring 5 [TSEC3] */
#define RBPTR6		0x3B4 /* RxBD pointer for ring 6 [TSEC3] */
#define RBPTR7		0x3BC /* RxBD pointer for ring 7 [TSEC3] */
#define RBPTRn(n)	(RBPTR0 + 8*(n))
#define RBASEH		0x400 /* RxBD base address high bits [TSEC3] */
#define RBASE0		0x404 /* RxBD base address of ring 0 */
#define RBASE1		0x40C /* RxBD base address of ring 1 [TSEC3] */
#define RBASE2		0x414 /* RxBD base address of ring 2 [TSEC3] */
#define RBASE3		0x41C /* RxBD base address of ring 3 [TSEC3] */
#define RBASE4		0x424 /* RxBD base address of ring 4 [TSEC3] */
#define RBASE5		0x42C /* RxBD base address of ring 5 [TSEC3] */
#define RBASE6		0x434 /* RxBD base address of ring 6 [TSEC3] */
#define RBASE7		0x43C /* RxBD base address of ring 7 [TSEC3] */
#define RBASEn(n)	(RBASE0 + 8*(n))
#define TMR_RXTS_H	0x4C0 /* Rx timer time stamp register high [TSEC3] */
#define TMR_RXTS_L	0x4C4 /* Rx timer time stamp register low [TSEC3] */

/*	0x500-0x5ff MAC registers */

#define MACCFG1		0x500 /* MAC configuration register 1 */
#define	MACCFG1_SOFT_RESET	__PPCBIT(0)
#define	MACCFG1_RESET_RX_MAC	__PPCBIT(12)
#define	MACCFG1_RESET_TX_MAC	__PPCBIT(13)
#define	MACCFG1_RESET_RX_FUNC	__PPCBIT(14)
#define	MACCFG1_RESET_TX_FUNC	__PPCBIT(15)
#define	MACCFG1_LOOPBACK	__PPCBIT(23)
#define	MACCFG1_RX_FLOW		__PPCBIT(26)
#define	MACCFG1_TX_FLOW		__PPCBIT(27)
#define	MACCFG1_SYNC_RX_EN	__PPCBIT(28)
#define	MACCFG1_RX_EN		__PPCBIT(29)
#define	MACCFG1_SYNC_TX_EN	__PPCBIT(30)
#define	MACCFG1_TX_EN		__PPCBIT(31)
#define MACCFG2		0x504 /* MAC configuration register 2 */
#define	MACCFG2_PRELEN	__PPCBITS(16,19)
#define	MACCFG2_PRELEN_DEFAULT	__SHIFTIN(7,MACCFG2_PRELEN)
#define	MACCFG2_IFMODE	__PPCBITS(22,23)
#define	MACCFG2_IFMODE_MII	__SHIFTIN(1,MACCFG2_IFMODE)
#define	MACCFG2_IFMODE_GMII	__SHIFTIN(2,MACCFG2_IFMODE)
#define	MACCFG2_PRERXEN	__PPCBIT(24)
#define	MACCFG2_PRETXEN	__PPCBIT(25)
#define	MACCFG2_HG	__PPCBIT(26)
#define	MACCFG2_LENCHK	__PPCBIT(27)
#define	MACCFG2_MPEN	__PPCBIT(28)
#define	MACCFG2_PADCRC	__PPCBIT(29)
#define	MACCFG2_CRCEN	__PPCBIT(30)
#define	MACCFG2_FD	__PPCBIT(31)
#define	MACCFG2_DEFAULT	(MACCFG2_FD|MACCFG2_PADCRC|MACCFG2_PRELEN_DEFAULT)
#define IPGIFG		0x508 /* Inter-packet/inter-frame gap register */
#define HAFDUP		0x50C /* Half-duplex control */
#define MAXFRM		0x510 /* Maximum frame length */
#define MIIMCFG		0x520 /* MII management configuration */
#define	MIIMCFG_RESET	__PPCBIT(0) /* Reset management */
#define	MIIMCFG_NOPRE	__PPCBIT(27) /* Preamble suppess */
#define MIIMCOM		0x524 /* MII management command */
#define	MIIMCOM_SCAN	__PPCBIT(30)
#define	MIIMCOM_READ	__PPCBIT(31)
#define MIIMADD		0x528 /* MII management address */
#define	MIIMADD_PHY	__PPCBITS(19,23)
#define	MIIMADD_REG	__PPCBITS(27,31)
#define MIIMCON		0x52C /* MII management control */
#define MIIMSTAT	0x530 /* MII management status */
#define MIIMIND		0x534 /* MII management indicator */
#define MIIMIND_NOTVALID __PPCBIT(29)
#define MIIMIND_SCAN	__PPCBIT(30)
#define MIIMIND_BUSY	__PPCBIT(31)
#define IFSTAT		0x53C /* Interface status */
#define MACSTNADDR1	0x540 /* MAC station address register 1 */
#define MACSTNADDR2	0x544 /* MAC station address register 2 */
#define MAC01ADDR1	0x548 /* MAC exact match address 1, part 1 [TSEC3] */
#define MAC01ADDR2	0x54C /* MAC exact match address 1, part 2 [TSEC3] */
#define MAC02ADDR1	0x550 /* MAC exact match address 2, part 1 [TSEC3] */
#define MAC02ADDR2	0x554 /* MAC exact match address 2, part 2 [TSEC3] */
#define MAC03ADDR1	0x558 /* MAC exact match address 3, part 1 [TSEC3] */
#define MAC03ADDR2	0x55C /* MAC exact match address 3, part 2 [TSEC3] */
#define MAC04ADDR1	0x560 /* MAC exact match address 4, part 1 [TSEC3] */
#define MAC04ADDR2	0x564 /* MAC exact match address 4, part 2 [TSEC3] */
#define MAC05ADDR1	0x568 /* MAC exact match address 5, part 1 [TSEC3] */
#define MAC05ADDR2	0x56C /* MAC exact match address 5, part 2 [TSEC3] */
#define MAC06ADDR1	0x570 /* MAC exact match address 6, part 1 [TSEC3] */
#define MAC06ADDR2	0x574 /* MAC exact match address 6, part 2 [TSEC3] */
#define MAC07ADDR1	0x578 /* MAC exact match address 7, part 1 [TSEC3] */
#define MAC07ADDR2	0x57C /* MAC exact match address 7, part 2 [TSEC3] */
#define MAC08ADDR1	0x580 /* MAC exact match address 8, part 1 [TSEC3] */
#define MAC08ADDR2	0x584 /* MAC exact match address 8, part 2 [TSEC3] */
#define MAC09ADDR1	0x588 /* MAC exact match address 9, part 1 [TSEC3] */
#define MAC09ADDR2	0x58C /* MAC exact match address 9, part 2 [TSEC3] */
#define MAC10ADDR1	0x590 /* MAC exact match address 10, part 1 [TSEC3] */
#define MAC10ADDR2	0x594 /* MAC exact match address 10, part 2 [TSEC3] */
#define MAC11ADDR1	0x598 /* MAC exact match address 11, part 1 [TSEC3] */
#define MAC11ADDR2	0x59C /* MAC exact match address 11, part 2 [TSEC3] */
#define MAC12ADDR1	0x5A0 /* MAC exact match address 12, part 1 [TSEC3] */
#define MAC12ADDR2	0x5A4 /* MAC exact match address 12, part 2 [TSEC3] */
#define MAC13ADDR1	0x5A8 /* MAC exact match address 13, part 1 [TSEC3] */
#define MAC13ADDR2	0x5AC /* MAC exact match address 13, part 2 [TSEC3] */
#define MAC14ADDR1	0x5B0 /* MAC exact match address 14, part 1 [TSEC3] */
#define MAC14ADDR2	0x5B4 /* MAC exact match address 14, part 2 [TSEC3] */
#define MAC15ADDR1	0x5B8 /* MAC exact match address 15, part 1 [TSEC3] */
#define MAC15ADDR2	0x5BC /* MAC exact match address 15, part 2 [TSEC3] */
#define MACnADDR1(n)	(MAC01ADDR1 + 8*(n))
#define MACnADDR2(n)	(MAC01ADDR2 + 8*(n))

/* 0x600-0x7ff RMON MIB registers */

/* eTSEC Transmit and Receive Counters */
#define TR64		0x680 /* Transmit and receive 64 byte frame counter */
#define TR127		0x684 /* Transmit and receive 65 to 127-byte frame counter */
#define TR255		0x688 /* Transmit and receive 128 to 255-byte frame counter */
#define TR511		0x68C /* Transmit and receive 256 to 511-byte frame counter */
#define TR1K		0x690 /* Transmit and receive 512 to 1023-byte frame counter */
#define TRMAX		0x694 /* Transmit and receive 1024 to 1518-byte frame counter */
#define TRMGV		0x698 /* Transmit and receive 1519 to 1522-byte good VLAN frame count */

/* eTSEC Receive Counters Registers */
#define RBYT		0x69C /* Receive byte counter */
#define RPKT		0x6A0 /* Receive packet counter */
#define RFCS		0x6A4 /* Receive FCS error counter */
#define RMCA		0x6A8 /* Receive multicast packet counter */
#define RBCA		0x6AC /* Receive broadcast packet counter */
#define RXCF		0x6B0 /* Receive control frame packet counter */
#define RXPF		0x6B4 /* Receive PAUSE frame packet counter */
#define RXUO		0x6B8 /* Receive unknown OP code counter */
#define RALN		0x6BC /* Receive alignment error counter */
#define RFLR		0x6C0 /* Receive frame length error counter */
#define RCDE		0x6C4 /* Receive code error counter */
#define RCSE		0x6C8 /* Receive carrier sense error counter */
#define RUND		0x6CC /* Receive undersize packet counter */
#define ROVR		0x6D0 /* Receive oversize packet counter */
#define RFRG		0x6D4 /* Receive fragments counter */
#define RJBR		0x6D8 /* Receive jabber counter */
#define RDRP		0x6DC /* Receive drop counter */

/* eTSEC Transmit Counters Registers */
#define TBYT		0x6E0 /* Transmit byte counter */
#define TPKT		0x6E4 /* Transmit packet counter */
#define TMCA		0x6E8 /* Transmit multicast packet counter */
#define TBCA		0x6EC /* Transmit broadcast packet counter */
#define TXPF		0x6F0 /* Transmit PAUSE control frame counter */
#define TDFR		0x6F4 /* Transmit deferral packet counter */
#define TEDF		0x6F8 /* Transmit excessive deferral packet counter */
#define TSCL		0x6FC /* Transmit single collision packet counter */
#define TMCL		0x700 /* Transmit multiple collision packet counter */
#define TLCL		0x704 /* Transmit late collision packet counter */

#define TXCL		0x708 /* Transmit excessive collision packet counter */
#define TNCL		0x70C /* Transmit total collision counter */
#define TDRP		0x714 /* Transmit drop frame counter */
#define TJBR		0x718 /* Transmit jabber frame counter */
#define TFCS		0x71C /* Transmit FCS error counter */
#define TXCF		0x720 /* Transmit control frame counter */
#define TOVR		0x724 /* Transmit oversize frame counter */
#define TUND		0x728 /* Transmit undersize frame counter */
#define TFRG		0x72C /* Transmit fragments frame counter */

/* eTSEC Counter Control and TOE Statistics Registers */
#define CAR1		0x730 /* Carry register one register 3 */
#define CAR2		0x734 /* Carry register two register 3 */
#define CAM1		0x738 /* Carry register one mask register */
#define CAM2		0x73C /* Carry register two mask register */
#define RREJ		0x740 /* Receive filter rejected packet counter [TSEC3] */

/*	0x800-0x8ff Hash table registers */

#define IGADDR0		0x800 /* Individual/group address register 0 */
#define IGADDR1		0x804 /* Individual/group address register 1 */
#define IGADDR2		0x808 /* Individual/group address register 2 */
#define IGADDR3		0x80C /* Individual/group address register 3 */
#define IGADDR4		0x810 /* Individual/group address register 4 */
#define IGADDR5		0x814 /* Individual/group address register 5 */
#define IGADDR6		0x818 /* Individual/group address register 6 */
#define IGADDR7		0x81C /* Individual/group address register 7 */
#define	IGADDR(n)	(IGADDR0 + 4*(n))

#define GADDR0		0x880 /* Group address register 0 */
#define GADDR1		0x884 /* Group address register 1 */
#define GADDR2		0x888 /* Group address register 2 */
#define GADDR3		0x88C /* Group address register 3 */
#define GADDR4		0x890 /* Group address register 4 */
#define GADDR5		0x894 /* Group address register 5 */
#define GADDR6		0x898 /* Group address register 6 */
#define GADDR7		0x89C /* Group address register 7 */
#define	GADDR(n)	(GADDR0 + 4*(n))

/* 0x900-0x9ff unused */
/* 0xa00-0xaff FIFO control/status registers */

#define FIFOCFG		0xA00 /* FIFO interface configuration register */

/* 0xb00-0xbff DMA system registers */

#define ATTR		0xBF8 /* Attribute register */
#define	ATTR_ELCWT	__PPCBITS(17,18)
#define	ATTR_ELCWT_L2	__SHIFTIN(2, ATTR_ELCWT)
#define	ATTR_BDLWT	__PPCBITS(20,21)
#define	ATTR_BDLWT_L2	__SHIFTIN(2, ATTR_BDLWT)
#define ATTR_RDSEN	__PPCBIT(24)
#define ATTR_RBDSEN	__PPCBIT(25)
#define	ATTR_DEFAULT	(ATTR_ELCWT_L2|ATTR_BDLWT_L2|ATTR_RDSEN|ATTR_RBDSEN)

#define ATTRELI		0xBFC /* Attribute extract length and extract index register [TSEC3] */
#define	ATTRELI_EL	__PPCBITS(2,12)		/* extracted length */
#define	ATTRELI_EI	__PPCBITS(18,28)	/* extracted index */
#define	ATTRELI_DEFAULT	(__SHIFTIN(72, ATTRELI_EL))

/* 0xc00-0xc3f Lossless Flow Control registers */

#define RQPRM0		0xC00 /* Receive Queue Parameters register 0 [TSEC3] */
#define RQPRM1		0xC04 /* Receive Queue Parameters register 1 [TSEC3] */
#define RQPRM2		0xC08 /* Receive Queue Parameters register 2 [TSEC3] */
#define RQPRM3		0xC0C /* Receive Queue Parameters register 3 [TSEC3] */
#define RQPRM4		0xC10 /* Receive Queue Parameters register 4 [TSEC3] */
#define RQPRM5		0xC14 /* Receive Queue Parameters register 5 [TSEC3] */
#define RQPRM6		0xC18 /* Receive Queue Parameters register 6 [TSEC3] */
#define RQPRM7		0xC1C /* Receive Queue Parameters register 7 [TSEC3] */
#define	RQPRMn(n)	(RQPRM0 + 4*(n))
#define	RQPRM_FBTHR	__PPCBITS(0,7)
#define	RQPRM_FBTHR_SET(n)	__SHIFTIN((n),RQPRM_FBTHR)
#define	RQPRM_LEN	__PPCBITS(8,31)
#define	RQPRM_LEN_SET(n)	__SHIFTIN((n),RQPRM_LEN)

#define RFBPTR0		0xC44 /* Last Free RxBD pointer for ring 0 [TSEC3] */
#define RFBPTR1		0xC4C /* Last Free RxBD pointer for ring 1 [TSEC3] */
#define RFBPTR2		0xC54 /* Last Free RxBD pointer for ring 2 [TSEC3] */
#define RFBPTR3		0xC58 /* Last Free RxBD pointer for ring 3 [TSEC3] */
#define RFBPTR4		0xC64 /* Last Free RxBD pointer for ring 4 [TSEC3] */
#define RFBPTR5		0xC6C /* Last Free RxBD pointer for ring 5 [TSEC3] */
#define RFBPTR6		0xC74 /* Last Free RxBD pointer for ring 6 [TSEC3] */
#define RFBPTR7		0xC7C /* Last Free RxBD pointer for ring 7 [TSEC3] */
#define	RFBPTRn(n)	(RFBPTR0 + 4*(n))

/* 0xc40-0xdff unused */
/* 0xe00-0xeaf 1588 Hardware Assist */

#define TMR_CTRL	0xE00 /* Timer control register [TSEC3] */
#define TMR_TEVENT	0xE04 /* time stamp event register [TSEC3] */
#define TMR_TEMASK	0xE08 /* Timer event mask register [TSEC3] */
#define TMR_PEVENT	0xE0C /* time stamp event register [TSEC3] */
#define TMR_PEMASK	0xE10 /* Timer event mask register [TSEC3] */
#define TMR_STAT	0xE14 /* time stamp status register [TSEC3] */
#define TMR_CNT_H	0xE18 /* timer counter high register [TSEC3] */
#define TMR_CNT_L	0xE1C /* timer counter low register [TSEC3] */
#define TMR_ADD		0xE20 /* Timer drift compensation addend register [TSEC3] */
#define TMR_ACC		0xE24 /* Timer accumulator register [TSEC3] */
#define TMR_PRSC	0xE28 /* timer prescale [TSEC3] */
#define TMROFF_H	0xE30 /* Timer offset high [TSEC3] */
#define TMROFF_L	0xE34 /* Timer offset low [TSEC3] */
#define TMR_ALARM1_H	0xE40 /* Timer alarm 1 high register [TSEC3] */
#define TMR_ALARM1_L	0xE44 /* Timer alarm 1 high register [TSEC3] */
#define TMR_ALARM2_H	0xE48 /* Timer alarm 2 high register [TSEC3] */
#define TMR_ALARM2_L	0xE4C /* Timer alarm 2 high register [TSEC3] */
#define TMR_FIPER1	0xE80 /* Timer fixed period interval [TSEC3] */
#define TMR_FIPER2	0xE84 /* Timer fixed period interval [TSEC3] */
#define TMR_FIPER	0xE88 /* Timer fixed period interval [TSEC3] */
#define TMR_ETTS1_H	0xEA0 /* Time stamp of general purpose external trigger [TSEC3] */
#define TMR_ETTS1_L	0xEA4 /* Time stamp of general purpose external trigger [TSEC3] */
#define TMR_ETTS2_H	0xEA8 /* Time stamp of general purpose external trigger [TSEC3] */
#define TMR_ETTS2_L	0xEAC /* Time stamp of general purpose external trigger [TSEC3] */

/* 0xeb0-0xeff Interrupt steering and coalescing */

#define ISRG0		0xeb0	/* Interrupt steering register group 0 */
#define ISRG1		0xeb4	/* Interrupt steering register group 1 */
#define ISRGn(n)	(ISRG0+4*(n))
#define ISRG_RRn(n)	__PPCBIT(n)
#define ISRG_TRn(n)	__PPCBIT(8+(n))

#define RXIC0		0xed0	/* Ring 0 Rx interrupt coalescing register */
#define RXIC1		0xed4	/* Ring 1 Rx interrupt coalescing register */
#define RXIC2		0xed8	/* Ring 2 Rx interrupt coalescing register */
#define RXIC3		0xedc	/* Ring 3 Rx interrupt coalescing register */
#define RXIC4		0xee0	/* Ring 4 Rx interrupt coalescing register */
#define RXIC5		0xee4	/* Ring 5 Rx interrupt coalescing register */
#define RXIC6		0xee8	/* Ring 6 Rx interrupt coalescing register */
#define RXIC7		0xeec	/* Ring 7 Rx interrupt coalescing register */
#define RXICn(n)	(RXIC0+4*(n))

#define TXIC0		0xf10	/* Ring 0 Tx interrupt coalescing register */
#define TXIC1		0xf14	/* Ring 1 Tx interrupt coalescing register */
#define TXIC2		0xf18	/* Ring 2 Tx interrupt coalescing register */
#define TXIC3		0xf1c	/* Ring 3 Tx interrupt coalescing register */
#define TXIC4		0xf20	/* Ring 4 Tx interrupt coalescing register */
#define TXIC5		0xf24	/* Ring 5 Tx interrupt coalescing register */
#define TXIC6		0xf28	/* Ring 6 Tx interrupt coalescing register */
#define TXIC7		0xf2c	/* Ring 7 Tx interrupt coalescing register */
#define TXICn(n)	(TXIC0+4*(n))

#endif /* _POWERPC_BOOKE_ETSECREG_H_ */
