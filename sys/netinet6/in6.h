/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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
 * 3. Neither the name of the project nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE PROJECT AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE PROJECT OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/*
 * Copyright (c) 1982, 1986, 1990, 1993
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
 *	@(#)in.h	8.3 (Berkeley) 1/3/94
 */

#ifndef _NETINET6_IN6_H_
#define _NETINET6_IN6_H_

#include <sys/queue.h>

/*
 * Identification of the network protocol stack
 */
#define __KAME__
#define __KAME_VERSION		"SNAP 19990628/NetBSD-current"

/*
 * Local port number conventions:
 *
 * Ports < IPPORT_RESERVED are reserved for privileged processes (e.g. root),
 * unless a kernel is compiled with IPNOPRIVPORTS defined.
 *
 * When a user does a bind(2) or connect(2) with a port number of zero,
 * a non-conflicting local port address is chosen.
 *
 * The default range is IPPORT_ANONMIX to IPPORT_ANONMAX, although
 * that is settable by sysctl(3); net.inet.ip.anonportmin and
 * net.inet.ip.anonportmax respectively.
 *
 * A user may set the IPPROTO_IP option IP_PORTRANGE to change this
 * default assignment range.
 *
 * The value IP_PORTRANGE_DEFAULT causes the default behavior.
 *
 * The value IP_PORTRANGE_HIGH is the same as IP_PORTRANGE_DEFAULT,
 * and exists only for FreeBSD compatibility purposes.
 *
 * The value IP_PORTRANGE_LOW changes the range to the "low" are
 * that is (by convention) restricted to privileged processes.
 * This convention is based on "vouchsafe" principles only.
 * It is only secure if you trust the remote host to restrict these ports.
 * The range is IPPORT_RESERVEDMIN to IPPORT_RESERVEDMAX.
 */

#define	IPV6PORT_RESERVED	1024
#define	IPV6PORT_ANONMIN	49152
#define	IPV6PORT_ANONMAX	65535
#define	IPV6PORT_RESERVEDMIN	600
#define	IPV6PORT_RESERVEDMAX	(IPV6PORT_RESERVED-1)

/*
 * IPv6 address
 */
struct  in6_addr {
	union {
		u_int32_t  u6_addr32[4];
		u_int16_t  u6_addr16[8];
		u_int8_t   u6_addr8[16];
	} u6_addr;			/* 128 bit IP6 address */
};

#define s6_addr32 u6_addr.u6_addr32
#define s6_addr16 u6_addr.u6_addr16
#define s6_addr8  u6_addr.u6_addr8
#define s6_addr   u6_addr.u6_addr8

#define INET6_ADDRSTRLEN	46

/*
 * Socket address for IPv6
 */
#define SIN6_LEN
struct sockaddr_in6 {
	u_char		sin6_len;	/* length of this struct(sa_family_t)*/
	u_char		sin6_family;	/* AF_INET6 (sa_family_t) */
	u_int16_t	sin6_port;	/* Transport layer port # (in_port_t)*/
	u_int32_t	sin6_flowinfo;	/* IP6 flow information */
	struct in6_addr	sin6_addr;	/* IP6 address */
	u_int32_t	sin6_scope_id;	/* intface scope id */
};

/*
 * Local definition for masks
 */
#define IN6MASK0	{{{ 0, 0, 0, 0 }}}
#define IN6MASK32	{{{ 0xffffffff, 0, 0, 0 }}}
#define IN6MASK64	{{{ 0xffffffff, 0xffffffff, 0, 0 }}}
#define IN6MASK96	{{{ 0xffffffff, 0xffffffff, 0xffffffff, 0 }}}
#define IN6MASK128	{{{ 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }}}

#ifdef _KERNEL
extern const struct in6_addr in6mask0;
extern const struct in6_addr in6mask32;
extern const struct in6_addr in6mask64;
extern const struct in6_addr in6mask96;
extern const struct in6_addr in6mask128;
#endif /* _KERNEL */

/*
 * Macros started with IPV6_ADDR is KAME local
 */

