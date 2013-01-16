/*	$Id: at91emacreg.h,v 1.3.12.1 2013/01/16 05:32:45 yamt Exp $	*/
/*      $NetBSD: at91emacreg.h,v 1.3.12.1 2013/01/16 05:32:45 yamt Exp $	*/
/*-
 * Copyright (c) 2007 Embedtronics Oy
 * All rights reserved
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
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef	_AT91EMACREG_H_
#define	_AT91EMACREG_H_	1

/* Ethernet MAC (EMAC),
 * at91rm9200.pdf, page 573 */

#define	ETH_CTL		0x00U	/* 0x00: Control Register		*/
#define	ETH_CFG		0x04U	/* 0x04: Configuration Register		*/
#define	ETH_SR		0x08U	/* 0x08: Status Register		*/
#define	ETH_TAR		0x0CU	/* 0x0C: Transmit Address Register	*/
#define	ETH_TCR		0x10U	/* 0x10: Transmit Control Register	*/
#define	ETH_TSR		0x14U	/* 0x14: Transmit Status Register	*/
#define	ETH_RBQP	0x18U	/* 0x18: Receive Buffer Queue Pointer	*/
#define	ETH_RSR		0x20U	/* 0x20: Receive Status Register	*/
#define	ETH_ISR		0x24U	/* 0x24: Interrupt Status Register	*/
#define	ETH_IER		0x28U	/* 0x28: Interrupt Enable Register	*/
#define	ETH_IDR		0x2CU	/* 0x2C: Interrupt Disable Register	*/
#define	ETH_IMR		0x30U	/* 0x30: Interrupt Mask Register	*/
#define	ETH_MAN		0x34U	/* 0x34: PHY Maintenance Register	*/

#define	ETH_FRA		0x40U	/* 0x40: Frames Transmitted OK		*/
#define	ETH_SCOL	0x44U	/* 0x44: Single Collision Frames	*/
#define	ETH_MCOL	0x48U	/* 0x48: Multiple Collision Frames	*/
#define	ETH_OK		0x4CU	/* 0x4C: Frames Received OK		*/
#define	ETH_SEQE	0x50U	/* 0x50: Frame Check Sequence Errors	*/
#define	ETH_ALE		0x54U	/* 0x54: Alignment Errors		*/
#define	ETH_DTE		0x58U	/* 0x58: Deferred Transmission Frame	*/
#define	ETH_LCOL	0x5CU	/* 0x5C: Late Collisions		*/
#define	ETH_ECOL	0x60U	/* 0x60: Excessive Collisions		*/
#define	ETH_CSE		0x64U	/* 0x64: Carrier Sense Errors		*/
#define	ETH_TUE		0x68U	/* 0x68: Transmit Underrun Errors	*/
#define	ETH_CDE		0x6CU	/* 0x6C: Code Errors			*/
#define	ETH_ELR		0x70U	/* 0x70: Excessive Length Errors	*/
#define	ETH_RJB		0x74U	/* 0x74: Receive Jabbers		*/
#define	ETH_USF		0x78U	/* 0x78: Undersize Frames		*/
#define	ETH_SQEE	0x7CU	/* 0x7C: SQE Test Errors		*/
#define	ETH_DRFC	0x80U	/* 0x80: Discarded RX Frames		*/

#define	ETH_HSH		0x90U	/* 0x90: Hash Address High		*/
#define	ETH_HSL		0x94U	/* 0x94: Hash Address Low		*/

#define	ETH_SA1L	0x98U	/* 0x98: Specific Address 1 Low		*/
#define	ETH_SA1H	0x9CU	/* 0x9C: Specific Address 1 High	*/

#define	ETH_SA2L	0xA0U	/* 0xA0: Specific Address 2 Low		*/
#define	ETH_SA2H	0xA4U	/* 0xA4: Specific Address 2 High	*/

#define	ETH_SA3L	0xA8U	/* 0xA8: Specific Address 3 Low		*/
#define	ETH_SA3H	0xACU	/* 0xAC: Specific Address 3 High	*/

#define	ETH_SA4L	0xB0U	/* 0xB0: Specific Address 4 Low		*/
#define	ETH_SA4H	0xB4U	/* 0xB4: Specific Address 4 High	*/


/* Control Register bits: */
#define	ETH_CTL_BP	0x100U	/* 1 = back pressure enabled		*/
#define	ETH_CTL_WES	0x080U	/* 1 = statistics registers writeable	*/
#define	ETH_CTL_ISR	0x040U	/* 1 = increment statistics registers	*/
#define	ETH_CTL_CSR	0x020U	/* 1 = clear statistics registers	*/
#define	ETH_CTL_MPE	0x010U	/* 1 = management port enabled		*/
#define	ETH_CTL_TE	0x008U	/* 1 = transmit enable			*/
#define	ETH_CTL_RE	0x004U	/* 1 = receive enable			*/
#define	ETH_CTL_LBL	0x002U	/* 1 = local loopback enabled		*/
#define	ETH_CTL_LB	0x001U	/* 1 = loopback signal is at high level	*/


/* Configuration Register bits: */
#define	ETH_CFG_RMII	0x2000U	/* 1 = enable RMII (Reduce MII)		*/
#define	ETH_CFG_RTY	0x1000U	/* 1 = retry test enabled		*/

#define	ETH_CFG_CLK	0x0C00U	/* clock				*/
#define	ETH_CFG_CLK_8	0x0000U
#define	ETH_CFG_CLK_16	0x0400U
#define	ETH_CFG_CLK_32	0x0800U
#define	ETH_CFG_CLK_64	0x0C00U

