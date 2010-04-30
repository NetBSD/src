/*	$NetBSD: emacreg.h,v 1.2.76.1 2010/04/30 14:39:42 uebayasi Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Simon Burge and Eduardo Horvath for Wasabi Systems, Inc.
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

#ifndef _IBM4XX_EMACREG_H_
#define	_IBM4XX_EMACREG_H_

#define EMAC_MAX_MTU		9022


/* Number of Ethernet MAC Registers */
#define	EMAC_NREG		0x100

/* Ethernet MAC Registers */
#define	EMAC_MR0		0x00	/* Mode Register 0 */
#define	  MR0_RXI		  0x80000000	/* Receive MAC Idle */
#define	  MR0_TXI		  0x40000000	/* Transmit MAC Idle */
#define	  MR0_SRST		  0x20000000	/* Soft Reset */
#define	  MR0_TXE		  0x10000000	/* Transmit MAC Enable */
#define	  MR0_RXE		  0x08000000	/* Receive MAC Enable */
#define	  MR0_WKE		  0x04000000	/* Wake-up Enable */

#define	EMAC_MR1		0x04	/* Mode Register 1 */
#define	  MR1_FDE		  0x80000000	/* Full-Duplex Enable */
#define	  MR1_ILE		  0x40000000	/* Internal Loop-back Enable */
#define	  MR1_VLE		  0x20000000	/* VLAN Enable */
#define	  MR1_EIFC		  0x10000000	/* Enable Integrated Flow Control */
#define	  MR1_APP		  0x08000000	/* Allow Pause Packet */
#define	  MR1_IST		  0x01000000	/* Ignore SQE Test */
#define	  MR1_MF_MASK		  0x00c00000	/* Medium Frequency mask */
#define	  MR1_MF_10MBS		  0x00000000	/* 10MB/sec */
#define	  MR1_MF_100MBS		  0x00400000	/* 100MB/sec */
#define	  MR1_MF_1000MBS	  0x00800000	/* 1000MB/sec */
#define	  MR1_RFS(fs)				/* Receive FIFO size */ \
				  (((fs) << 20) & 0x00300000)
#define	  MR1_TFS(fs)				/* Transmit FIFO size */ \
				  (((fs) << 18) & 0x000c0000)
#define	  MR1_RFS_GBE(fs)			/* GbE's Receive FIFO size */ \
				  (((fs) << 19) & 0x00380000)
#define	  MR1_TFS_GBE(fs)			/* GbE's Transmit FIFO size */ \
				  (((fs) << 16) & 0x00070000)
#define	  MR1__FS_512		  0
#define	  MR1__FS_1KB		  1
#define	  MR1__FS_2KB		  2
#define	  MR1__FS_4KB		  3
#define	  MR1__FS_8KB		  4
#define	  MR1__FS_16KB		  5
#define	  MR1_TR0_MASK		  0x00018000	/* Transmit Request 0 */
#define	  MR1_TR0_SINGLE	  0x00000000	/* Single Packet mode */
#define	  MR1_TR0_MULTIPLE	  0x00008000	/* Multiple Packet mode */
#define	  MR1_TR0_DEPENDANT	  0x00010000	/* Dependent Mode */
#define	  MR1_TR1_MASK		  0x00006000	/* Transmit Request 1 */
#define	  MR1_TR1_SINGLE	  0x00000000	/* Single Packet mode */
#define	  MR1_TR1_MULTIPLE	  0x00002000	/* Multiply Packet mode */
#define	  MR1_TR1_DEPENDANT	  0x00004000	/* Dependent Mode */
#define	  MR1_MWSW_MASK		  0x00007000	/* Maximum Waiting Status Words (GbE) */
#define	  MR1_MWSW_SHIFT	  12
#define	  MR1_JPSM		  0x00000800	/* Jumbo Packet Support Mode (GbE) */
#define	  MR1_IPPA_MASK		  0x000007c0	/* Internal PCS PHY Address (GbE) */
#define	  MR1_IPPA_SHIFT	  6
#define	  MR1_OBCI(opbc)	  ((opbc) << 3)	/* OPB Bus Clock Indication (GbE) */