#if BYTE_ORDER == BIG_ENDIAN
#define IPV6_ADDR_INT32_ONE	1
#define IPV6_ADDR_INT32_TWO	2
#define IPV6_ADDR_INT32_MNL	0xff010000
#define IPV6_ADDR_INT32_MLL	0xff020000
#define IPV6_ADDR_INT32_SMP	0x0000ffff
#define IPV6_ADDR_INT16_ULL	0xfe80
#define IPV6_ADDR_INT16_USL	0xfec0
#define IPV6_ADDR_INT16_MLL	0xff02
#elif BYTE_ORDER == LITTLE_ENDIAN
#define IPV6_ADDR_INT32_ONE	0x01000000
#define IPV6_ADDR_INT32_TWO	0x02000000
#define IPV6_ADDR_INT32_MNL	0x000001ff
#define IPV6_ADDR_INT32_MLL	0x000002ff
#define IPV6_ADDR_INT32_SMP	0xffff0000
#define IPV6_ADDR_INT16_ULL	0x80fe
#define IPV6_ADDR_INT16_USL	0xc0fe
#define IPV6_ADDR_INT16_MLL	0x02ff
#endif

/*
 * Definition of some useful macros to handle IP6 addresses
 */
#define IN6ADDR_ANY_INIT		{{{ 0, 0, 0, 0 }}}
#define IN6ADDR_LOOPBACK_INIT		{{{ 0, 0, 0, IPV6_ADDR_INT32_ONE }}}
#define IN6ADDR_NODELOCAL_ALLNODES_INIT	\
	{{{ IPV6_ADDR_INT32_MNL, 0, 0, IPV6_ADDR_INT32_ONE }}}
#define IN6ADDR_LINKLOCAL_ALLNODES_INIT	\
	{{{ IPV6_ADDR_INT32_MLL, 0, 0, IPV6_ADDR_INT32_ONE }}}
#define IN6ADDR_LINKLOCAL_ALLROUTERS_INIT \
	{{{ IPV6_ADDR_INT32_MLL, 0, 0, IPV6_ADDR_INT32_TWO }}}

extern const struct in6_addr in6addr_any;
extern const struct in6_addr in6addr_loopback;
extern const struct in6_addr in6addr_nodelocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allnodes;
extern const struct in6_addr in6addr_linklocal_allrouters;

/*
 * Equality
 */
#define IN6_ARE_ADDR_EQUAL(a, b)			\
	(((a)->s6_addr32[0] == (b)->s6_addr32[0]) &&	\
	 ((a)->s6_addr32[1] == (b)->s6_addr32[1]) &&	\
	 ((a)->s6_addr32[2] == (b)->s6_addr32[2]) &&	\
	 ((a)->s6_addr32[3] == (b)->s6_addr32[3]))

/*
 * Unspecified
 */
#define IN6_IS_ADDR_UNSPECIFIED(a)	\
	(((a)->s6_addr32[0] == 0) &&	\
	 ((a)->s6_addr32[1] == 0) &&	\
	 ((a)->s6_addr32[2] == 0) &&	\
	 ((a)->s6_addr32[3] == 0))

/*
 * Loopback
 */
#define IN6_IS_ADDR_LOOPBACK(a)		\
	(((a)->s6_addr32[0] == 0) &&	\
	 ((a)->s6_addr32[1] == 0) &&	\
	 ((a)->s6_addr32[2] == 0) &&	\
	 ((a)->s6_addr32[3] == IPV6_ADDR_INT32_ONE))

/*
 * IPv4 compatible
 */
#define IN6_IS_ADDR_V4COMPAT(a)		\
	(((a)->s6_addr32[0] == 0) &&	\
	 ((a)->s6_addr32[1] == 0) &&	\
	 ((a)->s6_addr32[2] == 0) &&	\
	 ((a)->s6_addr32[3] != 0) &&	\
	 ((a)->s6_addr32[3] != IPV6_ADDR_INT32_ONE))

/*
 * Mapped
 */
#define IN6_IS_ADDR_V4MAPPED(a)		      \
	(((a)->s6_addr32[0] == 0) &&	      \
	 ((a)->s6_addr32[1] == 0) &&	      \
	 ((a)->s6_addr32[2] == IPV6_ADDR_INT32_SMP))

/*
 * KAME Scope Values
 */

