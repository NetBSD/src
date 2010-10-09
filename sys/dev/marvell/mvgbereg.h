/*	$NetBSD: mvgbereg.h,v 1.1.4.3 2010/10/09 03:32:08 yamt Exp $	*/
/*
 * Copyright (c) 2007 KIYOHARA Takashi
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef _MVGBEREG_H_
#define _MVGBEREG_H_

#define MVGBE_SIZE		0x4000

#define MVGBE_NWINDOW		6
#define MVGBE_NREMAP		4

#define MVGBE_PHY_TIMEOUT	10000	/* msec */

#define MVGBE_RX_CSUM_MIN_BYTE	72


/*
 * Ethernet Unit Registers
 */
/* Ethernet Unit Global Registers */
#define MVGBE_PHYADDR		0x2000
#if defined(MV88W8660)
#define MVGBE_SMI		0x8010
#else
#define MVGBE_SMI		0x2004
#endif
#define MVGBE_EUDA		0x2008	/* Ethernet Unit Default Address */
#define MVGBE_EUDID		0x200c	/* Ethernet Unit Default ID */
#define MVGBE_EU 		0x2014	/* Ethernet Unit Reserved */
#define MVGBE_EUIC 		0x2080	/* Ethernet Unit Interrupt Cause */
#define MVGBE_EUIM 		0x2084	/* Ethernet Unit Interrupt Mask */
#define MVGBE_EUEA 		0x2094	/* Ethernet Unit Error Address */
#define MVGBE_EUIAE 		0x2098	/* Ethernet Unit Internal Addr Error */
#define MVGBE_EUPCR 		0x20a0	/* EthernetUnit Port Pads Calibration */
#define MVGBE_EUC 		0x20b0	/* Ethernet Unit Control */

#define MVGBE_BASEADDR(n)	(0x2200 + ((n) << 3))	/* Base Address */
#define MVGBE_S(n)		(0x2204 + ((n) << 3))	/* Size */
#define MVGBE_HA(n)		(0x2280 + ((n) << 2))	/* High Address Remap */
#define MVGBE_BARE 		0x2290	/* Base Address Enable */
#define MVGBE_EPAP 		0x2294	/* Ethernet Port Access Protect */

/* Ethernet Unit Port Registers */
#define MVGBE_PORTR_BASE	0x2400
#define MVGBE_PORTR_SIZE	 0x400

#define MVGBE_PXC		0x000	/* Port Configuration */
#define MVGBE_PXCX		0x004	/* Port Configuration Extend */
#define MVGBE_MIISP		0x008	/* MII Serial Parameters */
#define MVGBE_GMIISP		0x00c	/* GMII Serial Params */
#define MVGBE_EVLANE		0x010	/* VLAN EtherType */
#define MVGBE_MACAL		0x014	/* MAC Address Low */
#define MVGBE_MACAH		0x018	/* MAC Address High */
#define MVGBE_SDC		0x01c	/* SDMA Configuration */
#define MVGBE_DSCP(n)		(0x020 + ((n) << 2))
#define MVGBE_PSC		0x03c	/* Port Serial Control0 */
#define MVGBE_VPT2P		0x040	/* VLAN Priority Tag to Priority */
#define MVGBE_PS		0x044	/* Ethernet Port Status */
#define MVGBE_TQC		0x048	/* Transmit Queue Command */
#define MVGBE_PSC1		0x04c	/* Port Serial Control1 */
#define MVGBE_MTU		0x058	/* Max Transmit Unit */
#define MVGBE_IC		0x060	/* Port Interrupt Cause */
#define MVGBE_ICE		0x064	/* Port Interrupt Cause Extend */
#define MVGBE_PIM		0x068	/* Port Interrupt Mask */
#define MVGBE_PEIM		0x06c	/* Port Extend Interrupt Mask */
#define MVGBE_PRFUT		0x070	/* Port Rx FIFO Urgent Threshold */
#define MVGBE_PTFUT		0x074	/* Port Tx FIFO Urgent Threshold */
#define MVGBE_PMFS		0x07c	/* Port Rx Minimal Frame Size */
#define MVGBE_PXDFC		0x084	/* Port Rx Discard Frame Counter */
#define MVGBE_POFC		0x088	/* Port Overrun Frame Counter */
#define MVGBE_PIAE		0x094	/* Port Internal Address Error */
#define MVGBE_TQFPC		0x0dc	/* Transmit Queue Fixed Priority Cfg */
#define MVGBE_CRDP(n)		(0x20c + ((n) << 4))
			/* Ethernet Current Receive Descriptor Pointers */