#define	EMAC_TMR0		0x08	/* Transmit Mode Register 0 */
#define	  TMR0_GNP0		  0x80000000	/* Get New Packet for Channel 0 */
#define	  TMR0_GNP1		  0x40000000	/* Get New Packet for Channel 1 */
#define	  TMR0_GNPD		  0x20000000	/* Get New Packet for Dependent mode */
#define	  TMR0_FC_MASK		  0x10000000	/* First Channel */
#define	  TMR0_FC_CHAN0		  0x00000000	/* Channel 0 */
#define	  TMR0_FC_CHAN1		  0x10000000	/* Channel 1 */
#define	  TMR0_TFAE_MASK	  0x00000007	/* TX FIFO Almost Empty */
#define	  TMR0_TFAE_2		  0x00000001	/*   Number of used entries <= 2(32B) */
#define	  TMR0_TFAE_4		  0x00000002	/*   Number of used entries <= 4(64B) */
#define	  TMR0_TFAE_8		  0x00000003	/*   Number of used entries <= 8(128B) */
#define	  TMR0_TFAE_16		  0x00000004	/*   Number of used entries <= 16(256B) */
#define	  TMR0_TFAE_32		  0x00000005	/*   Number of used entries <= 32(512B) */
#define	  TMR0_TFAE_64		  0x00000006	/*   Number of used entries <= 64(1024B) */
#define	  TMR0_TFAE_128		  0x00000007	/*   Number of used entries <= 128(2048B) */

#define	EMAC_TMR1		0x0c	/* Transmit Mode Register 1 */
#define	  TMR1_TLR_MASK		  0xf8000000	/* Transmit Low Request */
#define	  TMR1_TLR_SHIFT	  27
#define	  TMR1_TUR_MASK		  0x00ff0000	/* Transmit Urgent Request */
#define	  TMR1_TUR_SHIFT	  16

#define	EMAC_RMR		0x10	/* Receive Mode Register */
#define	  RMR_SP		  0x80000000	/* Strip Padding */
#define	  RMR_SFCS		  0x40000000	/* Strip FCS */
#define	  RMR_RRP		  0x20000000	/* Receive Runt Packets */
#define	  RMR_RFP		  0x10000000	/* Receive FCS Packets */
#define	  RMR_ROP		  0x08000000	/* Receive Oversize Packets */
#define	  RMR_RPIR		  0x04000000	/* Receive Packets with In Range Error */
#define	  RMR_PPP		  0x02000000	/* Propagate Pause Packet */
#define	  RMR_PME		  0x01000000	/* Promiscuous Mode Enable */
#define	  RMR_PMME		  0x00800000	/* Promiscuous Multicast Mode Enable */
#define	  RMR_IAE		  0x00400000	/* Individual Address Enable */
#define	  RMR_MIAE		  0x00200000	/* Multiple Individual Address Enable */
#define	  RMR_BAE		  0x00100000	/* Broadcast Address Enable */
#define	  RMR_MAE		  0x00080000	/* Multicast Address Enable */
#define	  RMR_NIPMAE		  0x00040000	/* Non-IP Multicast Address Enable */
#define	  RMR_RFAF_MASK		  0x00000007	/* RX FIFO Almost Full - IRQ threshold */
#define	  RMR_TFAE_2		  0x00000001	/*   Number of used entries <= 2(32B) */
#define	  RMR_TFAE_4		  0x00000002	/*   Number of used entries <= 4(64B) */
#define	  RMR_TFAE_8		  0x00000003	/*   Number of used entries <= 8(128B) */
#define	  RMR_TFAE_16		  0x00000004	/*   Number of used entries <= 16(256B) */
#define	  RMR_TFAE_32		  0x00000005	/*   Number of used entries <= 32(512B) */
#define	  RMR_TFAE_64		  0x00000006	/*   Number of used entries <= 64(1024B) */
#define	  RMR_TFAE_128		  0x00000007	/*   Number of used entries <= 128(2048B) */

