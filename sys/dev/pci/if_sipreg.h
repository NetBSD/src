/*	$NetBSD: if_sipreg.h,v 1.1.4.2 2001/03/12 13:31:08 bouyer Exp $	*/

/*-
 * Copyright (c) 1999 Network Computer, Inc.
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
 * 3. Neither the name of Network Computer, Inc. nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY NETWORK COMPUTER, INC. AND CONTRIBUTORS
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

#ifndef _DEV_PCI_IF_SIPREG_H_
#define	_DEV_PCI_IF_SIPREG_H_

/*
 * Register description for the Silicon Integrated Systems SiS 900,
 * SiS 7016, and National Semiconductor DP83815 10/100 PCI Ethernet
 * controller.
 *
 * Written by Jason R. Thorpe for Network Computer, Inc.
 */

/*
 * Transmit FIFO size.  Used to compute the transmit drain threshold.
 *
 * The transmit FIFO is arranged as a 512 32-bit memory array.
 */
#define	SIP_TXFIFO_SIZE	(512 * 4)

/*
 * The SiS900 uses a single descriptor format for both transmit
 * and receive descriptor chains.
 */
struct sip_desc {
	u_int32_t	sipd_link;	/* link to next descriptor */
	u_int32_t	sipd_cmdsts;	/* command/status word */
	u_int32_t	sipd_bufptr;	/* pointer to DMA segment */
};

/*
 * CMDSTS bits common to transmit and receive.
 */
#define	CMDSTS_OWN	0x80000000	/* owned by consumer */
#define	CMDSTS_MORE	0x40000000	/* more descriptors */
#define	CMDSTS_INTR	0x20000000	/* interrupt when ownership changes */
#define	CMDSTS_SUPCRC	0x10000000	/* suppress CRC */
#define	CMDSTS_OK	0x08000000	/* packet ok */
#define	CMDSTS_SIZE_MASK 0x000007ff	/* packet size */

#define	CMDSTS_SIZE(x)	((x) & CMDSTS_SIZE_MASK)

/*
 * CMDSTS bits for transmit.
 */
#define	CMDSTS_Tx_TXA	0x04000000	/* transmit abort */
#define	CMDSTS_Tx_TFU	0x02000000	/* transmit FIFO underrun */
#define	CMDSTS_Tx_CRS	0x01000000	/* carrier sense lost */
#define	CMDSTS_Tx_TD	0x00800000	/* transmit deferred */
#define	CMDSTS_Tx_ED	0x00400000	/* excessive deferral */
#define	CMDSTS_Tx_OWC	0x00200000	/* out of window collision */
#define	CMDSTS_Tx_EC	0x00100000	/* excessive collisions */
#define	CMDSTS_Tx_CCNT	0x000f0000	/* collision count */

#define	CMDSTS_COLLISIONS(x)	(((x) & CMDSTS_Tx_CCNT) >> 16)

/*
 * CMDSTS bits for receive.
 */
#define	CMDSTS_Rx_RXA	0x04000000	/* receive abort */
#define	CMDSTS_Rx_RXO	0x02000000	/* receive overrun */
#define	CMDSTS_Rx_DEST	0x01800000	/* destination class */
#define	CMDSTS_Rx_LONG	0x00400000	/* packet too long */
#define	CMDSTS_Rx_RUNT	0x00200000	/* runt packet */
#define	CMDSTS_Rx_ISE	0x00100000	/* invalid symbol error */
#define	CMDSTS_Rx_CRCE	0x00080000	/* CRC error */
#define	CMDSTS_Rx_FAE	0x00040000	/* frame alignment error */
#define	CMDSTS_Rx_LBP	0x00020000	/* loopback packet */
#define	CMDSTS_Rx_COL	0x00010000	/* collision activity */

#define	CMDSTS_Rx_DEST_REJ 0x00000000	/* packet rejected */
#define	CMDSTS_Rx_DEST_STA 0x00800000	/* matched station address */
#define	CMDSTS_Rx_DEST_MUL 0x01000000	/* multicast address */
#define	CMDSTS_Rx_DEST_BRD 0x01800000	/* broadcast address */

/*
 * PCI Configuration space registers.
 */
#define	SIP_PCI_CFGIOA	(PCI_MAPREG_START + 0x00)

#define	SIP_PCI_CFGMA	(PCI_MAPREG_START + 0x04)

#define	SIP_PCI_CFGEROMA 0x30		/* expansion ROM address */

#define	SIP_PCI_CFGPMC	 0x40		/* power management cap. */