#define IPV6_ADDR_SCOPE_NODELOCAL	0x01
#define IPV6_ADDR_SCOPE_LINKLOCAL	0x02
#define IPV6_ADDR_SCOPE_SITELOCAL	0x05
#define IPV6_ADDR_SCOPE_ORGLOCAL	0x08	/* just used in this file */
#define IPV6_ADDR_SCOPE_GLOBAL		0x0e

/*
 * Unicast Scope
 */
#define IN6_IS_ADDR_LINKLOCAL(a)	\
	((a)->s6_addr16[0] == IPV6_ADDR_INT16_ULL)
#define IN6_IS_ADDR_SITELOCAL(a)	\
	((a)->s6_addr16[0] == IPV6_ADDR_INT16_USL)

/*
 * Multicast
 */
#define IN6_IS_ADDR_MULTICAST(a)	((a)->s6_addr8[0] == 0xff)

#define IPV6_ADDR_MC_SCOPE(a)		((a)->s6_addr8[1] & 0x0f)

/*
 * Multicast Scope
 */
#define IN6_IS_ADDR_MC_NODELOCAL(a)	\
	(IN6_IS_ADDR_MULTICAST(a) &&	\
	 (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_NODELOCAL))
#define IN6_IS_ADDR_MC_LINKLOCAL(a)	\
	(IN6_IS_ADDR_MULTICAST(a) &&	\
	 (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_LINKLOCAL))
#define IN6_IS_ADDR_MC_SITELOCAL(a)	\
	(IN6_IS_ADDR_MULTICAST(a) && 	\
	 (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_SITELOCAL))
#define IN6_IS_ADDR_MC_ORGLOCAL(a)	\
	(IN6_IS_ADDR_MULTICAST(a) &&	\
	 (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_ORGLOCAL))
#define IN6_IS_ADDR_MC_GLOBAL(a)	\
	(IN6_IS_ADDR_MULTICAST(a) &&	\
	 (IPV6_ADDR_MC_SCOPE(a) == IPV6_ADDR_SCOPE_GLOBAL))

/*
 * Wildcard Socket
 */
#define IN6_IS_ADDR_ANY(a)	IN6_IS_ADDR_UNSPECIFIED(a)

/*
 * KAME Scope
 */
#define IN6_IS_SCOPE_LINKLOCAL(a)	\
	((IN6_IS_ADDR_LINKLOCAL(a)) ||	\
	 (IN6_IS_ADDR_MC_LINKLOCAL(a)))

/*
 * IP6 route structure
 */
struct	route_in6 {
	struct	rtentry *ro_rt;
	struct	sockaddr_in6 ro_dst;
};

/*
 * Options for use with [gs]etsockopt at the IPV6 level.
 * First word of comment is data type; bool is stored in int.
 */
#define IPV6_OPTIONS		1  /* buf/ip6_opts; set/get IP6 options */
/* no hdrincl */
#define IPV6_SOCKOPT_RESERVED1	3  /* reserved for future use */
#define IPV6_UNICAST_HOPS	4  /* int; IP6 hops */
#define IPV6_RECVOPTS		5  /* bool; receive all IP6 opts w/dgram */
#define IPV6_RECVRETOPTS	6  /* bool; receive IP6 opts for response */
#define IPV6_RECVDSTADDR	7  /* bool; receive IP6 dst addr w/dgram */
#define IPV6_RETOPTS		8  /* ip6_opts; set/get IP6 options */
#define IPV6_MULTICAST_IF	9  /* u_char; set/get IP6 multicast i/f  */
#define IPV6_MULTICAST_HOPS	10 /* u_char; set/get IP6 multicast hops */
#define IPV6_MULTICAST_LOOP	11 /* u_char; set/get IP6 multicast loopback */
#define IPV6_JOIN_GROUP		12 /* ip6_mreq; join a group membership */
#define IPV6_LEAVE_GROUP	13 /* ip6_mreq; leave a group membership */
#define IPV6_PORTRANGE		14 /* int; range to choose for unspec port */
#define ICMP6_FILTER		18 /* icmp6_filter; icmp6 filter */
#define IPV6_PKTINFO		19 /* bool; send/rcv if, src/dst addr */
#define IPV6_HOPLIMIT		20 /* bool; hop limit */
#define IPV6_NEXTHOP		21 /* bool; next hop addr */
#define IPV6_HOPOPTS		22 /* bool; hop-by-hop option */
#define IPV6_DSTOPTS		23 /* bool; destination option */
#define IPV6_RTHDR		24 /* bool; routing header */
#define IPV6_PKTOPTIONS		25 /* buf/cmsghdr; set/get IPv6 options */
#define IPV6_CHECKSUM		26 /* int; checksum offset for raw socket */
#define IPV6_BINDV6ONLY		27 /* bool; only bind INET6 at null bind */

