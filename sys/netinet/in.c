/*	$NetBSD: in.c,v 1.148 2014/09/09 20:16:12 rmind Exp $	*/

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

/*-
 * Copyright (c) 1998 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Public Access Networks Corporation ("Panix").  It was developed under
 * contract to Panix by Eric Haszlakiewicz and Thor Lancelot Simon.
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

/*
 * Copyright (c) 1982, 1986, 1991, 1993
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
 *	@(#)in.c	8.4 (Berkeley) 1/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in.c,v 1.148 2014/09/09 20:16:12 rmind Exp $");

#include "opt_inet.h"
#include "opt_inet_conf.h"
#include "opt_mrouting.h"

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sysctl.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/syslog.h>
#include <sys/kauth.h>

#include <sys/cprng.h>

#include <net/if.h>
#include <net/route.h>
#include <net/pfil.h>

#include <net/if_ether.h>

#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <netinet/ip.h>
#include <netinet/ip_var.h>
#include <netinet/in_ifattach.h>
#include <netinet/in_pcb.h>
#include <netinet/if_inarp.h>
#include <netinet/ip_mroute.h>
#include <netinet/igmp_var.h>

#ifdef IPSELSRC
#include <netinet/in_selsrc.h>
#endif

static u_int	in_mask2len(struct in_addr *);
static void	in_len2mask(struct in_addr *, u_int);
static int	in_lifaddr_ioctl(struct socket *, u_long, void *,
	struct ifnet *);

static int	in_addprefix(struct in_ifaddr *, int);
static int	in_scrubprefix(struct in_ifaddr *);
static void	in_sysctl_init(struct sysctllog **);

#ifndef SUBNETSARELOCAL
#define	SUBNETSARELOCAL	1
#endif

#ifndef HOSTZEROBROADCAST
#define HOSTZEROBROADCAST 1
#endif

/* Note: 61, 127, 251, 509, 1021, 2039 are good. */
#ifndef IN_MULTI_HASH_SIZE
#define IN_MULTI_HASH_SIZE	509
#endif

static int			subnetsarelocal = SUBNETSARELOCAL;
static int			hostzeroisbroadcast = HOSTZEROBROADCAST;

/*
 * This list is used to keep track of in_multi chains which belong to
 * deleted interface addresses.  We use in_ifaddr so that a chain head
 * won't be deallocated until all multicast address record are deleted.
 */

LIST_HEAD(in_multihashhead, in_multi);		/* Type of the hash head */

static struct pool		inmulti_pool;
static u_int			in_multientries;
static struct in_multihashhead *in_multihashtbl;
static u_long			in_multihash;
static krwlock_t		in_multilock;

#define IN_MULTI_HASH(x, ifp) \
    (in_multihashtbl[(u_long)((x) ^ (ifp->if_index)) % IN_MULTI_HASH_SIZE])

struct in_ifaddrhashhead *	in_ifaddrhashtbl;
u_long				in_ifaddrhash;
struct in_ifaddrhead		in_ifaddrhead;

void
in_init(void)
{
	pool_init(&inmulti_pool, sizeof(struct in_multi), 0, 0, 0, "inmltpl",
	    NULL, IPL_SOFTNET);
	TAILQ_INIT(&in_ifaddrhead);

	in_ifaddrhashtbl = hashinit(IN_IFADDR_HASH_SIZE, HASH_LIST, true,
	    &in_ifaddrhash);
	in_multihashtbl = hashinit(IN_IFADDR_HASH_SIZE, HASH_LIST, true,
	    &in_multihash);
	rw_init(&in_multilock);

	in_sysctl_init(NULL);
}

/*
 * Return 1 if an internet address is for a ``local'' host
 * (one to which we have a connection).  If subnetsarelocal
 * is true, this includes other subnets of the local net.
 * Otherwise, it includes only the directly-connected (sub)nets.
 */
int
in_localaddr(struct in_addr in)
{
	struct in_ifaddr *ia;

	if (subnetsarelocal) {
		TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list)
			if ((in.s_addr & ia->ia_netmask) == ia->ia_net)
				return (1);
	} else {
		TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list)
			if ((in.s_addr & ia->ia_subnetmask) == ia->ia_subnet)
				return (1);
	}
	return (0);
}

/*
 * Determine whether an IP address is in a reserved set of addresses
 * that may not be forwarded, or whether datagrams to that destination
 * may be forwarded.
 */
int
in_canforward(struct in_addr in)
{
	u_int32_t net;

	if (IN_EXPERIMENTAL(in.s_addr) || IN_MULTICAST(in.s_addr))
		return (0);
	if (IN_CLASSA(in.s_addr)) {
		net = in.s_addr & IN_CLASSA_NET;
		if (net == 0 || net == htonl(IN_LOOPBACKNET << IN_CLASSA_NSHIFT))
			return (0);
	}
	return (1);
}

/*
 * Trim a mask in a sockaddr
 */
void
in_socktrim(struct sockaddr_in *ap)
{
	char *cplim = (char *) &ap->sin_addr;
	char *cp = (char *) (&ap->sin_addr + 1);

	ap->sin_len = 0;
	while (--cp >= cplim)
		if (*cp) {
			(ap)->sin_len = cp - (char *) (ap) + 1;
			break;
		}
}

/*
 *  Routine to take an Internet address and convert into a
 *  "dotted quad" representation for printing.
 */
