/* $NetBSD: if_aumacreg.h,v 1.1 2002/07/29 15:39:14 simonb Exp $ */

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
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

#ifndef _MIPS_ALCHEMY_DEV_AUMACREG_H_
#define	_MIPS_ALCHEMY_DEV_AUMACREG_H_

/*
 * Register description for the Alchemy Semiconductor Au1X00
 * Ethernet Media Access Controllers.
 */

#define	MAC_BUFLEN	 0x0800
#define	MAC_BUFLEN_JUMBO 0x2800

/*
 * MAC registers.
 */

#define	MAC_CONTROL	0x0000	/* MAC control */
#define	CONTROL_RA	(1U << 31)	/* receive all */
#define	CONTROL_EM	(1U << 30)	/* 1 = big endian */
#define	CONTROL_DO	(1U << 23)	/* disable receive own */
#define	CONTROL_LM(x)	((x) << 21)	/* loopback mode */
					/* 0 = normal */
					/* 1 = internal loopback */
					/* 2 = external loopback */
					/* 3 = reserved */
#define	CONTROL_F	(1U << 20)	/* full-duplex mode */
#define	CONTROL_PM	(1U << 19)	/* pass all multicast */
#define	CONTROL_PR	(1U << 18)	/* promiscuous mode */
#define	CONTROL_IF	(1U << 17)	/* inverse filtering */
#define	CONTROL_PB	(1U << 16)	/* pass bad frames */
#define	CONTROL_HO	(1U << 15)	/* hash-only filtering */
#define	CONTROL_HP	(1U << 13)	/* hash-perfect filtering */
#define	CONTROL_LC	(1U << 12)	/* re-tx on late collision */
#define	CONTROL_DB	(1U << 11)	/* disable broadcast frames */
#define	CONTROL_DR	(1U << 10)	/* disable retry */
#define	CONTROL_AP	(1U << 8)	/* automatic pad stripping */
#define	CONTROL_BL(x)	((x) << 6)	/* backoff limit */
#define	CONTROL_DC	(1U << 5)	/* deferral check */
#define	CONTROL_TE	(1U << 3)	/* transmitter enable */
#define	CONTROL_RE	(1U << 2)	/* receiver enable */

#define	MAC_ADDRHIGH	0x0004	/* high 16 bits of station address */

#define	MAC_ADDRLOW	0x0008	/* low 32 bits of station address */

#define	MAC_HASHHIGH	0x000c	/* high 32 bits of multicast hash */

#define	MAC_HASHLOW	0x0010	/* low 32 bits of multicast hash */

#define	MAC_MIICTRL	0x0014	/* MII PHY control */
#define	MIICTRL_PHYADDR(x) ((x) << 11)	/* PHY address */
#define	MIICTRL_MIIREG(x)  ((x) << 6)	/* MII register */
#define	MIICTRL_MW	   (1U << 1)	/* MII write */
#define	MIICTRL_MB	   (1U << 0)	/* MII busy */

#define	MAC_MIIDATA	0x0018	/* MII PHY data */
#define	MIIDATA_MASK	0xffff		/* MII data bits */

#define	MAC_FLOWCTRL	0x001c	/* control frame generation control */
#define	FLOWCTRL_PT(x)	((x) << 16)	/* pause time */
#define	FLOWCTRL_PC	(1U << 2)	/* pass control frame */
#define	FLOWCTRL_FE	(1U << 1)	/* flow control enable */
#define	FLOWCTRL_FB	(1U << 0)	/* flow control busy */

#define	MAC_VLAN1	0x0020	/* VLAN1 tag */

#define	MAC_VLAN2	0x0024	/* VLAN2 tag */

/*
 * MAC Enable registers.
 */

#define	MACEN_JP	(1U << 6)	/* jumbo packet enable */
#define	MACEN_E2	(1U << 5)	/* enable2 */
#define	MACEN_E1	(1U << 4)	/* enable1 */
#define	MACEN_C		(1U << 3)	/* 0 == coherent */
#define	MACEN_TS	(1U << 2)	/* disable toss */
#define	MACEN_E0	(1U << 1)	/* enable0 */
#define	MACEN_CE	(1U << 0)	/* clock enable */

