/*	$NetBSD: if_mecreg.h,v 1.1.4.2 2000/06/22 17:03:01 minoura Exp $	*/

/*
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

#define MEC_MAC_CONTROL		0x00
#define MAC_CORE_RESET		0x0000000000000001
#define MAC_FULL_DUPLEX		0x0000000000000002
#define MAC_INT_LOOPBACK	0x0000000000000004
#define MAC_SPEED_SELECT	0x0000000000000008
#define MAC_MII_SELECT		0x0000000000000010
#define MAC_FILTER_STATION	0x0000000000000000
#define MAC_FILTER_MATCHMULTI	0x0000000000000020
#define MAC_FILTER_ALLMULTI	0x0000000000000040
#define MAC_FILTER_PROMISC	0x0000000000000060
#define MAC_LINK_FAILURE	0x0000000000000080
#define MAC_IPGT		0x0000000000007f00
#define MAC_IPGT_SHIFT		8
#define MAC_IPGR1		0x00000000003f8000
#define MAC_IPGR1_SHIFT		15
#define MAC_IPGR2		0x000000001fc00000
#define MAC_IPGR2_SHIFT		22
#define MAC_REVISION		0x00000000e0000000
#define MAC_REVISION_SHIFT	29

#define MEC_INT_STATUS		0x08

#define MEC_DMA_CONTROl		0x10

#define MEC_PHY_DATA		0x60
#define PHY_DATA_BUSY		0x0000000000008000
#define PHY_DATA_VALUE		0x0000000000007fff

#define MEC_PHY_ADDRESS		0x68
#define PHY_ADDR_REGISTER	0x000000000000001f
#define PHY_ADDR_DEVICE		0x00000000000003e0
#define PHY_ADDR_DEVSHIFT	5

#define MEC_PHY_READ_INITATE	0x70

#define MEC_STATION		0xa0
#define MEC_STATION_ALT		0xa8
#define MEC_STATION_MASK	0x0000ffffffffffff
