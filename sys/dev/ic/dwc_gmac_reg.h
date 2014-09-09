/*-
 * Copyright (c) 2013, 2014 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Matt Thomas of 3am Software Foundry and Martin Husemann.
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

#define	AWIN_GMAC_MAC_CONF		0x0000
#define	AWIN_GMAC_MAC_FFILT		0x0004
#define	AWIN_GMAC_MAC_HTHIGH		0x0008
#define	AWIN_GMAC_MAC_HTLOW		0x000c
#define	AWIN_GMAC_MAC_MIIADDR		0x0010
#define	AWIN_GMAC_MAC_MIIDATA		0x0014
#define	AWIN_GMAC_MAC_FLOWCTRL		0x0018
#define	AWIN_GMAC_MAC_VLANTAG		0x001c
#define	AWIN_GMAC_MAC_VERSION		0x0020
#define	AWIN_GMAC_MAC_INTR		0x0038
#define	AWIN_GMAC_MAC_INTMASK		0x003c
#define	AWIN_GMAC_MAC_ADDR0HI		0x0040
#define	AWIN_GMAC_MAC_ADDR0LO		0x0044
#define	AWIN_GMAC_MII_STATUS		0x00D8

#define	AWIN_GMAC_DMA_BUSMODE		0x1000
#define	AWIN_GMAC_DMA_TXPOLL		0x1004
#define	AWIN_GMAC_DMA_RXPOLL		0x1008
#define	AWIN_GMAC_DMA_RX_ADDR		0x100c
#define	AWIN_GMAC_DMA_TX_ADDR		0x1010
#define	AWIN_GMAC_DMA_STATUS		0x1014
#define	AWIN_GMAC_DMA_OPMODE		0x1018
#define	AWIN_GMAC_DMA_INTENABLE		0x101c
#define	AWIN_GMAC_DMA_CUR_TX_DESC	0x1048
#define	AWIN_GMAC_DMA_CUR_RX_DESC	0x104c
#define	AWIN_GMAC_DMA_CUR_TX_BUFADDR	0x1050
#define	AWIN_GMAC_DMA_CUR_RX_BUFADDR	0x1054

#define GMAC_MII_PHY_SHIFT		11
#define	GMAC_MII_PHY_MASK		(0x1F << GMAC_MII_PHY_SHIFT)
#define	GMAC_MII_REG_SHIFT		6
#define	GMAC_MII_REG_MASK		(0x1F << GMAC_MII_REG_SHIFT)

#define	GMAC_MII_BUSY			1
#define	GMAC_MII_WRITE			2

#define	GMAC_BUSMODE_RESET		1

#define	AWIN_GMAC_MII_IRQ		1

#define GMAC_DMA_INT_NIE		0x10000	/* Normal/Summary */
#define GMAC_DMA_INT_AIE		0x08000	/* Abnormal/Summary */
#define	GMAC_DMA_INT_ERE		0x04000	/* Early receive */
#define	GMAC_DMA_INT_FBE		0x02000	/* Fatal bus error */
#define	GMAC_DMA_INT_ETE		0x00400	/* Early transmit */
#define	GMAC_DMA_INT_RWE		0x00200	/* Receive watchdog */
#define	GMAC_DMA_INT_RSE		0x00100	/* Receive stopped */
#define	GMAC_DMA_INT_RUE		0x00080	/* Receive buffer unavailable */
#define	GMAC_DMA_INT_RIE		0x00040	/* Receive interrupt */
#define	GMAC_DMA_INT_UNE		0x00020	/* Tx underflow */
#define	GMAC_DMA_INT_OVE		0x00010	/* Receive overflow */
#define	GMAC_DMA_INT_TJE		0x00008	/* Transmit jabber */
#define	GMAC_DMA_INT_TUE		0x00004	/* Transmit buffer unavailable */
#define	GMAC_DMA_INT_TSE		0x00002	/* Transmit stopped */
#define	GMAC_DMA_INT_TIE		0x00001	/* Transmit interrupt */

#define	GMAC_DEF_DMA_INT_MASK	(GMAC_DMA_INT_TIE|GMAC_DMA_INT_RIE| \
				GMAC_DMA_INT_NIE|GMAC_DMA_INT_AIE| \
				GMAC_DMA_INT_FBE|GMAC_DMA_INT_UNE)

#define	AWIN_DEF_MAC_INTRMASK	0x207	/* XXX ??? */

struct dwc_gmac_dev_dmadesc {
	uint32_t ddesc_status;
/* both: */
#define	DDESC_STATUS_OWNEDBYDEV		(1<<31)
/* for TX descriptors */
#define	DDESC_STATUS_TXINT		(1<<30)
#define DDESC_STATUS_TXLAST		(1<<29)
#define	DDESC_STATUS_TXFIRST		(1<<28)
#define	DDESC_STATUS_TXCRCDIS		(1<<27)
#define	DDESC_STATUS_TXPADDIS		(1<<26)
#define	DDESC_STATUS_TXCHECKINSCTRL	(1<<22)
#define	DDESC_STATUS_TXRINGEND		(1<<21)
#define	DDESC_STATUS_TXCHAIN		(1<<20)
#define	DDESC_STATUS_MASK		0x1ffff
/* for RX descriptors */
#define	DDESC_STATUS_DAFILTERFAIL	(1<<30)
#define	DDESC_STATUS_FRMLENMSK		(0x3fff << 16)
#define	DDESC_STATUS_FRMLENSHIFT	16
#define	DDESC_STATUS_RXERROR		(1<<15)
#define	DDESC_STATUS_RXTRUNCATED	(1<<14)
#define	DDESC_STATUS_SAFILTERFAIL	(1<<13)
#define	DDESC_STATUS_RXIPC_GIANTFRAME	(1<<12)
#define	DDESC_STATUS_RXDAMAGED		(1<<11)
#define	DDESC_STATUS_RXVLANTAG		(1<<10)
#define	DDESC_STATUS_RXFIRST		(1<<9)
#define	DDESC_STATUS_RXLAST		(1<<8)
#define	DDESC_STATUS_RXIPC_GIANT	(1<<7)
#define	DDESC_STATUS_RXCOLLISION	(1<<6)
#define	DDESC_STATUS_RXFRAMEETHER	(1<<5)
#define	DDESC_STATUS_RXWATCHDOG		(1<<4)
#define	DDESC_STATUS_RXMIIERROR		(1<<3)
#define	DDESC_STATUS_RXDRIBBLING	(1<<2)
#define	DDESC_STATUS_RXCRC		1

	uint32_t ddesc_cntl;
#define	DDESC_CNTL_SIZE1MASK		0x1fff
#define	DDESC_CNTL_SIZE1SHIFT		0
#define	DDESC_CNTL_SIZE2MASK		(0x1fff<<16)
#define	DDESC_CNTL_SIZE2SHIFT		16

	uint32_t ddesc_data;	/* pointer to buffer data */
	uint32_t ddesc_next;	/* link to next descriptor */
};