const char *
in_fmtaddr(struct in_addr addr)
{
	static char buf[sizeof("123.456.789.123")];

	addr.s_addr = ntohl(addr.s_addr);

	snprintf(buf, sizeof(buf), "%d.%d.%d.%d",
		(addr.s_addr >> 24) & 0xFF,
		(addr.s_addr >> 16) & 0xFF,
		(addr.s_addr >>  8) & 0xFF,
		(addr.s_addr >>  0) & 0xFF);
	return buf;
}

/*
 * Maintain the "in_maxmtu" variable, which is the largest
 * mtu for non-local interfaces with AF_INET addresses assigned
 * to them that are up.
 */
unsigned long in_maxmtu;

void
in_setmaxmtu(void)
{
	struct in_ifaddr *ia;
	struct ifnet *ifp;
	unsigned long maxmtu = 0;

	TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
		if ((ifp = ia->ia_ifp) == 0)
			continue;
		if ((ifp->if_flags & (IFF_UP|IFF_LOOPBACK)) != IFF_UP)
			continue;
		if (ifp->if_mtu > maxmtu)
			maxmtu = ifp->if_mtu;
	}
	if (maxmtu)
		in_maxmtu = maxmtu;
}

static u_int
in_mask2len(struct in_addr *mask)
{
	u_int x, y;
	u_char *p;

	p = (u_char *)mask;
	for (x = 0; x < sizeof(*mask); x++) {
		if (p[x] != 0xff)
			break;
	}
	y = 0;
	if (x < sizeof(*mask)) {
		for (y = 0; y < NBBY; y++) {
			if ((p[x] & (0x80 >> y)) == 0)
				break;
		}
	}
	return x * NBBY + y;
}

static void
in_len2mask(struct in_addr *mask, u_int len)
{
	u_int i;
	u_char *p;

	p = (u_char *)mask;
	memset(mask, 0, sizeof(*mask));
	for (i = 0; i < len / NBBY; i++)
		p[i] = 0xff;
	if (len % NBBY)
		p[i] = (0xff00 >> (len % NBBY)) & 0xff;
}

/*
 * Generic internet control operations (ioctl's).
 * Ifp is 0 if not an interface-specific ioctl.
 */
