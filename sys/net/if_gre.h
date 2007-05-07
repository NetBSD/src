/*	$NetBSD: if_gre.h,v 1.20.2.2 2007/05/07 10:55:54 yamt Exp $ */

/*
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Heiko W.Rupp <hwr@pilhuhn.de>
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#ifndef _NET_IF_GRE_H_
#define _NET_IF_GRE_H_

#include <sys/queue.h>
#include <sys/mutex.h>
#include <sys/condvar.h>

#ifdef _KERNEL
struct gre_soparm {
	struct in_addr	sp_src;		/* source address of gre packets */
	struct in_addr	sp_dst;		/* destination address of gre packets */
	in_port_t	sp_srcport;	/* source port of gre packets */
	in_port_t	sp_dstport;	/* destination port of gre packets */
};

struct gre_softc {
	struct ifnet		sc_if;
	kmutex_t		sc_mtx;
	kcondvar_t		sc_soparm_cv;
	kcondvar_t		sc_join_cv;
	kcondvar_t		sc_work_cv;
	int			sc_haswork;
	int			sc_running;
	struct ifqueue		sc_snd;
	struct gre_soparm	sc_soparm;
	struct file		*sc_fp;
	LIST_ENTRY(gre_softc)	sc_list;
	struct route route;	/* routing entry that determines, where a
				   encapsulated packet should go */
	int			sc_proto;	/* protocol of encapsulator */
};
#define	g_src		sc_soparm.sp_src
#define	g_srcport	sc_soparm.sp_srcport
#define	g_dst		sc_soparm.sp_dst
#define	g_dstport	sc_soparm.sp_dstport

struct gre_h {
	u_int16_t flags;	/* GRE flags */
	u_int16_t ptype;	/* protocol type of payload typically
				   Ether protocol type*/
/*
 *  from here on: fields are optional, presence indicated by flags
 *
	u_int_16 checksum	checksum (one-complements of GRE header
				and payload
				Present if (ck_pres | rt_pres == 1).
				Valid if (ck_pres == 1).
	u_int_16 offset		offset from start of routing filed to
				first octet of active SRE (see below).
				Present if (ck_pres | rt_pres == 1).
				Valid if (rt_pres == 1).
	u_int_32 key		inserted by encapsulator e.g. for
				authentication
				Present if (key_pres ==1 ).
	u_int_32 seq_num	Sequence number to allow for packet order
				Present if (seq_pres ==1 ).
	struct gre_sre[] routing Routing fileds (see below)
				Present if (rt_pres == 1)
 */
} __attribute__((__packed__));

struct greip {
	struct ip gi_i;
	struct gre_h  gi_g;
} __attribute__((__packed__));

#define gi_pr		gi_i.ip_p
#define gi_len		gi_i.ip_len
#define gi_src		gi_i.ip_src
#define gi_dst		gi_i.ip_dst
#define gi_ptype	gi_g.ptype
#define gi_flags	gi_g.flags

#define GRE_CP		0x8000  /* Checksum Present */
#define GRE_RP		0x4000  /* Routing Present */
#define GRE_KP		0x2000  /* Key Present */
#define GRE_SP		0x1000  /* Sequence Present */
#define GRE_SS		0x0800	/* Strict Source Route */

/*
 * gre_sre defines a Source route Entry. These are needed if packets
 * should be routed over more than one tunnel hop by hop
 */
struct gre_sre {
	u_int16_t sre_family;	/* address family */
	u_char	sre_offset;	/* offset to first octet of active entry */
	u_char	sre_length;	/* number of octets in the SRE.
				   sre_lengthl==0 -> last entry. */
	u_char	*sre_rtinfo;	/* the routing information */
};

/* for mobile encaps */

struct mobile_h {
	u_int16_t proto;		/* protocol and S-bit */
	u_int16_t hcrc;			/* header checksum */
	u_int32_t odst;			/* original destination address */
	u_int32_t osrc;			/* original source addr, if S-bit set */
} __attribute__((__packed__));

struct mobip_h {
	struct ip	mi;
	struct mobile_h	mh;
} __attribute__((__packed__));


#define MOB_H_SIZ_S		(sizeof(struct mobile_h) - sizeof(u_int32_t))
#define MOB_H_SIZ_L		(sizeof(struct mobile_h))
#define MOB_H_SBIT	0x0080

#define	GRE_TTL	30
extern int ip_gre_ttl;
#endif /* _KERNEL */

/*
 * ioctls needed to manipulate the interface
 */

#define GRESADDRS	_IOW('i', 101, struct ifreq)
#define GRESADDRD	_IOW('i', 102, struct ifreq)
#define GREGADDRS	_IOWR('i', 103, struct ifreq)
#define GREGADDRD	_IOWR('i', 104, struct ifreq)
#define GRESPROTO	_IOW('i' , 105, struct ifreq)
#define GREGPROTO	_IOWR('i', 106, struct ifreq)
#define GRESSOCK	_IOW('i' , 107, struct ifreq)
#define GREDSOCK	_IOW('i' , 108, struct ifreq)

#ifdef _KERNEL
LIST_HEAD(gre_softc_head, gre_softc);
extern struct gre_softc_head gre_softc_list;

u_int16_t gre_in_cksum(u_short *, u_int);
int gre_input3(struct gre_softc *, struct mbuf *, int, const struct gre_h *,
    int);
#endif /* _KERNEL */

#endif /* !_NET_IF_GRE_H_ */
