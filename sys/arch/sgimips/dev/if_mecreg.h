/*	$NetBSD: if_mecreg.h,v 1.3 2002/12/26 22:24:46 pooka Exp $	*/

/*
 * Copyright (c) 2001 Christopher Sekiya
 * Copyright (c) 2000 Soren S. Jorvang
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *          This product includes software developed for the
 *          NetBSD Project.  See http://www.netbsd.org/ for
 *          information about NetBSD.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
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

/*
 * MACE MAC110 ethernet register definitions
 */

#define MEC_MAC_CONTROL			0

#define MEC_MAC_CORE_RESET		0x0000000000000001
#define MEC_MAC_FULL_DUPLEX		0x0000000000000002 /* 1 to enable */
#define MEC_MAC_INT_LOOPBACK		0x0000000000000004 /* 0 = normal oper */
#define MEC_MAC_SPEED_SELECT		0x0000000000000008 /* 0/1 10/100 */
#define MEC_MAC_MII_SELECT		0x0000000000000010
#define MEC_MAC_FILTER_STATION		0x0000000000000000
#define MEC_MAC_FILTER_MATCHMULTI	0x0000000000000020
#define MEC_MAC_FILTER_ALLMULTI		0x0000000000000040
#define MEC_MAC_FILTER_PROMISC		0x0000000000000060
#define MEC_MAC_LINK_FAILURE		0x0000000000000080
#define MEC_MAC_IPGT			0x0000000000007f00
#define MEC_MAC_IPGT_SHIFT		8
#define MEC_MAC_IPGR1			0x00000000003f8000
#define MEC_MAC_IPGR1_SHIFT		15
#define MEC_MAC_IPGR2			0x000000001fc00000
#define MEC_MAC_IPGR2_SHIFT		22
#define MEC_MAC_REVISION		0x00000000e0000000
#define MEC_MAC_REVISION_SHIFT		29

#define MEC_INT_STATUS		1 << 3
#define MEC_DMA_CONTROL		2 << 3
#define MEC_TIMER		3 << 3
#define MEC_TX_ALIAS		4 << 3
#define MEC_RX_ALIAS		5 << 3

/* 64-32 register, 31-16 write, 15-0 read */
#define MEC_RING_PTR		6 << 3

#define MEC_RX_FIFO		8 << 3
#define MEC_TX_VECTOR1		11 << 3
#define MEC_TX_VECTOR2		11 << 3

#define MEC_PHY_DATA		12 << 3
#define MEC_PHY_DATA_BUSY	0x8000
#define MEC_PHY_DATA_VALUE	0x7fff

#define MEC_PHY_ADDRESS		13 << 3
#define MEC_PHY_ADDR_REGISTER	0x000000000000001f
#define MEC_PHY_ADDR_DEVICE	0x00000000000003e0
#define MEC_PHY_ADDR_DEVSHIFT	5

#define MEC_PHY_READ_INITIATE	14 << 3

#define MEC_PHY_BACKOFF		15 << 3

#define MEC_MSGQUEUE		16 << 3

#define MEC_STATION		0xa0
#define MEC_STATION_ALT		0xa8
#define MEC_STATION_MASK	0x0000ffffffffffff


/* DMA control values */
#define MEC_DMA_TX_INT_EN           0x0001
#define MEC_DMA_TX_DMA_EN           0x0002
#define MEC_DMA_TX_RINGMSK          0x000c
#define MEC_DMA_TX_8K               0x0000
#define MEC_DMA_TX_16K              0x0004
#define MEC_DMA_TX_32K              0x0008
#define MEC_DMA_TX_64K              0x000c
#define MEC_DMA_TX_RINGMSK_SHIFT    2
#define MEC_DMA_RX_THRSHD           0x01f0
#define MEC_DMA_RX_INT_EN           0x0200
#define MEC_DMA_RX_RUNTS_EN         0x0400
#define MEC_DMA_RX_GATHER_EN        0x0800
#define MEC_DMA_RX_OFFSET           0x7000
#define MEC_DMA_RX_OFFSET_SHIFT     12
#define MEC_DMA_RX_DMA_EN           0x8000