/* ARGSUSED */
int
in_control(struct socket *so, u_long cmd, void *data, struct ifnet *ifp)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct in_ifaddr *ia = NULL;
	struct in_aliasreq *ifra = (struct in_aliasreq *)data;
	struct sockaddr_in oldaddr;
	int error, hostIsNew, maskIsNew;
	int newifaddr = 0;

	switch (cmd) {
	case SIOCALIFADDR:
	case SIOCDLIFADDR:
	case SIOCGLIFADDR:
		if (ifp == NULL)
			return EINVAL;
		return in_lifaddr_ioctl(so, cmd, data, ifp);
	case SIOCGIFADDRPREF:
	case SIOCSIFADDRPREF:
		if (ifp == NULL)
			return EINVAL;
		return ifaddrpref_ioctl(so, cmd, data, ifp);
	}

	/*
	 * Find address for this interface, if it exists.
	 */
	if (ifp != NULL)
		IFP_TO_IA(ifp, ia);

	switch (cmd) {
	case SIOCAIFADDR:
	case SIOCDIFADDR:
	case SIOCGIFALIAS:
		if (ifra->ifra_addr.sin_family == AF_INET)
			LIST_FOREACH(ia,
			    &IN_IFADDR_HASH(ifra->ifra_addr.sin_addr.s_addr),
			    ia_hash) {
				if (ia->ia_ifp == ifp &&
				    in_hosteq(ia->ia_addr.sin_addr,
				    ifra->ifra_addr.sin_addr))
					break;
			}
		if ((cmd == SIOCDIFADDR || cmd == SIOCGIFALIAS) && ia == NULL)
			return (EADDRNOTAVAIL);

		if (cmd == SIOCDIFADDR &&
		    ifra->ifra_addr.sin_family == AF_UNSPEC) {
			ifra->ifra_addr.sin_family = AF_INET;
		}
		/* FALLTHROUGH */
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
		if (ifra->ifra_addr.sin_family != AF_INET)
			return (EAFNOSUPPORT);
		/* FALLTHROUGH */
	case SIOCSIFNETMASK:
		if (ifp == NULL)
			panic("in_control");

		if (cmd == SIOCGIFALIAS)
			break;

		if (ia == NULL &&
		    (cmd == SIOCSIFNETMASK || cmd == SIOCSIFDSTADDR))
			return (EADDRNOTAVAIL);

		if (kauth_authorize_network(curlwp->l_cred, KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL) != 0)
			return (EPERM);

		if (ia == NULL) {
			ia = malloc(sizeof(*ia), M_IFADDR, M_WAITOK|M_ZERO);
			if (ia == NULL)
				return (ENOBUFS);
			TAILQ_INSERT_TAIL(&in_ifaddrhead, ia, ia_list);
			ifaref(&ia->ia_ifa);
			ifa_insert(ifp, &ia->ia_ifa);
			ia->ia_ifa.ifa_addr = sintosa(&ia->ia_addr);
			ia->ia_ifa.ifa_dstaddr = sintosa(&ia->ia_dstaddr);
			ia->ia_ifa.ifa_netmask = sintosa(&ia->ia_sockmask);
#ifdef IPSELSRC
			ia->ia_ifa.ifa_getifa = in_getifa;
#else /* IPSELSRC */
			ia->ia_ifa.ifa_getifa = NULL;
#endif /* IPSELSRC */
			ia->ia_sockmask.sin_len = 8;
			if (ifp->if_flags & IFF_BROADCAST) {
				ia->ia_broadaddr.sin_len = sizeof(ia->ia_addr);
				ia->ia_broadaddr.sin_family = AF_INET;
			}
			ia->ia_ifp = ifp;
			ia->ia_idsalt = cprng_fast32() % 65535;
			LIST_INIT(&ia->ia_multiaddrs);
			newifaddr = 1;
		}
		break;

	case SIOCSIFBRDADDR:
		if (kauth_authorize_network(curlwp->l_cred, KAUTH_NETWORK_INTERFACE,
		    KAUTH_REQ_NETWORK_INTERFACE_SETPRIV, ifp, (void *)cmd,
		    NULL) != 0)
			return (EPERM);
		/* FALLTHROUGH */

	case SIOCGIFADDR:
	case SIOCGIFNETMASK:
	case SIOCGIFDSTADDR:
	case SIOCGIFBRDADDR:
		if (ia == NULL)
			return (EADDRNOTAVAIL);
		break;
	}
	error = 0;
	switch (cmd) {

	case SIOCGIFADDR:
		ifreq_setaddr(cmd, ifr, sintocsa(&ia->ia_addr));
		break;

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		ifreq_setdstaddr(cmd, ifr, sintocsa(&ia->ia_broadaddr));
		break;

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		ifreq_setdstaddr(cmd, ifr, sintocsa(&ia->ia_dstaddr));
		break;

	case SIOCGIFNETMASK:
		ifreq_setaddr(cmd, ifr, sintocsa(&ia->ia_sockmask));
		break;

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		oldaddr = ia->ia_dstaddr;
		ia->ia_dstaddr = *satocsin(ifreq_getdstaddr(cmd, ifr));
		if ((error = if_addr_init(ifp, &ia->ia_ifa, false)) != 0) {
			ia->ia_dstaddr = oldaddr;
			return error;
		}
		if (ia->ia_flags & IFA_ROUTE) {
			ia->ia_ifa.ifa_dstaddr = sintosa(&oldaddr);
			rtinit(&ia->ia_ifa, RTM_DELETE, RTF_HOST);
			ia->ia_ifa.ifa_dstaddr = sintosa(&ia->ia_dstaddr);
			rtinit(&ia->ia_ifa, RTM_ADD, RTF_HOST|RTF_UP);
		}
		break;

	case SIOCSIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return EINVAL;
		ia->ia_broadaddr = *satocsin(ifreq_getbroadaddr(cmd, ifr));
		break;

	case SIOCSIFADDR:
		error = in_ifinit(ifp, ia, satocsin(ifreq_getaddr(cmd, ifr)),
		    1);
		if (error == 0) {
			(void)pfil_run_hooks(if_pfil,
			    (struct mbuf **)SIOCSIFADDR, ifp, PFIL_IFADDR);
		}
		break;

	case SIOCSIFNETMASK:
		in_ifscrub(ifp, ia);
		ia->ia_sockmask = *satocsin(ifreq_getaddr(cmd, ifr));
		ia->ia_subnetmask = ia->ia_sockmask.sin_addr.s_addr;
		error = in_ifinit(ifp, ia, NULL, 0);
		break;

	case SIOCAIFADDR:
		maskIsNew = 0;
		hostIsNew = 1;
		if (ia->ia_addr.sin_family != AF_INET)
			;
		else if (ifra->ifra_addr.sin_len == 0) {
			ifra->ifra_addr = ia->ia_addr;
			hostIsNew = 0;
		} else if (in_hosteq(ia->ia_addr.sin_addr,
		           ifra->ifra_addr.sin_addr))
			hostIsNew = 0;
		if (ifra->ifra_mask.sin_len) {
			/* Only scrub if we control the prefix route,
			 * otherwise userland gets a bogus message */
			if ((ia->ia_flags & IFA_ROUTE))
				in_ifscrub(ifp, ia);
			ia->ia_sockmask = ifra->ifra_mask;
			ia->ia_subnetmask = ia->ia_sockmask.sin_addr.s_addr;
			maskIsNew = 1;
		}
		if ((ifp->if_flags & IFF_POINTOPOINT) &&
		    (ifra->ifra_dstaddr.sin_family == AF_INET)) {
			/* Only scrub if we control the prefix route,
			 * otherwise userland gets a bogus message */
			if ((ia->ia_flags & IFA_ROUTE))
				in_ifscrub(ifp, ia);
			ia->ia_dstaddr = ifra->ifra_dstaddr;
			maskIsNew  = 1; /* We lie; but the effect's the same */
		}
		if (ifra->ifra_addr.sin_family == AF_INET &&
		    (hostIsNew || maskIsNew)) {
			error = in_ifinit(ifp, ia, &ifra->ifra_addr, 0);
		}
		if ((ifp->if_flags & IFF_BROADCAST) &&
		    (ifra->ifra_broadaddr.sin_family == AF_INET))
			ia->ia_broadaddr = ifra->ifra_broadaddr;
		if (error == 0)
			(void)pfil_run_hooks(if_pfil,
			    (struct mbuf **)SIOCAIFADDR, ifp, PFIL_IFADDR);
		break;

	case SIOCGIFALIAS:
		ifra->ifra_mask = ia->ia_sockmask;
		if ((ifp->if_flags & IFF_POINTOPOINT) &&
		    (ia->ia_dstaddr.sin_family == AF_INET))
			ifra->ifra_dstaddr = ia->ia_dstaddr;
		else if ((ifp->if_flags & IFF_BROADCAST) &&
		    (ia->ia_broadaddr.sin_family == AF_INET))
			ifra->ifra_broadaddr = ia->ia_broadaddr;
		else
			memset(&ifra->ifra_broadaddr, 0,
			      sizeof(ifra->ifra_broadaddr));
		break;

	case SIOCDIFADDR:
		in_purgeaddr(&ia->ia_ifa);
		(void)pfil_run_hooks(if_pfil, (struct mbuf **)SIOCDIFADDR,
		    ifp, PFIL_IFADDR);
		break;

#ifdef MROUTING
	case SIOCGETVIFCNT:
	case SIOCGETSGCNT:
		error = mrt_ioctl(so, cmd, data);
		break;
#endif /* MROUTING */

	default:
		return ENOTTY;
	}

	if (error != 0 && newifaddr) {
		KASSERT(ia != NULL);
		in_purgeaddr(&ia->ia_ifa);
	}

	return error;
}

