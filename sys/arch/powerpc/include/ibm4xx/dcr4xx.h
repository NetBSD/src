/*	$NetBSD: dcr4xx.h,v 1.2 2010/11/06 16:32:08 uebayasi Exp $	*/

/*
 * Copyright 2002 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Eduardo Horvath for Wasabi Systems, Inc.
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
 *      Wasabi Systems, Inc.
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

#ifndef _DCR405GP_H_
#define	_DCR405GP_H_

/* Device Control Register declarations */

/* DCRs used for indirect access */
#define	DCR_CPR0_CFGADDR	0x00c	/* Clocking Configuration Address Register */
#define	DCR_CPR0_CFGDATA	0x00d	/* Clocking Configuration Data Register */
#define	DCR_SDR0_CFGADDR	0x00e	/* System DCR Configuration Address Register */
#define	DCR_SDR0_CFGDATA	0x00f	/* System DCR Configuration Data Register */
#define	DCR_SDRAM0_CFGADDR	0x010	/* Memory Controller Address Register */
#define	DCR_SDRAM0_CFGDATA	0x011	/* Memory Controller Data Register */
#define	DCR_EBC0_CFGADDR	0x012	/* Peripheral Controller Address Register */
#define	DCR_EBC0_CFGDATA	0x013	/* Peripheral Controller Data Register */
#define	DCR_DCP0_CFGADDR	0x014	/* Decompression Controller Address Register */
#define	DCR_DCP0_CFGDATA	0x015	/* Decompression Controller Data Register */

/* On-Chip memory */
#define	DCR_OCM0_ISARC		0x018	/* OCM Instruction-Side Address Range Compare Register */
#define	DCR_OCM0_ISCNTL		0x019	/* OCM Instruction-Side Control Register */
#define	DCR_OCM0_DSARC		0x01a	/* OCM Data-Side Address Range Compare Register */
#define	DCR_OCM0_DSCNTL		0x01b	/* OCM Data-Side Control Register */

/* On-Chip busses */
#define	DCR_PLB0_BESR		0x084	/* PLB Bus Error Status Register */
#define	DCR_PLB0_BEAR		0x086	/* PLB Bus Error Address Register */
#define	DCR_PLB0_ACR		0x087	/* PLB Arbiter Control Register */
#define	DCR_POB0_BESR0		0x0a0	/* PLB to OPB Bus Error Status Register 0 */
#define	DCR_POB0_BEAR		0x0a2	/* PLB to OPB Bus Error Address Register */
#define	DCR_POB0_BESR1		0x0a4	/* PLB to OPB Bus Error Status Register 1 */

/* Clocking, Power management and Chip Control */
#define	DCR_CPC0_PLLMR		0x0b0	/* PLL Mode Register */
#define	  CPC0_PLLMR_CBDV(pllmr)  ((((pllmr) & 0x00060000) >> 17) + 1)
#define	  CPC0_PLLMR_OPDV(pllmr)  ((((pllmr) & 0x00018000) >> 15) + 1)
#define	DCR_CPC0_CR0		0x0b1	/* Chip Control Register 0 */
#define	DCR_CPC0_CR1		0x0b2	/* Chip Control Register 1 */
#define	  CPC0_CR1_CETE		  0x00800000	/* CPU External Timer Enable */
#define	DCR_CPC0_PSR		0x0b4	/* Chip Pin Strapping Register */
#define	DCR_CPC0_JTAGID		0x0b5	/* JTAG ID Register */
#define	DCR_CPC0_SR		0x0b8	/* CPM Status Register */
#define	DCR_CPC0_ER		0x0b9	/* CPM Enable Register */
#define	DCR_CPC0_FR		0x0ba	/* CPM Force Register */

/* Universal Interrupt Controllers */
#define	DCR_UIC0_BASE		0x0c0	/* UIC0 Registers Base */
#define	DCR_UIC1_BASE		0x0d0	/* UIC1 Registers Base */
#define	DCR_UIC2_BASE		0x0e0	/* UIC2 Registers Base */
#define	DCR_UIC3_BASE		0x0f0	/* UIC3 Registers Base */
#define	DCR_UICB_BASE		0x200	/* UICB Registers Base */
#define	DCR_UIC2_BASE_440GX	0x210	/* UIC2 Registers Base (440GX only) */

