/*	$NetBSD: rtl81x9reg.h,v 1.5 2000/05/19 13:42:29 tsutsui Exp $	*/

/*
 * Copyright (c) 1997, 1998
 *	Bill Paul <wpaul@ctr.columbia.edu>.  All rights reserved.
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
 *	This product includes software developed by Bill Paul.
 * 4. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Bill Paul AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL Bill Paul OR THE VOICES IN HIS HEAD
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	FreeBSD Id: if_rlreg.h,v 1.9 1999/06/20 18:56:09 wpaul Exp
 */

/*
 * RealTek 8129/8139 register offsets
 */
#define	RTK_IDR0	0x0000		/* ID register 0 (station addr) */
#define RTK_IDR1	0x0001		/* Must use 32-bit accesses (?) */
#define RTK_IDR2	0x0002
#define RTK_IDR3	0x0003
#define RTK_IDR4	0x0004
#define RTK_IDR5	0x0005
					/* 0006-0007 reserved */
#define RTK_MAR0	0x0008		/* Multicast hash table */
#define RTK_MAR1	0x0009
#define RTK_MAR2	0x000A
#define RTK_MAR3	0x000B
#define RTK_MAR4	0x000C
#define RTK_MAR5	0x000D
#define RTK_MAR6	0x000E
#define RTK_MAR7	0x000F

#define RTK_TXSTAT0	0x0010		/* status of TX descriptor 0 */
#define RTK_TXSTAT1	0x0014		/* status of TX descriptor 1 */
#define RTK_TXSTAT2	0x0018		/* status of TX descriptor 2 */
#define RTK_TXSTAT3	0x001C		/* status of TX descriptor 3 */

#define RTK_TXADDR0	0x0020		/* address of TX descriptor 0 */
#define RTK_TXADDR1	0x0024		/* address of TX descriptor 1 */
#define RTK_TXADDR2	0x0028		/* address of TX descriptor 2 */
#define RTK_TXADDR3	0x002C		/* address of TX descriptor 3 */

#define RTK_RXADDR		0x0030	/* RX ring start address */
#define RTK_RX_EARLY_BYTES	0x0034	/* RX early byte count */
#define RTK_RX_EARLY_STAT	0x0036	/* RX early status */
#define RTK_COMMAND	0x0037		/* command register */
#define RTK_CURRXADDR	0x0038		/* current address of packet read */
#define RTK_CURRXBUF	0x003A		/* current RX buffer address */
#define RTK_IMR		0x003C		/* interrupt mask register */
#define RTK_ISR		0x003E		/* interrupt status register */
#define RTK_TXCFG	0x0040		/* transmit config */
#define RTK_RXCFG	0x0044		/* receive config */
#define RTK_TIMERCNT	0x0048		/* timer count register */
#define RTK_MISSEDPKT	0x004C		/* missed packet counter */
#define RTK_EECMD	0x0050		/* EEPROM command register */
#define RTK_CFG0	0x0051		/* config register #0 */
#define RTK_CFG1	0x0052		/* config register #1 */
					/* 0053-0057 reserved */
#define RTK_MEDIASTAT	0x0058		/* media status register (8139) */
					/* 0059-005A reserved */
#define RTK_MII		0x005A		/* 8129 chip only */
#define RTK_HALTCLK	0x005B
#define RTK_MULTIINTR	0x005C		/* multiple interrupt */
#define RTK_PCIREV	0x005E		/* PCI revision value */
					/* 005F reserved */
#define RTK_TXSTAT_ALL	0x0060		/* TX status of all descriptors */

/* Direct PHY access registers only available on 8139 */
#define RTK_BMCR	0x0062		/* PHY basic mode control */
#define RTK_BMSR	0x0064		/* PHY basic mode status */
#define RTK_ANAR	0x0066		/* PHY autoneg advert */
#define RTK_LPAR	0x0068		/* PHY link partner ability */
#define RTK_ANER	0x006A		/* PHY autoneg expansion */

#define RTK_DISCCNT	0x006C		/* disconnect counter */
#define RTK_FALSECAR	0x006E		/* false carrier counter */
#define RTK_NWAYTST	0x0070		/* NWAY test register */
#define RTK_RX_ER	0x0072		/* RX_ER counter */
#define RTK_CSCFG	0x0074		/* CS configuration register */


/*
 * TX config register bits
 */
#define RTK_TXCFG_CLRABRT	0x00000001	/* retransmit aborted pkt */
#define RTK_TXCFG_MAXDMA	0x00000700	/* max DMA burst size */
#define RTK_TXCFG_CRCAPPEND	0x00010000	/* CRC append (0 = yes) */
#define RTK_TXCFG_LOOPBKTST	0x00060000	/* loopback test */
#define RTK_TXCFG_IFG		0x03000000	/* interframe gap */