#define	EMAC_ISR		0x14	/* Interrupt Status Register */
#define	  ISR_TXPE		  0x20000000	/* TX Parity Error */
#define	  ISR_RXPE		  0x10000000	/* RX Parity Error */
#define	  ISR_TXUE		  0x08000000	/* TX Underrun Event */
#define	  ISR_RXOE		  0x04000000	/* RX Overrun Event */
#define	  ISR_OVR		  0x02000000	/* Overrun Error */
#define	  ISR_PP		  0x01000000	/* Pause Packet */
#define	  ISR_BP		  0x00800000	/* Bad Packet */
#define	  ISR_RP		  0x00400000	/* Runt Packet */
#define	  ISR_SE		  0x00200000	/* Short Event */
#define	  ISR_ALE		  0x00100000	/* Alignment Error */
#define	  ISR_BFCS		  0x00080000	/* Bad FCS */
#define	  ISR_PTLE		  0x00040000	/* Packet Too Long Error */
#define	  ISR_ORE		  0x00020000	/* Out of Range Error */
#define	  ISR_IRE		  0x00010000	/* In Range Error */
#define	  ISR_DBDM		  0x00000200	/* Dead Bit Dependent Mode */
#define	  ISR_DB0		  0x00000100	/* Dead Bit 0 */
#define	  ISR_SE0		  0x00000080	/* Signal Quality Error 0 (SQE) */
#define	  ISR_TE0		  0x00000040	/* Transmit Error 0 */
#define	  ISR_DB1		  0x00000020	/* Dead Bit 1 */
#define	  ISR_SE1		  0x00000010	/* Signal Quality Error 1 */
#define	  ISR_TE1		  0x00000008	/* Transmit Error 1 */
#define	  ISR_MOS		  0x00000002	/* MMA Operation Succeeded */
#define	  ISR_MOF		  0x00000001	/* MMA Operation Failed */

#define	  ISR_ALL	(                    ISR_TXPE| ISR_RXPE| \
			 ISR_TXUE| ISR_RXOE| ISR_OVR | ISR_PP  | \
			 ISR_BP  | ISR_RP  | ISR_SE  | ISR_ALE | \
			 ISR_BFCS| ISR_PTLE| ISR_ORE | ISR_IRE | \
			                     ISR_DBDM| ISR_DB0 | \
			 ISR_SE0 | ISR_TE0 | ISR_DB1 | ISR_SE1 | \
			 ISR_TE1 |           ISR_MOS | ISR_MOF)

#define	EMAC_ISER		0x18	/* Interrupt Status Enable Register */
#define	  ISER_TXPE		  ISR_TXPE
#define	  ISER_RXPE		  ISR_RXPE
#define	  ISER_TXUE		  ISR_TXUE
#define	  ISER_RXOE		  ISR_RXOE
#define	  ISER_OVR		  ISR_OVR
#define	  ISER_PP		  ISR_PP
#define	  ISER_BP		  ISR_BP
#define	  ISER_RP		  ISR_RP
#define	  ISER_SE		  ISR_SE
#define	  ISER_ALE		  ISR_ALE
#define	  ISER_BFCS		  ISR_BFCS
#define	  ISER_PTLE		  ISR_PTLE
#define	  ISER_ORE		  ISR_ORE
#define	  ISER_IRE		  ISR_IRE
#define	  ISER_DBDM		  ISR_DBDM
#define	  ISER_DB0		  ISR_DB0
#define	  ISER_SE0		  ISR_SE0
#define	  ISER_TE0		  ISR_TE0
#define	  ISER_DB1		  ISR_DB1
#define	  ISER_SE1		  ISR_SE1
#define	  ISER_TE1		  ISR_TE1
#define	  ISER_MOS		  ISR_MOS
#define	  ISER_MOF		  ISR_MOF

#define	EMAC_IAHR		0x1c	/* Individual Address High Register */
#define	EMAC_IALR		0x20	/* Individual Address Low Register */
#define	EMAC_VTPID		0x24	/* VLAN TPID Register */
#define	EMAC_VTCI		0x28	/* VLAN TCI Register */
#define	EMAC_PTR		0x2c	/* Pause Timer Register */
#define	EMAC_NHT64		4
#define	EMAC_IAHT64(n)		0x30	/* 64b Individual Address Hash Table */
#define	EMAC_GAHT64(n)		0x40	/* 64b Group Address Hash Table */
#define	EMAC_LSAH		0x50	/* Last Source Address High */
#define	EMAC_LSAL		0x54	/* Last Source Address Low */
#define	EMAC_IPGVR		0x58	/* Inter-Packet Gap Value Register */

