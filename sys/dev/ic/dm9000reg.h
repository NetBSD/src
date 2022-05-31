/*	$NetBSD: dm9000reg.h,v 1.8 2022/05/31 08:43:15 andvar Exp $	*/

/*
 * Copyright (c) 2009 Paul Fleischer
 * All rights reserved.
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the company nor the name of the author may be used to
 *    endorse or promote products derived from this software without specific
 *    prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef _DEV_IC_DM9000REG_H_
#define _DEV_IC_DM9000REG_H_

/*
 * Registers accessible on the DM9000, extracted from pp. 11-12 from
 * the data sheet
 */

/*
 * There are two interesting addresses for the DM9000 (at least in
 * the context of the FriendlyARM MINI2440) The I/O or register select
 * address, which is the base address.  The DATA address, which is
 * located at offset 4 from the base address.
 *
 * Chances are that this will not work generally, as it really depends
 * on how the address lines are mapped from the CPU to the DM9000.
 * But for now it is a good starting point.
 */

#define DM9000_IOSIZE 4

#define DM9000_NCR		0x00	/* "network" control */
#define  DM9000_NCR_RST		(1<<0)	/* reset chip, self clear */
#define  DM9000_NCR_LBK_MASK	(0x06)	/* loopback test selection */
#define  DM9000_NCR_LBK_SHIFT	(1)
#define  DM9000_NCR_LBK_NORMAL	(0<<1)	/* normal operation */
#define  DM9000_NCR_LBK_MAC_INTERNAL (1<<1) /* MAC loopback */
#define  DM9000_NCR_LBK_INT_PHY (2<<1)	/* PHY loopback */
#define  DM9000_NCR_FDX		(1<<3)	/* use full-duplex, RO when int PHY */
#define  DM9000_NCR_FCOL	(1<<4)	/* force coll. mode. test only */
#define  DM9000_NCR_WAKEEN	(1<<6)	/* wakeup event enable */
#define  DM9000_NCR_EXY_PHY	(1<<7)	/* select ext. PHY, immune SW reset */
#define DM9000_NSR		0x01	/* "network" status */
#define  DM9000_NSR_RXOV	(1<<1)	/* receive overflow deteced */
#define  DM9000_NSR_TX1END	(1<<2)	/* transmit 1 completed, W1C */
#define  DM9000_NSR_TX2END	(1<<3) 	/* transmit 2 completed, W1C */
#define  DM9000_NSR_WAKEST	(1<<5)	/* wakeup event, W1C */
#define  DM9000_NSR_LINKST	(1<<6)	/* link is up */
#define  DM9000_NSR_SPEED	(1<<7)	/* 1: 100Mbps, 0: 10Mbps */
#define DM9000_TCR		0x02	/* Tx control */
#define  DM9000_TCR_TXREQ	(1<<0)	/* request to start Tx, self clear */
#define  DM9000_TCR_CRC_DIS1	(1<<1)	/* disable PAD op on Tx1 */
#define  DM9000_TCR_PAD_DIS1	(1<<2)	/* disbale CRC append on Tx1 */
#define  DM9000_TCR_CRC_DIS2	(1<<3)	/* disable PAD op on Tx2 */
#define  DM9000_TCR_PAD_DIS2	(1<<4)	/* disbale CRC append on Tx2 */
#define  DM9000_TCR_EXCECM	(1<<5)	/* allow infinite colli. retries */
#define  DM9000_TCR_TJDIS	(1<<6)	/* disable xmit jabber, otherwise on */
#define DM9000_TSR1		0x03	/* transmit completion status 1 */
#define DM9000_TSR2		0x04	/* transmit completion status 2 */
#define  DM9000_TSR_EC		(1<<2)	/* aborted after 16 collision */
#define  DM9000_TSR_COL		(1<<3)	/* collision detected while xmit */
#define  DM9000_TSR_LCOL	(1<<4)	/* out of window "late" collision */
#define  DM9000_TSR_NC		(1<<5)	/* no carrier signal found */
#define  DM9000_TSR_CLOSS	(1<<6)	/* loss of carrier */
#define  DM9000_TSR_TJTO	(1<<7)	/* Tx jabber time out */
#define DM9000_RCR		0x05	/* Rx control */
#define  DM9000_RCR_RXEN	(1<<0)	/* activate Rx */
#define  DM9000_RCR_PRMSC	(1<<1)	/* enable promisc mode */
#define  DM9000_RCR_RUNT	(1<<2)	/* accept damaged runt frame */
#define  DM9000_RCR_ALL		(1<<3)	/* accept all multicast */
#define  DM9000_RCR_DIS_CRC	(1<<4)	/* drop bad CRC frame */
#define  DM9000_RCR_DIS_LONG	(1<<5)	/* drop too long frame >1522 */
#define  DM9000_RCR_WTDIS	(1<<6)	/* disable >2048 Rx detect timer */
#define DM9000_RSR		0x06	/* Rx status */
#define  DM9000_RSR_FOE		(1<<0)	/* Rx FIFO overflow detected */
#define  DM9000_RSR_CE		(1<<1)	/* CRC error found */
#define  DM9000_RSR_AE		(1<<2)	/* tail not ended in byte boundary */
#define  DM9000_RSR_PLE		(1<<3)	/* physical layer error */
#define  DM9000_RSR_RWTO	(1<<4)	/* >2048 condition detected */
#define  DM9000_RSR_LCS		(1<<5)	/* late colli. detected */
#define  DM9000_RSR_MF		(1<<6)	/* mcast/bcast frame received */
#define  DM9000_RSR_RF		(1<<7)	/* damaged runt frame received <64 */
#define DM9000_ROCR		0x07	/* receive overflow counter */
/* 7: OVF detected, 6:0 statistic couner */
#define DM9000_BPTR		0x08	/* back pressure threshold */
/* 7:4 back pressure high watermark (3 def), 3:0 jam pattern time (7 def) */
#define DM9000_FCTR		0x09	/* flow control threshold */
/* 7:4 Rx FIFO high w.m. (3 def), low w.m. (8 def) */
#define DM9000_FCR		0x0A	/* Rx flow control */
#define  DM9000_FCR_FLCE	(1<<0)	/* flow control enable */
#define  DM9000_FCR_RXPCS	(1<<1)	/* Rx PAUSE current status */
#define  DM9000_FCR_RXPS	(1<<2)	/* Rx PAUSE status, latched R2C */
#define  DM9000_FCR_BKPM	(1<<3)	/* HDX back pressure for my frames */
#define  DM9000_FCR_BKPA	(1<<4)	/* HDX back pressure for any frames */
#define  DM9000_FCR_TXPEN	(1<<5)	/* activate auto PAUSE operation */
#define  DM9000_FCR_TXPF	(1<<6)	/* Tx PAUSE packet (when full) */
#define  DM9000_FCR_TXP0	(1<<7)	/* Tx PAUSE packet (when empty) */
#define DM9000_EPCR		0x0B	/* EEPROM / PHY control */
#define  DM9000_EPCR_ERRE	(1<<0)	/* operation in progress, busy bit */
#define  DM9000_EPCR_ERPRW	(1<<1)	/* instruct to write, not SC */
#define  DM9000_EPCR_ERPRR	(1<<2)	/* instruct to read, not SC */
#define  DM9000_EPCR_EPOS_EEPROM (0<<3)	/* EEPROM operation */
#define  DM9000_EPCR_EPOS_PHY    (1<<3)	/* PHY operation */
#define  DM9000_EPCR_WEP	(1<<4)	/* EEPROM write enable */
#define  DM9000_EPCR_REEP	(1<<5)	/* reload EEPROM contents, not SC */
#define DM9000_EPAR		0x0C	/* EEPROM / PHY address */
#define  DM9000_EPAR_EROA_MASK	0x3F	/* 7:6 (!!) PHY id, 5:0 addr/reg */
#define  DM9000_EPAR_INT_PHY	0x40	/* EPAR[7:6] = 01 for internal PHY */
#define DM9000_EPDRL		0x0D	/* EEPROM / PHY data 7:0 */
#define DM9000_EPDRH		0x0E	/* EEPROM / PHY data 15:8 */
#define DM9000_WCR		0x0F	/* wakeup control and status */
#define  DM9000_MAGIC		(1<<0)	/* magic frame arrived */
#define  DM9000_SAMPLE		(1<<1)	/* sample frame arrived */
#define  DM9000_LINK		(1<<2)	/* link change / status change found */
#define  DM9000_MAGICEN		(1<<3)	/* enable magic frame event detect */
#define  DM9000_SMAPLEEN	(1<<4)	/* enable sample frame event detect */
#define  DM9000_LINKEN		(1<<5)	/* enable link change event detect */