#define	SIP_PCI_CFGPMCSR 0x44		/* power management ctl. */

/*
 * MAC Operation Registers
 */
#define	SIP_CR		0x00	/* command register */
#define	CR_RST		0x00000100	/* software reset */
#define	CR_SWI		0x00000080	/* software interrupt */
#define	CR_RXR		0x00000020	/* receiver reset */
#define	CR_TXR		0x00000010	/* transmit reset */
#define	CR_RXD		0x00000008	/* receiver disable */
#define	CR_RXE		0x00000004	/* receiver enable */
#define	CR_TXD		0x00000002	/* transmit disable */
#define	CR_TXE		0x00000001	/* transmit enable */

#define	SIP_CFG		0x04	/* configuration register */
#define	CFG_LNKSTS	0x80000000	/* link status (83815) */
#define	CFG_SPEED100	0x40000000	/* 100Mb/s (83815) */
#define	CFG_FDUP	0x20000000	/* full duplex (83815) */
#define	CFG_POL		0x10000000	/* 10Mb/s polarity (83815) */
#define	CFG_ANEG_DN	0x08000000	/* autonegotiation done (83815) */
#define	CFG_PHY_CFG	0x00fc0000	/* PHY configuration (83815) */
#define	CFG_PINT_ACEN	0x00020000	/* PHY interrupt auto clear (83815) */
#define	CFG_PAUSE_ADV	0x00010000	/* pause advertise (83815) */
#define	CFG_ANEG_SEL	0x0000e000	/* autonegotiation select (83815) */
#define	CFG_PHY_RST	0x00000400	/* PHY reset (83815) */
#define	CFG_PHY_DIS	0x00000200	/* PHY disable (83815) */
#define	CFG_EUPHCOMP	0x00000100	/* 83810 descriptor compat (83815) */
#define	CFG_REQALG	0x00000080	/* PCI bus request alg. */
#define	CFG_SB		0x00000040	/* single backoff */
#define	CFG_POW		0x00000020	/* program out of window timer */
#define	CFG_EXD		0x00000010	/* excessive defferal timer disable */
#define	CFG_PESEL	0x00000008	/* parity error detection action */
#define	CFG_BEM		0x00000001	/* big-endian mode */

#define	SIP_EROMAR	0x08	/* EEPROM access register */
#define	EROMAR_EECS	0x00000008	/* chip select */
#define	EROMAR_EESK	0x00000004	/* clock */
#define	EROMAR_EEDO	0x00000002	/* data out */
#define	EROMAR_EEDI	0x00000001	/* data in */

#define	SIP_PTSCR	0x0c	/* PCI test control register */
#define	PTSCR_DIS_TEST	0x40000000	/* discard timer test mode */
#define	PTSCR_EROM_TACC	0x0f000000	/* boot rom access time */
#define	PTSCR_TRRAMADR	0x001ff000	/* TX/RX RAM address */
#define	PTSCR_BMTEN	0x00000200	/* bus master test enable */
#define	PTSCR_RRTMEN	0x00000080	/* receive RAM test mode enable */
#define	PTSCR_TRTMEN	0x00000040	/* transmit RAM test mode enable */
#define	PTSCR_SRTMEN	0x00000020	/* status RAM test mode enable */
#define	PTSCR_SRAMADR	0x0000001f	/* status RAM address */

#define	SIP_ISR		0x10	/* interrupt status register */
#define	ISR_WAKEEVT	0x10000000	/* wake up event */
#define	ISR_PAUSE_END	0x08000000	/* end of transmission pause */
#define	ISR_PAUSE_ST	0x04000000	/* start of transmission pause */
#define	ISR_TXRCMP	0x02000000	/* transmit reset complete */
#define	ISR_RXRCMP	0x01000000	/* receive reset complete */
#define	ISR_DPERR	0x00800000	/* detected parity error */
#define	ISR_SSERR	0x00400000	/* signalled system error */
#define	ISR_RMABT	0x00200000	/* received master abort */
#define	ISR_RTABT	0x00100000	/* received target abort */
#define	ISR_RXSOVR	0x00010000	/* Rx status FIFO overrun */
#define	ISR_HIBERR	0x00008000	/* high bits error set */
#define	ISR_SWI		0x00001000	/* software interrupt */
#define	ISR_TXURN	0x00000400	/* Tx underrun */
#define	ISR_TXIDLE	0x00000200	/* Tx idle */
#define	ISR_TXERR	0x00000100	/* Tx error */
#define	ISR_TXDESC	0x00000080	/* Tx descriptor interrupt */
#define	ISR_TXOK	0x00000040	/* Tx okay */
#define	ISR_RXORN	0x00000020	/* Rx overrun */
#define	ISR_RXIDLE	0x00000010	/* Rx idle */
#define	ISR_RXEARLY	0x00000008	/* Rx early */
#define	ISR_RXERR	0x00000004	/* Rx error */
#define	ISR_RXDESC	0x00000002	/* Rx descriptor interrupt */
#define	ISR_RXOK	0x00000001	/* Rx okay */

