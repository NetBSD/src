/*	$NetBSD: if.h,v 1.65.2.3 2001/09/21 22:36:44 nathanw Exp $	*/

/*-
 * Copyright (c) 1999, 2000, 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by William Studnemund and Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
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

/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *	@(#)if.h	8.3 (Berkeley) 2/9/95
 */

#ifndef _NET_IF_H_
#define _NET_IF_H_

#if !defined(_XOPEN_SOURCE)

#include <sys/queue.h>
#include <net/dlt.h>
#include <net/pfil.h>

/*
 * Always include ALTQ glue here -- we use the ALTQ interface queue
 * structure even when ALTQ is not configured into the kernel so that
 * the size of struct ifnet does not changed based on the option.  The
 * ALTQ queue structure is API-compatible with the legacy ifqueue.
 */
#include <altq/if_altq.h>

/*
 * Structures defining a network interface, providing a packet
 * transport mechanism (ala level 0 of the PUP protocols).
 *
 * Each interface accepts output datagrams of a specified maximum
 * length, and provides higher level routines with input datagrams
 * received from its medium.
 *
 * Output occurs when the routine if_output is called, with four parameters:
 *	(*ifp->if_output)(ifp, m, dst, rt)
 * Here m is the mbuf chain to be sent and dst is the destination address.
 * The output routine encapsulates the supplied datagram if necessary,
 * and then transmits it on its medium.
 *
 * On input, each interface unwraps the data received by it, and either
 * places it on the input queue of a internetwork datagram routine
 * and posts the associated software interrupt, or passes the datagram to a raw
 * packet input routine.
 *
 * Routines exist for locating interfaces by their addresses
 * or for locating a interface on a certain network, as well as more general
 * routing and gateway routines maintaining information used to locate
 * interfaces.  These routines live in the files if.c and route.c
 */
/*  XXX fast fix for SNMP, going away soon */
#include <sys/time.h>

#if defined(_KERNEL_OPT)
#include "opt_compat_netbsd.h"
#endif

struct mbuf;
struct proc;
struct rtentry;
struct socket;
struct ether_header;
struct ifnet;
struct rt_addrinfo;

/*
 * Length of interface external name, including terminating '\0'.
 * Note: this is the same size as a generic device's external name.
 */
#define	IFNAMSIZ	16
#define	IF_NAMESIZE	IFNAMSIZ

/*
 * Structure describing a `cloning' interface.
 */
struct if_clone {
	LIST_ENTRY(if_clone) ifc_list;	/* on list of cloners */
	const char *ifc_name;		/* name of device, e.g. `gif' */
	size_t ifc_namelen;		/* length of name */

	int	(*ifc_create)(struct if_clone *, int);
	void	(*ifc_destroy)(struct ifnet *);
};

#define	IF_CLONE_INITIALIZER(name, create, destroy)			\
	{ { 0 }, name, sizeof(name) - 1, create, destroy }

/*
 * Structure used to query names of interface cloners.
 */
struct if_clonereq {
	int	ifcr_total;		/* total cloners (out) */
	int	ifcr_count;		/* room for this many in user buffer */
	char	*ifcr_buffer;		/* buffer for cloner names */
};

/*
 * Structure defining statistics and other data kept regarding a network
 * interface.
 */
struct if_data {
	/* generic interface information */
	u_char	ifi_type;		/* ethernet, tokenring, etc. */
	u_char	ifi_addrlen;		/* media address length */
	u_char	ifi_hdrlen;		/* media header length */
	int	ifi_link_state;		/* current link state */
	u_quad_t ifi_mtu;		/* maximum transmission unit */
	u_quad_t ifi_metric;		/* routing metric (external only) */
	u_quad_t ifi_baudrate;		/* linespeed */
	/* volatile statistics */
	u_quad_t ifi_ipackets;		/* packets received on interface */
	u_quad_t ifi_ierrors;		/* input errors on interface */
	u_quad_t ifi_opackets;		/* packets sent on interface */
	u_quad_t ifi_oerrors;		/* output errors on interface */
	u_quad_t ifi_collisions;	/* collisions on csma interfaces */
	u_quad_t ifi_ibytes;		/* total number of octets received */
	u_quad_t ifi_obytes;		/* total number of octets sent */
	u_quad_t ifi_imcasts;		/* packets received via multicast */
	u_quad_t ifi_omcasts;		/* packets sent via multicast */
	u_quad_t ifi_iqdrops;		/* dropped on input, this interface */
	u_quad_t ifi_noproto;		/* destined for unsupported protocol */
	struct	timeval ifi_lastchange;	/* last operational state change */
};