void
in_purgeaddr(struct ifaddr *ifa)
{
	struct ifnet *ifp = ifa->ifa_ifp;
	struct in_ifaddr *ia = (void *) ifa;

	in_ifscrub(ifp, ia);
	LIST_REMOVE(ia, ia_hash);
	ifa_remove(ifp, &ia->ia_ifa);
	TAILQ_REMOVE(&in_ifaddrhead, ia, ia_list);
	if (ia->ia_allhosts != NULL)
		in_delmulti(ia->ia_allhosts);
	ifafree(&ia->ia_ifa);
	in_setmaxmtu();
}

void
in_purgeif(struct ifnet *ifp)		/* MUST be called at splsoftnet() */
{
	if_purgeaddrs(ifp, AF_INET, in_purgeaddr);
	igmp_purgeif(ifp);		/* manipulates pools */
#ifdef MROUTING
	ip_mrouter_detach(ifp);
#endif
}

/*
 * SIOC[GAD]LIFADDR.
 *	SIOCGLIFADDR: get first address. (???)
 *	SIOCGLIFADDR with IFLR_PREFIX:
 *		get first address that matches the specified prefix.
 *	SIOCALIFADDR: add the specified address.
 *	SIOCALIFADDR with IFLR_PREFIX:
 *		EINVAL since we can't deduce hostid part of the address.
 *	SIOCDLIFADDR: delete the specified address.
 *	SIOCDLIFADDR with IFLR_PREFIX:
 *		delete the first address that matches the specified prefix.
 * return values:
 *	EINVAL on invalid parameters
 *	EADDRNOTAVAIL on prefix match failed/specified address not found
 *	other values may be returned from in_ioctl()
 */
static int
in_lifaddr_ioctl(struct socket *so, u_long cmd, void *data,
    struct ifnet *ifp)
{
	struct if_laddrreq *iflr = (struct if_laddrreq *)data;
	struct ifaddr *ifa;
	struct sockaddr *sa;

	/* sanity checks */
	if (data == NULL || ifp == NULL) {
		panic("invalid argument to in_lifaddr_ioctl");
		/*NOTRECHED*/
	}