#define DM9000_PAB0		0x10	/* my station address 7:0 */
#define DM9000_PAB1		0x11
#define DM9000_PAB2		0x12
#define DM9000_PAB3		0x13
#define DM9000_PAB4		0x14
#define DM9000_PAB5		0x15	/* my station address 47:40 */

#define DM9000_MAB0		0x16	/* 64bit mcast hash filter 7:0 */
#define DM9000_MAB1		0x17
#define DM9000_MAB2		0x18
#define DM9000_MAB3		0x19
#define DM9000_MAB4		0x1A
#define DM9000_MAB5		0x1B
#define DM9000_MAB6		0x1C
#define DM9000_MAB7		0x1D	/* 63:56, needs 0x80 to catch bcast */

#define DM9000_GPCR		0x1E	/* GPIO control */
#define  DM9000_GPCR_GPIO0_OUT	(1<<0)	/* bit-0 to control PHY */
/* 3:0 select pin I/O direction (0001 def) */
#define DM9000_GPR		0x1F	/* GPIO value to read / write */
#define  DM9000_GPR_PHY_PWROFF	(1<<0)	/* power down internal PHY */
#define DM9000_TRPAL		0x22	/* Tx SRAM read pointer 7:0 */
#define DM9000_TRPAH		0x23	/* Tx SRAM read pointer 15:8 */
#define DM9000_RWPAL		0x24	/* Rx SRAM read pointer 7:0 */
#define DM9000_RWPAH		0x25	/* Rx SRAM read pointer 15:8 */