/*
 * Values for if_link_state.
 */
#define	LINK_STATE_UNKNOWN	0	/* link invalid/unknown */
#define	LINK_STATE_DOWN		1	/* link is down */
#define	LINK_STATE_UP		2	/* link is up */

#if defined(_KERNEL) && defined(COMPAT_14)
/* Pre-1.5 if_data struct */
struct if_data14 {
	/* generic interface information */
	u_char	ifi_type;		/* ethernet, tokenring, etc. */
	u_char	ifi_addrlen;		/* media address length */
	u_char	ifi_hdrlen;		/* media header length */
	u_long	ifi_mtu;		/* maximum transmission unit */
	u_long	ifi_metric;		/* routing metric (external only) */
	u_long	ifi_baudrate;		/* linespeed */
	/* volatile statistics */
	u_long	ifi_ipackets;		/* packets received on interface */
	u_long	ifi_ierrors;		/* input errors on interface */
	u_long	ifi_opackets;		/* packets sent on interface */
	u_long	ifi_oerrors;		/* output errors on interface */
	u_long	ifi_collisions;		/* collisions on csma interfaces */
	u_long	ifi_ibytes;		/* total number of octets received */
	u_long	ifi_obytes;		/* total number of octets sent */
	u_long	ifi_imcasts;		/* packets received via multicast */
	u_long	ifi_omcasts;		/* packets sent via multicast */
	u_long	ifi_iqdrops;		/* dropped on input, this interface */
	u_long	ifi_noproto;		/* destined for unsupported protocol */
	struct	timeval ifi_lastchange;	/* last operational state change */
};
#endif /* _KERNEL && COMPAT_14 */

/*
 * Structure defining a queue for a network interface.
 */
struct ifqueue {
	struct	mbuf *ifq_head;
	struct	mbuf *ifq_tail;
	int	ifq_len;
	int	ifq_maxlen;
	int	ifq_drops;
};

/*
 * Structure defining a queue for a network interface.
 *
 * (Would like to call this struct ``if'', but C isn't PL/1.)
 */
TAILQ_HEAD(ifnet_head, ifnet);		/* the actual queue head */

struct ifnet {				/* and the entries */
	void	*if_softc;		/* lower-level data for this if */
	TAILQ_ENTRY(ifnet) if_list;	/* all struct ifnets are chained */
	TAILQ_HEAD(, ifaddr) if_addrlist; /* linked list of addresses per if */
	char	if_xname[IFNAMSIZ];	/* external name (name + unit) */
	int	if_pcount;		/* number of promiscuous listeners */
	caddr_t	if_bpf;			/* packet filter structure */
	u_short	if_index;		/* numeric abbreviation for this if */
	short	if_timer;		/* time 'til if_watchdog called */
	short	if_flags;		/* up/down, broadcast, etc. */
	short	if__pad1;		/* be nice to m68k ports */
	struct	if_data if_data;	/* statistics and other data about if */
	/*
	 * Procedure handles.  If you add more of these, don't forget the
	 * corresponding NULL stub in if.c.
	 */
	int	(*if_output)		/* output routine (enqueue) */
		__P((struct ifnet *, struct mbuf *, struct sockaddr *,
		     struct rtentry *));
	void	(*if_input)		/* input routine (from h/w driver) */
		__P((struct ifnet *, struct mbuf *));
	void	(*if_start)		/* initiate output routine */
		__P((struct ifnet *));
	int	(*if_ioctl)		/* ioctl routine */
		__P((struct ifnet *, u_long, caddr_t));
	int	(*if_init)		/* init routine */
		__P((struct ifnet *));
	void	(*if_stop)		/* stop routine */
		__P((struct ifnet *, int));
	void	(*if_watchdog)		/* timer routine */
		__P((struct ifnet *));
	void	(*if_drain)		/* routine to release resources */
		__P((struct ifnet *));
	struct ifaltq if_snd;		/* output queue (includes altq) */
	struct	sockaddr_dl *if_sadl;	/* pointer to our sockaddr_dl */
	u_int8_t *if_broadcastaddr;	/* linklevel broadcast bytestring */
	struct ifprefix *if_prefixlist; /* linked list of prefixes per if */
	void	*if_bridge;		/* bridge glue */
	int	if_dlt;			/* data link type (<net/dlt.h>) */
	struct pfil_head if_pfil;	/* filtering point */
	uint64_t if_capabilities;	/* interface capabilities */
	uint64_t if_capenable;		/* capabilities enabled */