	switch (cmd) {
	case SIOCGLIFADDR:
		/* address must be specified on GET with IFLR_PREFIX */
		if ((iflr->flags & IFLR_PREFIX) == 0)
			break;
		/*FALLTHROUGH*/
	case SIOCALIFADDR:
	case SIOCDLIFADDR:
		/* address must be specified on ADD and DELETE */
		sa = (struct sockaddr *)&iflr->addr;
		if (sa->sa_family != AF_INET)
			return EINVAL;
		if (sa->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		/* XXX need improvement */
		sa = (struct sockaddr *)&iflr->dstaddr;
		if (sa->sa_family != AF_UNSPEC && sa->sa_family != AF_INET)
			return EINVAL;
		if (sa->sa_len != 0 && sa->sa_len != sizeof(struct sockaddr_in))
			return EINVAL;
		break;
	default: /*shouldn't happen*/
#if 0
		panic("invalid cmd to in_lifaddr_ioctl");
		/*NOTREACHED*/
#else
		return EOPNOTSUPP;
#endif
	}
	if (sizeof(struct in_addr) * NBBY < iflr->prefixlen)
		return EINVAL;

	switch (cmd) {
	case SIOCALIFADDR:
	    {
		struct in_aliasreq ifra;

		if (iflr->flags & IFLR_PREFIX)
			return EINVAL;

		/* copy args to in_aliasreq, perform ioctl(SIOCAIFADDR). */
		memset(&ifra, 0, sizeof(ifra));
		memcpy(ifra.ifra_name, iflr->iflr_name,
			sizeof(ifra.ifra_name));

		memcpy(&ifra.ifra_addr, &iflr->addr,
			((struct sockaddr *)&iflr->addr)->sa_len);

		if (((struct sockaddr *)&iflr->dstaddr)->sa_family) {	/*XXX*/
			memcpy(&ifra.ifra_dstaddr, &iflr->dstaddr,
				((struct sockaddr *)&iflr->dstaddr)->sa_len);
		}

		ifra.ifra_mask.sin_family = AF_INET;
		ifra.ifra_mask.sin_len = sizeof(struct sockaddr_in);
		in_len2mask(&ifra.ifra_mask.sin_addr, iflr->prefixlen);

		return in_control(so, SIOCAIFADDR, &ifra, ifp);
	    }
	case SIOCGLIFADDR:
	case SIOCDLIFADDR:
	    {
		struct in_ifaddr *ia;
		struct in_addr mask, candidate, match;
		struct sockaddr_in *sin;
		int cmp;

		memset(&mask, 0, sizeof(mask));
		memset(&match, 0, sizeof(match));	/* XXX gcc */
		if (iflr->flags & IFLR_PREFIX) {
			/* lookup a prefix rather than address. */
			in_len2mask(&mask, iflr->prefixlen);

			sin = (struct sockaddr_in *)&iflr->addr;
			match.s_addr = sin->sin_addr.s_addr;
			match.s_addr &= mask.s_addr;

			/* if you set extra bits, that's wrong */
			if (match.s_addr != sin->sin_addr.s_addr)
				return EINVAL;

			cmp = 1;
		} else {
			if (cmd == SIOCGLIFADDR) {
				/* on getting an address, take the 1st match */
				cmp = 0;	/*XXX*/
			} else {
				/* on deleting an address, do exact match */
				in_len2mask(&mask, 32);
				sin = (struct sockaddr_in *)&iflr->addr;
				match.s_addr = sin->sin_addr.s_addr;

				cmp = 1;
			}
		}

		IFADDR_FOREACH(ifa, ifp) {
			if (ifa->ifa_addr->sa_family != AF_INET)
				continue;
			if (cmp == 0)
				break;
			candidate.s_addr = ((struct sockaddr_in *)ifa->ifa_addr)->sin_addr.s_addr;
			candidate.s_addr &= mask.s_addr;
			if (candidate.s_addr == match.s_addr)
				break;
		}
		if (ifa == NULL)
			return EADDRNOTAVAIL;
		ia = (struct in_ifaddr *)ifa;

		if (cmd == SIOCGLIFADDR) {
			/* fill in the if_laddrreq structure */
			memcpy(&iflr->addr, &ia->ia_addr, ia->ia_addr.sin_len);

			if ((ifp->if_flags & IFF_POINTOPOINT) != 0) {
				memcpy(&iflr->dstaddr, &ia->ia_dstaddr,
					ia->ia_dstaddr.sin_len);
			} else
				memset(&iflr->dstaddr, 0, sizeof(iflr->dstaddr));

			iflr->prefixlen =
				in_mask2len(&ia->ia_sockmask.sin_addr);

			iflr->flags = 0;	/*XXX*/

			return 0;
		} else {
			struct in_aliasreq ifra;

			/* fill in_aliasreq and do ioctl(SIOCDIFADDR) */
			memset(&ifra, 0, sizeof(ifra));
			memcpy(ifra.ifra_name, iflr->iflr_name,
				sizeof(ifra.ifra_name));

			memcpy(&ifra.ifra_addr, &ia->ia_addr,
				ia->ia_addr.sin_len);
			if ((ifp->if_flags & IFF_POINTOPOINT) != 0) {
				memcpy(&ifra.ifra_dstaddr, &ia->ia_dstaddr,
					ia->ia_dstaddr.sin_len);
			}
			memcpy(&ifra.ifra_dstaddr, &ia->ia_sockmask,
				ia->ia_sockmask.sin_len);

			return in_control(so, SIOCDIFADDR, &ifra, ifp);
		}
	    }
	}

	return EOPNOTSUPP;	/*just for safety*/
}

/*
 * Delete any existing route for an interface.
 */
void
in_ifscrub(struct ifnet *ifp, struct in_ifaddr *ia)
{

	in_scrubprefix(ia);
}

/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
int
in_ifinit(struct ifnet *ifp, struct in_ifaddr *ia,
    const struct sockaddr_in *sin, int scrub)
{
	u_int32_t i;
	struct sockaddr_in oldaddr;
	int s = splnet(), flags = RTF_UP, error;

	if (sin == NULL)
		sin = &ia->ia_addr;

	/*
	 * Set up new addresses.
	 */
	oldaddr = ia->ia_addr;
	if (ia->ia_addr.sin_family == AF_INET)
		LIST_REMOVE(ia, ia_hash);
	ia->ia_addr = *sin;
	LIST_INSERT_HEAD(&IN_IFADDR_HASH(ia->ia_addr.sin_addr.s_addr), ia, ia_hash);

	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	if ((error = if_addr_init(ifp, &ia->ia_ifa, true)) != 0)
		goto bad;
	splx(s);
	if (scrub) {
		ia->ia_ifa.ifa_addr = sintosa(&oldaddr);
		in_ifscrub(ifp, ia);
		ia->ia_ifa.ifa_addr = sintosa(&ia->ia_addr);
	}

	i = ia->ia_addr.sin_addr.s_addr;
	if (IN_CLASSA(i))
		ia->ia_netmask = IN_CLASSA_NET;
	else if (IN_CLASSB(i))
		ia->ia_netmask = IN_CLASSB_NET;
	else
		ia->ia_netmask = IN_CLASSC_NET;
	/*
	 * The subnet mask usually includes at least the standard network part,
	 * but may may be smaller in the case of supernetting.
	 * If it is set, we believe it.
	 */
	if (ia->ia_subnetmask == 0) {
		ia->ia_subnetmask = ia->ia_netmask;
		ia->ia_sockmask.sin_addr.s_addr = ia->ia_subnetmask;
	} else
		ia->ia_netmask &= ia->ia_subnetmask;

	ia->ia_net = i & ia->ia_netmask;
	ia->ia_subnet = i & ia->ia_subnetmask;
	in_socktrim(&ia->ia_sockmask);
	/* re-calculate the "in_maxmtu" value */
	in_setmaxmtu();
	/*
	 * Add route for the network.
	 */
	ia->ia_ifa.ifa_metric = ifp->if_metric;
	if (ifp->if_flags & IFF_BROADCAST) {
		ia->ia_broadaddr.sin_addr.s_addr =
			ia->ia_subnet | ~ia->ia_subnetmask;
		ia->ia_netbroadcast.s_addr =
			ia->ia_net | ~ia->ia_netmask;
	} else if (ifp->if_flags & IFF_LOOPBACK) {
		ia->ia_dstaddr = ia->ia_addr;
		flags |= RTF_HOST;
	} else if (ifp->if_flags & IFF_POINTOPOINT) {
		if (ia->ia_dstaddr.sin_family != AF_INET)
			return (0);
		flags |= RTF_HOST;
	}
	error = in_addprefix(ia, flags);
	/*
	 * If the interface supports multicast, join the "all hosts"
	 * multicast group on that interface.
	 */
	if ((ifp->if_flags & IFF_MULTICAST) != 0 && ia->ia_allhosts == NULL) {
		struct in_addr addr;

		addr.s_addr = INADDR_ALLHOSTS_GROUP;
		ia->ia_allhosts = in_addmulti(&addr, ifp);
	}
	return (error);
bad:
	splx(s);
	LIST_REMOVE(ia, ia_hash);
	ia->ia_addr = oldaddr;
	if (ia->ia_addr.sin_family == AF_INET)
		LIST_INSERT_HEAD(&IN_IFADDR_HASH(ia->ia_addr.sin_addr.s_addr),
		    ia, ia_hash);
	return (error);
}

#define rtinitflags(x) \
	((((x)->ia_ifp->if_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) != 0) \
	    ? RTF_HOST : 0)

/*
 * add a route to prefix ("connected route" in cisco terminology).
 * does nothing if there's some interface address with the same prefix already.
 */
static int
in_addprefix(struct in_ifaddr *target, int flags)
{
	struct in_ifaddr *ia;
	struct in_addr prefix, mask, p;
	int error;

	if ((flags & RTF_HOST) != 0)
		prefix = target->ia_dstaddr.sin_addr;
	else {
		prefix = target->ia_addr.sin_addr;
		mask = target->ia_sockmask.sin_addr;
		prefix.s_addr &= mask.s_addr;
	}

	TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
		if (rtinitflags(ia))
			p = ia->ia_dstaddr.sin_addr;
		else {
			p = ia->ia_addr.sin_addr;
			p.s_addr &= ia->ia_sockmask.sin_addr.s_addr;
		}

		if (prefix.s_addr != p.s_addr)
			continue;

		/*
		 * if we got a matching prefix route inserted by other
		 * interface address, we don't need to bother
		 *
		 * XXX RADIX_MPATH implications here? -dyoung
		 *
		 * But we should still notify userland of the new address
		 */
		if (ia->ia_flags & IFA_ROUTE) {
			rt_newaddrmsg(RTM_NEWADDR, &target->ia_ifa, 0, NULL);
			return 0;
		}
	}

