/*	$NetBSD: svr4_sockio.h,v 1.5.6.1 2006/04/22 11:38:21 simonb Exp $	 */

/*-
 * Copyright (c) 1995 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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

#ifndef	_SVR4_SOCKIO_H_
#define	_SVR4_SOCKIO_H_


#define	SVR4_IFF_UP		0x00000001
#define	SVR4_IFF_BROADCAST	0x00000002
#define	SVR4_IFF_DEBUG		0x00000004
#define	SVR4_IFF_LOOPBACK	0x00000008
#define	SVR4_IFF_POINTOPOINT	0x00000010
#define	SVR4_IFF_NOTRAILERS	0x00000020
#define	SVR4_IFF_RUNNING	0x00000040
#define	SVR4_IFF_NOARP		0x00000080
#define	SVR4_IFF_PROMISC	0x00000100
#define	SVR4_IFF_ALLMULTI	0x00000200
#define	SVR4_IFF_INTELLIGENT	0x00000400
#define	SVR4_IFF_MULTICAST	0x00000800
#define	SVR4_IFF_MULTI_BCAST	0x00001000
#define	SVR4_IFF_UNNUMBERED	0x00002000
#define	SVR4_IFF_DHCPRUNNING	0x00004000
#define	SVR4_IFF_PRIVATE	0x00008000
#define	SVR4_IFF_NOXMIT		0x00010000
#define	SVR4_IFF_NOLOCAL	0x00020000
#define	SVR4_IFF_DEPRECATED	0x00040000
#define	SVR4_IFF_ADDRCONF	0x00080000
#define	SVR4_IFF_ROUTER		0x00100000
#define	SVR4_IFF_NONUD		0x00200000
#define	SVR4_IFF_ANYCAST	0x00400000
#define	SVR4_IFF_NORTEXCH	0x00800000
#define	SVR4_IFF_IPV4		0x01000000
#define	SVR4_IFF_IPV6		0x02000000
#define	SVR4_IFF_MIPRUNNING	0x04000000
#define	SVR4_IFF_NOFAILOVER	0x08000000
#define	SVR4_IFF_FAILED		0x10000000
#define	SVR4_IFF_STANDBY	0x20000000
#define	SVR4_IFF_INACTIVE	0x40000000
#define	SVR4_IFF_OFFLINE	0x80000000

struct svr4_ifreq {
#define	SVR4_IFNAMSIZ	16
	char	svr4_ifr_name[SVR4_IFNAMSIZ];
	union {
		struct	osockaddr	ifru_addr;
		struct	osockaddr	ifru_dstaddr;
		struct	osockaddr	ifru_broadaddr;
		short			ifru_flags;
		int			ifru_metric;
		char			ifru_data;
		char			ifru_enaddr[6];
		int			if_muxid[2];

	} ifr_ifru;

#define	svr4_ifr_addr			ifr_ifru.ifru_addr
#define	svr4_ifr_dstaddr		ifr_ifru.ifru_dstaddr
#define	svr4_ifr_broadaddr		ifr_ifru.ifru_broadaddr
#define	svr4_ifr_flags			ifr_ifru.ifru_flags
#define	svr4_ifr_metric			ifr_ifru.ifru_metric
#define	svr4_ifr_data			ifr_ifru.ifru_data
#define	svr4_ifr_enaddr			ifr_ifru.ifru_enaddr
#define	svr4_ifr_muxid			ifr_ifru.ifru_muxid

};

typedef struct lif_ifinfo_req {
	uint8_t			lir_maxhops;
	uint32_t		lir_reachtime;
	uint32_t		lir_reachretrans;
	uint32_t		lir_maxmtu;
} lif_ifinfo_req_t;

#define	SVR4_ND_MAX_HDW_LEN	64
typedef struct svr4_lif_nd_req {
	struct sockaddr_storage	lnr_addr;
	uint8_t			lnr_state_create;
	uint8_t			lnr_state_same_lla;
	uint8_t			lnr_state_diff_lla;
	int			lnr_hdw_len;
	int			lnr_flags;
	int			lnr_pad0;
	char			lnr_hdw_addr[SVR4_ND_MAX_HDW_LEN];
} lif_nd_req_t;

struct svr4_lifreq {
#define	SVR4_LIFNAMSIZ	32
	char			lifr_name[SVR4_LIFNAMSIZ];
	union {
		int		lifru_addrlen;
		u_int		lifru_ppa;
	} lifr_lifru1;
#define	lifr_addrlen		lifr_lifru1.lifru_addrlen
#define	lifr_ppa		lifr_lifru1.lifru_ppa
	u_int			lifr_movetoindex;
	union {
		struct sockaddr_storage lifru_addr;
		struct sockaddr_storage lifru_dstaddr;
		struct sockaddr_storage lifru_broadaddr;
		struct sockaddr_storage lifru_token;
		struct sockaddr_storage lifru_subnet;
		int		lifru_index;
		uint64_t	lifru_flags;
		int		lifru_metric;
		u_int		lifru_mtu;
		char		lifru_data[1];
		char		lifru_enaddr[6];
		int		lif_muxid[2];
		struct svr4_lif_nd_req	lifru_nd_req;
		struct lif_ifinfo_req	lifru_ifinfo_req;
		char		lifru_groupname[SVR4_LIFNAMSIZ];
		u_int		lifru_delay;
	} lifr_lifru;
};

#define	svr4_lifr_addr		lifr_lifru.lifru_addr
#define	svr4_lifr_dstaddr	lifr_lifru.lifru_dstaddr
#define	svr4_lifr_broadaddr	lifr_lifru.lifru_broadaddr
#define	svr4_lifr_token		lifr_lifru.lifru_token
#define	svr4_lifr_subnet	lifr_lifru.lifru_subnet
#define	svr4_lifr_index		lifr_lifru.lifru_index
#define	svr4_lifr_flags		lifr_lifru.lifru_flags
#define	svr4_lifr_metric	lifr_lifru.lifru_metric
#define	svr4_lifr_mtu		lifr_lifru.lifru_mtu
#define	svr4_lifr_data		lifr_lifru.lifru_data
#define	svr4_lifr_enaddr	lifr_lifru.lifru_enaddr
#define	svr4_lifr_index		lifr_lifru.lifru_index
#define	svr4_lifr_ip_muxid	lifr_lifru.lif_muxid[0]
#define	svr4_lifr_arp_muxid	lifr_lifru.lif_muxid[1]
#define	svr4_lifr_nd		lifr_lifru.lifru_nd_req
#define	svr4_lifr_ifinfo	lifr_lifru.lifru_ifinfo_req
#define	svr4_lifr_groupname	lifr_lifru.lifru_groupname
#define	svr4_lifr_delay		lifr_lifru.lifru_delay


typedef u_short svr4_sa_family_t;

struct svr4_lifnum {
	svr4_sa_family_t	lifn_family;
	int			lifn_flags;
	int			lifn_count;
};

struct svr4_ifconf {
	int	svr4_ifc_len;
	union {
		caddr_t			 ifcu_buf;
		struct svr4_ifreq 	*ifcu_req;
	} ifc_ifcu;

#define	svr4_ifc_buf	ifc_ifcu.ifcu_buf
#define	svr4_ifc_req	ifc_ifcu.ifcu_req
};

#define SVR4_SIOC	('i' << 8)

#define	SVR4_SIOCGIFFLAGS	SVR4_IOWR('i', 17, struct svr4_ifreq)
#define	SVR4_SIOCGIFCONF	SVR4_IOWR('i', 20, struct svr4_ifconf)
#define	SVR4_SIOCGIFNUM		SVR4_IOR('i', 87, int)
#define	SVR4_SIOCGLIFFLAGS	SVR4_IOWR('i', 117, struct svr4_lifreq)
#define	SVR4_SIOCGLIFNUM	SVR4_IOWR('i', 130, struct svr4_lifnum)

#endif /* !_SVR4_SOCKIO_H_ */