	/*
	 * These are pre-computed based on an interfaces enabled
	 * capabilities, for speed elsewhere.
	 */
	int	if_csum_flags_tx;	/* M_CSUM_* flags for Tx */
	int	if_csum_flags_rx;	/* M_CSUM_* flags for Rx */
};
#define	if_mtu		if_data.ifi_mtu
#define	if_type		if_data.ifi_type
#define	if_addrlen	if_data.ifi_addrlen
#define	if_hdrlen	if_data.ifi_hdrlen
#define	if_metric	if_data.ifi_metric
#define	if_link_state	if_data.ifi_link_state
#define	if_baudrate	if_data.ifi_baudrate
#define	if_ipackets	if_data.ifi_ipackets
#define	if_ierrors	if_data.ifi_ierrors
#define	if_opackets	if_data.ifi_opackets
#define	if_oerrors	if_data.ifi_oerrors
#define	if_collisions	if_data.ifi_collisions
#define	if_ibytes	if_data.ifi_ibytes
#define	if_obytes	if_data.ifi_obytes
#define	if_imcasts	if_data.ifi_imcasts
#define	if_omcasts	if_data.ifi_omcasts
#define	if_iqdrops	if_data.ifi_iqdrops
#define	if_noproto	if_data.ifi_noproto
#define	if_lastchange	if_data.ifi_lastchange

#define	IFF_UP		0x0001		/* interface is up */
#define	IFF_BROADCAST	0x0002		/* broadcast address valid */
#define	IFF_DEBUG	0x0004		/* turn on debugging */
#define	IFF_LOOPBACK	0x0008		/* is a loopback net */
#define	IFF_POINTOPOINT	0x0010		/* interface is point-to-point link */
#define	IFF_NOTRAILERS	0x0020		/* avoid use of trailers */
#define	IFF_RUNNING	0x0040		/* resources allocated */
#define	IFF_NOARP	0x0080		/* no address resolution protocol */
#define	IFF_PROMISC	0x0100		/* receive all packets */
#define	IFF_ALLMULTI	0x0200		/* receive all multicast packets */
#define	IFF_OACTIVE	0x0400		/* transmission in progress */
#define	IFF_SIMPLEX	0x0800		/* can't hear own transmissions */
#define	IFF_LINK0	0x1000		/* per link layer defined bit */
#define	IFF_LINK1	0x2000		/* per link layer defined bit */
#define	IFF_LINK2	0x4000		/* per link layer defined bit */
#define	IFF_MULTICAST	0x8000		/* supports multicast */

/* flags set internally only: */
#define	IFF_CANTCHANGE \
	(IFF_BROADCAST|IFF_POINTOPOINT|IFF_RUNNING|IFF_OACTIVE|\
	    IFF_SIMPLEX|IFF_MULTICAST|IFF_ALLMULTI|IFF_PROMISC)