#if 1 /*IPSEC*/
#define IPV6_IPSEC_POLICY	28 /* struct; get/set security policy */
#endif
#define IPV6_FAITH		32 /* bool; accept FAITH'ed connections */

#define IPV6_RTHDR_LOOSE     0 /* this hop need not be a neighbor. XXX old spec */
#define IPV6_RTHDR_STRICT    1 /* this hop must be a neighbor. XXX old spec */
#define IPV6_RTHDR_TYPE_0    0 /* IPv6 routing header type 0 */

/*
 * Defaults and limits for options
 */
#define IPV6_DEFAULT_MULTICAST_HOPS 1	/* normally limit m'casts to 1 hop  */
#define IPV6_DEFAULT_MULTICAST_LOOP 1	/* normally hear sends if a member  */

/*
 * Argument structure for IPV6_JOIN_GROUP and IPV6_LEAVE_GROUP.
 */
struct ipv6_mreq {
	struct in6_addr	ipv6mr_multiaddr;
	u_int		ipv6mr_interface;
};

/*
 * IPV6_PKTINFO: Packet information(RFC2292 sec 5)
 */
struct in6_pktinfo {
	struct in6_addr ipi6_addr;	/* src/dst IPv6 address */
	u_int ipi6_ifindex;		/* send/recv interface index */
};

/*
 * Argument for IPV6_PORTRANGE:
 * - which range to search when port is unspecified at bind() or connect()
 */
#define	IPV6_PORTRANGE_DEFAULT	0	/* default range */
#define	IPV6_PORTRANGE_HIGH	1	/* "high" - request firewall bypass */
#define	IPV6_PORTRANGE_LOW	2	/* "low" - vouchsafe security */

/*
 * Definitions for inet6 sysctl operations.
 *
 * Third level is protocol number.
 * Fourth level is desired variable within that protocol.
 */
#define IPV6PROTO_MAXID	(IPPROTO_PIM + 1)	/* don't list to IPV6PROTO_MAX */

#define CTL_IPV6PROTO_NAMES { \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ "tcp6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "udp6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ "ip6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ "ipsec6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "icmp6", CTLTYPE_NODE }, \
	{ 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, { 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "pim6", CTLTYPE_NODE }, \
}

/*
 * Names for IP sysctl objects
 */
#define IPV6CTL_FORWARDING	1	/* act as router */
#define IPV6CTL_SENDREDIRECTS	2	/* may send redirects when forwarding*/
#define IPV6CTL_DEFHLIM		3	/* default Hop-Limit */
#ifdef notyet
#define IPV6CTL_DEFMTU		4	/* default MTU */
#endif
#define IPV6CTL_FORWSRCRT	5	/* forward source-routed dgrams */
#define IPV6CTL_STATS		6	/* stats */
#define IPV6CTL_MRTSTATS	7	/* multicast forwarding stats */
#define IPV6CTL_MRTPROTO	8	/* multicast routing protocol */
#define IPV6CTL_MAXFRAGPACKETS	9	/* max packets reassembly queue */
#define IPV6CTL_SOURCECHECK	10	/* verify source route and intf */
#define IPV6CTL_SOURCECHECK_LOGINT 11	/* minimume logging interval */
#define IPV6CTL_ACCEPT_RTADV	12
#define IPV6CTL_KEEPFAITH	13
#define IPV6CTL_LOG_INTERVAL	14
#define IPV6CTL_HDRNESTLIMIT	15
#define IPV6CTL_DAD_COUNT	16
#define IPV6CTL_AUTO_FLOWLABEL	17
#define IPV6CTL_DEFMCASTHLIM	18
#define IPV6CTL_GIF_HLIM	19	/* default HLIM for gif encap packet */
#define IPV6CTL_KAME_VERSION	20
/* New entries should be added here from current IPV6CTL_MAXID value. */
#define IPV6CTL_MAXID		21