#define	DCR_UIC_SR		  0x000	/* UIC Status Register */
#define	DCR_UIC_ER		  0x002	/* UIC Enable Register */
#define	DCR_UIC_CR		  0x003	/* UIC Critical Register */
#define	DCR_UIC_PR		  0x004	/* UIC Polarity Register */
#define	DCR_UIC_TR		  0x005	/* UIC Triggering Register */
#define	DCR_UIC_MSR		  0x006	/* UIC Masked Status Register */
#define	DCR_UIC_VR		  0x007	/* UIC Vector Register */
#define	DCR_UIC_VCR		  0x008	/* UIC Vector Configuration Register */

/* Direct Memory Access */
#define	DCR_DMA0_CR0		0x100	/* DMA Channel Control Register 0 */
#define	DCR_DMA0_CT0		0x101	/* DMA Count Register 0 */
#define	DCR_DMA0_DA0		0x102	/* DMA Destination Address Register 0 */
#define	DCR_DMA0_SA0		0x103	/* DMA Source Address Register 0 */
#define	DCR_DMA0_SG0		0x104	/* DMA Scatter/Gather Descriptor Address Register 0 */

#define	DCR_DMA0_CR1		0x108	/* DMA Channel Control Register 1 */
#define	DCR_DMA0_CT1		0x109	/* DMA Count Register 1 */
#define	DCR_DMA0_DA1		0x10a	/* DMA Destination Address Register 1 */
#define	DCR_DMA0_SA1		0x10b	/* DMA Source Address Register 1 */
#define	DCR_DMA0_SG1		0x10c	/* DMA Scatter/Gather Descriptor Address Register 1 */

#define	DCR_DMA0_CR2		0x110	/* DMA Channel Control Register 2 */
#define	DCR_DMA0_CT2		0x111	/* DMA Count Register 2 */
#define	DCR_DMA0_DA2		0x112	/* DMA Destination Address Register 2 */
#define	DCR_DMA0_SA2		0x113	/* DMA Source Address Register 2 */
#define	DCR_DMA0_SG2		0x114	/* DMA Scatter/Gather Descriptor Address Register 2 */

#define	DCR_DMA0_CR3		0x118	/* DMA Channel Control Register 3 */
#define	DCR_DMA0_CT3		0x119	/* DMA Count Register 3 */
#define	DCR_DMA0_DA3		0x11a	/* DMA Destination Address Register 3 */
#define	DCR_DMA0_SA3		0x11b	/* DMA Source Address Register 3 */
#define	DCR_DMA0_SG3		0x11c	/* DMA Scatter/Gather Descriptor Address Register 3 */

#define	DCR_DMA0_SR		0x120	/* DMA Status Register */
#define	DCR_DMA0_SGC		0x123	/* DMA Scatter/Gather Control Register */
#define	DCR_DMA0_SLP		0x125	/* DMA Sleep Mode Register */
#define	DCR_DMA0_POL		0x126	/* DMA Polarity Configuration Register */

/* Memory Access Layer */
#define	DCR_MAL0_CFG		0x180	/* MAL Configuration Register */
#define	  MAL0_CFG_SR		  0x80000000	/* Software Reset */
#define	  MAL0_CFG_PLBP_MASK	  0x00c00000	/* PLB priority mask */
#define	  MAL0_CFG_PLBP_0	  0x00000000	/* PLB priority 0 */
#define	  MAL0_CFG_PLBP_1	  0x00400000	/* PLB priority 1 */
#define	  MAL0_CFG_PLBP_2	  0x00800000	/* PLB priority 2 */
#define	  MAL0_CFG_PLBP_3	  0x00c00000	/* PLB priority 3 */
#define	  MAL0_CFG_GA		  0x00200000	/* Guarded Active */
#define	  MAL0_CFG_OA		  0x00100000	/* Ordered Active */
#define	  MAL0_CFG_PLBLE	  0x00080000	/* PLB Lock Error */
#define	  MAL0_CFG_PLBLT	  0x00078000	/* PLB Latency Timer */
#define	  MAL0_CFG_PLBLTSHIFT	  15		/* PLB Latency Timer shift */
#define	  MAL0_CFG_PLBB		  0x00004000	/* PLB Burst */
#define	  MAL0_CFG_OPBBL	  0x00000080	/* OPB Bus Lock */
#define	  MAL0_CFG_EOPIE	  0x00000004	/* End Of Packet Interrupt Enable */
#define	  MAL0_CFG_LEA		  0x00000002	/* Locked Error Active */
#define	  MAL0_CFG_SD		  0x00000001	/* MAL Scroll Descriptor */