#define MVGBE_RQC		0x280	/* Receive Queue Command */
#define MVGBE_TCSDP		0x284	/* Tx Current Served Desc Pointer */
#define MVGBE_TCQDP		0x2c0	/* Tx Current Queue Desc Pointer */
#define MVGBE_TQTBCOUNT(q)	(0x300 + ((q) << 4))
				/* Transmit Queue Token-Bucket Counter */
#define MVGBE_TQTBCONFIG(q)	(0x304 + ((q) << 4))
				/* Transmit Queue Token-Bucket Configuration */
#define MVGBE_TQAC(q)		(0x308 + ((q) << 4))
				/* Transmit Queue Arbiter Configuration */

#define MVGBE_PORTDAFR_BASE	0x3400
#define MVGBE_PORTDAFR_SIZE	 0x400

#define MVGBE_NDFSMT		 0x40
#define MVGBE_DFSMT		0x000
			/* Destination Address Filter Special Multicast Table */
#define MVGBE_NDFOMT		 0x40
#define MVGBE_DFOMT		0x100
			/* Destination Address Filter Other Multicast Table */
#define MVGBE_NDFUT		  0x4
#define MVGBE_DFUT		0x200
			/* Destination Address Filter Unicast Table */


/* MAC MIB Counters 		0x3000 - 0x307c */



/* PHY Address (MVGBE_PHYADDR) */
#define MVGBE_PHYADDR_PHYAD_MASK	0x1f
#define MVGBE_PHYADDR_PHYAD(port, phy)	((phy) << ((port) * 5))

/* SMI register fields (MVGBE_SMI) */
#define MVGBE_SMI_DATA_MASK		0x0000ffff
#define MVGBE_SMI_PHYAD(phy)		(((phy) & 0x1f) << 16)
#define MVGBE_SMI_REGAD(reg)		(((reg) & 0x1f) << 21)
#define MVGBE_SMI_OPCODE_WRITE		(0 << 26)
#define MVGBE_SMI_OPCODE_READ		(1 << 26)
#define MVGBE_SMI_READVALID		(1 << 27)
#define MVGBE_SMI_BUSY			(1 << 28)

/* Ethernet Unit Default ID (MVGBE_EUDID) */
#define MVGBE_EUDID_DIDR_MASK		0x0000000f
#define MVGBE_EUDID_DATTR_MASK		0x00000ff0

/* Ethernet Unit Reserved (MVGBE_EU) */
#define MVGBE_EU_FASTMDC 		(1 << 0)
#define MVGBE_EU_ACCS 			(1 << 1)

/* Ethernet Unit Interrupt Cause (MVGBE_EUIC) */
#define MVGBE_EUIC_ETHERINTSUM 		(1 << 0)
#define MVGBE_EUIC_PARITY 		(1 << 1)
#define MVGBE_EUIC_ADDRVIOL		(1 << 2)
#define MVGBE_EUIC_ADDRVNOMATCH		(1 << 3)
#define MVGBE_EUIC_SMIDONE		(1 << 4)
#define MVGBE_EUIC_COUNTWA		(1 << 5)
#define MVGBE_EUIC_INTADDRERR		(1 << 7)
#define MVGBE_EUIC_PORT0DPERR		(1 << 9)
#define MVGBE_EUIC_TOPDPERR		(1 << 12)

/* Ethernet Unit Internal Addr Error (MVGBE_EUIAE) */
#define MVGBE_EUIAE_INTADDR_MASK 	0x000001ff

/* Ethernet Unit Port Pads Calibration (MVGBE_EUPCR) */
#define MVGBE_EUPCR_DRVN_MASK		0x0000001f
#define MVGBE_EUPCR_TUNEEN		(1 << 16)
#define MVGBE_EUPCR_LOCKN_MASK		0x003e0000
#define MVGBE_EUPCR_OFFSET_MASK		0x1f000000	/* Reserved */
#define MVGBE_EUPCR_WREN		(1 << 31)