	/*
	 * noone seem to have prefix route.  insert it.
	 */
	error = rtinit(&target->ia_ifa, RTM_ADD, flags);
	if (error == 0)
		target->ia_flags |= IFA_ROUTE;
	else if (error == EEXIST) {
		/* 
		 * the fact the route already exists is not an error.
		 */ 
		error = 0;
	}
	return error;
}

/*
 * remove a route to prefix ("connected route" in cisco terminology).
 * re-installs the route by using another interface address, if there's one
 * with the same prefix (otherwise we lose the route mistakenly).
 */
static int
in_scrubprefix(struct in_ifaddr *target)
{
	struct in_ifaddr *ia;
	struct in_addr prefix, mask, p;
	int error;

	/* If we don't have IFA_ROUTE we should still inform userland */
	if ((target->ia_flags & IFA_ROUTE) == 0) {
		rt_newaddrmsg(RTM_DELADDR, &target->ia_ifa, 0, NULL);
		return 0;
	}

	if (rtinitflags(target))
		prefix = target->ia_dstaddr.sin_addr;
	else {
		prefix = target->ia_addr.sin_addr;
		mask = target->ia_sockmask.sin_addr;
		prefix.s_addr &= mask.s_addr;
	}

	TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
		if (rtinitflags(ia))
			p = ia->ia_dstaddr.sin_addr;
		else {
			p = ia->ia_addr.sin_addr;
			p.s_addr &= ia->ia_sockmask.sin_addr.s_addr;
		}

		if (prefix.s_addr != p.s_addr)
			continue;

		/*
		 * if we got a matching prefix route, move IFA_ROUTE to him
		 */
		if ((ia->ia_flags & IFA_ROUTE) == 0) {
			rtinit(&target->ia_ifa, RTM_DELETE,
			    rtinitflags(target));
			target->ia_flags &= ~IFA_ROUTE;

			error = rtinit(&ia->ia_ifa, RTM_ADD,
			    rtinitflags(ia) | RTF_UP);
			if (error == 0)
				ia->ia_flags |= IFA_ROUTE;
			return error;
		}
	}

	/*
	 * noone seem to have prefix route.  remove it.
	 */
	rtinit(&target->ia_ifa, RTM_DELETE, rtinitflags(target));
	target->ia_flags &= ~IFA_ROUTE;
	return 0;
}