#define	  MAL0_CFG_RPP_MASK	  0x00c00000	/* Read priority mask */
#define	  MAL0_CFG_RPP_0	  0x00000000	/*   Lowest */
#define	  MAL0_CFG_RPP_1	  0x00400000
#define	  MAL0_CFG_RPP_2	  0x00800000
#define	  MAL0_CFG_RPP_3	  0x00c00000	/*   Highest */
#define	  MAL0_CFG_RMBS_MASK	  0x00300000	/* Read Max Burst Size */
#define	  MAL0_CFG_RMBS_4	  0x00000000	/*   Max burst size of 4 */
#define	  MAL0_CFG_RMBS_8	  0x00100000	/*   Max burst size of 8 */
#define	  MAL0_CFG_RMBS_16	  0x00200000	/*   Max burst size of 16 */
#define	  MAL0_CFG_RMBS_32	  0x00300000	/*   Max burst size of 32 */
#define	  MAL0_CFG_WPP_MASK	  0x000c0000	/* Write PLB Priority */
#define	  MAL0_CFG_WPP_0	  0x00000000	/*   Lowest */
#define	  MAL0_CFG_WPP_1	  0x00040000
#define	  MAL0_CFG_WPP_2	  0x00080000
#define	  MAL0_CFG_WPP_3	  0x000c0000	/*   Highest */
#define	  MAL0_CFG_WMBS_MASK	  0x00030000	/* Write Max Burst Size */
#define	  MAL0_CFG_WMBS_4	  0x00000000	/*   Max burst size of 4 */
#define	  MAL0_CFG_WMBS_8	  0x00010000	/*   Max burst size of 8 */
#define	  MAL0_CFG_WMBS_16	  0x00020000	/*   Max burst size of 16 */
#define	  MAL0_CFG_WMBS_32	  0x00030000	/*   Max burst size of 32 */
#define	  MAL0_CFG_PLBLE__EX	  0x00008000	/* PLB Lock Error */