/* Ethernet Unit Control (MVGBE_EUC) */
#define MVGBE_EUC_PORT0DPPAR 		(1 << 0)
#define MVGBE_EUC_TOPDPPAR	 	(1 << 3)
#define MVGBE_EUC_PORT0PW 		(1 << 16)

/* Base Address (MVGBE_BASEADDR) */
#define MVGBE_BASEADDR_TARGET(target)	((target) & 0xf)
#define MVGBE_BASEADDR_ATTR(attr)	(((attr) & 0xff) << 8)
#define MVGBE_BASEADDR_BASE(base)	((base) & 0xffff0000)

/* Size (MVGBE_S) */
#define MVGBE_S_SIZE(size)		(((size) - 1) & 0xffff0000)

/* Base Address Enable (MVGBE_BARE) */
#define MVGBE_BARE_EN_MASK		((1 << MVGBE_NWINDOW) - 1)
#define MVGBE_BARE_EN(win)		((1 << (win)) & MVGBE_BARE_EN_MASK)

/* Ethernet Port Access Protect (MVGBE_EPAP) */
#define MVGBE_EPAP_AC_NAC		0x0	/* No access allowed */
#define MVGBE_EPAP_AC_RO		0x1	/* Read Only */
#define MVGBE_EPAP_AC_FA		0x3	/* Full access (r/w) */
#define MVGBE_EPAP_EPAR(win, ac)	((ac) << ((win) * 2))

/* Port Configuration (MVGBE_PXC) */
#define MVGBE_PXC_UPM			(1 << 0) /* Uni Promisc mode */
#define MVGBE_PXC_RXQ(q)		((q) << 1)
#define MVGBE_PXC_RXQ_MASK		MVGBE_PXC_RXQ(7)
#define MVGBE_PXC_RXQARP(q)		((q) << 4)
#define MVGBE_PXC_RXQARP_MASK		MVGBE_PXC_RXQARP(7)
#define MVGBE_PXC_RB			(1 << 7) /* Rej mode of MAC */
#define MVGBE_PXC_RBIP			(1 << 8)
#define MVGBE_PXC_RBARP			(1 << 9)
#define MVGBE_PXC_AMNOTXES		(1 << 12)
#define MVGBE_PXC_TCPCAPEN		(1 << 14)
#define MVGBE_PXC_UDPCAPEN		(1 << 15)
#define MVGBE_PXC_TCPQ(q)		((q) << 16)
#define MVGBE_PXC_TCPQ_MASK		MVGBE_PXC_TCPQ(7)
#define MVGBE_PXC_UDPQ(q)		((q) << 19)
#define MVGBE_PXC_UDPQ_MASK		MVGBE_PXC_UDPQ(7)
#define MVGBE_PXC_BPDUQ(q)		((q) << 22)
#define MVGBE_PXC_BPDUQ_MASK		MVGBE_PXC_BPDUQ(7)
#define MVGBE_PXC_RXCS			(1 << 25)

/* Port Configuration Extend (MVGBE_PXCX) */
#define MVGBE_PXCX_SPAN			(1 << 1)

/* MII Serial Parameters (MVGBE_MIISP) */
#define MVGBE_MIISP_JAMLENGTH_12KBIT	0x00000000
#define MVGBE_MIISP_JAMLENGTH_24KBIT	0x00000001
#define MVGBE_MIISP_JAMLENGTH_32KBIT	0x00000002
#define MVGBE_MIISP_JAMLENGTH_48KBIT	0x00000003
#define MVGBE_MIISP_JAMIPG(x)		(((x) & 0x7c) << 0)
#define MVGBE_MIISP_IPGJAMTODATA(x)	(((x) & 0x7c) << 5)
#define MVGBE_MIISP_IPGDATA(x)		(((x) & 0x7c) << 10)
#define MVGBE_MIISP_DATABLIND(x)	(((x) & 0x1f) << 17)

/* GMII Serial Parameters (MVGBE_GMIISP) */
#define MVGBE_GMIISP_IPGDATA(x)		(((x) >> 4) & 0x7)

