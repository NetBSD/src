/*	$NetBSD: if_arc.h,v 1.1 1995/02/23 07:19:54 glass Exp $	*/

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
 * from: NetBSD: if_ether.h,v 1.10 1994/06/29 06:37:55 cgd Exp
 *       @(#)if_ether.h	8.1 (Berkeley) 6/10/93
 */

/*
 * Arcnet address - 1 octets
 * don't know who uses this.
 */
struct arc_addr {
	u_char arc_addr_octet[1];
};

/*
 * Structure of a 2.5MB/s Arcnet header.
 * as given to interface code.
 */
struct	arc_header {
	u_char	arc_shost;
	u_char	arc_dhost;
	u_char	arc_type;
};

#define ARC_HDRLEN      3

/* RFC 1051 */
#define	ARCTYPE_IP_OLD	240	/* IP protocol */
#define	ARCTYPE_ARP_OLD	241	/* address resolution protocol */

/* RFC 1201 */
#define	ARCTYPE_IP	212	/* IP protocol */
#define	ARCTYPE_ARP	213	/* address resolution protocol */
#define	ARCTYPE_REVARP	214	/* reverse addr resolution protocol */

#define	ARCMTU		507
#define	ARCMIN		0

struct	arccom {
	struct 	ifnet ac_if;		/* network-visible interface */
	u_char	ac_anaddr;		/* arcnet hardware address */
					/* only first byte used for arc */
	struct	in_addr ac_ipaddr;	/* copy of ip address- XXX */
};

#ifdef	KERNEL
u_char	arcbroadcastaddr;
#endif