#define	SIP_IMR		0x14	/* interrupt mask register */
/* See bits in SIP_ISR */

#define	SIP_IER		0x18	/* interrupt enable register */
#define	IER_IE		0x00000001	/* master interrupt enable */

#define	SIP_ENPHY	0x1c	/* enhanced PHY access register */
#define	ENPHY_PHYDATA	0xffff0000	/* PHY data */
#define	ENPHY_DATA_SHIFT 16
#define	ENPHY_PHYADDR	0x0000f800	/* PHY number (7016 only) */
#define	ENPHY_PHYADDR_SHIFT 11
#define	ENPHY_REGADDR	0x000007c0	/* PHY register */
#define	ENPHY_REGADDR_SHIFT 6
#define	ENPHY_RWCMD	0x00000020	/* 1 == read, 0 == write */
#define	ENPHY_ACCESS	0x00000010	/* PHY access enable */

#define	SIP_TXDP	0x20	/* transmit descriptor pointer reg */

#define	SIP_TXCFG	0x24	/* transmit configuration register */
#define	TXCFG_CSI	0x80000000	/* carrier sense ignore */
#define	TXCFG_HBI	0x40000000	/* heartbeat ignore */
#define	TXCFG_MLB	0x20000000	/* MAC loopback */
#define	TXCFG_ATP	0x10000000	/* automatic transmit padding */
#define	TXCFG_MXDMA	0x00700000	/* max DMA burst size */
#define	TXCFG_MXDMA_512	0x00000000	/*     512 bytes */
#define	TXCFG_MXDMA_4	0x00100000	/*       4 bytes */
#define	TXCFG_MXDMA_8	0x00200000	/*       8 bytes */
#define	TXCFG_MXDMA_16	0x00300000	/*      16 bytes */
#define	TXCFG_MXDMA_32	0x00400000	/*      32 bytes */
#define	TXCFG_MXDMA_64	0x00500000	/*      64 bytes */
#define	TXCFG_MXDMA_128	0x00600000	/*     128 bytes */
#define	TXCFG_MXDMA_256	0x00700000	/*     256 bytes */
#define	TXCFG_FLTH	0x00003f00	/* Tx fill threshold */
#define	TXCFG_FLTH_SHIFT 8
#define	TXCFG_DRTH	0x0000003f	/* Tx drain threshold */

#define	SIP_RXDP	0x30	/* receive desciptor pointer reg */

#define	SIP_RXCFG	0x34	/* receive configuration register */
#define	RXCFG_AEP	0x80000000	/* accept error packets */
#define	RXCFG_ARP	0x40000000	/* accept runt packets */
#define	RXCFG_ATX	0x10000000	/* accept transmit packets */
#define	RXCFG_AJAB	0x08000000	/* accept jabber packets */
#define	RXCFG_MXDMA	0x00700000	/* max DMA burst size */
#define	RXCFG_MXDMA_512	0x00000000	/*     512 bytes */
#define	RXCFG_MXDMA_4	0x00100000	/*       4 bytes */
#define	RXCFG_MXDMA_8	0x00200000	/*       8 bytes */
#define	RXCFG_MXDMA_16	0x00300000	/*      16 bytes */
#define	RXCFG_MXDMA_32	0x00400000	/*      32 bytes */
#define	RXCFG_MXDMA_64	0x00500000	/*      64 bytes */
#define	RXCFG_MXDMA_128	0x00600000	/*     128 bytes */
#define	RXCFG_MXDMA_256	0x00700000	/*     256 bytes */
#define	RXCFG_DRTH	0x0000003e
#define	RXCFG_DRTH_SHIFT 1

#define	SIP_FLOWCTL	0x38	/* flow control register */
#define	FLOWCTL_PAUSE	0x00000002	/* PAUSE flag */
#define	FLOWCTL_FLOWEN	0x00000001	/* enable flow control */