/*
 * Some convenience macros used for setting ifi_baudrate.
 * XXX 1000 vs. 1024? --thorpej@netbsd.org
 */
#define	IF_Kbps(x)	((x) * 1000)		/* kilobits/sec. */
#define	IF_Mbps(x)	(IF_Kbps((x) * 1000))	/* megabits/sec. */
#define	IF_Gbps(x)	(IF_Mbps((x) * 1000))	/* gigabits/sec. */

/* Capabilities that interfaces can advertise. */
#define	IFCAP_CSUM_IPv4		0x0001	/* can do IPv4 header checksums */
#define	IFCAP_CSUM_TCPv4	0x0002	/* can do IPv4/TCP checksums */
#define	IFCAP_CSUM_UDPv4	0x0004	/* can do IPv4/UDP checksums */
#define	IFCAP_CSUM_TCPv6	0x0008	/* can do IPv6/TCP checksums */
#define	IFCAP_CSUM_UDPv6	0x0010	/* can do IPv6/UDP checksums */
#define	IFCAP_CSUM_TCPv4_Rx	0x0020	/* can do IPv4/TCP (Rx only) */
#define	IFCAP_CSUM_UDPv4_Rx	0x0040	/* can do IPv4/UDP (Rx only) */

/*
 * Output queues (ifp->if_snd) and internetwork datagram level (pup level 1)
 * input routines have queues of messages stored on ifqueue structures
 * (defined above).  Entries are added to and deleted from these structures
 * by these macros, which should be called with ipl raised to splimp().
 */
#define	IF_QFULL(ifq)		((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define	IF_DROP(ifq)		((ifq)->ifq_drops++)
#define	IF_ENQUEUE(ifq, m) { \
	(m)->m_nextpkt = 0; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_head = m; \
	else \
		(ifq)->ifq_tail->m_nextpkt = m; \
	(ifq)->ifq_tail = m; \
	(ifq)->ifq_len++; \
}
#define	IF_PREPEND(ifq, m) { \
	(m)->m_nextpkt = (ifq)->ifq_head; \
	if ((ifq)->ifq_tail == 0) \
		(ifq)->ifq_tail = (m); \
	(ifq)->ifq_head = (m); \
	(ifq)->ifq_len++; \
}
#define	IF_DEQUEUE(ifq, m) { \
	(m) = (ifq)->ifq_head; \
	if (m) { \
		if (((ifq)->ifq_head = (m)->m_nextpkt) == 0) \
			(ifq)->ifq_tail = 0; \
		(m)->m_nextpkt = 0; \
		(ifq)->ifq_len--; \
	} \
}
#define	IF_POLL(ifq, m)		((m) = (ifq)->ifq_head)
#define	IF_PURGE(ifq)							\
do {									\
	struct mbuf *__m0;						\
									\
	for (;;) {							\
		IF_DEQUEUE((ifq), __m0);				\
		if (__m0 == NULL)					\
			break;						\
		else							\
			m_freem(__m0);					\
	}								\
} while (0)
#define	IF_IS_EMPTY(ifq)	((ifq)->ifq_len == 0)

#define	IFQ_MAXLEN	50
#define	IFNET_SLOWHZ	1		/* granularity is 1 second */

/*
 * Structure defining statistics and other data kept regarding an address
 * on a network interface.
 */
struct ifaddr_data {
	int64_t	ifad_inbytes;
	int64_t	ifad_outbytes;
};

/*
 * The ifaddr structure contains information about one address
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an address is set, and are linked
 * together so all addresses for an interface can be located.
 */