/* VID 0x0a46, PID 0x9000 */
#define DM9000_VID0	0x28	/* vender ID 7:0 */
#define DM9000_VID1	0x29	/* vender ID 15:8 */
#define DM9000_PID0	0x2A	/* product ID 7:0 */
#define DM9000_PID1	0x2B	/* product ID 15:8 */
#define DM9000_CHIPR	0x2C	/* chip revision */

#define DM9000_TCR2	0x2D	/* Tx control 2 */
#define DM9000_OTCR	0x2E	/* operation test control */
#define DM9000_SMCR	0x2F	/* special mode control */
#define  DM9000_FB0	(1<<0)	/* force shortest back-off time */
#define  DM9000_FB1	(1<<1)	/* force longeset back-off time */
#define  DM9000_FLC	(1<<2)	/* force late collsion */
#define  DM9000_SM_EN	(1<<7)	/* serial mode enable */
#define DM9000_ETXCSR	0x30	/* early xmit control and status */
#define DM9000_TCSCR	0x31	/* xmit checksum control */
#define DM9000_RCSCSR	0x32	/* recv checksum control and status */
#define DM9000_MPAR	0x33	/* MII PHY address */
#define DM9000_LEDCR	0x34	/* LED pin control */
#define DM9000_BUSCR	0x38	/* processor bus control */
#define DM9000_INTCR	0x39	/* INT pin control */
#define DM9000_SCCR	0x50	/* system clock turn on control */
#define DM9000_RSCCR	0x51	/* resume system clock control */
#define DM9000_MRCMDX	0xF0	/* "no increment" pre-fetch read */
#define DM9000_MRCMDX1	0xF1	/* "no increment" read */
#define DM9000_MRCMD	0xF2	/* "auto increment" read */
#define DM9000_MRRL	0xF4	/* memory read address 7:0 */
#define DM9000_MRRH	0xF5	/* memory read address 15:8 */
#define DM9000_MWCMDX	0xF6	/* "no increment" write */
#define DM9000_MWCMD	0xF8	/* "auto increment" write */
#define DM9000_MWRL	0xFA	/* memory write address 7:0 */
#define DM9000_MWRH	0xFB	/* memory write address 15:8 */
#define DM9000_TXPLL	0xFC	/* frame len 7:0 to transmit */
#define DM9000_TXPLH	0xFD	/* frame len 15:8 to transmit */
#define DM9000_ISR	0xFE	/* interrupt status report */
#define  DM9000_IOMODE_MASK	0xC0
#define  DM9000_IOMODE_SHIFT	6
/* 7:6 I/O size (hard wired) 10b: 8-bit, 00b: 16-bit, 01b: 32-bit */
#define  DM9000_ISR_PRS		(1<<0)	/* receive completed, W1C */
#define  DM9000_ISR_PTS		(1<<1)	/* transmit completed, W1C */
#define  DM9000_ISR_ROS		(1<<2)	/* Rx overflow latch, W1C */
#define  DM9000_ISR_ROOS	(1<<3)	/* Rx overflow cntr overflowed, W1C */
#define  DM9000_ISR_UNDERRUN	(1<<4)	/* Tx underrun detected */
#define  DM9000_ISR_LNKCHNG	(1<<5)	/* link status change detected */
#define DM9000_IMR	0xFF	/* interrupt mask */
#define  DM9000_IMR_PRM 	(1<<0)	/* enable receive event report */
#define  DM9000_IMR_PTM 	(1<<1)	/* enable xmit done event report */
#define  DM9000_IMR_ROM 	(1<<2)	/* enable Rx overflow event report */
#define  DM9000_IMR_ROOM	(1<<3)	/* enable Rx overflow cntr ov report */
#define  DM9000_IMR_PAR 	(1<<7)	/* use 3/13K SRAM w/ auto wrap */

#endif
