/*	$NetBSD: ethertypes.h,v 1.7 1999/03/20 03:37:52 thorpej Exp $	*/

/*
 * Copyright (c) 1982, 1986, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
 *	@(#)if_ether.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Ethernet protocol types.
 *
 * According to "assigned numbers", the Ethernet protocol numbers are also
 * used as ARP protocol type numbers.
 *
 * I factor them out here to avoid pulling all the Ethernet header file
 * into the hardware independent ARP code. -is
 */

#ifndef _NET_ETHERTYPES_H_
#define _NET_ETHERTYPES_H_

#define ETHERTYPE_8023		0x0004	/* IEEE 802.3 packet */
#define	ETHERTYPE_PUP		0x0200	/* PUP protocol */
#define	ETHERTYPE_SPRITE	0x0500	/* ??? */
#define	ETHERTYPE_NS		0x0600	/* XNS */
#define	ETHERTYPE_IP		0x0800	/* IP protocol */
#define	ETHERTYPE_ARP		0x0806	/* Address resolution protocol */
#define	ETHERTYPE_MOPDL		0x6001	/* DEC MOP dump/load */
#define	ETHERTYPE_MOPRC		0x6002	/* DEC MOP remote console */
#define	ETHERTYPE_DECnet	0x6003	/* DEC DECNET Phase IV route */
#define	ETHERTYPE_DN		ETHERTYPE_DECnet	/* libpcap, tcpdump */
#define	ETHERTYPE_LAT		0x6004	/* DEC LAT */
#define ETHERTYPE_SCA		0x6007	/* DEC LAVC, SCA */
#define ETHERTYPE_HP		0x8005	/* HP Probe */
#define ETHERTYPE_SG_DIAG	0x8013	/* SGI diagnostic type */
#define ETHERTYPE_SG_NETGAMES	0x8014	/* SGI network games */
#define ETHERTYPE_SG_RESV	0x8015	/* SGI reserved type */
#define ETHERTYPE_SG_BOUNCE	0x8016	/* SGI bounce server */
#define	ETHERTYPE_REVARP	0x8035	/* Reverse addr resolution protocol */
#define	ETHERTYPE_LANBRIDGE	0x8038	/* DEC LANBridge */
#define	ETHERTYPE_DECDNS	0x803C	/* DEC DNS */
#define	ETHERTYPE_DECDTS	0x803E	/* DEC DTS */
#define	ETHERTYPE_VEXP		0x805B	/* Stanford V Kernel exp. */
#define	ETHERTYPE_VPROD		0x805C	/* Stanford V Kernel prod. */
#define	ETHERTYPE_ATALK		0x809B	/* AppleTalk */
#define ETHERTYPE_AT		ETHERTYPE_ATALK		/* old NetBSD */
#define ETHERTYPE_APPLETALK	ETHERTYPE_ATALK		/* HP-UX */
#define	ETHERTYPE_AARP		0x80F3	/* AppleTalk AARP */
#define	ETHERTYPE_IPX		0x8137	/* Novell IPX */
#define ETHERTYPE_XTP		0x817D	/* Protocol Engines XTP */
#define ETHERTYPE_STP		0x8181	/* Scheduled Transfer STP, HIPPI-ST */
#define	ETHERTYPE_IPV6		0x86DD	/* IP protocol version 6 */
#define	ETHERTYPE_PPP		0x880B	/* PPP (obsolete by PPPOE) */
#define	ETHERTYPE_PPPOEDISC	0x8863	/* PPP Over Ethernet Discovery Stage */
#define	ETHERTYPE_PPPOE		0x8864	/* PPP Over Ethernet Session Stage */
#define	ETHERTYPE_LOOPBACK	0x9000	/* Loopback */
#define ETHERTYPE_LBACK		ETHERTYPE_LOOPBACK	/* DEC MOP loopback */
#define	ETHERTYPE_MAX		0xFFFF	/* Maximum valid ethernet type */

/*
 * The ETHERTYPE_NTRAILER packet types starting at ETHERTYPE_TRAIL have
 * (type-ETHERTYPE_TRAIL)*512 bytes of data followed
 * by an ETHER type (as given above) and then the (variable-length) header.
 */
#define	ETHERTYPE_TRAIL		0x1000		/* Trailer packet */
#define	ETHERTYPE_NTRAILER	16

#endif /* _NET_ETHERTYPES_H_ */