#define RTK_TXDMA_16BYTES	0x00000000
#define RTK_TXDMA_32BYTES	0x00000100
#define RTK_TXDMA_64BYTES	0x00000200
#define RTK_TXDMA_128BYTES	0x00000300
#define RTK_TXDMA_256BYTES	0x00000400
#define RTK_TXDMA_512BYTES	0x00000500
#define RTK_TXDMA_1024BYTES	0x00000600
#define RTK_TXDMA_2048BYTES	0x00000700

/*
 * Transmit descriptor status register bits.
 */
#define RTK_TXSTAT_LENMASK	0x00001FFF
#define RTK_TXSTAT_OWN		0x00002000
#define RTK_TXSTAT_TX_UNDERRUN	0x00004000
#define RTK_TXSTAT_TX_OK	0x00008000
#define RTK_TXSTAT_EARLY_THRESH	0x003F0000
#define RTK_TXSTAT_COLLCNT	0x0F000000
#define RTK_TXSTAT_CARR_HBEAT	0x10000000
#define RTK_TXSTAT_OUTOFWIN	0x20000000
#define RTK_TXSTAT_TXABRT	0x40000000
#define RTK_TXSTAT_CARRLOSS	0x80000000

/*
 * Interrupt status register bits.
 */
#define RTK_ISR_RX_OK		0x0001
#define RTK_ISR_RX_ERR		0x0002
#define RTK_ISR_TX_OK		0x0004
#define RTK_ISR_TX_ERR		0x0008
#define RTK_ISR_RX_OVERRUN	0x0010
#define RTK_ISR_PKT_UNDERRUN	0x0020
#define RTK_ISR_FIFO_OFLOW	0x0040	/* 8139 only */
#define RTK_ISR_PCS_TIMEOUT	0x4000	/* 8129 only */
#define RTK_ISR_SYSTEM_ERR	0x8000

#define RTK_INTRS	\
	(RTK_ISR_TX_OK|RTK_ISR_RX_OK|RTK_ISR_RX_ERR|RTK_ISR_TX_ERR|	\
	RTK_ISR_RX_OVERRUN|RTK_ISR_PKT_UNDERRUN|RTK_ISR_FIFO_OFLOW|	\
	RTK_ISR_PCS_TIMEOUT|RTK_ISR_SYSTEM_ERR)

/*
 * Media status register. (8139 only)
 */
#define RTK_MEDIASTAT_RXPAUSE	0x01
#define RTK_MEDIASTAT_TXPAUSE	0x02
#define RTK_MEDIASTAT_LINK	0x04
#define RTK_MEDIASTAT_SPEED10	0x08
#define RTK_MEDIASTAT_RXFLOWCTL	0x40	/* duplex mode */
#define RTK_MEDIASTAT_TXFLOWCTL	0x80	/* duplex mode */

/*
 * Receive config register.
 */
#define RTK_RXCFG_RX_ALLPHYS	0x00000001	/* accept all nodes */
#define RTK_RXCFG_RX_INDIV	0x00000002	/* match filter */
#define RTK_RXCFG_RX_MULTI	0x00000004	/* accept all multicast */
#define RTK_RXCFG_RX_BROAD	0x00000008	/* accept all broadcast */
#define RTK_RXCFG_RX_RUNT	0x00000010
#define RTK_RXCFG_RX_ERRPKT	0x00000020
#define RTK_RXCFG_WRAP		0x00000080
#define RTK_RXCFG_MAXDMA	0x00000700
#define RTK_RXCFG_BUFSZ		0x00001800
#define RTK_RXCFG_FIFOTHRESH	0x0000E000
#define RTK_RXCFG_EARLYTHRESH	0x07000000

#define RTK_RXDMA_16BYTES	0x00000000
#define RTK_RXDMA_32BYTES	0x00000100
#define RTK_RXDMA_64BYTES	0x00000200
#define RTK_RXDMA_128BYTES	0x00000300
#define RTK_RXDMA_256BYTES	0x00000400
#define RTK_RXDMA_512BYTES	0x00000500
#define RTK_RXDMA_1024BYTES	0x00000600
#define RTK_RXDMA_UNLIMITED	0x00000700

#define RTK_RXBUF_8		0x00000000
#define RTK_RXBUF_16		0x00000800
#define RTK_RXBUF_32		0x00001000
#define RTK_RXBUF_64		0x00001800

#define RTK_RXFIFO_16BYTES	0x00000000
#define RTK_RXFIFO_32BYTES	0x00002000
#define RTK_RXFIFO_64BYTES	0x00004000
#define RTK_RXFIFO_128BYTES	0x00006000
#define RTK_RXFIFO_256BYTES	0x00008000
#define RTK_RXFIFO_512BYTES	0x0000A000
#define RTK_RXFIFO_1024BYTES	0x0000C000
#define RTK_RXFIFO_NOTHRESH	0x0000E000

/*
 * Bits in RX status header (included with RX'ed packet
 * in ring buffer).
 */
