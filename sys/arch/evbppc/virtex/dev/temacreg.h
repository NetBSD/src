/* 	$NetBSD: temacreg.h,v 1.1 2006/12/02 22:18:47 freza Exp $ */

/*
 * Copyright (c) 2006 Jachym Holecek
 * All rights reserved.
 *
 * Written for DFC Design, s.r.o.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _VIRTEX_DEV_TEMACREG_H_
#define _VIRTEX_DEV_TEMACREG_H_

/*
 * Ethernet peripheral control (single register, see temac_control()).
 * This goes over normal DCR bus and is configured on EMAC block.
 */
#define TEMAC_SIZE 		0x001c

#define TEMAC_RESET 		0x0000
#define TEMAC_RESET_PERIPH 	0x80000000 	/* Reset ethernet peripheral */
#define TEMAC_RESET_EMAC 	0x40000000 	/* Reset EMAC core */
#define TEMAC_RESET_PHY 	0x20000000 	/* Reset PHY core */

/* LocalLink GMAC registers. Only ERRCNT implemented in temac. */
#define TEMAC_GMAC_ERRCNT 	0x0018
#define GMAC_ERR_FRAME(val) 	(((val) >> 16) & 0xffff)
#define GMAC_ERR_OVERRUN(val) 	((val) & 0xffff)

/*
 * Host interface ("GMI") registers, accessed indirectly via IDCR.
 */

/* Pause frame address, bytes 0-3 */
#define TEMAC_GMI_RXCF0 	0x0200 		/* Receiver conf word 0 */

#define TEMAC_GMI_RXCF1 	0x0240 		/* Receiver conf word 1 */
#define GMI_RX_RESET 		0x80000000 	/* Receiver reset */
#define GMI_RX_JUMBO 		0x40000000 	/* Jumbo frame enable */
#define GMI_RX_FCS 		0x20000000 	/* Pass FCS on Rx */
#define GMI_RX_ENABLE 		0x10000000 	/* Enable receiver block */
#define GMI_RX_VLAN 		0x08000000 	/* Receive VLAN tagged frames */
#define GMI_RX_HDX 		0x04000000 	/* Half duplex Rx */
#define GMI_RX_NOCHECK 		0x02000000 	/* Disable Length/Type check */
#define GMI_RX_PAUSE_MASK 	0x0000ffff 	/* Pause frame addr 4-5 */

#define TEMAC_GMI_TXCF 		0x0280 		/* Transmitter configuration */
#define GMI_TX_RESET 		0x80000000 	/* Transmitter reset */
#define GMI_TX_JUMBO 		0x40000000 	/* Jumbo frame enable */
#define GMI_TX_FCS 		0x20000000 	/* Take FCS field from client */
#define GMI_TX_ENABLE 		0x10000000 	/* Enable transmitter block */
#define GMI_TX_VLAN 		0x08000000 	/* Transmit VLAN frames */
#define GMI_TX_HDX 		0x04000000 	/* Half duplex Tx */
#define GMI_TX_IFG 		0x02000000 	/* IFG adjustment enable */

#define TEMAC_GMI_FLOWCF 	0x02c0 		/* Flow control configuration */
#define GMI_FLOWCF_TX 		0x40000000 	/* Honor CLIENTEMAC#PAUSEREQ */
#define GMI_FLOWCF_RX 		0x20000000 	/* HW pause frame handling */

#define TEMAC_GMI_MMC 		0x0300 		/* MAC mode configuration */
#define GMI_MMC_SPEED_MASK 	0xc0000000
#define GMI_MMC_SPEED_NA 	0xc0000000
#define GMI_MMC_SPEED_1000 	0x80000000
#define GMI_MMC_SPEED_100 	0x40000000
#define GMI_MMC_SPEED_10 	0x00000000
#define GMI_MMC_RGMII 		0x20000000 	/* Enable RGMII mode */
#define GMI_MMC_SGMII 		0x10000000 	/* Enable SGMII mode */
#define GMI_MMC_1000BaseX 	0x08000000 	/* Enable 1000Base-X mode */
#define GMI_MMC_HIE 		0x04000000 	/* Host interface enable */
#define GMI_MMC_TX16 		0x02000000 	/* [1000BaseX] 16bit TX lane */
#define GMI_MMC_RX16 		0x01000000 	/* [1000BaseX] 16bit RX lane */

#define TEMAC_GMI_MGMTCF 	0x0340 		/* Management configuration */
#define GMI_MGMT_CLKDIV_MASK 	0x0000003f 	/* MDIO clock divisor */
#define GMI_MGMT_MDIO 		0x00000040 	/* MDIO link enable */

/* MII clock divisor constant for DCR running at 100MHz. */
#define GMI_MGMT_CLKDIV_100MHz 	0x00000028

#define TEMAC_GMI_UNI0 		0x0380 		/* Unicast address word 0 */
#define TEMAC_GMI_UNI1 		0x0384 		/* Unicast address word 1 */
#define TEMAC_GMI_MAT0 		0x0388 		/* Multicast filter word 0 */
#define TEMAC_GMI_MAT1 		0x038c 		/* Multicast filter word 1 */

#define TEMAC_GMI_AFM 		0x0390 		/* Address filter mode */
#define GMI_AFM_PROMISC 	0x80000000 	/* Promiscuous mode */

#define TEMAC_GMI_IRQSTAT 	0x03a0
#define TEMAC_GMI_IRQEN 	0x03a4

#define TEMAC_GMI_MII_WRVAL 	0x03b0 		/* MII write data */
#define TEMAC_GMI_MII_ADDR 	0x03b4 		/* MII address */
#define GMI_MII_ADDR_REG(val) 	((val) & 0x01f)
#define GMI_MII_ADDR_PHY(val) 	(((val) & 0x01f) << 5)

#endif /*_VIRTEX_DEV_TEMACREG_H_*/