#define	SIP_NS_CCSR	0x3c	/* CLKRUN control/status register (83815) */
#define	CCSR_PMESTS	0x00008000	/* PME status */
#define	CCSR_PMEEN	0x00000100	/* PME enable */
#define	CCSR_CLKRUN_EN	0x00000001	/* clkrun enable */

#define	SIP_NS_WCSR	0x40	/* WoL control/status register (83815) */

#define	SIP_NS_PCR	0x44	/* pause control/status register (83815) */

#define	SIP_RFCR	0x48	/* receive filter control register */
#define	RFCR_RFEN	0x80000000	/* Rx filter enable */
#define	RFCR_AAB	0x40000000	/* accept all broadcast */
#define	RFCR_AAM	0x20000000	/* accept all multicast */
#define	RFCR_AAP	0x10000000	/* accept all physical */
#define	RFCR_APM	0x08000000	/* accept perfect match (83815) */
#define	RFCR_APAT	0x07800000	/* accept pattern match (83815) */
#define	RFCR_AARP	0x00400000	/* accept ARP (83815) */
#define	RFCR_MHEN	0x00200000	/* multicast hash enable (83815) */
#define	RFCR_UHEN	0x00100000	/* unicast hash enable (83815) */
#define	RFCR_ULM	0x00080000	/* U/L bit mask (83815) */
#define	RFCR_NS_RFADDR	0x000003ff	/* Rx filter ext reg address (83815) */
#define	RFCR_RFADDR	0x000f0000	/* Rx filter address */
#define	RFCR_RFADDR_NODE0 0x00000000	/* node address 1, 0 */
#define	RFCR_RFADDR_NODE2 0x00010000	/* node address 3, 2 */
#define	RFCR_RFADDR_NODE4 0x00020000	/* node address 5, 4 */
#define	RFCR_RFADDR_MC0	  0x00040000	/* multicast hash word 0 */
#define	RFCR_RFADDR_MC1	  0x00050000	/* multicast hash word 1 */
#define	RFCR_RFADDR_MC2	  0x00060000	/* multicast hash word 2 */
#define	RFCR_RFADDR_MC3	  0x00070000	/* multicast hash word 3 */
#define	RFCR_RFADDR_MC4	  0x00080000	/* multicast hash word 4 */
#define	RFCR_RFADDR_MC5	  0x00090000	/* multicast hash word 5 */
#define	RFCR_RFADDR_MC6	  0x000a0000	/* multicast hash word 6 */
#define	RFCR_RFADDR_MC7	  0x000b0000	/* multicast hash word 7 */

#define	RFCR_NS_RFADDR_PMATCH0	0x0000	/* perfect match octets 1-0 */
#define	RFCR_NS_RFADDR_PMATCH2	0x0002	/* perfect match octets 3-2 */
#define	RFCR_NS_RFADDR_PMATCH4	0x0004	/* perfect match octets 5-4 */
#define	RFCR_NS_RFADDR_PCOUNT	0x0006	/* pattern count */
#define	RFCR_NS_RFADDR_FILTMEM	0x0200	/* filter memory (hash/pattern) */

#define	SIP_RFDR	0x4c	/* receive filter data register */
#define	RFDR_BMASK	0x00030000	/* byte mask (83815) */
#define	RFDR_DATA	0x0000ffff	/* data bits */

#define	SIP_NS_BRAR	0x50	/* boot rom address (83815) */
#define	BRAR_AUTOINC	0x80000000	/* autoincrement */
#define	BRAR_ADDR	0x0000ffff	/* address */

#define	SIP_NS_BRDR	0x54	/* boot rom data (83815) */

#define	SIP_NS_SRR	0x58	/* silicon revision register (83815) */
#define	SRR_REV_A	0x00000101
#define	SRR_REV_B_1	0x00000200
#define	SRR_REV_B_2	0x00000201
#define	SRR_REV_B_3	0x00000203
#define	SRR_REV_C_1	0x00000300
#define	SRR_REV_C_2	0x00000302

#define	SIP_NS_MIBC	0x5c	/* mib control register (83815) */
#define	MIBC_MIBS	0x00000008	/* mib counter strobe */
#define	MIBC_ACLR	0x00000004	/* clear all counters */
#define	MIBC_FRZ	0x00000002	/* freeze all counters */
#define	MIBC_WRN	0x00000001	/* warning test indicator */

#define	SIP_NS_MIB(mibreg)	/* mib data registers (83815) */	\
	(0x60 + (mibreg))
#define	MIB_RXErroredPkts	0x00
#define	MIB_RXFCSErrors		0x04
#define	MIB_RXMsdPktErrors	0x08
#define	MIB_RXFAErrors		0x0c
#define	MIB_RXSymbolErrors	0x10
#define	MIB_RXFrameTooLong	0x14
#define	MIB_RXTXSQEErrors	0x18