#define RTK_RXSTAT_RXOK		0x00000001
#define RTK_RXSTAT_ALIGNERR	0x00000002
#define RTK_RXSTAT_CRCERR	0x00000004
#define RTK_RXSTAT_GIANT	0x00000008
#define RTK_RXSTAT_RUNT		0x00000010
#define RTK_RXSTAT_BADSYM	0x00000020
#define RTK_RXSTAT_BROAD	0x00002000
#define RTK_RXSTAT_INDIV	0x00004000
#define RTK_RXSTAT_MULTI	0x00008000
#define RTK_RXSTAT_LENMASK	0xFFFF0000

#define RTK_RXSTAT_UNFINISHED	0xFFF0		/* DMA still in progress */
/*
 * Command register.
 */
#define RTK_CMD_EMPTY_RXBUF	0x0001
#define RTK_CMD_TX_ENB		0x0004
#define RTK_CMD_RX_ENB		0x0008
#define RTK_CMD_RESET		0x0010

/*
 * EEPROM control register
 */
#define RTK_EE_DATAOUT		0x01	/* Data out */
#define RTK_EE_DATAIN		0x02	/* Data in */
#define RTK_EE_CLK		0x04	/* clock */
#define RTK_EE_SEL		0x08	/* chip select */
#define RTK_EE_MODE		(0x40|0x80)

#define RTK_EEMODE_OFF		0x00
#define RTK_EEMODE_AUTOLOAD	0x40
#define RTK_EEMODE_PROGRAM	0x80
#define RTK_EEMODE_WRITECFG	(0x80|0x40)

/* 9346/9356 EEPROM commands */
#define RTK_EEADDR_LEN0		6	/* 9346 */
#define RTK_EEADDR_LEN1		8	/* 9356 */
#define RTK_EECMD_LEN		4

#define RTK_EECMD_WRITE		0x5	/* 0101b */
#define RTK_EECMD_READ		0x6	/* 0110b */
#define RTK_EECMD_ERASE		0x7	/* 0111b */

#define RTK_EE_ID		0x00
#define RTK_EE_PCI_VID		0x01
#define RTK_EE_PCI_DID		0x02
/* Location of station address inside EEPROM */
#define RTK_EE_EADDR0		0x07
#define RTK_EE_EADDR1		0x08
#define RTK_EE_EADDR2		0x09

/*
 * MII register (8129 only)
 */
#define RTK_MII_CLK		0x01
#define RTK_MII_DATAIN		0x02
#define RTK_MII_DATAOUT		0x04
#define RTK_MII_DIR		0x80	/* 0 == input, 1 == output */

/*
 * Config 0 register
 */
#define RTK_CFG0_ROM0		0x01
#define RTK_CFG0_ROM1		0x02
#define RTK_CFG0_ROM2		0x04
#define RTK_CFG0_PL0		0x08
#define RTK_CFG0_PL1		0x10
#define RTK_CFG0_10MBPS		0x20	/* 10 Mbps internal mode */
#define RTK_CFG0_PCS		0x40
#define RTK_CFG0_SCR		0x80

/*
 * Config 1 register
 */
#define RTK_CFG1_PWRDWN		0x01
#define RTK_CFG1_SLEEP		0x02
#define RTK_CFG1_IOMAP		0x04
#define RTK_CFG1_MEMMAP		0x08
#define RTK_CFG1_RSVD		0x10
#define RTK_CFG1_DRVLOAD	0x20
#define RTK_CFG1_LED0		0x40
#define RTK_CFG1_FULLDUPLEX	0x40	/* 8129 only */
#define RTK_CFG1_LED1		0x80

/*
 * The RealTek doesn't use a fragment-based descriptor mechanism.
 * Instead, there are only four register sets, each or which represents
 * one 'descriptor.' Basically, each TX descriptor is just a contiguous
 * packet buffer (32-bit aligned!) and we place the buffer addresses in
 * the registers so the chip knows where they are.
 *
 * We can sort of kludge together the same kind of buffer management
 * used in previous drivers, but we have to do buffer copies almost all
 * the time, so it doesn't really buy us much.
 *
 * For reception, there's just one large buffer where the chip stores
 * all received packets.
 */

#define RTK_RX_BUF_SZ		RTK_RXBUF_64
#define RTK_RXBUFLEN		(1 << ((RTK_RX_BUF_SZ >> 11) + 13))
#define RTK_TX_LIST_CNT		4
#define RTK_TX_EARLYTHRESH	((256 / 32) << 16)
#define RTK_RX_FIFOTHRESH	RTK_RXFIFO_256BYTES
#define RTK_RX_MAXDMA		RTK_RXDMA_256BYTES
#define RTK_TX_MAXDMA		RTK_TXDMA_256BYTES

#define RTK_RXCFG_CONFIG 	(RTK_RX_FIFOTHRESH|RTK_RX_MAXDMA|RTK_RX_BUF_SZ)
#define RTK_TXCFG_CONFIG	(RTK_TXCFG_IFG|RTK_TX_MAXDMA)