#undef rtinitflags

/*
 * Return 1 if the address might be a local broadcast address.
 */
int
in_broadcast(struct in_addr in, struct ifnet *ifp)
{
	struct ifaddr *ifa;

	if (in.s_addr == INADDR_BROADCAST ||
	    in_nullhost(in))
		return 1;
	if ((ifp->if_flags & IFF_BROADCAST) == 0)
		return 0;
	/*
	 * Look through the list of addresses for a match
	 * with a broadcast address.
	 */
#define ia (ifatoia(ifa))
	IFADDR_FOREACH(ifa, ifp)
		if (ifa->ifa_addr->sa_family == AF_INET &&
		    !in_hosteq(in, ia->ia_addr.sin_addr) &&
		    (in_hosteq(in, ia->ia_broadaddr.sin_addr) ||
		     in_hosteq(in, ia->ia_netbroadcast) ||
		     (hostzeroisbroadcast &&
		      /*
		       * Check for old-style (host 0) broadcast.
		       */
		      (in.s_addr == ia->ia_subnet ||
		       in.s_addr == ia->ia_net))))
			return 1;
	return (0);
#undef ia
}

/*
 * in_lookup_multi: look up the in_multi record for a given IP
 * multicast address on a given interface.  If no matching record is
 * found, return NULL.
 */
struct in_multi *
in_lookup_multi(struct in_addr addr, ifnet_t *ifp)
{
	struct in_multi *inm;

	KASSERT(rw_lock_held(&in_multilock));

	LIST_FOREACH(inm, &IN_MULTI_HASH(addr.s_addr, ifp), inm_list) {
		if (in_hosteq(inm->inm_addr, addr) && inm->inm_ifp == ifp)
			break;
	}
	return inm;
}

/*
 * in_multi_group: check whether the address belongs to an IP multicast
 * group we are joined on this interface.  Returns true or false.
 */
bool
in_multi_group(struct in_addr addr, ifnet_t *ifp, int flags)
{
	bool ingroup;

	if (__predict_true(flags & IP_IGMP_MCAST) == 0) {
		rw_enter(&in_multilock, RW_READER);
		ingroup = in_lookup_multi(addr, ifp) != NULL;
		rw_exit(&in_multilock);
	} else {
		/* XXX Recursive call from ip_output(). */
		KASSERT(rw_lock_held(&in_multilock));
		ingroup = in_lookup_multi(addr, ifp) != NULL;
	}
	return ingroup;
}

/*
 * Add an address to the list of IP multicast addresses for a given interface.
 */
struct in_multi *
in_addmulti(struct in_addr *ap, ifnet_t *ifp)
{
	struct sockaddr_in sin;
	struct in_multi *inm;

	/*
	 * See if address already in list.
	 */
	rw_enter(&in_multilock, RW_WRITER);
	inm = in_lookup_multi(*ap, ifp);
	if (inm != NULL) {
		/*
		 * Found it; just increment the reference count.
		 */
		inm->inm_refcount++;
		rw_exit(&in_multilock);
		return inm;
	}

	/*
	 * New address; allocate a new multicast record.
	 */
	inm = pool_get(&inmulti_pool, PR_NOWAIT);
	if (inm == NULL) {
		rw_exit(&in_multilock);
		return NULL;
	}
	inm->inm_addr = *ap;
	inm->inm_ifp = ifp;
	inm->inm_refcount = 1;

	/*
	 * Ask the network driver to update its multicast reception
	 * filter appropriately for the new address.
	 */
	sockaddr_in_init(&sin, ap, 0);
	if (if_mcast_op(ifp, SIOCADDMULTI, sintosa(&sin)) != 0) {
		rw_exit(&in_multilock);
		pool_put(&inmulti_pool, inm);
		return NULL;
	}

	/*
	 * Let IGMP know that we have joined a new IP multicast group.
	 */
	if (igmp_joingroup(inm) != 0) {
		rw_exit(&in_multilock);
		pool_put(&inmulti_pool, inm);
		return NULL;
	}
	LIST_INSERT_HEAD(
	    &IN_MULTI_HASH(inm->inm_addr.s_addr, ifp),
	    inm, inm_list);
	in_multientries++;
	rw_exit(&in_multilock);

	return inm;
}

/*
 * Delete a multicast address record.
 */
void
in_delmulti(struct in_multi *inm)
{
	struct sockaddr_in sin;

	rw_enter(&in_multilock, RW_WRITER);
	if (--inm->inm_refcount > 0) {
		rw_exit(&in_multilock);
		return;
	}

	/*
	 * No remaining claims to this record; let IGMP know that
	 * we are leaving the multicast group.
	 */
	igmp_leavegroup(inm);

	/*
	 * Notify the network driver to update its multicast reception
	 * filter.
	 */
	sockaddr_in_init(&sin, &inm->inm_addr, 0);
	if_mcast_op(inm->inm_ifp, SIOCDELMULTI, sintosa(&sin));

	/*
	 * Unlink from list.
	 */
	LIST_REMOVE(inm, inm_list);
	in_multientries--;
	rw_exit(&in_multilock);

	pool_put(&inmulti_pool, inm);
}