struct ifaddr {
	struct	sockaddr *ifa_addr;	/* address of interface */
	struct	sockaddr *ifa_dstaddr;	/* other end of p-to-p link */
#define	ifa_broadaddr	ifa_dstaddr	/* broadcast address interface */
	struct	sockaddr *ifa_netmask;	/* used to determine subnet */
	struct	ifnet *ifa_ifp;		/* back-pointer to interface */
	TAILQ_ENTRY(ifaddr) ifa_list;	/* list of addresses for interface */
	struct	ifaddr_data	ifa_data;	/* statistics on the address */
	void	(*ifa_rtrequest)	/* check or clean routes (+ or -)'d */
		    __P((int, struct rtentry *, struct rt_addrinfo *));
	u_int	ifa_flags;		/* mostly rt_flags for cloning */
	int	ifa_refcnt;		/* count of references */
	int	ifa_metric;		/* cost of going out this interface */
};
#define	IFA_ROUTE	RTF_UP /* 0x01 *//* route installed */

/*
 * The prefix structure contains information about one prefix
 * of an interface.  They are maintained by the different address families,
 * are allocated and attached when an prefix or an address is set,
 * and are linked together so all prfefixes for an interface can be located.
 */
struct ifprefix {
	struct	sockaddr *ifpr_prefix;	/* prefix of interface */
	struct	ifnet *ifpr_ifp;	/* back-pointer to interface */
	struct ifprefix *ifpr_next;
	u_char	ifpr_plen;		/* prefix length in bits */
	u_char	ifpr_type;		/* protocol dependent prefix type */
};

/*
 * Message format for use in obtaining information about interfaces
 * from sysctl and the routing socket.
 */
struct if_msghdr {
	u_short	ifm_msglen;	/* to skip over non-understood messages */
	u_char	ifm_version;	/* future binary compatibility */
	u_char	ifm_type;	/* message type */
	int	ifm_addrs;	/* like rtm_addrs */
	int	ifm_flags;	/* value of if_flags */
	u_short	ifm_index;	/* index for associated ifp */
	struct	if_data ifm_data;/* statistics and other data about if */
};

#if defined(_KERNEL) && defined(COMPAT_14)
/* pre-1.5 if_msghdr (ifm_data changed) */
struct if_msghdr14 {
	u_short	ifm_msglen;	/* to skip over non-understood messages */
	u_char	ifm_version;	/* future binary compatibility */
	u_char	ifm_type;	/* message type */
	int	ifm_addrs;	/* like rtm_addrs */
	int	ifm_flags;	/* value of if_flags */
	u_short	ifm_index;	/* index for associated ifp */
	struct	if_data14 ifm_data; /* statistics and other data about if */
};
#endif /* _KERNEL && COMPAT_14 */

/*
 * Message format for use in obtaining information about interface addresses
 * from sysctl and the routing socket.
 */
struct ifa_msghdr {
	u_short	ifam_msglen;	/* to skip over non-understood messages */
	u_char	ifam_version;	/* future binary compatibility */
	u_char	ifam_type;	/* message type */
	int	ifam_addrs;	/* like rtm_addrs */
	int	ifam_flags;	/* value of ifa_flags */
	u_short	ifam_index;	/* index for associated ifp */
	int	ifam_metric;	/* value of ifa_metric */
};

/*
 * Message format announcing the arrival or departure of a network interface.
 */
struct if_announcemsghdr {
	u_short	ifan_msglen;	/* to skip over non-understood messages */
	u_char	ifan_version;	/* future binary compatibility */
	u_char	ifan_type;	/* message type */
	u_short	ifan_index;	/* index for associated ifp */
	char	ifan_name[IFNAMSIZ]; /* if name, e.g. "en0" */
	u_short	ifan_what;	/* what type of announcement */
};

#define	IFAN_ARRIVAL	0	/* interface arrival */
#define	IFAN_DEPARTURE	1	/* interface departure */

/*
 * Interface request structure used for socket
 * ioctl's.  All interface ioctl's must have parameter
 * definitions which begin with ifr_name.  The
 * remainder may be interface specific.
 */