/*
 * MAC DMA registers.
 */

#define	MACDMA_TX_ENTRY(x)	((x) << 4)
#define	MACDMA_RX_ENTRY(x)	(((x) << 4) + 0x100)

#define	MACDMA_TX_STAT(x)	(MACDMA_TX_ENTRY(x) + 0x00)
#define	MACDMA_TX_ADDR(x)	(MACDMA_TX_ENTRY(x) + 0x04)
#define	MACDMA_TX_LEN(x)	(MACDMA_TX_ENTRY(x) + 0x08)

#define	MACDMA_RX_STAT(x)	(MACDMA_RX_ENTRY(x) + 0x00)
#define	MACDMA_RX_ADDR(x)	(MACDMA_RX_ENTRY(x) + 0x04)

#define	RX_STAT_MI	(1U << 31)	/* missed frame */
#define	RX_STAT_PF	(1U << 30)	/* packet filter pass */
#define	RX_STAT_FF	(1U << 29)	/* filtering fail */
#define	RX_STAT_BF	(1U << 28)	/* broadcast frame */
#define	RX_STAT_MF	(1U << 27)	/* multicast frame */
#define	RX_STAT_UC	(1U << 26)	/* unsupported control frame */
#define	RX_STAT_CF	(1U << 25)	/* control frame */
#define	RX_STAT_LE	(1U << 24)	/* length error */
#define	RX_STAT_V2	(1U << 23)	/* VLAN2 match */
#define	RX_STAT_V1	(1U << 22)	/* VLAN1 match */
#define	RX_STAT_CR	(1U << 21)	/* CRC error */
#define	RX_STAT_DB	(1U << 20)	/* dribbling bit */
#define	RX_STAT_ME	(1U << 19)	/* MII error */
#define	RX_STAT_FT	(1U << 18)	/* 0 = 802.3, 1 = Ethernet */
#define	RX_STAT_CS	(1U << 17)	/* collision seen */
#define	RX_STAT_FL	(1U << 16)	/* frame too long */
#define	RX_STAT_RF	(1U << 15)	/* runt frame */
#define	RX_STAT_WT	(1U << 14)	/* watchdog timeout */
#define	RX_STAT_L(x)	((x) & 0x3fff)	/* frame length */

#define	RX_STAT_ERRS	(RX_STAT_MI | RX_STAT_UC | RX_STAT_LE | RX_STAT_CR | \
			 RX_STAT_DB | RX_STAT_ME | RX_STAT_CS | RX_STAT_FL | \
			 RX_STAT_RF | RX_STAT_WT)

#define	RX_ADDR_CB(x)	(((x) >> 2) & 3)/* current buffer */
#define	RX_ADDR_DN	(1U << 1)	/* transaction done */
#define	RX_ADDR_EN	(1U << 0)	/* enable this buffer */

#define	TX_STAT_PR	(1U << 31)	/* packet retry */
#define	TX_STAT_CC(x)	(((x) >> 10) & 0xf) /* collision count */
#define	TX_STAT_LO	(1U << 9)	/* late collision observed */
#define	TX_STAT_DF	(1U << 8)	/* deferred transmission */
#define	TX_STAT_UR	(1U << 7)	/* data underrun */
#define	TX_STAT_EC	(1U << 6)	/* excessive collisions */
#define	TX_STAT_LC	(1U << 5)	/* late collision */
#define	TX_STAT_ED	(1U << 4)	/* excessive deferral */
#define	TX_STAT_LS	(1U << 3)	/* loss of carrier */
#define	TX_STAT_NC	(1U << 2)	/* no carrier */
#define	TX_STAT_JT	(1U << 1)	/* jabber timeout */
#define	TX_STAT_FA	(1U << 0)	/* frame aborted */

#define	TX_ADDR_CB(x)	(((x) >> 2) & 3)/* current buffer */
#define	TX_ADDR_DN	(1U << 1)	/* transaction done */
#define	TX_ADDR_EN	(1U << 0)	/* enable this buffer */

#endif /* _MIPS_ALCHEMY_DEV_AUMACREG_H_ */