#define	DCR_MAL0_ESR		0x181	/* Error Status Register */
#define	  MAL0_ESR_EVB		  0x80000000	/* Error Valid Bit */
#define	  MAL0_ESR_CID_RX	  0x40000000	/* Receive Channel */
#define	  MAL0_ESR_CID_MASK	  0x3e000000	/* Channel ID */
#define	  MAL0_ESR_CID_SHIFT	  25
#define	  MAL0_ESR_PTE		  0x00800000	/* PLB Timeout Error */
#define	  MAL0_ESR_PRE		  0x00400000	/* PLB Read Error */
#define	  MAL0_ESR_PWE		  0x00200000	/* PLB Write Error */
#define	  MAL0_ESR_DE		  0x00100000	/* Descriptor Error */
#define	  MAL0_ESR_ONE		  0x00080000	/* OPB Non-fullword Error */
#define	  MAL0_ESR_OTE		  0x00040000	/* OPB Timeout Error */
#define	  MAL0_ESR_OSE		  0x00020000	/* OPB Slave Error */
#define	  MAL0_ESR_PEIN		  0x00010000	/* PLB Bus Error Indication */
#define	  MAL0_ESR_PTEI		  0x00000080	/* PLB Timeout Error Interrupt */
#define	  MAL0_ESR_PREI		  0x00000040	/* PLB Read Error Interrupt */
#define	  MAL0_ESR_PWEI		  0x00000020	/* PLB Write Error Interrupt */
#define	  MAL0_ESR_DEI		  0x00000010	/* Descriptor Error Interrupt */
#define	  MAL0_ESR_ONEI		  0x00000008	/* OPB Non-fullword Error Interrupt */
#define	  MAL0_ESR_OTEI		  0x00000004	/* OPB Timeout Error Interrupt */
#define	  MAL0_ESR_OSEI		  0x00000002	/* OPB Slave Error Interrupt */
#define	  MAL0_ESR_PBEI		  0x00000001	/* PLB Bus Error Interrupt */
#define	DCR_MAL0_IER		0x182	/* Interrupt Enable Register */
#define	  MAL0_IER_PT		  0x00000080	/* PLB Timeout Interrupt */
#define	  MAL0_IER_PRE		  0x00000040	/* PLB Read Interrupt */
#define	  MAL0_IER_PWE		  0x00000020	/* PLB Write Interrupt */
#define	  MAL0_IER_DE		  0x00000010	/* Descriptor Error Interrupt */
#define	  MAL0_IER_NWE		  0x00000008	/* Non-Word Transfer Error Interrupt */
#define	  MAL0_IER_TO		  0x00000004	/* Time Out Error Interrupt */
#define	  MAL0_IER_OPB		  0x00000002	/* OPB Error Interrupt */
#define	  MAL0_IER_PLB		  0x00000001	/* PLB Error Interrupt */
#define DCR_MALDBR		0x183	/* MAL Debug register */
#define	DCR_MAL0_TXCASR		0x184	/* Tx Channel Active Register (Set) */
#define	DCR_MAL0_TXCARR		0x185	/* Tx Channel Active Register (Reset) */
#define	DCR_MAL0_TXEOBISR	0x186	/* Tx End of Buffer Interrupt Status Register */
#define	DCR_MAL0_TXDEIR		0x187	/* Tx Descriptor Error Interrupt Register */
#define	DCR_MAL0_RXCASR		0x190	/* Rx Channel Active Register (Set) */
#define	DCR_MAL0_RXCARR		0x191	/* Rx Channel Active Register (Reset) */
#define	DCR_MAL0_RXEOBISR	0x192	/* Rx End of Buffer Interrupt Status Register */
#define	DCR_MAL0_RXDEIR		0x193	/* Rx Descriptor Error Interrupt Register */
#define   MAL0__XCAR_CHAN(c)	  (0x80000000 >> (c))
#define	DCR_MAL0_TXCTP0R	0x1a0	/* Channel Tx 0 Channel Table Pointer Register */
#define	DCR_MAL0_TXCTP1R	0x1a1	/* Channel Tx 1 Channel Table Pointer Register */
#define	DCR_MAL0_TXCTP2R	0x1a2	/* Channel Tx 2 Channel Table Pointer Register */
#define	DCR_MAL0_TXCTP3R	0x1a3	/* Channel Tx 3 Channel Table Pointer Register */
#define	DCR_MAL0_RXCTP0R	0x1c0	/* Channel Rx 0 Channel Table Pointer Register */
#define	DCR_MAL0_RXCTP1R	0x1c1	/* Channel Rx 1 Channel Table Pointer Register */
#define	DCR_MAL0_RXCTP2R	0x1c2	/* Channel Rx 2 Channel Table Pointer Register */
#define	DCR_MAL0_RXCTP3R	0x1c3	/* Channel Rx 3 Channel Table Pointer Register */
#define	DCR_MAL0_RCBS0		0x1e0	/* Channel Rx 0 Channel Buffer Size Register */
#define	DCR_MAL0_RCBS1		0x1e1	/* Channel Rx 1 Channel Buffer Size Register */
#define	DCR_MAL0_RCBS2		0x1e2	/* Channel Rx 2 Channel Buffer Size Register */
#define	DCR_MAL0_RCBS3		0x1e3	/* Channel Rx 3 Channel Buffer Size Register */


/* Indirectly accessed Clocking Controller DCRs */

#define	DCR_CPR0_CLKUPD		0x020	/* Clocking Update Register */
#define	DCR_CPR0_PLLC		0x040	/* SYS_PLL Control Register */
#define	DCR_CPR0_PLLD		0x060	/* SYS_PLL Divider Register */
#define	DCR_CPR0_CPUD		0x080	/* CPU Clock Divider Register */
#define	DCR_CPR0_PLBD		0x0a0	/* PLB Clock Divider Register */
#define	  CPR0_PLBDV0(x) \
	((((x) & 0x07000000) >> 24) == 0 ? 8 : (((x) & 0x07000000) >> 24))
#define	DCR_CPR0_OPBD		0x0c0	/* OPB Clock Divider Register */
#define	  CPR0_OPBDV0(x) \
	((((x) & 0x03000000) >> 24) == 0 ? 4 : (((x) & 0x03000000) >> 24))
#define	DCR_CPR0_PERD		0x0e0	/* Peripheral Clock Divider Register */
#define	DCR_CPR0_AHBD		0x100	/* AHB Clock Divider Register */
#define	DCR_CPR0_ICFG		0x140	/* Initial Configuration Register */