#define	EMAC_STACR		0x5c	/* STA Control Register */
#define	  STACR_PHYD		  0xffff0000	/* PHY data mask */
#define	  STACR_PHYD_SHIFT	  16
#define	  STACR_OC		  0x00008000	/* operation complete */
#define	  STACR_PHYE		  0x00004000	/* PHY error */
#define	  STACR_WRITE		  0x00002000	/* STA command - write */
#define	  STACR_READ		  0x00001000	/* STA command - read */
#define	  STACR_OPBC_50MHZ	  0x0		/*   -  50MHz */
#define	  STACR_OPBC_66MHZ	  0x1		/*   -  66MHz */
#define	  STACR_OPBC_83MHZ	  0x2		/*   -  83MHz */
#define	  STACR_OPBC_100MHZ	  0x3		/*   - 100MHz */
#define	  STACR_OPBC_A100MHZ	  0x4		/*   - Abobe 100MHz (GbE) */
#define	  STACR_OPBC(opbc)	  ((opbc) << 10)	/* OPB bus clock freq (!GbE)*/
#define	  STACR_PCDA		  0x000003e0	/* PHY cmd dest address mask */
#define	  STACR_PCDA_SHIFT	  5
#define	  STACR_PRA		  0x0000001f	/* PHY register address mask */
#define	  STACR_PRA_SHIFT	  0

#define	  STACR_IMS		  0x00002000	/* Indirect Mode Selection (405EX/440SPe) */
#define	  STACR_STAOPC_MASK	  0x00001800	/* STA Opcode (405EX/440SPe) */
#define	  STACR_STAOPC_ADDRESS    0x00000000	/*   (IMS=1) Address */
#define	  STACR_STAOPC_WRITE	  0x00000800	/*   Write */
#define	  STACR_STAOPC_READ	  0x00001000	/*   Read */
#define	  STACR_STAOPC_READINC    0x00001800	/*   (IMS=1) Read Inc. */

#define	EMAC_TRTR		0x60	/* Transmit Request Threshold Register */
#define	  TRTR_64		  0x00000000	/* 64 bytes */
#define	  TRTR_128		  0x08000000	/* 128 bytes */
#define	  TRTR_192		  0x10000000	/* 192 bytes */
#define	  TRTR_256		  0x18000000	/* 256 bytes */
/* ... and so on +64 until ... */
#define	  TRTR_2048		  0xf8000000	/* 2048 bytes */

#define	EMAC_RWMR		0x64	/* Receive Low/High Water Mark Register */
#define	  RWMR_RLWM_MASK	  0xff800000	/* Receive Low Water Mark */
#define	  RWMR_RLWM_SHIFT	    23
#define	  RWMR_RHWM_MASK	  0x0000ff80	/* Receive High Water Mark */
#define	  RWMR_RHWM_SHIFT	    7

#define	EMAC_OCTX		0x68	/* Number of Octets Transmitted */
#define	EMAC_OCRX		0x6c	/* Number of Octets Received */

#define	EMAC_IPCR		0x70	/* Internal PCS Configuration Register */
#define	  IPCR_OUI_MASK		  0xfffffc00	/* OUI Value */
#define	  IPCR_OUI_SHIFT	  10
#define	  IPCR_MMN_MASK		  0x000003f0	/* Manufacture Model Number */
#define	  IPCR_MMN_SHIFT	  4
#define	  IPCR_REVID_MASK	  0x0000000f	/* Revision Number */
#define	  IPCR_REVID_SHIFT	  0

#define	EMAC_REVID		0x74	/* Revision ID Register */
#define	  REVID_REVISION(v)	  (((v) >> 8) & 0xfff)	/* Revision */
#define	  REVID_BRANCHREV(v)	  ((v) & 0xff)		/* Branch Revision */

#define	EMAC_NHT256		8
#define	EMAC_IAHT256(n)		0x80	/* 256b Individual Address Hash Table */
#define	EMAC_GAHT256(n)		0xa0	/* 256b Group Address Hash Table */

#define	EMAC_TPC		0xc0	/* Transmit Pause Control Register */
#define	  TPC_IPA		  0x80000000	/* Issue a Pause Packet */
#define	  TPC_TV_MASK		  0x7fff8000	/* Timer Value */
#define	  TPC_TV_SHIFT		  15

#endif	/* _IBM4XX_EMACREG_H_ */