/* SDMA Configuration (MVGBE_SDC) */
#define MVGBE_SDC_RIFB			(1 << 0)
#define MVGBE_SDC_RXBSZ(x)		((x) << 1)
#define MVGBE_SDC_RXBSZ_MASK		MVGBE_SDC_RXBSZ(7)
#define MVGBE_SDC_RXBSZ_1_64BITWORDS	MVGBE_SDC_RXBSZ(0)
#define MVGBE_SDC_RXBSZ_2_64BITWORDS	MVGBE_SDC_RXBSZ(1)
#define MVGBE_SDC_RXBSZ_4_64BITWORDS	MVGBE_SDC_RXBSZ(2)
#define MVGBE_SDC_RXBSZ_8_64BITWORDS	MVGBE_SDC_RXBSZ(3)
#define MVGBE_SDC_RXBSZ_16_64BITWORDS	MVGBE_SDC_RXBSZ(4)
#define MVGBE_SDC_BLMR			(1 << 4)
#define MVGBE_SDC_BLMT			(1 << 5)
#define MVGBE_SDC_SWAPMODE		(1 << 6)
#define MVGBE_SDC_IPGINTRX(n)		((n) << 8)
#define MVGBE_SDC_IPGINTRX_MASK		MVGBE_SDC_IPGINTRX(0x3fff)
#define MVGBE_SDC_TXBSZ(x)		((x) << 22)
#define MVGBE_SDC_TXBSZ_MASK		MVGBE_SDC_TXBSZ(7)
#define MVGBE_SDC_TXBSZ_1_64BITWORDS	MVGBE_SDC_TXBSZ(0)
#define MVGBE_SDC_TXBSZ_2_64BITWORDS	MVGBE_SDC_TXBSZ(1)
#define MVGBE_SDC_TXBSZ_4_64BITWORDS	MVGBE_SDC_TXBSZ(2)
#define MVGBE_SDC_TXBSZ_8_64BITWORDS	MVGBE_SDC_TXBSZ(3)
#define MVGBE_SDC_TXBSZ_16_64BITWORDS	MVGBE_SDC_TXBSZ(4)

/* Port Serial Control (MVGBE_PSC) */
#define MVGBE_PSC_PORTEN		(1 << 0)
#define MVGBE_PSC_FLP			(1 << 1) /* Force_Link_Pass */
#define MVGBE_PSC_ANDUPLEX		(1 << 2)	/* auto nego */
#define MVGBE_PSC_ANFC			(1 << 3)
#define MVGBE_PSC_PAUSEADV		(1 << 4)
#define MVGBE_PSC_FFCMODE		(1 << 5)	/* Force FC */
#define MVGBE_PSC_FBPMODE		(1 << 7)	/* Back pressure */
#define MVGBE_PSC_RESERVED		(1 << 9)	/* Must be set to 1 */
#define MVGBE_PSC_FLFAIL		(1 << 10)	/* Force Link Fail */
#define MVGBE_PSC_ANSPEED		(1 << 13)
#define MVGBE_PSC_DTEADVERT		(1 << 14)
#define MVGBE_PSC_MRU(x)		((x) << 17)
#define MVGBE_PSC_MRU_MASK		MVGBE_PSC_MRU(7)
#define MVGBE_PSC_MRU_1518		0
#define MVGBE_PSC_MRU_1522		1
#define MVGBE_PSC_MRU_1552		2
#define MVGBE_PSC_MRU_9022		3
#define MVGBE_PSC_MRU_9192		4
#define MVGBE_PSC_MRU_9700		5
#define MVGBE_PSC_SETFULLDX		(1 << 21)
#define MVGBE_PSC_SETFCEN		(1 << 22)
#define MVGBE_PSC_SETGMIISPEED		(1 << 23)
#define MVGBE_PSC_SETMIISPEED		(1 << 24)

/* Ethernet Port Status (MVGBE_PS) */
#define MVGBE_PS_LINKUP			(1 << 1)
#define MVGBE_PS_FULLDX			(1 << 2)
#define MVGBE_PS_ENFC			(1 << 3)
#define MVGBE_PS_GMIISPEED		(1 << 4)
#define MVGBE_PS_MIISPEED		(1 << 5)
#define MVGBE_PS_TXINPROG		(1 << 7)
#define MVGBE_PS_TXFIFOEMP		(1 << 10)	/* FIFO Empty */

/* Transmit Queue Command (MVGBE_TQC) */
#define MVGBE_TQC_ENQ			(1 << 0)	/* Enable Q */
#define MVGBE_TQC_DISQ			(1 << 8)	/* Disable Q */