#define	ETH_CFG_EAE	0x0200U	/* 1 = external address match enable	*/
#define	ETH_CFG_BIG	0x0100U	/* 1 = receive up to 1522 bytes	(VLAN)	*/
#define	ETH_CFG_UNI	0x0080U	/* 1 = enable unicast hash		*/
#define	ETH_CFG_MTI	0x0040U	/* 1 = enable multicast hash		*/
#define	ETH_CFG_NBC	0x0020U	/* 1 = ignore received broadcasts	*/
#define	ETH_CFG_CAF	0x0010U	/* 1 = receive all valid frames		*/
#define	ETH_CFG_BR	0x0004U
#define	ETH_CFG_FD	0x0002U	/* 1 = force full duplex		*/
#define	ETH_CFG_SPD	0x0001U	/* 1 = 100 Mbps				*/


/* Status Register bits: */
#define	ETH_SR_IDLE	0x0004U	/* 1 = PHY logic is running		*/
#define	ETH_SR_MDIO	0x0002U	/* 1 = MDIO pin set			*/
#define	ETH_SR_LINK	0x0001U


/* Transmit Control Register bits: */
#define	ETH_TCR_NCRC	0x8000U	/* 1 = don't append CRC			*/
#define	ETH_TCR_LEN	0x07FFU	/* transmit frame length		*/


/* Transmit Status Register bits: */
#define	ETH_TSR_UND	0x40U	/* 1 = transmit underrun detected	*/
#define	ETH_TSR_COMP	0x20U	/* 1 = transmit complete		*/
#define	ETH_TSR_BNQ	0x10U	/* 1 = transmit buffer not queued	*/
#define	ETH_TSR_IDLE	0x08U	/* 1 = transmitter idle			*/
#define	ETH_TSR_RLE	0x04U	/* 1 = retry limit exceeded		*/
#define	ETH_TSR_COL	0x02U	/* 1 = collision occurred		*/
#define	ETH_TSR_OVR	0x01U	/* 1 = transmit buffer overrun		*/


/* Receive Status Register bits: */
#define	ETH_RSR_OVR	0x04U	/* 1 = RX overrun			*/
#define	ETH_RSR_REC	0x02U	/* 1 = frame received			*/
#define	ETH_RSR_BNA	0x01U	/* 1 = buffer not available		*/


/* Interrupt bits: */
#define	ETH_ISR_ABT	0x0800U	/* 1 = abort during DMA transfer	*/
#define	ETH_ISR_ROVR	0x0400U	/* 1 = RX overrun			*/
#define	ETH_ISR_LINK	0x0200U	/* 1 = link pin changed			*/
#define	ETH_ISR_TIDLE	0x0100U	/* 1 = transmitter idle			*/
#define	ETH_ISR_TCOM	0x0080U	/* 1 = transmit complete		*/
#define	ETH_ISR_TBRE	0x0040U	/* 1 = transmit buffer register empty	*/
#define	ETH_ISR_RTRY	0x0020U	/* 1 = retry limit exceeded		*/
#define	ETH_ISR_TUND	0x0010U	/* 1 = transmit buffer underrun		*/
#define	ETH_ISR_TOVR	0x0008U	/* 1 = transmit buffer overrun		*/
#define	ETH_ISR_RBNA	0x0004U	/* 1 = receive buffer not available	*/
#define	ETH_ISR_RCOM	0x0002U	/* 1 = receive complete			*/
#define	ETH_ISR_DONE	0x0001U	/* 1 = management done			*/


/* PHY Maintenance Register bits: */
#define	ETH_MAN_LOW	0x80000000U /* must not be set			*/
#define	ETH_MAN_HIGH	0x40000000U /* must be set			*/

#define	ETH_MAN_RW	0x30000000U
#define	ETH_MAN_RW_RD	0x20000000U
#define	ETH_MAN_RW_WR	0x10000000U

#define	ETH_MAN_PHYA	0x0F800000U /* PHY address (normally 0)		*/
#define	ETH_MAN_PHYA_SHIFT 23U
#define	ETH_MAN_REGA	0x007C0000U
#define	ETH_MAN_REGA_SHIFT 18U
#define	ETH_MAN_CODE	0x00030000U /* must be 10			*/
#define	ETH_MAN_CODE_IEEE802_3 \
			0x00020000U
#define	ETH_MAN_DATA	0x0000FFFFU /* data to be written to the PHY	*/

#define	ETH_MAN_VAL	(ETH_MAN_HIGH|ETH_MAN_CODE_IEEE802_3)


/* received buffer descriptor: */
#define	ETH_RDSC_ADDR		0x00U
#define	ETH_RDSC_FLAGS		0x00U
#define	ETH_RDSC_INFO		0x04U
#define	ETH_RDSC_SIZE		0x08U

typedef struct eth_rdsc {
	volatile uint32_t	Addr;
	volatile uint32_t	Info;
} __attribute__ ((aligned(4))) eth_rdsc_t;

/* flags: */
#define	ETH_RDSC_F_WRAP		0x00000002U
#define	ETH_RDSC_F_USED		0x00000001U

/* frame info bits: */
#define	ETH_RDSC_I_BCAST	0x80000000U
#define	ETH_RDSC_I_MULTICAST	0x40000000U
#define	ETH_RDSC_I_UNICAST	0x20000000U
#define	ETH_RDSC_I_VLAN		0x10000000U
#define	ETH_RDSC_I_UNKNOWN_SRC	0x08000000U
#define	ETH_RDSC_I_MATCH1	0x04000000U
#define	ETH_RDSC_I_MATCH2	0x02000000U
#define	ETH_RDSC_I_MATCH3	0x01000000U
#define	ETH_RDSC_I_MATCH4	0x00800000U
#define	ETH_RDSC_I_LEN		0x000007FFU

#define	ETHREG(offset)		*((volatile uint32_t *)(0xfffbc000 + (offset)))

#endif /* !_AT91EMACREG_H_ */
