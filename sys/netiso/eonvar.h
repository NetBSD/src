/*	$NetBSD: eonvar.h,v 1.18.28.1 2012/10/30 17:22:51 yamt Exp $	*/

/*-
 * Copyright (c) 1991, 1993
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
 * 3. Neither the name of the University nor the names of its contributors
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
 *	@(#)eonvar.h	8.1 (Berkeley) 6/10/93
 */

/***********************************************************
		Copyright IBM Corporation 1987

                      All Rights Reserved

Permission to use, copy, modify, and distribute this software and its
documentation for any purpose and without fee is hereby granted,
provided that the above copyright notice appear in all copies and that
both that copyright notice and this permission notice appear in
supporting documentation, and that the name of IBM not be
used in advertising or publicity pertaining to distribution of the
software without specific, written prior permission.

IBM DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE, INCLUDING
ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO EVENT SHALL
IBM BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR
ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION,
ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
SOFTWARE.

******************************************************************/

/*
 * ARGO Project, Computer Sciences Dept., University of Wisconsin - Madison
 */

#ifndef _NETISO_EONVAR_H_
#define _NETISO_EONVAR_H_

#include <sys/ansi.h>

#ifndef sa_family_t
typedef __sa_family_t	sa_family_t;
#define sa_family_t	__sa_family_t
#endif

#define EON_986_VERSION 0x3
#define EON_VERSION 0x1

#define EON_CACHESIZE 30

#define E_FREE 	1
#define E_LINK	2
#define E_ES 	3
#define E_IS 	4


/*
 * this overlays a sockaddr_iso
 */

struct sockaddr_eon {
	u_char          seon_len;	/* Length */
	sa_family_t     seon_family;	/* AF_ISO */
	u_char          seon_status;	/* overlays session suffixlen */
#define EON_ESLINK_UP		0x1
#define EON_ESLINK_DOWN		0x2
#define EON_ISLINK_UP		0x10
#define EON_ISLINK_DOWN		0x20
	/* no change is neither up or down */
	u_char          seon_pad1;	/* 0, overlays tsfxlen */
	u_char          seon_adrlen;
	u_char          seon_afi;	/* 47 */
	u_char          seon_idi[2];	/* 0006 */
	u_char          seon_vers;	/* 03 */
	u_char          seon_glbnum[2];	/* see RFC 1069 */
	u_char          seon_RDN[2];	/* see RFC 1070 */
	u_char          seon_pad2[3];	/* see RFC 1070 */
	u_char          seon_LAREA[2];	/* see RFC 1070 */
	u_char          seon_pad3[2];	/* see RFC 1070 */
	/*
	 * right now ip addr is  aligned  -- be careful -- future revisions
	 * may have it u_char[4]
	 */
	u_int           seon_ipaddr;	/* a.b.c.d */
	u_char          seon_protoid;	/* NSEL */
};

#ifdef EON_TEMPLATE
struct sockaddr_eon eon_template = {
	sizeof(eon_template), AF_ISO, 0, 0, 0x14,
	0x47, 0x0, 0x6, 0x3, 0
};
#endif

#define DOWNBITS ( EON_ESLINK_DOWN | EON_ISLINK_DOWN )
#define UPBITS ( EON_ESLINK_UP | EON_ISLINK_UP )

struct eon_hdr {
	u_char          eonh_vers;	/* value 1 */
	u_char          eonh_class;	/* address multicast class, below */
#define		EON_NORMAL_ADDR		0x0
#define		EON_MULTICAST_ES	0x1
#define		EON_MULTICAST_IS	0x2
#define		EON_BROADCAST		0x3
	u_short         eonh_csum;	/* osi checksum (choke) */
};
struct eon_iphdr {
	struct ip       ei_ip;
	struct eon_hdr  ei_eh;
};
#define EONIPLEN (sizeof(struct eon_hdr) + sizeof(struct ip))

/* stole these 2 fields of the flags for I-am-ES and I-am-IS */
#define	IFF_ES	0x400
#define	IFF_IS	0x800

extern struct eon_stat {
	int             es_in_multi_es;
	int             es_in_multi_is;
	int             es_in_broad;
	int             es_in_normal;
	int             es_out_multi_es;
	int             es_out_multi_is;
	int             es_out_broad;
	int             es_out_normal;
	int             es_ipout;

	int             es_icmp[PRC_NCMDS];
	/* errors */
	int             es_badcsum;
	int             es_badhdr;
}               eonstat;

#undef IncStat
#define IncStat(xxx) eonstat.xxx++

typedef struct qhdr {
	struct qhdr    *link, *rlink;
}              *queue_t;

struct eon_llinfo {
	struct qhdr     el_qhdr;/* keep all in a list */
	int             el_flags;	/* cache valid ? */
	int             el_snpaoffset;	/* IP address contained in dst nsap */
	struct rtentry *el_rt;	/* back pointer to parent route */
	struct eon_iphdr el_ei;	/* precomputed portion of hdr */
	struct route    el_iproute;	/* if direct route cache IP info */
	/* if gateway, cache secondary route */
};
#define el_iphdr el_ei.ei_ip
#define el_eonhdr el_ei.ei_eh

#ifdef _KERNEL
void eonprotoinit (void);
void eonattach   (void);
int eonioctl    (struct ifnet *, u_long, void *);
void eoniphdr(struct eon_iphdr *, const void *, struct route *, int);
void eonrtrequest (int, struct rtentry *, const struct rt_addrinfo *);
int eonoutput(struct ifnet *, struct mbuf *, const struct sockaddr *,
		     struct rtentry *);
void eoninput    (struct mbuf *, ...);
void *eonctlinput(int, const struct sockaddr *, void *);
#endif

#endif /* !_NETISO_EONVAR_H_ */