struct	ifreq {
	char	ifr_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	union {
		struct	sockaddr ifru_addr;
		struct	sockaddr ifru_dstaddr;
		struct	sockaddr ifru_broadaddr;
		short	ifru_flags;
		int	ifru_metric;
		int	ifru_mtu;
		int	ifru_dlt;
		u_int	ifru_value;
		caddr_t	ifru_data;
	} ifr_ifru;
#define	ifr_addr	ifr_ifru.ifru_addr	/* address */
#define	ifr_dstaddr	ifr_ifru.ifru_dstaddr	/* other end of p-to-p link */
#define	ifr_broadaddr	ifr_ifru.ifru_broadaddr	/* broadcast address */
#define	ifr_flags	ifr_ifru.ifru_flags	/* flags */
#define	ifr_metric	ifr_ifru.ifru_metric	/* metric */
#define	ifr_mtu		ifr_ifru.ifru_mtu	/* mtu */
#define	ifr_dlt		ifr_ifru.ifru_dlt	/* data link type (DLT_*) */
#define	ifr_value	ifr_ifru.ifru_value	/* generic value */
#define	ifr_media	ifr_ifru.ifru_metric	/* media options (overload) */
#define	ifr_data	ifr_ifru.ifru_data	/* for use by interface */
};

struct ifcapreq {
	char		ifcr_name[IFNAMSIZ];	/* if name, e.g. "en0" */
	uint64_t	ifcr_capabilities;	/* supported capabiliites */
	uint64_t	ifcr_capenable;		/* capabilities enabled */
};

struct ifaliasreq {
	char	ifra_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	struct	sockaddr ifra_addr;
	struct	sockaddr ifra_dstaddr;
#define	ifra_broadaddr	ifra_dstaddr
	struct	sockaddr ifra_mask;
};

struct ifmediareq {
	char	ifm_name[IFNAMSIZ];		/* if name, e.g. "en0" */
	int	ifm_current;			/* current media options */
	int	ifm_mask;			/* don't care mask */
	int	ifm_status;			/* media status */
	int	ifm_active;			/* active options */
	int	ifm_count;			/* # entries in ifm_ulist
						   array */
	int	*ifm_ulist;			/* media words */
};

 
struct  ifdrv {
	char		ifd_name[IFNAMSIZ];	/* if name, e.g. "en0" */
	unsigned long	ifd_cmd;
	size_t		ifd_len;
	void		*ifd_data;
}; 

/*
 * Structure used in SIOCGIFCONF request.
 * Used to retrieve interface configuration
 * for machine (useful for programs which
 * must know all networks accessible).
 */
struct	ifconf {
	int	ifc_len;		/* size of associated buffer */
	union {
		caddr_t	ifcu_buf;
		struct	ifreq *ifcu_req;
	} ifc_ifcu;
#define	ifc_buf	ifc_ifcu.ifcu_buf	/* buffer address */
#define	ifc_req	ifc_ifcu.ifcu_req	/* array of structures returned */
};

/*
 * Structure for SIOC[AGD]LIFADDR
 */
struct if_laddrreq {
	char iflr_name[IFNAMSIZ];
	unsigned int flags;
#define IFLR_PREFIX	0x8000	/* in: prefix given  out: kernel fills id */
	unsigned int prefixlen;		/* in/out */
	struct sockaddr_storage addr;	/* in/out */
	struct sockaddr_storage dstaddr; /* out */
};

#include <net/if_arp.h>

#endif /* !_XOPEN_SOURCE */

#ifdef _KERNEL
#ifdef IFAREF_DEBUG
#define	IFAREF(ifa)							\
do {									\
	printf("IFAREF: %s:%d %p -> %d\n", __FILE__, __LINE__,		\
	    (ifa), ++(ifa)->ifa_refcnt);				\
} while (0)

#define	IFAFREE(ifa)							\
do {									\
	if ((ifa)->ifa_refcnt <= 0)					\
		panic("%s:%d: %p ifa_refcnt <= 0", __FILE__,		\
		    __LINE__, (ifa));					\
	printf("IFAFREE: %s:%d %p -> %d\n", __FILE__, __LINE__,		\
	    (ifa), --(ifa)->ifa_refcnt);				\
	if ((ifa)->ifa_refcnt == 0)					\
		ifafree(ifa);						\
} while (0)
#else
#define	IFAREF(ifa)	(ifa)->ifa_refcnt++

