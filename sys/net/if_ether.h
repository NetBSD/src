/*	$NetBSD: if_ether.h,v 1.2 1997/03/15 18:12:25 is Exp $	*/

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
 * Ethernet address - 6 octets
 * this is only used by the ethers(3) functions.
 */
struct ether_addr {
	u_int8_t ether_addr_octet[6];
};

/*
 * Structure of a 10Mb/s Ethernet header.
 */
#define	ETHER_ADDR_LEN	6

struct	ether_header {
	u_int8_t  ether_dhost[ETHER_ADDR_LEN];
	u_int8_t  ether_shost[ETHER_ADDR_LEN];
	u_int16_t ether_type;
};

#include <net/ethertypes.h>

#define	ETHER_IS_MULTICAST(addr) (*(addr) & 0x01) /* is address mcast/bcast? */

#define	ETHERMTU	1500
#define	ETHERMIN	(60-14)

#ifdef _KERNEL
/*
 * Macro to map an IP multicast address to an Ethernet multicast address.
 * The high-order 25 bits of the Ethernet address are statically assigned,
 * and the low-order 23 bits are taken from the low end of the IP address.
 */
#define ETHER_MAP_IP_MULTICAST(ipaddr, enaddr)				\
	/* struct in_addr *ipaddr; */					\
	/* u_int8_t enaddr[ETHER_ADDR_LEN]; */				\
{									\
	(enaddr)[0] = 0x01;						\
	(enaddr)[1] = 0x00;						\
	(enaddr)[2] = 0x5e;						\
	(enaddr)[3] = ((u_int8_t *)ipaddr)[1] & 0x7f;			\
	(enaddr)[4] = ((u_int8_t *)ipaddr)[2];				\
	(enaddr)[5] = ((u_int8_t *)ipaddr)[3];				\
}
#endif

/*
 * Structure shared between the ethernet driver modules and
 * the multicast list code.  For example, each ec_softc or il_softc
 * begins with this structure.
 */
struct	ethercom {
	struct	 ifnet ec_if;			/* network-visible interface */
	LIST_HEAD(, ether_multi) ec_multiaddrs;	/* list of ether multicast addrs */
	int	 ec_multicnt;			/* length of ac_multiaddrs list */
};

#ifdef	_KERNEL
u_int8_t etherbroadcastaddr[ETHER_ADDR_LEN];
u_int8_t ether_ipmulticast_min[ETHER_ADDR_LEN];
u_int8_t ether_ipmulticast_max[ETHER_ADDR_LEN];

int	ether_addmulti __P((struct ifreq *, struct ethercom *));
int	ether_delmulti __P((struct ifreq *, struct ethercom *));
#endif /* _KERNEL */

/*
 * Ethernet multicast address structure.  There is one of these for each
 * multicast address or range of multicast addresses that we are supposed
 * to listen to on a particular interface.  They are kept in a linked list,
 * rooted in the interface's ethercom structure.  (This really has nothing to
 * do with ARP, or with the Internet address family, but this appears to be
 * the minimally-disrupting place to put it.)
 */
struct ether_multi {
	u_int8_t enm_addrlo[ETHER_ADDR_LEN]; /* low  or only address of range */
	u_int8_t enm_addrhi[ETHER_ADDR_LEN]; /* high or only address of range */
	struct	 ethercom *enm_ec;	/* back pointer to ethercom */
	u_int	 enm_refcount;		/* no. claims to this addr/range */
	LIST_ENTRY(ether_multi) enm_list;
};

/*
 * Structure used by macros below to remember position when stepping through
 * all of the ether_multi records.
 */
struct ether_multistep {
	struct ether_multi  *e_enm;
};

/*
 * Macro for looking up the ether_multi record for a given range of Ethernet
 * multicast addresses connected to a given ethercom structure.  If no matching
 * record is found, "enm" returns NULL.
 */
#define ETHER_LOOKUP_MULTI(addrlo, addrhi, ec, enm)			\
	/* u_int8_t addrlo[ETHER_ADDR_LEN]; */				\
	/* u_int8_t addrhi[ETHER_ADDR_LEN]; */				\
	/* struct ethercom *ec; */					\
	/* struct ether_multi *enm; */					\
{									\
	for ((enm) = (ec)->ec_multiaddrs.lh_first;			\
	    (enm) != NULL &&						\
	    (bcmp((enm)->enm_addrlo, (addrlo), ETHER_ADDR_LEN) != 0 ||	\
	     bcmp((enm)->enm_addrhi, (addrhi), ETHER_ADDR_LEN) != 0);	\
		(enm) = (enm)->enm_list.le_next);			\
}

/*
 * Macro to step through all of the ether_multi records, one at a time.
 * The current position is remembered in "step", which the caller must
 * provide.  ETHER_FIRST_MULTI(), below, must be called to initialize "step"
 * and get the first record.  Both macros return a NULL "enm" when there
 * are no remaining records.
 */
#define ETHER_NEXT_MULTI(step, enm) \
	/* struct ether_multistep step; */  \
	/* struct ether_multi *enm; */  \
{ \
	if (((enm) = (step).e_enm) != NULL) \
		(step).e_enm = (enm)->enm_list.le_next; \
}

#define ETHER_FIRST_MULTI(step, ec, enm) \
	/* struct ether_multistep step; */ \
	/* struct ethercom *ec; */ \
	/* struct ether_multi *enm; */ \
{ \
	(step).e_enm = (ec)->ec_multiaddrs.lh_first; \
	ETHER_NEXT_MULTI((step), (enm)); \
}

/*
 * Prototype ethers(3) functions.
 */
#ifndef _KERNEL
#include <sys/cdefs.h>
__BEGIN_DECLS
char *	ether_ntoa __P((struct ether_addr *));
struct ether_addr *
	ether_aton __P((char *));
int	ether_ntohost __P((char *, struct ether_addr *));
int	ether_hostton __P((char *, struct ether_addr *));
int	ether_line __P((char *, struct ether_addr *, char *));
__END_DECLS
#endif