/* Port Serial Control 1 (MVGBE_PSC1) */
#define MVGBE_PSC1_PCSLB		(1 << 1)
#define MVGBE_PSC1_RGMIIEN		(1 << 3)	/* RGMII */
#define MVGBE_PSC1_PRST			(1 << 4)	/* Port Reset */

/* Port Interrupt Cause (MVGBE_IC) */
#define MVGBE_IC_RXBUF			(1 << 0)
#define MVGBE_IC_EXTEND			(1 << 1)
#define MVGBE_IC_RXBUFQ_MASK		(0xff << 2)
#define MVGBE_IC_RXBUFQ(q)		(1 << ((q) + 2))
#define MVGBE_IC_RXERROR		(1 << 10)
#define MVGBE_IC_RXERRQ_MASK		(0xff << 11)
#define MVGBE_IC_RXERRQ(q)		(1 << ((q) + 11))
#define MVGBE_IC_TXEND			(1 << 19)
#define MVGBE_IC_ETHERINTSUM		(1 << 31)

/* Port Interrupt Cause Extend (MVGBE_ICE) */
#define MVGBE_ICE_TXBUF			(1 << 0)
#define MVGBE_ICE_TXERR			(1 << 8)
#define MVGBE_ICE_PHYSTC		(1 << 16)
#define MVGBE_ICE_RXOVR			(1 << 18)
#define MVGBE_ICE_TXUDR			(1 << 19)
#define MVGBE_ICE_LINKCHG		(1 << 20)
#define MVGBE_ICE_INTADDRERR		(1 << 23)
#define MVGBE_ICE_ETHERINTSUM		(1 << 31)

/* Port Rx Minimal Frame Size (MVGBE_PMFS) */
#define MVGBE_PMFS_RXMFS(rxmfs)		(((rxmfs) - 40) & 0x7c)
					/* RxMFS = 40,44,48,52,56,60,64 bytes */

/* Transmit Queue Fixed Priority Configuration */
#define MVGBE_TQFPC_EN(q)		(1 << (q))

/* Receive Queue Command (MVGBE_RQC) */
#define MVGBE_RQC_ENQ_MASK		(0xff << 0)	/* Enable Q */
#define MVGBE_RQC_ENQ(n)		(1 << (0 + (n)))
#define MVGBE_RQC_DISQ_MASK		(0xff << 8)	/* Disable Q */
#define MVGBE_RQC_DISQ(n)		(1 << (8 + (n)))
#define MVGBE_RQC_DISQ_DISABLE(q)	((q) << 8)

/* Destination Address Filter Registers (MVGBE_DF{SM,OM,U}T) */
#define MVGBE_DF(n, x)			((x) << (8 * (n)))
#define MVGBE_DF_PASS			(1 << 0)
#define MVGBE_DF_QUEUE(q)		((q) << 1)
#define MVGBE_DF_QUEUE_MASK		((7) << 1)


#define MVGBE_MRU		9700	/* The Maximal Receive Packet Size */

#define MVGBE_BUF_ALIGN		8
#define MVGBE_BUF_MASK		(MVGBE_BUF_ALIGN - 1)
#define MVGBE_HWHEADER_SIZE	2


/*
 * DMA descriptors
 *    It is 32byte alignment.
 */
struct mvgbe_tx_desc {
#if BYTE_ORDER == BIG_ENDIAN
	uint16_t bytecnt;		/* Descriptor buffer byte count */
	uint16_t l4ichk;		/* CPU provided TCP Checksum */
	uint32_t cmdsts;		/* Descriptor command status */
	uint32_t nextdescptr;		/* Next descriptor pointer */
	uint32_t bufptr;		/* Descriptor buffer pointer */
#else	/* LITTLE_ENDIAN */
	uint32_t cmdsts;		/* Descriptor command status */
	uint16_t l4ichk;		/* CPU provided TCP Checksum */
	uint16_t bytecnt;		/* Descriptor buffer byte count */
	uint32_t bufptr;		/* Descriptor buffer pointer */
	uint32_t nextdescptr;		/* Next descriptor pointer */
#endif
	u_long returninfo;		/* User resource return information */
	uint8_t *alignbufptr;		/* Pointer to 8 byte aligned buffer */

	uint32_t padding[2];		/* XXXX: required */
} __packed;