#define IPV6CTL_NAMES { \
	{ 0, 0 }, \
	{ "forwarding", CTLTYPE_INT }, \
	{ "redirect", CTLTYPE_INT }, \
	{ "hlim", CTLTYPE_INT }, \
	{ "mtu", CTLTYPE_INT }, \
	{ "forwsrcrt", CTLTYPE_INT }, \
	{ 0, 0 }, \
	{ 0, 0 }, \
	{ "mrtproto", CTLTYPE_INT }, \
	{ "maxfragpackets", CTLTYPE_INT }, \
	{ "sourcecheck", CTLTYPE_INT }, \
	{ "sourcecheck_logint", CTLTYPE_INT }, \
	{ "accept_rtadv", CTLTYPE_INT }, \
	{ "keepfaith", CTLTYPE_INT }, \
	{ "log_interval", CTLTYPE_INT }, \
	{ "hdrnestlimit", CTLTYPE_INT }, \
	{ "dad_count", CTLTYPE_INT }, \
	{ "auto_flowlabel", CTLTYPE_INT }, \
	{ "defmcasthlim", CTLTYPE_INT }, \
	{ "gifhlim", CTLTYPE_INT }, \
	{ "kame_version", CTLTYPE_STRING }, \
}

#define IPV6CTL_VARS { \
	0, \
	&ip6_forwarding, \
	&ip6_sendredirects, \
	&ip6_defhlim, \
	0, \
	&ip6_forward_srcrt, \
	0, \
	0, \
	0, \
	&ip6_maxfragpackets, \
	&ip6_sourcecheck, \
	&ip6_sourcecheck_interval, \
	&ip6_accept_rtadv, \
	&ip6_keepfaith, \
	&ip6_log_interval, \
	&ip6_hdrnestlimit, \
	&ip6_dad_count, \
	&auto_flowlabel, \
	&ip6_defmcasthlim, \
	&ip6_gif_hlim, \
	0, \
}

#ifdef _KERNEL
struct cmsghdr;

int	in6_canforward __P((struct in6_addr *, struct in6_addr *));
int	in6_cksum __P((struct mbuf *, u_int8_t, int, int));
int	in6_localaddr __P((struct in6_addr *));
int	in6_addrscope __P((struct in6_addr *));
struct	in6_ifaddr *in6_ifawithscope __P((struct ifnet *, struct in6_addr *));
struct	in6_ifaddr *in6_ifawithifp __P((struct ifnet *, struct in6_addr *));
extern void in6_if_up __P((struct ifnet *));

#define	satosin6(sa)	((struct sockaddr_in6 *)(sa))
#define	sin6tosa(sin6)	((struct sockaddr *)(sin6))
#define	ifatoia6(ifa)	((struct in6_ifaddr *)(ifa))
#endif /* _KERNEL */

__BEGIN_DECLS
struct cmsghdr;

extern int inet6_option_space(int);
extern int inet6_option_init(void *, struct cmsghdr **, int);
extern int inet6_option_append(struct cmsghdr *, const u_int8_t *, int, int);
extern u_int8_t *inet6_option_alloc(struct cmsghdr *, int, int, int);
extern int inet6_option_next(const struct cmsghdr *, u_int8_t **);
extern int inet6_option_find(const struct cmsghdr *, u_int8_t **, int);

extern size_t inet6_rthdr_space __P((int, int));
extern struct cmsghdr *inet6_rthdr_init __P((void *, int));
extern int inet6_rthdr_add __P((struct cmsghdr *, const struct in6_addr *,
		unsigned int));
extern int inet6_rthdr_lasthop __P((struct cmsghdr *, unsigned int));
#if 0 /* not implemented yet */
extern int inet6_rthdr_reverse __P((const struct cmsghdr *, struct cmsghdr *));
#endif
extern int inet6_rthdr_segments __P((const struct cmsghdr *));
extern struct in6_addr *inet6_rthdr_getaddr __P((struct cmsghdr *, int));
extern int inet6_rthdr_getflags __P((const struct cmsghdr *, int));
__END_DECLS

#endif /* !_NETINET6_IN6_H_ */