/* Indirectly accessed Clocking Controller DCRs */

#define	DCR_SDR0_MFR		0x4300	/* Miscellaneous Function Register */
#define	  SDR0_MFR_ECS(n)	  (1 << (27 - (n)))	/* Ethernet n Clock Selection */
#define	  SDR0_MFR_ETXFL(n)	  (1 << (15 - ((n) << 2)))	/* Force Parity Error EMACn Tx FIFO Bits 0:63 */
#define	  SDR0_MFR_ETXFH(n)	  (1 << (14 - ((n) << 2)))	/* Force Parity Error EMACn Tx FIFO Bits 64:127 */
#define	  SDR0_MFR_ERXFL(n)	  (1 << (13 - ((n) << 2)))	/* Force Parity Error EMACn Rx FIFO Bits 0:63 */
#define	  SDR0_MFR_ERXFH(n)	  (1 << (12 - ((n) << 2)))	/* Force Parity Error EMACn Rx FIFO Bits 64:127 */

/* Indirectly accessed SDRAM Controller DCRs */

#define DCR_SDRAM0_BESR0	0x00
#define DCR_SDRAM0_BESR1	0x08
#define DCR_SDRAM0_BEAR		0x10
#define DCR_SDRAM0_CFG		0x20
#define DCR_SDRAM0_STATUS	0x24
#define DCR_SDRAM0_RTR		0x30
#define DCR_SDRAM0_PMIT		0x34
#define DCR_SDRAM0_B0CR		0x40
#define DCR_SDRAM0_B1CR		0x44
#define DCR_SDRAM0_B2CR		0x48
#define DCR_SDRAM0_B3CR		0x4c
#define DCR_SDRAM0_TR		0x80
#define DCR_SDRAM0_ECCCFG	0x94
#define DCR_SDRAM0_ECCESR	0x98
#define   SDRAM0_ECCESR_BLCE	  0xf0000000
#define   SDRAM0_ECCESR_CBE	  0x00c00000
#define   SDRAM0_ECCESR_CE	  0x00200000
#define   SDRAM0_ECCESR_UE	  0x00100000
#define   SDRAM0_ECCESR_BKE	  0x0000f000

#define SDRAM0_ECCESR_BLCEN(n)	  (0x80000000 >> (n))
#define SDRAM0_ECCESR_BKEN(n)	  (0x00008000 >> (n))
#define SDRAM0_ECCESR_CBEN(n)	  (0x00800000 >> (n))

/* Indirectly accessed External Bus Controller (EBC) DCRs */

#define DCR_EBC0_B0CR		0x00
#define DCR_EBC0_B1CR		0x01
#define DCR_EBC0_B2CR		0x02
#define DCR_EBC0_B3CR		0x03
#define DCR_EBC0_B4CR		0x04
#define DCR_EBC0_B5CR		0x05
#define DCR_EBC0_B6CR		0x06
#define DCR_EBC0_B7CR		0x07
#define DCR_EBC0_B0AP		0x10
#define DCR_EBC0_B1AP		0x11
#define DCR_EBC0_B2AP		0x12
#define DCR_EBC0_B3AP		0x13
#define DCR_EBC0_B4AP		0x14
#define DCR_EBC0_B5AP		0x15
#define DCR_EBC0_B6AP		0x16
#define DCR_EBC0_B7AP		0x17
#define DCR_EBC0_BEAR		0x20
#define DCR_EBC0_BESR0		0x21
#define DCR_EBC0_BESR1		0x22
#define DCR_EBC0_CFG		0x23

/* Indirectly accessed Decompression Controller DCRs */

#define DCR_DCP0_ITOR0		0x00
#define DCR_DCP0_ITOR1		0x01
#define DCR_DCP0_ITOR2		0x02
#define DCR_DCP0_ITOR3		0x03
#define DCR_DCP0_ADDR0		0x04
#define DCR_DCP0_ADDR1		0x05
#define DCR_DCP0_CFG		0x40
#define DCR_DCP0_ID		0x41
#define DCR_DCP0_VER		0x42
#define DCR_DCP0_PLBBEAR	0x50
#define DCR_DCP0_MEMBEAR	0x51
#define DCR_DCP0_ESR		0x52
#define DCR_DCP0_RAM0		0x400

#endif /* _DCR405GP_H_ */