/*
 * in_next_multi: step through all of the in_multi records, one at a time.
 * The current position is remembered in "step", which the caller must
 * provide.  in_first_multi(), below, must be called to initialize "step"
 * and get the first record.  Both macros return a NULL "inm" when there
 * are no remaining records.
 */
struct in_multi *
in_next_multi(struct in_multistep *step)
{
	struct in_multi *inm;

	KASSERT(rw_lock_held(&in_multilock));

	while (step->i_inm == NULL && step->i_n < IN_MULTI_HASH_SIZE) {
		step->i_inm = LIST_FIRST(&in_multihashtbl[++step->i_n]);
	}
	if ((inm = step->i_inm) != NULL) {
		step->i_inm = LIST_NEXT(inm, inm_list);
	}
	return inm;
}

struct in_multi *
in_first_multi(struct in_multistep *step)
{
	KASSERT(rw_lock_held(&in_multilock));

	step->i_n = 0;
	step->i_inm = LIST_FIRST(&in_multihashtbl[0]);
	return in_next_multi(step);
}

void
in_multi_lock(int op)
{
	rw_enter(&in_multilock, op);
}

void
in_multi_unlock(void)
{
	rw_exit(&in_multilock);
}

int
in_multi_lock_held(void)
{
	return rw_lock_held(&in_multilock);
}

struct sockaddr_in *
in_selectsrc(struct sockaddr_in *sin, struct route *ro,
    int soopts, struct ip_moptions *mopts, int *errorp)
{
	struct rtentry *rt = NULL;
	struct in_ifaddr *ia = NULL;

	/*
         * If route is known or can be allocated now, take the
         * source address from the interface.  Otherwise, punt.
	 */
	if ((soopts & SO_DONTROUTE) != 0)
		rtcache_free(ro);
	else {
		union {
			struct sockaddr		dst;
			struct sockaddr_in	dst4;
		} u;

		sockaddr_in_init(&u.dst4, &sin->sin_addr, 0);
		rt = rtcache_lookup(ro, &u.dst);
	}
	/*
	 * If we found a route, use the address
	 * corresponding to the outgoing interface
	 * unless it is the loopback (in case a route
	 * to our address on another net goes to loopback).
	 *
	 * XXX Is this still true?  Do we care?
	 */
	if (rt != NULL && (rt->rt_ifp->if_flags & IFF_LOOPBACK) == 0)
		ia = ifatoia(rt->rt_ifa);
	if (ia == NULL) {
		u_int16_t fport = sin->sin_port;

		sin->sin_port = 0;
		ia = ifatoia(ifa_ifwithladdr(sintosa(sin)));
		sin->sin_port = fport;
		if (ia == NULL) {
			/* Find 1st non-loopback AF_INET address */
			TAILQ_FOREACH(ia, &in_ifaddrhead, ia_list) {
				if (!(ia->ia_ifp->if_flags & IFF_LOOPBACK))
					break;
			}
		}
		if (ia == NULL) {
			*errorp = EADDRNOTAVAIL;
			return NULL;
		}
	}
	/*
	 * If the destination address is multicast and an outgoing
	 * interface has been set as a multicast option, use the
	 * address of that interface as our source address.
	 */
	if (IN_MULTICAST(sin->sin_addr.s_addr) && mopts != NULL) {
		struct ip_moptions *imo;
		struct ifnet *ifp;

		imo = mopts;
		if (imo->imo_multicast_ifp != NULL) {
			ifp = imo->imo_multicast_ifp;
			IFP_TO_IA(ifp, ia);		/* XXX */
			if (ia == 0) {
				*errorp = EADDRNOTAVAIL;
				return NULL;
			}
		}
	}
	if (ia->ia_ifa.ifa_getifa != NULL) {
		ia = ifatoia((*ia->ia_ifa.ifa_getifa)(&ia->ia_ifa,
		                                      sintosa(sin)));
	}
#ifdef GETIFA_DEBUG
	else
		printf("%s: missing ifa_getifa\n", __func__);
#endif
	return satosin(&ia->ia_addr);
}

static void
in_sysctl_init(struct sysctllog **clog)
{
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "inet",
		       SYSCTL_DESCR("PF_INET related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT,
		       CTLTYPE_NODE, "ip",
		       SYSCTL_DESCR("IPv4 related settings"),
		       NULL, 0, NULL, 0,
		       CTL_NET, PF_INET, IPPROTO_IP, CTL_EOL);

	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "subnetsarelocal",
		       SYSCTL_DESCR("Whether logical subnets are considered "
				    "local"),
		       NULL, 0, &subnetsarelocal, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_SUBNETSARELOCAL, CTL_EOL);
	sysctl_createv(clog, 0, NULL, NULL,
		       CTLFLAG_PERMANENT|CTLFLAG_READWRITE,
		       CTLTYPE_INT, "hostzerobroadcast",
		       SYSCTL_DESCR("All zeroes address is broadcast address"),
		       NULL, 0, &hostzeroisbroadcast, 0,
		       CTL_NET, PF_INET, IPPROTO_IP,
		       IPCTL_HOSTZEROBROADCAST, CTL_EOL);
}