#ifdef DIAGNOSTIC
#define	IFAFREE(ifa)							\
do {									\
	if ((ifa)->ifa_refcnt <= 0)					\
		panic("%s:%d: %p ifa_refcnt <= 0", __FILE__,		\
		    __LINE__, (ifa));					\
	if (--(ifa)->ifa_refcnt == 0)					\
		ifafree(ifa);						\
} while (0)
#else
#define	IFAFREE(ifa)							\
do {									\
	if (--(ifa)->ifa_refcnt == 0)					\
		ifafree(ifa);						\
} while (0)
#endif /* DIAGNOSTIC */
#endif /* IFAREF_DEBUG */

#ifdef ALTQ
#define	ALTQ_DECL(x)		x

#define IFQ_ENQUEUE(ifq, m, pattr, err)					\
do {									\
	if (ALTQ_IS_ENABLED((ifq)))					\
		ALTQ_ENQUEUE((ifq), (m), (pattr), (err));		\
	else {								\
		if (IF_QFULL((ifq))) {					\
			m_freem((m));					\
			(err) = ENOBUFS;				\
		} else {						\
			IF_ENQUEUE((ifq), (m));				\
			(err) = 0;					\
		}							\
	}								\
	if ((err))							\
		(ifq)->ifq_drops++;					\
} while (0)

#define IFQ_DEQUEUE(ifq, m)						\
do {									\
	if (TBR_IS_ENABLED((ifq)))					\
		(m) = tbr_dequeue((ifq), ALTDQ_REMOVE);			\
	else if (ALTQ_IS_ENABLED((ifq)))				\
		ALTQ_DEQUEUE((ifq), (m));				\
	else								\
		IF_DEQUEUE((ifq), (m));					\
} while (0)

#define	IFQ_POLL(ifq, m)						\
do {									\
	if (TBR_IS_ENABLED((ifq)))					\
		(m) = tbr_dequeue((ifq), ALTDQ_POLL);			\
	else if (ALTQ_IS_ENABLED((ifq)))				\
		ALTQ_POLL((ifq), (m));					\
	else								\
		IF_POLL((ifq), (m));					\
} while (0)

#define	IFQ_PURGE(ifq)							\
do {									\
	if (ALTQ_IS_ENABLED((ifq)))					\
		ALTQ_PURGE((ifq));					\
	else								\
		IF_PURGE((ifq));					\
} while (0)

#define	IFQ_SET_READY(ifq)						\
do {									\
	(ifq)->altq_flags |= ALTQF_READY;				\
} while (0)

#define	IFQ_CLASSIFY(ifq, m, af, pa)					\
do {									\
	if (ALTQ_IS_ENABLED((ifq))) {					\
		if (ALTQ_NEEDS_CLASSIFY((ifq)))				\
			(pa)->pattr_class = (*(ifq)->altq_classify)	\
				((ifq)->altq_clfier, (m), (af));	\
		(pa)->pattr_af = (af);					\
		(pa)->pattr_hdr = mtod((m), caddr_t);			\
	}								\
} while (0)
#else /* ! ALTQ */
#define	ALTQ_DECL(x)		/* nothing */

#define	IFQ_ENQUEUE(ifq, m, pattr, err)					\
do {									\
	if (IF_QFULL((ifq))) {						\
		m_freem((m));						\
		(err) = ENOBUFS;					\
	} else {							\
		IF_ENQUEUE((ifq), (m));					\
		(err) = 0;						\
	}								\
	if ((err))							\
		(ifq)->ifq_drops++;					\
} while (0)

#define	IFQ_DEQUEUE(ifq, m)	IF_DEQUEUE((ifq), (m))

#define	IFQ_POLL(ifq, m)	IF_POLL((ifq), (m))

#define	IFQ_PURGE(ifq)		IF_PURGE((ifq))

