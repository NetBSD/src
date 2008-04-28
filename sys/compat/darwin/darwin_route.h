/*	$NetBSD: darwin_route.h,v 1.5 2008/04/28 20:23:41 martin Exp $ */

/*-
 * Copyright (c) 2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Emmanuel Dreyfus
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

#ifndef	_DARWIN_ROUTE_H_
#define	_DARWIN_ROUTE_H_

#include <net/if.h>

struct darwin_if_data {
	u_char did_type;
	u_char did_typelen;
	u_char did_physical;
	u_char did_addrlen;
	u_char did_hdrlen;
	u_char did_recquota;
	u_char did_xmitquota;
	u_long did_mtu;
	u_long did_metric;
	u_long did_baudrate;
	u_long did_ipackets;
	u_long did_ierrors;
	u_long did_opackets;
	u_long did_oerrors;
	u_long did_collisions;
	u_long did_ibytes;
	u_long did_obytes;
	u_long did_imcasts;
	u_long did_omcasts;
	u_long did_iqdrops;
	u_long did_noproto;
	u_long did_recvtiming;
	u_long did_xmittiming;
	struct timeval did_lastchange;
	u_long did_default_proto;
	u_long did_hwassist;
	u_long did_reserved1;
	u_long did_reserved2;
} __packed;


struct darwin_if_msghdr {
	u_short	dim_len;
	u_char	dim_vers;
	u_char	dim_type;	/* DARWIN_RTM_IFINFO */
	int	dim_addrs;
	int	dim_flags;
	u_short	dim_index;
	struct darwin_if_data dim_data;
	char	dim_pad1[3];
	/* Followed by a struct sockaddr_dl */
} __packed;

struct darwin_ifa_msghdr {
	u_short	diam_len;
	u_char	diam_vers;
	u_char	diam_type;	/* DARWIN_RTM_NEWADDR */
	int	diam_addrs;
	int 	diam_flags;
	u_short	diam_index;
	u_short diam_pad;
	int	diam_metric;
	/* Followed by struct sockaddr for netmask, addr, remote, broadcast */
} __packed;

struct darwin_ifma_msghdr {
	u_short	dimam_len;
	u_char	dimam_vers;
	u_char	dimam_type;	/* DARWIN_RTM_NEWMADDR ? */
	int	dimam_addrs;
	int 	dimam_flags;
	u_short	dimam_index;
} __packed;

/* dim_vers */
#define DARWIN_RTM_VERSION	5

/* dim_type */
#define DARWIN_RTM_ADD		0x1
#define DARWIN_RTM_DELETE	0x2
#define DARWIN_RTM_CHANGE	0x3
#define DARWIN_RTM_GET		0x4
#define DARWIN_RTM_LOSING	0x5
#define DARWIN_RTM_REDIRECT	0x6
#define DARWIN_RTM_MISS		0x7
#define DARWIN_RTM_LOCK		0x8
#define DARWIN_RTM_OLDADD	0x9
#define DARWIN_RTM_OLDDEL	0xa
#define DARWIN_RTM_RESOLVE	0xb
#define DARWIN_RTM_NEWADDR	0xc
#define DARWIN_RTM_DELADDR	0xd
#define DARWIN_RTM_IFINFO	0xe
#define DARWIN_RTM_NEWMADDR	0xf
#define DARWIN_RTM_DELMADDR	0x10

#define DARWIN_RTA_DST		0x1
#define DARWIN_RTA_GATEWAY	0x2
#define DARWIN_RTA_NETMASK	0x4
#define DARWIN_RTA_GENMASK	0x8
#define DARWIN_RTA_IFP		0x10
#define DARWIN_RTA_IFA		0x20
#define DARWIN_RTA_AUTHOR	0x40
#define DARWIN_RTA_BRD		0x80

/* sysctl for PF_ROUTE */
#define DARWIN_NET_RT_DUMP	1
#define DARWIN_NET_RT_FLAGS	2
#define DARWIN_NET_RT_IFLIST	3

int darwin_ifaddrs(int, char *, size_t *);

#endif /* _DARWIN_ROUTE_H */