struct mvgbe_rx_desc {
#if BYTE_ORDER == BIG_ENDIAN
	uint16_t bytecnt;		/* Descriptor buffer byte count */
	uint16_t bufsize;		/* Buffer size */
	uint32_t cmdsts;		/* Descriptor command status */
	uint32_t nextdescptr;		/* Next descriptor pointer */
	uint32_t bufptr;		/* Descriptor buffer pointer */
#else	/* LITTLE_ENDIAN */
	uint32_t cmdsts;		/* Descriptor command status */
	uint16_t bufsize;		/* Buffer size */
	uint16_t bytecnt;		/* Descriptor buffer byte count */
	uint32_t bufptr;		/* Descriptor buffer pointer */
	uint32_t nextdescptr;		/* Next descriptor pointer */
#endif
	u_long returninfo;		/* User resource return information */

	uint32_t padding[3];		/* XXXX: required */
} __packed;

#define MVGBE_ERROR_SUMMARY		(1 << 0)
#define MVGBE_BUFFER_OWNED_MASK		(1 << 31)
#define MVGBE_BUFFER_OWNED_BY_HOST	(0 << 31)
#define MVGBE_BUFFER_OWNED_BY_DMA	(1 << 31)

#define MVGBE_TX_ERROR_CODE_MASK	(3 << 1)
#define MVGBE_TX_LATE_COLLISION_ERROR	(0 << 1)
#define MVGBE_TX_UNDERRUN_ERROR		(1 << 1)
#define MVGBE_TX_EXCESSIVE_COLLISION_ERRO (2 << 1)
#define MVGBE_TX_LLC_SNAP_FORMAT	(1 << 9)
#define MVGBE_TX_IP_NO_FRAG		(1 << 10)
#define MVGBE_TX_IP_HEADER_LEN(len)	((len) << 11)
#define MVGBE_TX_VLAN_TAGGED_FRAME	(1 << 15)
#define MVGBE_TX_L4_TYPE_TCP		(0 << 16)
#define MVGBE_TX_L4_TYPE_UDP		(1 << 16)
#define MVGBE_TX_GENERATE_L4_CHKSUM	(1 << 17)
#define MVGBE_TX_GENERATE_IP_CHKSUM	(1 << 18)
#define MVGBE_TX_ZERO_PADDING		(1 << 19)
#define MVGBE_TX_LAST_DESC		(1 << 20)
#define MVGBE_TX_FIRST_DESC		(1 << 21)
#define MVGBE_TX_GENERATE_CRC		(1 << 22)
#define MVGBE_TX_ENABLE_INTERRUPT	(1 << 23)
#define MVGBE_TX_AUTO_MODE		(1 << 30)

#define MVGBE_RX_ERROR_CODE_MASK	(3 << 1)
#define MVGBE_RX_CRC_ERROR		(0 << 1)
#define MVGBE_RX_OVERRUN_ERROR		(1 << 1)
#define MVGBE_RX_MAX_FRAME_LEN_ERROR	(2 << 1)
#define MVGBE_RX_RESOURCE_ERROR		(3 << 1)
#define MVGBE_RX_L4_CHECKSUM_MASK	(0xffff << 3)
#define MVGBE_RX_VLAN_TAGGED_FRAME	(1 << 19)
#define MVGBE_RX_BPDU_FRAME		(1 << 20)
#define MVGBE_RX_L4_TYPE_MASK		(3 << 21)
#define MVGBE_RX_L4_TYPE_TCP		(0 << 21)
#define MVGBE_RX_L4_TYPE_UDP		(1 << 21)
#define MVGBE_RX_L4_TYPE_OTHER		(2 << 21)
#define MVGBE_RX_NOT_LLC_SNAP_FORMAT	(1 << 23)
#define MVGBE_RX_IP_FRAME_TYPE		(1 << 24)
#define MVGBE_RX_IP_HEADER_OK		(1 << 25)
#define MVGBE_RX_LAST_DESC		(1 << 26)
#define MVGBE_RX_FIRST_DESC		(1 << 27)
#define MVGBE_RX_UNKNOWN_DA		(1 << 28)
#define MVGBE_RX_ENABLE_INTERRUPT	(1 << 29)
#define MVGBE_RX_L4_CHECKSUM		(1 << 30)

#endif	/* _MVGEREG_H_ */