#define	SIP_NS_PHY(miireg)	/* PHY registers (83815) */		\
	(0x80 + ((miireg) << 2))

#define	SIP_PMCTL	0xb0	/* power management control register */
#define	PMCTL_GATECLK	0x80000000	/* gate dual clock enable */
#define	PMCTL_WAKEALL	0x40000000	/* wake on all Rx OK */
#define	PMCTL_FRM3ACS	0x04000000	/* 3rd wake-up frame access */
#define	PMCTL_FRM2ACS	0x02000000	/* 2nd wake-up frame access */
#define	PMCTL_FRM1ACS	0x01000000	/* 1st wake-up frame access */
#define	PMCTL_FRM3EN	0x00400000	/* 3rd wake-up frame match enable */
#define	PMCTL_FRM2EN	0x00200000	/* 2nd wake-up frame match enable */
#define	PMCTL_FRM1EN	0x00100000	/* 1st wake-up frame match enable */
#define	PMCTL_ALGORITHM	0x00000800	/* Magic Packet match algorithm */
#define	PMCTL_MAGICPKT	0x00000400	/* Magic Packet match enable */
#define	PMCTL_LINKON	0x00000002	/* link on monitor enable */
#define	PMCTL_LINKLOSS	0x00000001	/* link loss monitor enable */

#define	SIP_PMEVT	0xb4	/* power management wake-up evnt reg */
#define	PMEVT_ALLFRMMAT	0x40000000	/* receive packet ok */
#define	PMEVT_FRM3MAT	0x04000000	/* match 3rd wake-up frame */
#define	PMEVT_FRM2MAT	0x02000000	/* match 2nd wake-up frame */
#define	PMEVT_FRM1MAT	0x01000000	/* match 1st wake-up frame */
#define	PMEVT_MAGICPKT	0x00000400	/* Magic Packet */
#define	PMEVT_ONEVT	0x00000002	/* link on event */
#define	PMEVT_LOSSEVT	0x00000001	/* link loss event */

#define	SIP_WAKECRC	0xbc	/* wake-up frame CRC register */

#define	SIP_WAKEMASK0	0xc0	/* wake-up frame mask registers */
#define	SIP_WAKEMASK1	0xc4
#define	SIP_WAKEMASK2	0xc8
#define	SIP_WAKEMASK3	0xcc
#define	SIP_WAKEMASK4	0xe0
#define	SIP_WAKEMASK5	0xe4
#define	SIP_WAKEMASK6	0xe8
#define	SIP_WAKEMASK7	0xec

/*
 * Serial EEPROM opcodes, including the start bit.
 */
#define	SIP_EEPROM_OPC_ERASE	0x04
#define	SIP_EEPROM_OPC_WRITE	0x05
#define	SIP_EEPROM_OPC_READ	0x06

/*
 * Serial EEPROM address map (byte address) for the SiS900.
 */
#define	SIP_EEPROM_SIGNATURE	0x00	/* SiS 900 signature */
#define	SIP_EEPROM_MASK		0x02	/* `enable' mask */
#define	SIP_EEPROM_VENDOR_ID	0x04	/* PCI vendor ID */
#define	SIP_EEPROM_DEVICE_ID	0x06	/* PCI device ID */
#define	SIP_EEPROM_SUBVENDOR_ID	0x08	/* PCI subvendor ID */
#define	SIP_EEPROM_SUBSYSTEM_ID	0x0a	/* PCI subsystem ID */
#define	SIP_EEPROM_PMC		0x0c	/* PCI power management capabilities */
#define	SIP_EEPROM_reserved	0x0e	/* reserved */
#define	SIP_EEPROM_ETHERNET_ID0	0x10	/* Ethernet address 0, 1 */
#define	SIP_EEPROM_ETHERNET_ID1	0x12	/* Ethernet address 2, 3 */
#define	SIP_EEPROM_ETHERNET_ID2	0x14	/* Ethernet address 4, 5 */
#define	SIP_EEPROM_CHECKSUM	0x16	/* checksum */

/*
 * Serial EEPROM data (byte addresses) for the DP83815.
 */
#define	SIP_DP83815_EEPROM_CHECKSUM	0x16	/* checksum */
#define	SIP_DP83815_EEPROM_LENGTH	0x18	/* length of EEPROM data */

#endif /* _DEV_PCI_IF_SIPREG_H_ */