#define	IFQ_SET_READY(ifq)	/* nothing */

#define	IFQ_CLASSIFY(ifq, m, af, pa) /* nothing */

#endif /* ALTQ */

#define	IFQ_IS_EMPTY(ifq)		IF_IS_EMPTY((ifq))
#define	IFQ_INC_LEN(ifq)		((ifq)->ifq_len++)
#define	IFQ_DEC_LEN(ifq)		(--(ifq)->ifq_len)
#define	IFQ_INC_DROPS(ifq)		((ifq)->ifq_drops++)
#define	IFQ_SET_MAXLEN(ifq, len)	((ifq)->ifq_maxlen = (len))

struct ifnet_head ifnet;
extern struct ifnet **ifindex2ifnet;
#if 0
struct ifnet loif[];
#endif
extern int if_index;

char	*ether_sprintf __P((const u_char *));

void	if_alloc_sadl __P((struct ifnet *));
void	if_free_sadl __P((struct ifnet *));
void	if_attach __P((struct ifnet *));
void	if_deactivate __P((struct ifnet *));
void	if_detach __P((struct ifnet *));
void	if_down __P((struct ifnet *));
void	if_slowtimo __P((void *));
void	if_up __P((struct ifnet *));
int	ifconf __P((u_long, caddr_t));
void	ifinit __P((void));
int	ifioctl __P((struct socket *, u_long, caddr_t, struct proc *));
int	ifpromisc __P((struct ifnet *, int));
struct	ifnet *ifunit __P((const char *));

struct	ifaddr *ifa_ifwithaddr __P((struct sockaddr *));
struct	ifaddr *ifa_ifwithaf __P((int));
struct	ifaddr *ifa_ifwithdstaddr __P((struct sockaddr *));
struct	ifaddr *ifa_ifwithnet __P((struct sockaddr *));
struct	ifaddr *ifa_ifwithladdr __P((struct sockaddr *));
struct	ifaddr *ifa_ifwithroute __P((int, struct sockaddr *,
					struct sockaddr *));
struct	ifaddr *ifaof_ifpforaddr __P((struct sockaddr *, struct ifnet *));
void	ifafree __P((struct ifaddr *));
void	link_rtrequest __P((int, struct rtentry *, struct rt_addrinfo *));

void	if_clone_attach __P((struct if_clone *));
void	if_clone_detach __P((struct if_clone *));

int	if_clone_create __P((const char *));
int	if_clone_destroy __P((const char *));

int	loioctl __P((struct ifnet *, u_long, caddr_t));
void	loopattach __P((int));
int	looutput __P((struct ifnet *,
	   struct mbuf *, struct sockaddr *, struct rtentry *));
void	lortrequest __P((int, struct rtentry *, struct rt_addrinfo *));

/*
 * These are exported because they're an easy way to tell if
 * an interface is going away without having to burn a flag.
 */
int	if_nulloutput __P((struct ifnet *, struct mbuf *,
	    struct sockaddr *, struct rtentry *));
void	if_nullinput __P((struct ifnet *, struct mbuf *));
void	if_nullstart __P((struct ifnet *));
int	if_nullioctl __P((struct ifnet *, u_long, caddr_t));
int	if_nullinit __P((struct ifnet *));
void	if_nullstop __P((struct ifnet *, int));
void	if_nullwatchdog __P((struct ifnet *));
void	if_nulldrain __P((struct ifnet *));
#else
struct if_nameindex {
	unsigned int	if_index;	/* 1, 2, ... */
	char		*if_name;	/* null terminated name: "le0", ... */
};

#include <sys/cdefs.h>
__BEGIN_DECLS
unsigned int if_nametoindex __P((const char *));
char *	if_indextoname __P((unsigned int, char *));
struct	if_nameindex * if_nameindex __P((void));
void	if_freenameindex __P((struct if_nameindex *));
__END_DECLS
#endif /* _KERNEL */
#endif /* !_NET_IF_H_ */
