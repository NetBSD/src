/*	$NetBSD: ppp.h,v 1.2 1999/05/11 02:54:29 thorpej Exp $	*/

/* @(#) Header: ppp.h,v 1.7 95/05/04 17:52:46 mccanne Exp  (LBL) */
/*
 * Point to Point Protocol (PPP) RFC1331
 *
 * Copyright 1989 by Carnegie Mellon.
 *
 * Permission to use, copy, modify, and distribute this program for any
 * purpose and without fee is hereby granted, provided that this copyright
 * and permission notice appear on all copies and supporting documentation,
 * the name of Carnegie Mellon not be used in advertising or publicity
 * pertaining to distribution of the program without specific prior
 * permission, and notice be given in supporting documentation that copying
 * and distribution is by permission of Carnegie Mellon and Stanford
 * University.  Carnegie Mellon makes no representations about the
 * suitability of this software for any purpose.  It is provided "as is"
 * without express or implied warranty.
 */
#define PPP_ADDRESS	0xff	/* The address byte value */
#define PPP_CONTROL	0x03	/* The control byte value */

/* Actual address byte values */
#define	PPP_ADDR_ALLSTATIONS	0xff	/* all stations broadcast addr */
#define	PPP_ADDR_CISCO_MULTICAST 0x8f	/* Cisco multicast address */
#define	PPP_ADDR_CISCO_UNICAST	0x0f	/* Cisco unicast address */

/* Actual control byte values */
#define	PPP_CTRL_UI		0x03	/* unnumbered information */

/* Protocol numbers */
#define PPP_IP		0x0021	/* Raw IP */
#define PPP_OSI		0x0023	/* OSI Network Layer */
#define PPP_NS		0x0025	/* Xerox NS IDP */
#define PPP_DECNET	0x0027	/* DECnet Phase IV */
#define PPP_APPLE	0x0029	/* Appletalk */
#define PPP_IPX		0x002b	/* Novell IPX */
#define PPP_VJC		0x002d	/* Van Jacobson Compressed TCP/IP */
#define PPP_VJNC	0x002f	/* Van Jacobson Uncompressed TCP/IP */
#define PPP_BRPDU	0x0031	/* Bridging PDU */
#define PPP_STII	0x0033	/* Stream Protocol (ST-II) */
#define PPP_VINES	0x0035	/* Banyan Vines */
#define	PPP_IPV6	0x0057	/* IPv6 */

#define PPP_HELLO	0x0201	/* 802.1d Hello Packets */
#define PPP_LUXCOM	0x0231	/* Luxcom */
#define PPP_SNS		0x0233	/* Sigma Network Systems */

#define PPP_IPCP	0x8021	/* IP Control Protocol */
#define PPP_OSICP	0x8023	/* OSI Network Layer Control Protocol */
#define PPP_NSCP	0x8025	/* Xerox NS IDP Control Protocol */
#define PPP_DECNETCP	0x8027	/* DECnet Control Protocol */
#define PPP_APPLECP	0x8029	/* Appletalk Control Protocol */
#define PPP_IPXCP	0x802b	/* Novell IPX Control Protocol */
#define PPP_STIICP	0x8033	/* Strean Protocol Control Protocol */
#define PPP_VINESCP	0x8035	/* Banyan Vines Control Protocol */

/*
 * XXX Note, this is overloaded with VINESCP, but we can tell based on
 * XXX the address byte if we're using Cisco protocol numbers (i.e.
 * XXX Ethertypes).
 */
#define	PPP_CISCO_KEEPALIVE 0x8035 /* Cisco keepalive protocol */

#define PPP_LCP		0xc021	/* Link Control Protocol */
#define PPP_PAP		0xc023	/* Password Authentication Protocol */
#define PPP_LQM		0xc025	/* Link Quality Monitoring */
#define	PPP_CBCP	0xc029	/* Callback Control Protocol */
#define PPP_CHAP	0xc223	/* Challenge Handshake Authentication Protocol */

/*
 * Structure of a Cisco keepalive packet.
 */
#define	CISCO_KEEP_TYPE_OFF	0	/* 4 bytes */
#define	CISCO_KEEP_PAR1_OFF	4	/* 4 bytes */
#define	CISCO_KEEP_PAR2_OFF	8	/* 4 bytes */
#define	CISCO_KEEP_REL_OFF	10	/* 2 bytes */
#define	CISCO_KEEP_TIME0_OFF	12	/* 2 bytes */
#define	CISCO_KEEP_TIME1_OFF	14	/* 2 bytes */

#define	CISCO_KEEP_LEN		18

#define	CISCO_KEEP_TYPE_ADDR_REQ	0
#define	CISCO_KEEP_TYPE_ADDR_REPLY	1
#define	CISCO_KEEP_TYPE_KEEP_REQ	2
