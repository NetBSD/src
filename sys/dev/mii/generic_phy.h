/*  $NetBSD: generic_phy.h,v 1.1 1997/10/17 17:33:51 bouyer Exp $   */
 
/*
 * Copyright (c) 1997 Manuel Bouyer.  All rights reserved.
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
 *  This product includes software developed by Manuel Bouyer.
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

/* MII commands */
#define MII_START 0x01
#define MII_READ  0x02
#define MII_WRITE 0x01
#define MII_ACK   0x02

/* generic phy registers */
#define PHY_CONTROL	0x00 	/* read/write */
#	define	CTRL_RESET		0x8000 /* reset */
#	define	CTRL_LOOPBK		0x4000 /* loopback */
#	define	CTRL_SPEED		0x2000 /* speed (10/100) select */
#	define	CTRL_AUTO_EN	0x1000 /* autonegotiation enable */
#	define	CTRL_ISO		0x0400 /* isolate */
#	define	CTRL_AUTORST	0x0200 /* restart autonegotiation */
#	define	CTRL_DUPLEX		0x0100 /* Set duplex mode */
#	define	CTRL_COL_TEST	0x0080 /* collision test */
#define PHY_STATUS	0x01	/* Read Only */
#	define	ST_100bT4		0x8000 /* 100 base T4 capable */
#	define	ST_100bTx_fd	0x4000 /* 100 base Tx full duplex capable */
#	define	ST_100bTx		0x2000 /* 100 base Tx half duplex capable */
#	define	ST_10bT_fd		0x1000 /* 10 base T full duplex capable */
#	define	ST_10bT			0x0800 /* 10 base T half duplex capable */
#	define	ST_AUTO_DONE	0x0020 /* Autonegotiation complete */
#	define	ST_RemFault		0x0010 /* Link partner fault */
#	define	ST_AUTO			0x0008 /* Autonegotiation capable */
#	define	ST_LINK			0x0004 /* Link status */
#	define	ST_JABBER		0x0002 /* Jabber detected */
#	define	ST_Ext			0x0001 /* Extended capability */
#define PHY_IDH		0x02	/* Read Only */
#define PHY_IDL		0x03	/* Read Only */
#define PHY_AN_Adv	0x04	/* Read/write */
#	define Adv_NP			0x8000 /* Next page */
#	define Adv_RemFault	0x2000 /* Remote fault */
#	define Adv_100bT4		0x0200 /* local device supports 100bT4 */
#	define Adv_100bTx_fd	0x0100 /* local device supports 100bTx FD */
#	define Adv_100bTx		0x0080 /* local device supports 100bTx */
#	define Adv_10bT_fd		0x0040 /* local device supports 10bT FD */
#	define Adv_10bT			0x0020 /* local device supports 10bT */
#	define Adv_Sel			0x001f /* Autoneg selector field */
#define PHY_AN_LPA	0x05	/* Read Only */
#	define LPA_NP			0x8000 /* Next page */
#	define LPA_RemFault		0x2000 /* remote fault */
#	define LPA_100bT4		0x0200 /* link partner supports 100bT4 */
#	define LPA_100bTx_fd	0x0100 /* link partner supports 100bTx FD */
#	define LPA_100bTx		0x0080 /* link partner supports 100bTx */
#	define LPA_10bT_fd		0x0040 /* link partner supports 10bT FD */
#	define LPA_10bT			0x0020 /* link partner supports 10bT */
#	define LPA_Sel			0x001f /* selector field */
#define PHY_AN_Epx	0x06	/* Read Only */
#	define Epx_ParDet		0x0010 /* multiple link detection fault */
#	define Epx_LP_NP		0x0008 /* link parter next page-able */
#	define Epx_NP			0x0004 /* next page-able */
#	define Epx_PRX			0x0002 /* Page received */
#	define Epx_LP_auto		0x0001 /* link parter autoneg-able */

/* generic phy softc */
struct phy_softc {
	struct device sc_dev;
	mii_phy_t *phy_link;
}; 

int phy_reset __P((struct phy_softc*));
int phy_media_probe __P((struct phy_softc*));
void phy_media_print __P((u_int32_t));
int phy_media_set_10_100 __P((struct phy_softc*, int media));
int phy_status __P((int, void*));
void phy_dumpreg __P((struct phy_softc*));
