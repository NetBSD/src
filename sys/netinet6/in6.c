/*	$NetBSD: in6.c,v 1.251 2017/11/17 07:37:12 ozaki-r Exp $	*/
/*	$KAME: in6.c,v 1.198 2001/07/18 09:12:38 itojun Exp $	*/

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
 *	@(#)in.c	8.2 (Berkeley) 11/15/93
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: in6.c,v 1.251 2017/11/17 07:37:12 ozaki-r Exp $");

#ifdef _KERNEL_OPT
#include "opt_inet.h"
#include "opt_compat_netbsd.h"
#include "opt_net_mpsafe.h"
#endif

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/errno.h>
#include <sys/malloc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/sockio.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/time.h>
#include <sys/kernel.h>
#include <sys/syslog.h>
#include <sys/kauth.h>
#include <sys/cprng.h>
#include <sys/kmem.h>

#include <net/if.h>
#include <net/if_types.h>
#include <net/if_llatbl.h>
#include <net/if_ether.h>
#include <net/if_dl.h>
#include <net/pfil.h>
#include <net/route.h>

#include <netinet/in.h>
#include <netinet/in_var.h>

#include <netinet/ip6.h>
#include <netinet6/ip6_var.h>
#include <netinet6/nd6.h>
#include <netinet6/mld6_var.h>
#include <netinet6/ip6_mroute.h>
#include <netinet6/in6_ifattach.h>
#include <netinet6/scope6_var.h>

#include <net/net_osdep.h>

#ifdef COMPAT_50
#include <compat/netinet6/in6_var.h>
#endif

MALLOC_DEFINE(M_IP6OPT, "ip6_options", "IPv6 options");

/* enable backward compatibility code for obsoleted ioctls */
#define COMPAT_IN6IFIOCTL

#ifdef	IN6_DEBUG
#define	IN6_DPRINTF(__fmt, ...)	printf(__fmt, __VA_ARGS__)
#else
#define	IN6_DPRINTF(__fmt, ...)	do { } while (/*CONSTCOND*/0) 
#endif /* IN6_DEBUG */

/*
 * Definitions of some constant IP6 addresses.
 */
const struct in6_addr in6addr_any = IN6ADDR_ANY_INIT;
const struct in6_addr in6addr_loopback = IN6ADDR_LOOPBACK_INIT;
const struct in6_addr in6addr_nodelocal_allnodes =
	IN6ADDR_NODELOCAL_ALLNODES_INIT;
const struct in6_addr in6addr_linklocal_allnodes =
	IN6ADDR_LINKLOCAL_ALLNODES_INIT;
const struct in6_addr in6addr_linklocal_allrouters =
	IN6ADDR_LINKLOCAL_ALLROUTERS_INIT;

const struct in6_addr in6mask0 = IN6MASK0;
const struct in6_addr in6mask32 = IN6MASK32;
const struct in6_addr in6mask64 = IN6MASK64;
const struct in6_addr in6mask96 = IN6MASK96;
const struct in6_addr in6mask128 = IN6MASK128;

const struct sockaddr_in6 sa6_any = {sizeof(sa6_any), AF_INET6,
				     0, 0, IN6ADDR_ANY_INIT, 0};

struct pslist_head	in6_ifaddr_list;
kmutex_t		in6_ifaddr_lock;

static int in6_lifaddr_ioctl(struct socket *, u_long, void *,
	struct ifnet *);
static int in6_ifaddprefix(struct in6_ifaddr *);
static int in6_ifremprefix(struct in6_ifaddr *);
static int in6_ifinit(struct ifnet *, struct in6_ifaddr *,
	const struct sockaddr_in6 *, int);
static void in6_unlink_ifa(struct in6_ifaddr *, struct ifnet *);
static int in6_update_ifa1(struct ifnet *, struct in6_aliasreq *,
    struct in6_ifaddr **, struct psref *, int);

void
in6_init(void)
{

	PSLIST_INIT(&in6_ifaddr_list);
	mutex_init(&in6_ifaddr_lock, MUTEX_DEFAULT, IPL_NONE);

	in6_sysctl_multicast_setup(NULL);
}

/*
 * Add ownaddr as loopback rtentry.  We previously add the route only if
 * necessary (ex. on a p2p link).  However, since we now manage addresses
 * separately from prefixes, we should always add the route.  We can't
 * rely on the cloning mechanism from the corresponding interface route
 * any more.
 */
void
in6_ifaddlocal(struct ifaddr *ifa)
{

	if (IN6_ARE_ADDR_EQUAL(IFA_IN6(ifa), &in6addr_any) ||
	    (ifa->ifa_ifp->if_flags & IFF_POINTOPOINT &&
	    IN6_ARE_ADDR_EQUAL(IFA_IN6(ifa), IFA_DSTIN6(ifa))))
	{
		rt_newaddrmsg(RTM_NEWADDR, ifa, 0, NULL);
		return;
	}

	rt_ifa_addlocal(ifa);
}

/*
 * Remove loopback rtentry of ownaddr generated by in6_ifaddlocal(),
 * if it exists.
 */
void
in6_ifremlocal(struct ifaddr *ifa)
{
	struct in6_ifaddr *ia;
	struct ifaddr *alt_ifa = NULL;
	int ia_count = 0;
	struct psref psref;
	int s;

	/*
	 * Some of BSD variants do not remove cloned routes
	 * from an interface direct route, when removing the direct route
	 * (see comments in net/net_osdep.h).  Even for variants that do remove
	 * cloned routes, they could fail to remove the cloned routes when
	 * we handle multple addresses that share a common prefix.
	 * So, we should remove the route corresponding to the deleted address.
	 */

	/*
	 * Delete the entry only if exactly one ifaddr matches the
	 * address, ifa->ifa_addr.
	 *
	 * If more than one ifaddr matches, replace the ifaddr in
	 * the routing table, rt_ifa, with a different ifaddr than
	 * the one we are purging, ifa.  It is important to do
	 * this, or else the routing table can accumulate dangling
	 * pointers rt->rt_ifa->ifa_ifp to destroyed interfaces,
	 * which will lead to crashes, later.  (More than one ifaddr
	 * can match if we assign the same address to multiple---probably
	 * p2p---interfaces.)
	 *
	 * XXX An old comment at this place said, "we should avoid
	 * XXX such a configuration [i.e., interfaces with the same
	 * XXX addressed assigned --ed.] in IPv6...".  I do not
	 * XXX agree, especially now that I have fixed the dangling
	 * XXX ifp-pointers bug.
	 */
	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (!IN6_ARE_ADDR_EQUAL(IFA_IN6(ifa), &ia->ia_addr.sin6_addr))
			continue;
		if (ia->ia_ifp != ifa->ifa_ifp)
			alt_ifa = &ia->ia_ifa;
		if (++ia_count > 1 && alt_ifa != NULL)
			break;
	}
	if (ia_count > 1 && alt_ifa != NULL)
		ifa_acquire(alt_ifa, &psref);
	pserialize_read_exit(s);

	if (ia_count == 0)
		return;

	rt_ifa_remlocal(ifa, ia_count == 1 ? NULL : alt_ifa);

	if (ia_count > 1 && alt_ifa != NULL)
		ifa_release(alt_ifa, &psref);
}

/* Add prefix route for the network. */
static int
in6_ifaddprefix(struct in6_ifaddr *ia)
{
	int error, flags = 0;

	if (in6_mask2len(&ia->ia_prefixmask.sin6_addr, NULL) == 128) {
		if (ia->ia_dstaddr.sin6_family != AF_INET6)
			/* We don't need to install a host route. */
			return 0;
		flags |= RTF_HOST;
	}

	/* Is this a connected route for neighbour discovery? */
	if (nd6_need_cache(ia->ia_ifp))
		flags |= RTF_CONNECTED;

	if ((error = rtinit(&ia->ia_ifa, RTM_ADD, RTF_UP | flags)) == 0)
		ia->ia_flags |= IFA_ROUTE;
	else if (error == EEXIST)
		/* Existance of the route is not an error. */
		error = 0;

	return error;
}

/* Delete network prefix route if present.
 * Re-add it to another address if the prefix matches. */
static int
in6_ifremprefix(struct in6_ifaddr *target)
{
	int error, s;
	struct in6_ifaddr *ia;

	if ((target->ia_flags & IFA_ROUTE) == 0)
		return 0;

	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (target->ia_dstaddr.sin6_len) {
			if (ia->ia_dstaddr.sin6_len == 0 ||
			    !IN6_ARE_ADDR_EQUAL(&ia->ia_dstaddr.sin6_addr,
			    &target->ia_dstaddr.sin6_addr))
				continue;
		} else {
			if (!IN6_ARE_MASKED_ADDR_EQUAL(&ia->ia_addr.sin6_addr,
			    &target->ia_addr.sin6_addr,
			    &target->ia_prefixmask.sin6_addr))
				continue;
		}

		/*
		 * if we got a matching prefix route, move IFA_ROUTE to him
		 */
		if ((ia->ia_flags & IFA_ROUTE) == 0) {
			struct psref psref;
			int bound = curlwp_bind();

			ia6_acquire(ia, &psref);
			pserialize_read_exit(s);

			rtinit(&target->ia_ifa, RTM_DELETE, 0);
			target->ia_flags &= ~IFA_ROUTE;

			error = in6_ifaddprefix(ia);

			ia6_release(ia, &psref);
			curlwp_bindx(bound);

			return error;
		}
	}
	pserialize_read_exit(s);

	/*
	 * noone seem to have prefix route.  remove it.
	 */
	rtinit(&target->ia_ifa, RTM_DELETE, 0);
	target->ia_flags &= ~IFA_ROUTE;
	return 0;
}

int
in6_mask2len(struct in6_addr *mask, u_char *lim0)
{
	int x = 0, y;
	u_char *lim = lim0, *p;

	/* ignore the scope_id part */
	if (lim0 == NULL || lim0 - (u_char *)mask > sizeof(*mask))
		lim = (u_char *)mask + sizeof(*mask);
	for (p = (u_char *)mask; p < lim; x++, p++) {
		if (*p != 0xff)
			break;
	}
	y = 0;
	if (p < lim) {
		for (y = 0; y < NBBY; y++) {
			if ((*p & (0x80 >> y)) == 0)
				break;
		}
	}

	/*
	 * when the limit pointer is given, do a stricter check on the
	 * remaining bits.
	 */
	if (p < lim) {
		if (y != 0 && (*p & (0x00ff >> y)) != 0)
			return -1;
		for (p = p + 1; p < lim; p++)
			if (*p != 0)
				return -1;
	}

	return x * NBBY + y;
}

#define ifa2ia6(ifa)	((struct in6_ifaddr *)(ifa))
#define ia62ifa(ia6)	(&((ia6)->ia_ifa))

static int
in6_control1(struct socket *so, u_long cmd, void *data, struct ifnet *ifp)
{
	struct	in6_ifreq *ifr = (struct in6_ifreq *)data;
	struct	in6_ifaddr *ia = NULL;
	struct	in6_aliasreq *ifra = (struct in6_aliasreq *)data;
	struct sockaddr_in6 *sa6;
	int error, bound;
	struct psref psref;

	switch (cmd) {
	case SIOCAADDRCTL_POLICY:
	case SIOCDADDRCTL_POLICY:
		/* Privileged. */
		return in6_src_ioctl(cmd, data);
	/*
	 * XXX: Fix me, once we fix SIOCSIFADDR, SIOCIFDSTADDR, etc.
	 */
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
	case SIOCSIFBRDADDR:
	case SIOCSIFNETMASK:
		return EOPNOTSUPP;
	case SIOCGETSGCNT_IN6:
	case SIOCGETMIFCNT_IN6:
		return mrt6_ioctl(cmd, data);
	case SIOCGIFADDRPREF:
	case SIOCSIFADDRPREF:
		if (ifp == NULL)
			return EINVAL;
		return ifaddrpref_ioctl(so, cmd, data, ifp);
	}

	if (ifp == NULL)
		return EOPNOTSUPP;

	switch (cmd) {
	case SIOCSNDFLUSH_IN6:
	case SIOCSPFXFLUSH_IN6:
	case SIOCSRTRFLUSH_IN6:
	case SIOCSDEFIFACE_IN6:
	case SIOCSIFINFO_FLAGS:
	case SIOCSIFINFO_IN6:
		/* Privileged. */
		/* FALLTHROUGH */
	case OSIOCGIFINFO_IN6:
	case SIOCGIFINFO_IN6:
	case SIOCGDRLST_IN6:
	case SIOCGPRLST_IN6:
	case SIOCGNBRINFO_IN6:
	case SIOCGDEFIFACE_IN6:
		return nd6_ioctl(cmd, data, ifp);
	}

	switch (cmd) {
	case SIOCSIFPREFIX_IN6:
	case SIOCDIFPREFIX_IN6:
	case SIOCAIFPREFIX_IN6:
	case SIOCCIFPREFIX_IN6:
	case SIOCSGIFPREFIX_IN6:
	case SIOCGIFPREFIX_IN6:
		log(LOG_NOTICE,
		    "prefix ioctls are now invalidated. "
		    "please use ifconfig.\n");
		return EOPNOTSUPP;
	}

	switch (cmd) {
	case SIOCALIFADDR:
	case SIOCDLIFADDR:
		/* Privileged. */
		/* FALLTHROUGH */
	case SIOCGLIFADDR:
		return in6_lifaddr_ioctl(so, cmd, data, ifp);
	}

	/*
	 * Find address for this interface, if it exists.
	 *
	 * In netinet code, we have checked ifra_addr in SIOCSIF*ADDR operation
	 * only, and used the first interface address as the target of other
	 * operations (without checking ifra_addr).  This was because netinet
	 * code/API assumed at most 1 interface address per interface.
	 * Since IPv6 allows a node to assign multiple addresses
	 * on a single interface, we almost always look and check the
	 * presence of ifra_addr, and reject invalid ones here.
	 * It also decreases duplicated code among SIOC*_IN6 operations.
	 */
	switch (cmd) {
	case SIOCAIFADDR_IN6:
#ifdef OSIOCAIFADDR_IN6
	case OSIOCAIFADDR_IN6:
#endif
#ifdef OSIOCSIFPHYADDR_IN6
	case OSIOCSIFPHYADDR_IN6:
#endif
	case SIOCSIFPHYADDR_IN6:
		sa6 = &ifra->ifra_addr;
		break;
	case SIOCSIFADDR_IN6:
	case SIOCGIFADDR_IN6:
	case SIOCSIFDSTADDR_IN6:
	case SIOCSIFNETMASK_IN6:
	case SIOCGIFDSTADDR_IN6:
	case SIOCGIFNETMASK_IN6:
	case SIOCDIFADDR_IN6:
	case SIOCGIFPSRCADDR_IN6:
	case SIOCGIFPDSTADDR_IN6:
	case SIOCGIFAFLAG_IN6:
	case SIOCSNDFLUSH_IN6:
	case SIOCSPFXFLUSH_IN6:
	case SIOCSRTRFLUSH_IN6:
	case SIOCGIFALIFETIME_IN6:
#ifdef OSIOCGIFALIFETIME_IN6
	case OSIOCGIFALIFETIME_IN6:
#endif
	case SIOCGIFSTAT_IN6:
	case SIOCGIFSTAT_ICMP6:
		sa6 = &ifr->ifr_addr;
		break;
	default:
		sa6 = NULL;
		break;
	}

	error = 0;
	bound = curlwp_bind();
	if (sa6 && sa6->sin6_family == AF_INET6) {
		if (sa6->sin6_scope_id != 0)
			error = sa6_embedscope(sa6, 0);
		else
			error = in6_setscope(&sa6->sin6_addr, ifp, NULL);
		if (error != 0)
			goto out;
		ia = in6ifa_ifpwithaddr_psref(ifp, &sa6->sin6_addr, &psref);
	} else
		ia = NULL;

	switch (cmd) {
	case SIOCSIFADDR_IN6:
	case SIOCSIFDSTADDR_IN6:
	case SIOCSIFNETMASK_IN6:
		/*
		 * Since IPv6 allows a node to assign multiple addresses
		 * on a single interface, SIOCSIFxxx ioctls are deprecated.
		 */
		error = EINVAL;
		goto release;

	case SIOCDIFADDR_IN6:
		/*
		 * for IPv4, we look for existing in_ifaddr here to allow
		 * "ifconfig if0 delete" to remove the first IPv4 address on
		 * the interface.  For IPv6, as the spec allows multiple
		 * interface address from the day one, we consider "remove the
		 * first one" semantics to be not preferable.
		 */
		if (ia == NULL) {
			error = EADDRNOTAVAIL;
			goto out;
		}
		/* FALLTHROUGH */
#ifdef OSIOCAIFADDR_IN6
	case OSIOCAIFADDR_IN6:
#endif
	case SIOCAIFADDR_IN6:
		/*
		 * We always require users to specify a valid IPv6 address for
		 * the corresponding operation.
		 */
		if (ifra->ifra_addr.sin6_family != AF_INET6 ||
		    ifra->ifra_addr.sin6_len != sizeof(struct sockaddr_in6)) {
			error = EAFNOSUPPORT;
			goto release;
		}
		/* Privileged. */

		break;

	case SIOCGIFADDR_IN6:
		/* This interface is basically deprecated. use SIOCGIFCONF. */
		/* FALLTHROUGH */
	case SIOCGIFAFLAG_IN6:
	case SIOCGIFNETMASK_IN6:
	case SIOCGIFDSTADDR_IN6:
	case SIOCGIFALIFETIME_IN6:
#ifdef OSIOCGIFALIFETIME_IN6
	case OSIOCGIFALIFETIME_IN6:
#endif
		/* must think again about its semantics */
		if (ia == NULL) {
			error = EADDRNOTAVAIL;
			goto out;
		}
		break;
	}

	switch (cmd) {

	case SIOCGIFADDR_IN6:
		ifr->ifr_addr = ia->ia_addr;
		error = sa6_recoverscope(&ifr->ifr_addr);
		break;

	case SIOCGIFDSTADDR_IN6:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0) {
			error = EINVAL;
			break;
		}
		/*
		 * XXX: should we check if ifa_dstaddr is NULL and return
		 * an error?
		 */
		ifr->ifr_dstaddr = ia->ia_dstaddr;
		error = sa6_recoverscope(&ifr->ifr_dstaddr);
		break;

	case SIOCGIFNETMASK_IN6:
		ifr->ifr_addr = ia->ia_prefixmask;
		break;

	case SIOCGIFAFLAG_IN6:
		ifr->ifr_ifru.ifru_flags6 = ia->ia6_flags;
		break;

	case SIOCGIFSTAT_IN6:
		if (ifp == NULL) {
			error = EINVAL;
			break;
		}
		memset(&ifr->ifr_ifru.ifru_stat, 0,
		    sizeof(ifr->ifr_ifru.ifru_stat));
		ifr->ifr_ifru.ifru_stat =
		    *((struct in6_ifextra *)ifp->if_afdata[AF_INET6])->in6_ifstat;
		break;

	case SIOCGIFSTAT_ICMP6:
		if (ifp == NULL) {
			error = EINVAL;
			break;
		}
		memset(&ifr->ifr_ifru.ifru_icmp6stat, 0,
		    sizeof(ifr->ifr_ifru.ifru_icmp6stat));
		ifr->ifr_ifru.ifru_icmp6stat =
		    *((struct in6_ifextra *)ifp->if_afdata[AF_INET6])->icmp6_ifstat;
		break;

#ifdef OSIOCGIFALIFETIME_IN6
	case OSIOCGIFALIFETIME_IN6:
#endif
	case SIOCGIFALIFETIME_IN6:
		ifr->ifr_ifru.ifru_lifetime = ia->ia6_lifetime;
		if (ia->ia6_lifetime.ia6t_vltime != ND6_INFINITE_LIFETIME) {
			time_t maxexpire;
			struct in6_addrlifetime *retlt =
			    &ifr->ifr_ifru.ifru_lifetime;

			/*
			 * XXX: adjust expiration time assuming time_t is
			 * signed.
			 */
			maxexpire = ((time_t)~0) &
			    ~((time_t)1 << ((sizeof(maxexpire) * NBBY) - 1));
			if (ia->ia6_lifetime.ia6t_vltime <
			    maxexpire - ia->ia6_updatetime) {
				retlt->ia6t_expire = ia->ia6_updatetime +
				    ia->ia6_lifetime.ia6t_vltime;
				retlt->ia6t_expire = retlt->ia6t_expire ?
				    time_mono_to_wall(retlt->ia6t_expire) :
				    0;
			} else
				retlt->ia6t_expire = maxexpire;
		}
		if (ia->ia6_lifetime.ia6t_pltime != ND6_INFINITE_LIFETIME) {
			time_t maxexpire;
			struct in6_addrlifetime *retlt =
			    &ifr->ifr_ifru.ifru_lifetime;

			/*
			 * XXX: adjust expiration time assuming time_t is
			 * signed.
			 */
			maxexpire = ((time_t)~0) &
			    ~((time_t)1 << ((sizeof(maxexpire) * NBBY) - 1));
			if (ia->ia6_lifetime.ia6t_pltime <
			    maxexpire - ia->ia6_updatetime) {
				retlt->ia6t_preferred = ia->ia6_updatetime +
				    ia->ia6_lifetime.ia6t_pltime;
				retlt->ia6t_preferred = retlt->ia6t_preferred ?
				    time_mono_to_wall(retlt->ia6t_preferred) :
				    0;
			} else
				retlt->ia6t_preferred = maxexpire;
		}
#ifdef OSIOCFIFALIFETIME_IN6
		if (cmd == OSIOCFIFALIFETIME_IN6)
			in6_addrlifetime_to_in6_addrlifetime50(
			    &ifr->ifru.ifru_lifetime);
#endif
		break;

#ifdef OSIOCAIFADDR_IN6
	case OSIOCAIFADDR_IN6:
		in6_aliasreq50_to_in6_aliasreq(ifra);
		/*FALLTHROUGH*/
#endif
	case SIOCAIFADDR_IN6:
	{
		struct in6_addrlifetime *lt;

		/* reject read-only flags */
		if ((ifra->ifra_flags & IN6_IFF_DUPLICATED) != 0 ||
		    (ifra->ifra_flags & IN6_IFF_DETACHED) != 0 ||
		    (ifra->ifra_flags & IN6_IFF_TENTATIVE) != 0 ||
		    (ifra->ifra_flags & IN6_IFF_NODAD) != 0 ||
		    (ifra->ifra_flags & IN6_IFF_AUTOCONF) != 0) {
			error = EINVAL;
			break;
		}
		/*
		 * ia6t_expire and ia6t_preferred won't be used for now,
		 * so just in case.
		 */
		lt = &ifra->ifra_lifetime;
		if (lt->ia6t_expire != 0)
			lt->ia6t_expire = time_wall_to_mono(lt->ia6t_expire);
		if (lt->ia6t_preferred != 0)
			lt->ia6t_preferred =
			    time_wall_to_mono(lt->ia6t_preferred);
		/*
		 * make (ia == NULL) or update (ia != NULL) the interface
		 * address structure, and link it to the list.
		 */
		int s = splsoftnet();
		error = in6_update_ifa1(ifp, ifra, &ia, &psref, 0);
		splx(s);
		if (error)
			break;
		pfil_run_addrhooks(if_pfil, cmd, &ia->ia_ifa);
		break;
	}

	case SIOCDIFADDR_IN6:
		ia6_release(ia, &psref);
		ifaref(&ia->ia_ifa);
		in6_purgeaddr(&ia->ia_ifa);
		pfil_run_addrhooks(if_pfil, cmd, &ia->ia_ifa);
		ifafree(&ia->ia_ifa);
		ia = NULL;
		break;

	default:
		error = ENOTTY;
	}
release:
	ia6_release(ia, &psref);
out:
	curlwp_bindx(bound);
	return error;
}

int
in6_control(struct socket *so, u_long cmd, void *data, struct ifnet *ifp)
{
	int error, s;

	switch (cmd) {
	case SIOCSNDFLUSH_IN6:
	case SIOCSPFXFLUSH_IN6:
	case SIOCSRTRFLUSH_IN6:
	case SIOCSDEFIFACE_IN6:
	case SIOCSIFINFO_FLAGS:
	case SIOCSIFINFO_IN6:

	case SIOCALIFADDR:
	case SIOCDLIFADDR:

	case SIOCDIFADDR_IN6:
#ifdef OSIOCAIFADDR_IN6
	case OSIOCAIFADDR_IN6:
#endif
	case SIOCAIFADDR_IN6:

	case SIOCAADDRCTL_POLICY:
	case SIOCDADDRCTL_POLICY:

		if (kauth_authorize_network(curlwp->l_cred,
		    KAUTH_NETWORK_SOCKET,
		    KAUTH_REQ_NETWORK_SOCKET_SETPRIV,
		    so, NULL, NULL))
			return EPERM;
		break;
	}

	s = splsoftnet();
	SOFTNET_LOCK_UNLESS_NET_MPSAFE();
	error = in6_control1(so , cmd, data, ifp);
	SOFTNET_UNLOCK_UNLESS_NET_MPSAFE();
	splx(s);
	return error;
}

static int
in6_get_llsol_addr(struct in6_addr *llsol, struct ifnet *ifp,
    struct in6_addr *ip6)
{
	int error;

	memset(llsol, 0, sizeof(struct in6_addr));
	llsol->s6_addr16[0] = htons(0xff02);
	llsol->s6_addr32[1] = 0;
	llsol->s6_addr32[2] = htonl(1);
	llsol->s6_addr32[3] = ip6->s6_addr32[3];
	llsol->s6_addr8[12] = 0xff;

	error = in6_setscope(llsol, ifp, NULL);
	if (error != 0) {
		/* XXX: should not happen */
		log(LOG_ERR, "%s: in6_setscope failed\n", __func__);
	}

	return error;
}

static int
in6_join_mcastgroups(struct in6_aliasreq *ifra, struct in6_ifaddr *ia,
    struct ifnet *ifp, int flags)
{
	int error;
	struct sockaddr_in6 mltaddr, mltmask;
	struct in6_multi_mship *imm;
	struct in6_addr llsol;
	struct rtentry *rt;
	int dad_delay;
	char ip6buf[INET6_ADDRSTRLEN];

	/* join solicited multicast addr for new host id */
	error = in6_get_llsol_addr(&llsol, ifp, &ifra->ifra_addr.sin6_addr);
	if (error != 0)
		goto out;
	dad_delay = 0;
	if ((flags & IN6_IFAUPDATE_DADDELAY)) {
		/*
		 * We need a random delay for DAD on the address
		 * being configured.  It also means delaying
		 * transmission of the corresponding MLD report to
		 * avoid report collision.
		 * [draft-ietf-ipv6-rfc2462bis-02.txt]
		 */
		dad_delay = cprng_fast32() % (MAX_RTR_SOLICITATION_DELAY * hz);
	}

#define	MLTMASK_LEN  4	/* mltmask's masklen (=32bit=4octet) */
	/* join solicited multicast addr for new host id */
	imm = in6_joingroup(ifp, &llsol, &error, dad_delay);
	if (!imm) {
		nd6log(LOG_ERR,
		    "addmulti failed for %s on %s (errno=%d)\n",
		    IN6_PRINT(ip6buf, &llsol), if_name(ifp), error);
		goto out;
	}
	mutex_enter(&in6_ifaddr_lock);
	LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
	mutex_exit(&in6_ifaddr_lock);

	sockaddr_in6_init(&mltmask, &in6mask32, 0, 0, 0);

	/*
	 * join link-local all-nodes address
	 */
	sockaddr_in6_init(&mltaddr, &in6addr_linklocal_allnodes,
	    0, 0, 0);
	if ((error = in6_setscope(&mltaddr.sin6_addr, ifp, NULL)) != 0)
		goto out; /* XXX: should not fail */

	/*
	 * XXX: do we really need this automatic routes?
	 * We should probably reconsider this stuff.  Most applications
	 * actually do not need the routes, since they usually specify
	 * the outgoing interface.
	 */
	rt = rtalloc1(sin6tosa(&mltaddr), 0);
	if (rt) {
		if (memcmp(&mltaddr.sin6_addr,
		    &satocsin6(rt_getkey(rt))->sin6_addr,
		    MLTMASK_LEN)) {
			rt_unref(rt);
			rt = NULL;
		} else if (rt->rt_ifp != ifp) {
			IN6_DPRINTF("%s: rt_ifp %p -> %p (%s) "
			    "network %04x:%04x::/32 = %04x:%04x::/32\n",
			    __func__, rt->rt_ifp, ifp, ifp->if_xname,
			    ntohs(mltaddr.sin6_addr.s6_addr16[0]),
			    ntohs(mltaddr.sin6_addr.s6_addr16[1]),
			    satocsin6(rt_getkey(rt))->sin6_addr.s6_addr16[0],
			    satocsin6(rt_getkey(rt))->sin6_addr.s6_addr16[1]);
			rt_replace_ifa(rt, &ia->ia_ifa);
			rt->rt_ifp = ifp;
		}
	}
	if (!rt) {
		struct rt_addrinfo info;

		memset(&info, 0, sizeof(info));
		info.rti_info[RTAX_DST] = sin6tosa(&mltaddr);
		info.rti_info[RTAX_GATEWAY] = sin6tosa(&ia->ia_addr);
		info.rti_info[RTAX_NETMASK] = sin6tosa(&mltmask);
		info.rti_info[RTAX_IFA] = sin6tosa(&ia->ia_addr);
		/* XXX: we need RTF_CONNECTED to fake nd6_rtrequest */
		info.rti_flags = RTF_UP | RTF_CONNECTED;
		error = rtrequest1(RTM_ADD, &info, NULL);
		if (error)
			goto out;
	} else {
		rt_unref(rt);
	}
	imm = in6_joingroup(ifp, &mltaddr.sin6_addr, &error, 0);
	if (!imm) {
		nd6log(LOG_WARNING,
		    "addmulti failed for %s on %s (errno=%d)\n",
		    IN6_PRINT(ip6buf, &mltaddr.sin6_addr),
		    if_name(ifp), error);
		goto out;
	}
	mutex_enter(&in6_ifaddr_lock);
	LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
	mutex_exit(&in6_ifaddr_lock);

	/*
	 * join node information group address
	 */
	dad_delay = 0;
	if ((flags & IN6_IFAUPDATE_DADDELAY)) {
		/*
		 * The spec doesn't say anything about delay for this
		 * group, but the same logic should apply.
		 */
		dad_delay = cprng_fast32() % (MAX_RTR_SOLICITATION_DELAY * hz);
	}
	if (in6_nigroup(ifp, hostname, hostnamelen, &mltaddr) != 0)
		;
	else if ((imm = in6_joingroup(ifp, &mltaddr.sin6_addr, &error,
		  dad_delay)) == NULL) { /* XXX jinmei */
		nd6log(LOG_WARNING,
		    "addmulti failed for %s on %s (errno=%d)\n",
		    IN6_PRINT(ip6buf, &mltaddr.sin6_addr),
		    if_name(ifp), error);
		/* XXX not very fatal, go on... */
	} else {
		mutex_enter(&in6_ifaddr_lock);
		LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
		mutex_exit(&in6_ifaddr_lock);
	}


	/*
	 * join interface-local all-nodes address.
	 * (ff01::1%ifN, and ff01::%ifN/32)
	 */
	mltaddr.sin6_addr = in6addr_nodelocal_allnodes;
	if ((error = in6_setscope(&mltaddr.sin6_addr, ifp, NULL)) != 0)
		goto out; /* XXX: should not fail */

	/* XXX: again, do we really need the route? */
	rt = rtalloc1(sin6tosa(&mltaddr), 0);
	if (rt) {
		/* 32bit came from "mltmask" */
		if (memcmp(&mltaddr.sin6_addr,
		    &satocsin6(rt_getkey(rt))->sin6_addr,
		    32 / NBBY)) {
			rt_unref(rt);
			rt = NULL;
		} else if (rt->rt_ifp != ifp) {
			IN6_DPRINTF("%s: rt_ifp %p -> %p (%s) "
			    "network %04x:%04x::/32 = %04x:%04x::/32\n",
			    __func__, rt->rt_ifp, ifp, ifp->if_xname,
			    ntohs(mltaddr.sin6_addr.s6_addr16[0]),
			    ntohs(mltaddr.sin6_addr.s6_addr16[1]),
			    satocsin6(rt_getkey(rt))->sin6_addr.s6_addr16[0],
			    satocsin6(rt_getkey(rt))->sin6_addr.s6_addr16[1]);
			rt_replace_ifa(rt, &ia->ia_ifa);
			rt->rt_ifp = ifp;
		}
	}
	if (!rt) {
		struct rt_addrinfo info;

		memset(&info, 0, sizeof(info));
		info.rti_info[RTAX_DST] = sin6tosa(&mltaddr);
		info.rti_info[RTAX_GATEWAY] = sin6tosa(&ia->ia_addr);
		info.rti_info[RTAX_NETMASK] = sin6tosa(&mltmask);
		info.rti_info[RTAX_IFA] = sin6tosa(&ia->ia_addr);
		info.rti_flags = RTF_UP | RTF_CONNECTED;
		error = rtrequest1(RTM_ADD, &info, NULL);
		if (error)
			goto out;
#undef	MLTMASK_LEN
	} else {
		rt_unref(rt);
	}
	imm = in6_joingroup(ifp, &mltaddr.sin6_addr, &error, 0);
	if (!imm) {
		nd6log(LOG_WARNING,
		    "addmulti failed for %s on %s (errno=%d)\n",
		    IN6_PRINT(ip6buf, &mltaddr.sin6_addr),
		    if_name(ifp), error);
		goto out;
	} else {
		mutex_enter(&in6_ifaddr_lock);
		LIST_INSERT_HEAD(&ia->ia6_memberships, imm, i6mm_chain);
		mutex_exit(&in6_ifaddr_lock);
	}
	return 0;

out:
	KASSERT(error != 0);
	return error;
}

/*
 * Update parameters of an IPv6 interface address.
 * If necessary, a new entry is created and linked into address chains.
 * This function is separated from in6_control().
 * XXX: should this be performed under splsoftnet()?
 */
static int
in6_update_ifa1(struct ifnet *ifp, struct in6_aliasreq *ifra,
    struct in6_ifaddr **iap, struct psref *psref, int flags)
{
	int error = 0, hostIsNew = 0, plen = -1;
	struct sockaddr_in6 dst6;
	struct in6_addrlifetime *lt;
	int dad_delay, was_tentative;
	struct in6_ifaddr *ia = iap ? *iap : NULL;
	char ip6buf[INET6_ADDRSTRLEN];

	KASSERT((iap == NULL && psref == NULL) ||
	    (iap != NULL && psref != NULL));

	/* Validate parameters */
	if (ifp == NULL || ifra == NULL) /* this maybe redundant */
		return EINVAL;

	/*
	 * The destination address for a p2p link must have a family
	 * of AF_UNSPEC or AF_INET6.
	 */
	if ((ifp->if_flags & IFF_POINTOPOINT) != 0 &&
	    ifra->ifra_dstaddr.sin6_family != AF_INET6 &&
	    ifra->ifra_dstaddr.sin6_family != AF_UNSPEC)
		return EAFNOSUPPORT;
	/*
	 * validate ifra_prefixmask.  don't check sin6_family, netmask
	 * does not carry fields other than sin6_len.
	 */
	if (ifra->ifra_prefixmask.sin6_len > sizeof(struct sockaddr_in6))
		return EINVAL;
	/*
	 * Because the IPv6 address architecture is classless, we require
	 * users to specify a (non 0) prefix length (mask) for a new address.
	 * We also require the prefix (when specified) mask is valid, and thus
	 * reject a non-consecutive mask.
	 */
	if (ia == NULL && ifra->ifra_prefixmask.sin6_len == 0)
		return EINVAL;
	if (ifra->ifra_prefixmask.sin6_len != 0) {
		plen = in6_mask2len(&ifra->ifra_prefixmask.sin6_addr,
		    (u_char *)&ifra->ifra_prefixmask +
		    ifra->ifra_prefixmask.sin6_len);
		if (plen <= 0)
			return EINVAL;
	} else {
		/*
		 * In this case, ia must not be NULL.  We just use its prefix
		 * length.
		 */
		plen = in6_mask2len(&ia->ia_prefixmask.sin6_addr, NULL);
	}
	/*
	 * If the destination address on a p2p interface is specified,
	 * and the address is a scoped one, validate/set the scope
	 * zone identifier.
	 */
	dst6 = ifra->ifra_dstaddr;
	if ((ifp->if_flags & (IFF_POINTOPOINT|IFF_LOOPBACK)) != 0 &&
	    (dst6.sin6_family == AF_INET6)) {
		struct in6_addr in6_tmp;
		u_int32_t zoneid;

		in6_tmp = dst6.sin6_addr;
		if (in6_setscope(&in6_tmp, ifp, &zoneid))
			return EINVAL; /* XXX: should be impossible */

		if (dst6.sin6_scope_id != 0) {
			if (dst6.sin6_scope_id != zoneid)
				return EINVAL;
		} else		/* user omit to specify the ID. */
			dst6.sin6_scope_id = zoneid;

		/* convert into the internal form */
		if (sa6_embedscope(&dst6, 0))
			return EINVAL; /* XXX: should be impossible */
	}
	/*
	 * The destination address can be specified only for a p2p or a
	 * loopback interface.  If specified, the corresponding prefix length
	 * must be 128.
	 */
	if (ifra->ifra_dstaddr.sin6_family == AF_INET6) {
#ifdef FORCE_P2PPLEN
		int i;
#endif

		if ((ifp->if_flags & (IFF_POINTOPOINT|IFF_LOOPBACK)) == 0) {
			/* XXX: noisy message */
			nd6log(LOG_INFO, "a destination can "
			    "be specified for a p2p or a loopback IF only\n");
			return EINVAL;
		}
		if (plen != 128) {
			nd6log(LOG_INFO, "prefixlen should "
			    "be 128 when dstaddr is specified\n");
#ifdef FORCE_P2PPLEN
			/*
			 * To be compatible with old configurations,
			 * such as ifconfig gif0 inet6 2001::1 2001::2
			 * prefixlen 126, we override the specified
			 * prefixmask as if the prefix length was 128.
			 */
			ifra->ifra_prefixmask.sin6_len =
			    sizeof(struct sockaddr_in6);
			for (i = 0; i < 4; i++)
				ifra->ifra_prefixmask.sin6_addr.s6_addr32[i] =
				    0xffffffff;
			plen = 128;
#else
			return EINVAL;
#endif
		}
	}
	/* lifetime consistency check */
	lt = &ifra->ifra_lifetime;
	if (lt->ia6t_pltime > lt->ia6t_vltime)
		return EINVAL;
	if (lt->ia6t_vltime == 0) {
		/*
		 * the following log might be noisy, but this is a typical
		 * configuration mistake or a tool's bug.
		 */
		nd6log(LOG_INFO, "valid lifetime is 0 for %s\n",
		    IN6_PRINT(ip6buf, &ifra->ifra_addr.sin6_addr));

		if (ia == NULL)
			return 0; /* there's nothing to do */
	}

	/*
	 * If this is a new address, allocate a new ifaddr and link it
	 * into chains.
	 */
	if (ia == NULL) {
		hostIsNew = 1;
		/*
		 * When in6_update_ifa() is called in a process of a received
		 * RA, it is called under an interrupt context.  So, we should
		 * call malloc with M_NOWAIT.
		 */
		ia = malloc(sizeof(*ia), M_IFADDR, M_NOWAIT|M_ZERO);
		if (ia == NULL)
			return ENOBUFS;
		LIST_INIT(&ia->ia6_memberships);
		/* Initialize the address and masks, and put time stamp */
		ia->ia_ifa.ifa_addr = sin6tosa(&ia->ia_addr);
		ia->ia_addr.sin6_family = AF_INET6;
		ia->ia_addr.sin6_len = sizeof(ia->ia_addr);
		ia->ia6_createtime = time_uptime;
		if ((ifp->if_flags & (IFF_POINTOPOINT | IFF_LOOPBACK)) != 0) {
			/*
			 * XXX: some functions expect that ifa_dstaddr is not
			 * NULL for p2p interfaces.
			 */
			ia->ia_ifa.ifa_dstaddr = sin6tosa(&ia->ia_dstaddr);
		} else {
			ia->ia_ifa.ifa_dstaddr = NULL;
		}
		ia->ia_ifa.ifa_netmask = sin6tosa(&ia->ia_prefixmask);

		ia->ia_ifp = ifp;
		IN6_ADDRLIST_ENTRY_INIT(ia);
		ifa_psref_init(&ia->ia_ifa);
	}

	/* update timestamp */
	ia->ia6_updatetime = time_uptime;

	/* set prefix mask */
	if (ifra->ifra_prefixmask.sin6_len) {
		if (ia->ia_prefixmask.sin6_len) {
			/*
			 * We prohibit changing the prefix length of an
			 * existing autoconf address, because the operation
			 * would confuse prefix management.
			 */
			if (ia->ia6_ndpr != NULL &&
			    in6_mask2len(&ia->ia_prefixmask.sin6_addr, NULL) !=
			    plen)
			{
				nd6log(LOG_INFO, "the prefix length of an"
				    " existing (%s) autoconf address should"
				    " not be changed\n",
				    IN6_PRINT(ip6buf,
				    &ia->ia_addr.sin6_addr));
				error = EINVAL;
				if (hostIsNew)
					free(ia, M_IFADDR);
				goto exit;
			}

			if (!IN6_ARE_ADDR_EQUAL(&ia->ia_prefixmask.sin6_addr,
			    &ifra->ifra_prefixmask.sin6_addr))
				in6_ifremprefix(ia);
		}
		ia->ia_prefixmask = ifra->ifra_prefixmask;
	}

	/* Set destination address. */
	if (dst6.sin6_family == AF_INET6) {
		if (!IN6_ARE_ADDR_EQUAL(&dst6.sin6_addr,
		    &ia->ia_dstaddr.sin6_addr))
			in6_ifremprefix(ia);
		ia->ia_dstaddr = dst6;
	}

	/*
	 * Set lifetimes.  We do not refer to ia6t_expire and ia6t_preferred
	 * to see if the address is deprecated or invalidated, but initialize
	 * these members for applications.
	 */
	ia->ia6_lifetime = ifra->ifra_lifetime;
	if (ia->ia6_lifetime.ia6t_vltime != ND6_INFINITE_LIFETIME) {
		ia->ia6_lifetime.ia6t_expire =
		    time_uptime + ia->ia6_lifetime.ia6t_vltime;
	} else
		ia->ia6_lifetime.ia6t_expire = 0;
	if (ia->ia6_lifetime.ia6t_pltime != ND6_INFINITE_LIFETIME) {
		ia->ia6_lifetime.ia6t_preferred =
		    time_uptime + ia->ia6_lifetime.ia6t_pltime;
	} else
		ia->ia6_lifetime.ia6t_preferred = 0;

	/*
	 * configure address flags.
	 * We need to preserve tentative state so DAD works if
	 * something adds the same address before DAD finishes.
	 */
	was_tentative = ia->ia6_flags & (IN6_IFF_TENTATIVE|IN6_IFF_DUPLICATED);
	ia->ia6_flags = ifra->ifra_flags;

	/*
	 * Make the address tentative before joining multicast addresses,
	 * so that corresponding MLD responses would not have a tentative
	 * source address.
	 */
	ia->ia6_flags &= ~IN6_IFF_DUPLICATED;	/* safety */
	if (ifp->if_link_state == LINK_STATE_DOWN) {
		ia->ia6_flags |= IN6_IFF_DETACHED;
		ia->ia6_flags &= ~IN6_IFF_TENTATIVE;
	} else if ((hostIsNew || was_tentative) && if_do_dad(ifp))
		ia->ia6_flags |= IN6_IFF_TENTATIVE;

	/*
	 * backward compatibility - if IN6_IFF_DEPRECATED is set from the
	 * userland, make it deprecated.
	 */
	if ((ifra->ifra_flags & IN6_IFF_DEPRECATED) != 0) {
		ia->ia6_lifetime.ia6t_pltime = 0;
		ia->ia6_lifetime.ia6t_preferred = time_uptime;
	}

	if (hostIsNew) {
		/*
		 * We need a reference to ia before calling in6_ifinit.
		 * Otherwise ia can be freed in in6_ifinit accidentally.
		 */
		ifaref(&ia->ia_ifa);
	}
	/* reset the interface and routing table appropriately. */
	error = in6_ifinit(ifp, ia, &ifra->ifra_addr, hostIsNew);
	if (error != 0) {
		if (hostIsNew)
			free(ia, M_IFADDR);
		goto exit;
	}

	/*
	 * We are done if we have simply modified an existing address.
	 */
	if (!hostIsNew)
		return error;

	/*
	 * Insert ia to the global list and ifa to the interface's list.
	 * A reference to it is already gained above.
	 */
	mutex_enter(&in6_ifaddr_lock);
	IN6_ADDRLIST_WRITER_INSERT_TAIL(ia);
	mutex_exit(&in6_ifaddr_lock);

	ifa_insert(ifp, &ia->ia_ifa);

	/*
	 * Beyond this point, we should call in6_purgeaddr upon an error,
	 * not just go to unlink.
	 */

	/* join necessary multicast groups */
	if ((ifp->if_flags & IFF_MULTICAST) != 0) {
		error = in6_join_mcastgroups(ifra, ia, ifp, flags);
		if (error != 0)
			goto cleanup;
	}

	if (nd6_need_cache(ifp)) {
		/* XXX maybe unnecessary */
		ia->ia_ifa.ifa_rtrequest = nd6_rtrequest;
		ia->ia_ifa.ifa_flags |= RTF_CONNECTED;
	}

	/*
	 * Perform DAD, if needed.
	 * XXX It may be of use, if we can administratively
	 * disable DAD.
	 */
	if (hostIsNew && if_do_dad(ifp) &&
	    ((ifra->ifra_flags & IN6_IFF_NODAD) == 0) &&
	    (ia->ia6_flags & IN6_IFF_TENTATIVE))
	{
		int mindelay, maxdelay;

		dad_delay = 0;
		if ((flags & IN6_IFAUPDATE_DADDELAY)) {
			struct in6_addr llsol;
			struct in6_multi *in6m_sol = NULL;
			/*
			 * We need to impose a delay before sending an NS
			 * for DAD.  Check if we also needed a delay for the
			 * corresponding MLD message.  If we did, the delay
			 * should be larger than the MLD delay (this could be
			 * relaxed a bit, but this simple logic is at least
			 * safe).
			 */
			mindelay = 0;
			error = in6_get_llsol_addr(&llsol, ifp,
			    &ifra->ifra_addr.sin6_addr);
			in6_multi_lock(RW_READER);
			if (error == 0)
				in6m_sol = in6_lookup_multi(&llsol, ifp);
			if (in6m_sol != NULL &&
			    in6m_sol->in6m_state == MLD_REPORTPENDING) {
				mindelay = in6m_sol->in6m_timer;
			}
			in6_multi_unlock();
			maxdelay = MAX_RTR_SOLICITATION_DELAY * hz;
			if (maxdelay - mindelay == 0)
				dad_delay = 0;
			else {
				dad_delay =
				    (cprng_fast32() % (maxdelay - mindelay)) +
				    mindelay;
			}
		}
		/* +1 ensures callout is always used */
		nd6_dad_start(&ia->ia_ifa, dad_delay + 1);
	}

	if (iap != NULL) {
		*iap = ia;
		if (hostIsNew)
			ia6_acquire(ia, psref);
	}

	return 0;

  cleanup:
	in6_purgeaddr(&ia->ia_ifa);
  exit:
	return error;
}

int
in6_update_ifa(struct ifnet *ifp, struct in6_aliasreq *ifra, int flags)
{
	int rc, s;

	s = splsoftnet();
	rc = in6_update_ifa1(ifp, ifra, NULL, NULL, flags);
	splx(s);
	return rc;
}

void
in6_purgeaddr(struct ifaddr *ifa)
{
	struct ifnet *ifp = ifa->ifa_ifp;
	struct in6_ifaddr *ia = (struct in6_ifaddr *) ifa;
	struct in6_multi_mship *imm;

	KASSERT(!ifa_held(ifa));

	ifa->ifa_flags |= IFA_DESTROYING;

	/* stop DAD processing */
	nd6_dad_stop(ifa);

	/* Delete any network route. */
	in6_ifremprefix(ia);

	/* Remove ownaddr's loopback rtentry, if it exists. */
	in6_ifremlocal(&(ia->ia_ifa));

	/*
	 * leave from multicast groups we have joined for the interface
	 */
	mutex_enter(&in6_ifaddr_lock);
	while ((imm = LIST_FIRST(&ia->ia6_memberships)) != NULL) {
		LIST_REMOVE(imm, i6mm_chain);
		mutex_exit(&in6_ifaddr_lock);
		in6_leavegroup(imm);
		mutex_enter(&in6_ifaddr_lock);
	}
	mutex_exit(&in6_ifaddr_lock);

	in6_unlink_ifa(ia, ifp);
}

static void
in6_unlink_ifa(struct in6_ifaddr *ia, struct ifnet *ifp)
{
	int	s = splsoftnet();

	mutex_enter(&in6_ifaddr_lock);
	IN6_ADDRLIST_WRITER_REMOVE(ia);
	ifa_remove(ifp, &ia->ia_ifa);
	mutex_exit(&in6_ifaddr_lock);

	/*
	 * Release the reference to the ND prefix.
	 */
	if (ia->ia6_ndpr != NULL) {
		nd6_prefix_unref(ia->ia6_ndpr);
		ia->ia6_ndpr = NULL;
	}

	/*
	 * Also, if the address being removed is autoconf'ed, call
	 * nd6_pfxlist_onlink_check() since the release might affect the status of
	 * other (detached) addresses.
	 */
	if ((ia->ia6_flags & IN6_IFF_AUTOCONF) != 0) {
		ND6_WLOCK();
		nd6_pfxlist_onlink_check();
		ND6_UNLOCK();
	}

	IN6_ADDRLIST_ENTRY_DESTROY(ia);

	/*
	 * release another refcnt for the link from in6_ifaddr.
	 * Note that we should decrement the refcnt at least once for all *BSD.
	 */
	ifafree(&ia->ia_ifa);

	splx(s);
}

void
in6_purgeif(struct ifnet *ifp)
{

	in6_ifdetach(ifp);
}

void
in6_purge_mcast_references(struct in6_multi *in6m)
{
	struct	in6_ifaddr *ia;

	KASSERT(in6_multi_locked(RW_WRITER));

	mutex_enter(&in6_ifaddr_lock);
	IN6_ADDRLIST_WRITER_FOREACH(ia) {
		struct in6_multi_mship *imm;
		LIST_FOREACH(imm, &ia->ia6_memberships, i6mm_chain) {
			if (imm->i6mm_maddr == in6m)
				imm->i6mm_maddr = NULL;
		}
	}
	mutex_exit(&in6_ifaddr_lock);
}

/*
 * SIOC[GAD]LIFADDR.
 *	SIOCGLIFADDR: get first address. (?)
 *	SIOCGLIFADDR with IFLR_PREFIX:
 *		get first address that matches the specified prefix.
 *	SIOCALIFADDR: add the specified address.
 *	SIOCALIFADDR with IFLR_PREFIX:
 *		add the specified prefix, filling hostid part from
 *		the first link-local address.  prefixlen must be <= 64.
 *	SIOCDLIFADDR: delete the specified address.
 *	SIOCDLIFADDR with IFLR_PREFIX:
 *		delete the first address that matches the specified prefix.
 * return values:
 *	EINVAL on invalid parameters
 *	EADDRNOTAVAIL on prefix match failed/specified address not found
 *	other values may be returned from in6_ioctl()
 *
 * NOTE: SIOCALIFADDR(with IFLR_PREFIX set) allows prefixlen less than 64.
 * this is to accommodate address naming scheme other than RFC2374,
 * in the future.
 * RFC2373 defines interface id to be 64bit, but it allows non-RFC2374
 * address encoding scheme. (see figure on page 8)
 */
static int
in6_lifaddr_ioctl(struct socket *so, u_long cmd, void *data, 
	struct ifnet *ifp)
{
	struct in6_ifaddr *ia = NULL; /* XXX gcc 4.8 maybe-uninitialized */
	struct if_laddrreq *iflr = (struct if_laddrreq *)data;
	struct ifaddr *ifa;
	struct sockaddr *sa;

	/* sanity checks */
	if (!data || !ifp) {
		panic("invalid argument to in6_lifaddr_ioctl");
		/* NOTREACHED */
	}

	switch (cmd) {
	case SIOCGLIFADDR:
		/* address must be specified on GET with IFLR_PREFIX */
		if ((iflr->flags & IFLR_PREFIX) == 0)
			break;
		/* FALLTHROUGH */
	case SIOCALIFADDR:
	case SIOCDLIFADDR:
		/* address must be specified on ADD and DELETE */
		sa = (struct sockaddr *)&iflr->addr;
		if (sa->sa_family != AF_INET6)
			return EINVAL;
		if (sa->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;
		/* XXX need improvement */
		sa = (struct sockaddr *)&iflr->dstaddr;
		if (sa->sa_family && sa->sa_family != AF_INET6)
			return EINVAL;
		if (sa->sa_len && sa->sa_len != sizeof(struct sockaddr_in6))
			return EINVAL;
		break;
	default: /* shouldn't happen */
#if 0
		panic("invalid cmd to in6_lifaddr_ioctl");
		/* NOTREACHED */
#else
		return EOPNOTSUPP;
#endif
	}
	if (sizeof(struct in6_addr) * NBBY < iflr->prefixlen)
		return EINVAL;

	switch (cmd) {
	case SIOCALIFADDR:
	    {
		struct in6_aliasreq ifra;
		struct in6_addr *xhostid = NULL;
		int prefixlen;
		int bound = curlwp_bind();
		struct psref psref;

		if ((iflr->flags & IFLR_PREFIX) != 0) {
			struct sockaddr_in6 *sin6;

			/*
			 * xhostid is to fill in the hostid part of the
			 * address.  xhostid points to the first link-local
			 * address attached to the interface.
			 */
			ia = in6ifa_ifpforlinklocal_psref(ifp, 0, &psref);
			if (ia == NULL) {
				curlwp_bindx(bound);
				return EADDRNOTAVAIL;
			}
			xhostid = IFA_IN6(&ia->ia_ifa);

		 	/* prefixlen must be <= 64. */
			if (64 < iflr->prefixlen) {
				ia6_release(ia, &psref);
				curlwp_bindx(bound);
				return EINVAL;
			}
			prefixlen = iflr->prefixlen;

			/* hostid part must be zero. */
			sin6 = (struct sockaddr_in6 *)&iflr->addr;
			if (sin6->sin6_addr.s6_addr32[2] != 0
			 || sin6->sin6_addr.s6_addr32[3] != 0) {
				ia6_release(ia, &psref);
				curlwp_bindx(bound);
				return EINVAL;
			}
		} else
			prefixlen = iflr->prefixlen;

		/* copy args to in6_aliasreq, perform ioctl(SIOCAIFADDR_IN6). */
		memset(&ifra, 0, sizeof(ifra));
		memcpy(ifra.ifra_name, iflr->iflr_name, sizeof(ifra.ifra_name));

		memcpy(&ifra.ifra_addr, &iflr->addr,
		    ((struct sockaddr *)&iflr->addr)->sa_len);
		if (xhostid) {
			/* fill in hostid part */
			ifra.ifra_addr.sin6_addr.s6_addr32[2] =
			    xhostid->s6_addr32[2];
			ifra.ifra_addr.sin6_addr.s6_addr32[3] =
			    xhostid->s6_addr32[3];
		}

		if (((struct sockaddr *)&iflr->dstaddr)->sa_family) { /* XXX */
			memcpy(&ifra.ifra_dstaddr, &iflr->dstaddr,
			    ((struct sockaddr *)&iflr->dstaddr)->sa_len);
			if (xhostid) {
				ifra.ifra_dstaddr.sin6_addr.s6_addr32[2] =
				    xhostid->s6_addr32[2];
				ifra.ifra_dstaddr.sin6_addr.s6_addr32[3] =
				    xhostid->s6_addr32[3];
			}
		}
		if (xhostid) {
			ia6_release(ia, &psref);
			ia = NULL;
		}
		curlwp_bindx(bound);

		ifra.ifra_prefixmask.sin6_len = sizeof(struct sockaddr_in6);
		in6_prefixlen2mask(&ifra.ifra_prefixmask.sin6_addr, prefixlen);

		ifra.ifra_lifetime.ia6t_vltime = ND6_INFINITE_LIFETIME;
		ifra.ifra_lifetime.ia6t_pltime = ND6_INFINITE_LIFETIME;
		ifra.ifra_flags = iflr->flags & ~IFLR_PREFIX;
		return in6_control(so, SIOCAIFADDR_IN6, &ifra, ifp);
	    }
	case SIOCGLIFADDR:
	case SIOCDLIFADDR:
	    {
		struct in6_addr mask, candidate, match;
		struct sockaddr_in6 *sin6;
		int cmp;
		int error, s;

		memset(&mask, 0, sizeof(mask));
		if (iflr->flags & IFLR_PREFIX) {
			/* lookup a prefix rather than address. */
			in6_prefixlen2mask(&mask, iflr->prefixlen);

			sin6 = (struct sockaddr_in6 *)&iflr->addr;
			memcpy(&match, &sin6->sin6_addr, sizeof(match));
			match.s6_addr32[0] &= mask.s6_addr32[0];
			match.s6_addr32[1] &= mask.s6_addr32[1];
			match.s6_addr32[2] &= mask.s6_addr32[2];
			match.s6_addr32[3] &= mask.s6_addr32[3];

			/* if you set extra bits, that's wrong */
			if (memcmp(&match, &sin6->sin6_addr, sizeof(match)))
				return EINVAL;

			cmp = 1;
		} else {
			if (cmd == SIOCGLIFADDR) {
				/* on getting an address, take the 1st match */
				cmp = 0;	/* XXX */
			} else {
				/* on deleting an address, do exact match */
				in6_prefixlen2mask(&mask, 128);
				sin6 = (struct sockaddr_in6 *)&iflr->addr;
				memcpy(&match, &sin6->sin6_addr, sizeof(match));

				cmp = 1;
			}
		}

		s = pserialize_read_enter();
		IFADDR_READER_FOREACH(ifa, ifp) {
			if (ifa->ifa_addr->sa_family != AF_INET6)
				continue;
			if (!cmp)
				break;

			/*
			 * XXX: this is adhoc, but is necessary to allow
			 * a user to specify fe80::/64 (not /10) for a
			 * link-local address.
			 */
			memcpy(&candidate, IFA_IN6(ifa), sizeof(candidate));
			in6_clearscope(&candidate);
			candidate.s6_addr32[0] &= mask.s6_addr32[0];
			candidate.s6_addr32[1] &= mask.s6_addr32[1];
			candidate.s6_addr32[2] &= mask.s6_addr32[2];
			candidate.s6_addr32[3] &= mask.s6_addr32[3];
			if (IN6_ARE_ADDR_EQUAL(&candidate, &match))
				break;
		}
		if (!ifa) {
			error = EADDRNOTAVAIL;
			goto error;
		}
		ia = ifa2ia6(ifa);

		if (cmd == SIOCGLIFADDR) {
			/* fill in the if_laddrreq structure */
			memcpy(&iflr->addr, &ia->ia_addr, ia->ia_addr.sin6_len);
			error = sa6_recoverscope(
			    (struct sockaddr_in6 *)&iflr->addr);
			if (error != 0)
				goto error;

			if ((ifp->if_flags & IFF_POINTOPOINT) != 0) {
				memcpy(&iflr->dstaddr, &ia->ia_dstaddr,
				    ia->ia_dstaddr.sin6_len);
				error = sa6_recoverscope(
				    (struct sockaddr_in6 *)&iflr->dstaddr);
				if (error != 0)
					goto error;
			} else
				memset(&iflr->dstaddr, 0, sizeof(iflr->dstaddr));

			iflr->prefixlen =
			    in6_mask2len(&ia->ia_prefixmask.sin6_addr, NULL);

			iflr->flags = ia->ia6_flags;	/* XXX */

			error = 0;
		} else {
			struct in6_aliasreq ifra;

			/* fill in6_aliasreq and do ioctl(SIOCDIFADDR_IN6) */
			memset(&ifra, 0, sizeof(ifra));
			memcpy(ifra.ifra_name, iflr->iflr_name,
			    sizeof(ifra.ifra_name));

			memcpy(&ifra.ifra_addr, &ia->ia_addr,
			    ia->ia_addr.sin6_len);
			if ((ifp->if_flags & IFF_POINTOPOINT) != 0) {
				memcpy(&ifra.ifra_dstaddr, &ia->ia_dstaddr,
				    ia->ia_dstaddr.sin6_len);
			} else {
				memset(&ifra.ifra_dstaddr, 0,
				    sizeof(ifra.ifra_dstaddr));
			}
			memcpy(&ifra.ifra_dstaddr, &ia->ia_prefixmask,
			    ia->ia_prefixmask.sin6_len);

			ifra.ifra_flags = ia->ia6_flags;
			pserialize_read_exit(s);

			return in6_control(so, SIOCDIFADDR_IN6, &ifra, ifp);
		}
	error:
		pserialize_read_exit(s);
		return error;
	    }
	}

	return EOPNOTSUPP;	/* just for safety */
}

/*
 * Initialize an interface's internet6 address
 * and routing table entry.
 */
static int
in6_ifinit(struct ifnet *ifp, struct in6_ifaddr *ia, 
	const struct sockaddr_in6 *sin6, int newhost)
{
	int	error = 0, ifacount = 0;
	int	s = splsoftnet();
	struct ifaddr *ifa;

	/*
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		ifacount++;
	}

	ia->ia_addr = *sin6;

	if (ifacount <= 0 &&
	    (error = if_addr_init(ifp, &ia->ia_ifa, true)) != 0) {
		splx(s);
		return error;
	}
	splx(s);

	ia->ia_ifa.ifa_metric = ifp->if_metric;

	/* we could do in(6)_socktrim here, but just omit it at this moment. */

	/* Add ownaddr as loopback rtentry, if necessary (ex. on p2p link). */
	if (newhost) {
		/* set the rtrequest function to create llinfo */
		if (ifp->if_flags & IFF_POINTOPOINT)
			ia->ia_ifa.ifa_rtrequest = p2p_rtrequest;
		else if ((ifp->if_flags & IFF_LOOPBACK) == 0)
			ia->ia_ifa.ifa_rtrequest = nd6_rtrequest;
		in6_ifaddlocal(&ia->ia_ifa);
	} else {
		/* Inform the routing socket of new flags/timings */
		rt_newaddrmsg(RTM_NEWADDR, &ia->ia_ifa, 0, NULL);
	}

	/* Add the network prefix route. */
	if ((error = in6_ifaddprefix(ia)) != 0) {
		if (newhost)
			in6_ifremlocal(&ia->ia_ifa);
		return error;
	}

	return error;
}

static struct ifaddr *
bestifa(struct ifaddr *best_ifa, struct ifaddr *ifa)
{
	if (best_ifa == NULL || best_ifa->ifa_preference < ifa->ifa_preference)
		return ifa;
	return best_ifa;
}

/*
 * Find an IPv6 interface link-local address specific to an interface.
 */
struct in6_ifaddr *
in6ifa_ifpforlinklocal(const struct ifnet *ifp, const int ignoreflags)
{
	struct ifaddr *best_ifa = NULL, *ifa;

	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		if (!IN6_IS_ADDR_LINKLOCAL(IFA_IN6(ifa)))
			continue;
		if ((((struct in6_ifaddr *)ifa)->ia6_flags & ignoreflags) != 0)
			continue;
		best_ifa = bestifa(best_ifa, ifa);
	}

	return (struct in6_ifaddr *)best_ifa;
}

struct in6_ifaddr *
in6ifa_ifpforlinklocal_psref(const struct ifnet *ifp, const int ignoreflags,
    struct psref *psref)
{
	struct in6_ifaddr *ia;
	int s = pserialize_read_enter();

	ia = in6ifa_ifpforlinklocal(ifp, ignoreflags);
	if (ia != NULL)
		ia6_acquire(ia, psref);
	pserialize_read_exit(s);

	return ia;
}

/*
 * find the internet address corresponding to a given address.
 * ifaddr is returned referenced.
 */
struct in6_ifaddr *
in6ifa_ifwithaddr(const struct in6_addr *addr, uint32_t zoneid)
{
	struct in6_ifaddr *ia;
	int s;

	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (IN6_ARE_ADDR_EQUAL(IA6_IN6(ia), addr)) {
			if (zoneid != 0 &&
			    zoneid != ia->ia_addr.sin6_scope_id)
				continue;
			ifaref(&ia->ia_ifa);
			break;
		}
	}
	pserialize_read_exit(s);

	return ia;
}

/*
 * find the internet address corresponding to a given interface and address.
 */
struct in6_ifaddr *
in6ifa_ifpwithaddr(const struct ifnet *ifp, const struct in6_addr *addr)
{
	struct ifaddr *best_ifa = NULL, *ifa;

	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		if (!IN6_ARE_ADDR_EQUAL(addr, IFA_IN6(ifa)))
			continue;
		best_ifa = bestifa(best_ifa, ifa);
	}

	return (struct in6_ifaddr *)best_ifa;
}

struct in6_ifaddr *
in6ifa_ifpwithaddr_psref(const struct ifnet *ifp, const struct in6_addr *addr,
    struct psref *psref)
{
	struct in6_ifaddr *ia;
	int s = pserialize_read_enter();

	ia = in6ifa_ifpwithaddr(ifp, addr);
	if (ia != NULL)
		ia6_acquire(ia, psref);
	pserialize_read_exit(s);

	return ia;
}

static struct in6_ifaddr *
bestia(struct in6_ifaddr *best_ia, struct in6_ifaddr *ia)
{
	if (best_ia == NULL ||
	    best_ia->ia_ifa.ifa_preference < ia->ia_ifa.ifa_preference)
		return ia;
	return best_ia;
}

/*
 * Determine if an address is on a local network.
 */
int
in6_localaddr(const struct in6_addr *in6)
{
	struct in6_ifaddr *ia;
	int s;

	if (IN6_IS_ADDR_LOOPBACK(in6) || IN6_IS_ADDR_LINKLOCAL(in6))
		return 1;

	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (IN6_ARE_MASKED_ADDR_EQUAL(in6, &ia->ia_addr.sin6_addr,
					      &ia->ia_prefixmask.sin6_addr)) {
			pserialize_read_exit(s);
			return 1;
		}
	}
	pserialize_read_exit(s);

	return 0;
}

int
in6_is_addr_deprecated(struct sockaddr_in6 *sa6)
{
	struct in6_ifaddr *ia;
	int s;

	s = pserialize_read_enter();
	IN6_ADDRLIST_READER_FOREACH(ia) {
		if (IN6_ARE_ADDR_EQUAL(&ia->ia_addr.sin6_addr,
		    &sa6->sin6_addr) &&
#ifdef SCOPEDROUTING
		    ia->ia_addr.sin6_scope_id == sa6->sin6_scope_id &&
#endif
		    (ia->ia6_flags & IN6_IFF_DEPRECATED) != 0) {
			pserialize_read_exit(s);
			return 1; /* true */
		}

		/* XXX: do we still have to go thru the rest of the list? */
	}
	pserialize_read_exit(s);

	return 0;		/* false */
}

/*
 * return length of part which dst and src are equal
 * hard coding...
 */
int
in6_matchlen(struct in6_addr *src, struct in6_addr *dst)
{
	int match = 0;
	u_char *s = (u_char *)src, *d = (u_char *)dst;
	u_char *lim = s + 16, r;

	while (s < lim)
		if ((r = (*d++ ^ *s++)) != 0) {
			while (r < 128) {
				match++;
				r <<= 1;
			}
			break;
		} else
			match += NBBY;
	return match;
}

/* XXX: to be scope conscious */
int
in6_are_prefix_equal(struct in6_addr *p1, struct in6_addr *p2, int len)
{
	int bytelen, bitlen;

	/* sanity check */
	if (len < 0 || len > 128) {
		log(LOG_ERR, "in6_are_prefix_equal: invalid prefix length(%d)\n",
		    len);
		return 0;
	}

	bytelen = len / NBBY;
	bitlen = len % NBBY;

	if (memcmp(&p1->s6_addr, &p2->s6_addr, bytelen))
		return 0;
	if (bitlen != 0 &&
	    p1->s6_addr[bytelen] >> (NBBY - bitlen) !=
	    p2->s6_addr[bytelen] >> (NBBY - bitlen))
		return 0;

	return 1;
}

void
in6_prefixlen2mask(struct in6_addr *maskp, int len)
{
	static const u_char maskarray[NBBY] = {0x80, 0xc0, 0xe0, 0xf0, 0xf8, 0xfc, 0xfe, 0xff};
	int bytelen, bitlen, i;

	/* sanity check */
	if (len < 0 || len > 128) {
		log(LOG_ERR, "in6_prefixlen2mask: invalid prefix length(%d)\n",
		    len);
		return;
	}

	memset(maskp, 0, sizeof(*maskp));
	bytelen = len / NBBY;
	bitlen = len % NBBY;
	for (i = 0; i < bytelen; i++)
		maskp->s6_addr[i] = 0xff;
	if (bitlen)
		maskp->s6_addr[bytelen] = maskarray[bitlen - 1];
}

/*
 * return the best address out of the same scope. if no address was
 * found, return the first valid address from designated IF.
 */
struct in6_ifaddr *
in6_ifawithifp(struct ifnet *ifp, struct in6_addr *dst)
{
	int dst_scope =	in6_addrscope(dst), blen = -1, tlen;
	struct ifaddr *ifa;
	struct in6_ifaddr *best_ia = NULL, *ia;
	struct in6_ifaddr *dep[2];	/* last-resort: deprecated */

	dep[0] = dep[1] = NULL;

	/*
	 * We first look for addresses in the same scope.
	 * If there is one, return it.
	 * If two or more, return one which matches the dst longest.
	 * If none, return one of global addresses assigned other ifs.
	 */
	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		ia = (struct in6_ifaddr *)ifa;
		if (ia->ia6_flags & IN6_IFF_ANYCAST)
			continue; /* XXX: is there any case to allow anycast? */
		if (ia->ia6_flags & IN6_IFF_NOTREADY)
			continue; /* don't use this interface */
		if (ia->ia6_flags & IN6_IFF_DETACHED)
			continue;
		if (ia->ia6_flags & IN6_IFF_DEPRECATED) {
			if (ip6_use_deprecated)
				dep[0] = ia;
			continue;
		}

		if (dst_scope != in6_addrscope(IFA_IN6(ifa)))
			continue;
		/*
		 * call in6_matchlen() as few as possible
		 */
		if (best_ia == NULL) {
			best_ia = ia;
			continue;
		}
		if (blen == -1)
			blen = in6_matchlen(&best_ia->ia_addr.sin6_addr, dst);
		tlen = in6_matchlen(IFA_IN6(ifa), dst);
		if (tlen > blen) {
			blen = tlen;
			best_ia = ia;
		} else if (tlen == blen)
			best_ia = bestia(best_ia, ia);
	}
	if (best_ia != NULL)
		return best_ia;

	IFADDR_READER_FOREACH(ifa, ifp) {
		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;
		ia = (struct in6_ifaddr *)ifa;
		if (ia->ia6_flags & IN6_IFF_ANYCAST)
			continue; /* XXX: is there any case to allow anycast? */
		if (ia->ia6_flags & IN6_IFF_NOTREADY)
			continue; /* don't use this interface */
		if (ia->ia6_flags & IN6_IFF_DETACHED)
			continue;
		if (ia->ia6_flags & IN6_IFF_DEPRECATED) {
			if (ip6_use_deprecated)
				dep[1] = (struct in6_ifaddr *)ifa;
			continue;
		}

		best_ia = bestia(best_ia, ia);
	}
	if (best_ia != NULL)
		return best_ia;

	/* use the last-resort values, that are, deprecated addresses */
	if (dep[0])
		return dep[0];
	if (dep[1])
		return dep[1];

	return NULL;
}

/*
 * perform DAD when interface becomes IFF_UP.
 */
void
in6_if_link_up(struct ifnet *ifp)
{
	struct ifaddr *ifa;
	struct in6_ifaddr *ia;
	int s, bound;
	char ip6buf[INET6_ADDRSTRLEN];

	/* Ensure it's sane to run DAD */
	if (ifp->if_link_state == LINK_STATE_DOWN)
		return;
	if ((ifp->if_flags & (IFF_UP|IFF_RUNNING)) != (IFF_UP|IFF_RUNNING))
		return;

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		struct psref psref;

		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		ifa_acquire(ifa, &psref);
		pserialize_read_exit(s);
		ia = (struct in6_ifaddr *)ifa;

		/* If detached then mark as tentative */
		if (ia->ia6_flags & IN6_IFF_DETACHED) {
			ia->ia6_flags &= ~IN6_IFF_DETACHED;
			if (if_do_dad(ifp)) {
				ia->ia6_flags |= IN6_IFF_TENTATIVE;
				nd6log(LOG_ERR, "%s marked tentative\n",
				    IN6_PRINT(ip6buf,
				    &ia->ia_addr.sin6_addr));
			} else if ((ia->ia6_flags & IN6_IFF_TENTATIVE) == 0)
				rt_newaddrmsg(RTM_NEWADDR, ifa, 0, NULL);
		}

		if (ia->ia6_flags & IN6_IFF_TENTATIVE) {
			int rand_delay;

			/* Clear the duplicated flag as we're starting DAD. */
			ia->ia6_flags &= ~IN6_IFF_DUPLICATED;

			/*
			 * The TENTATIVE flag was likely set by hand
			 * beforehand, implicitly indicating the need for DAD.
			 * We may be able to skip the random delay in this
			 * case, but we impose delays just in case.
			 */
			rand_delay = cprng_fast32() %
			    (MAX_RTR_SOLICITATION_DELAY * hz);
			/* +1 ensures callout is always used */
			nd6_dad_start(ifa, rand_delay + 1);
		}

		s = pserialize_read_enter();
		ifa_release(ifa, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);

	/* Restore any detached prefixes */
	ND6_WLOCK();
	nd6_pfxlist_onlink_check();
	ND6_UNLOCK();
}

void
in6_if_up(struct ifnet *ifp)
{

	/*
	 * special cases, like 6to4, are handled in in6_ifattach
	 */
	in6_ifattach(ifp, NULL);

	/* interface may not support link state, so bring it up also */
	in6_if_link_up(ifp);
}

/*
 * Mark all addresses as detached.
 */
void
in6_if_link_down(struct ifnet *ifp)
{
	struct ifaddr *ifa;
	struct in6_ifaddr *ia;
	int s, bound;
	char ip6buf[INET6_ADDRSTRLEN];

	/* Any prefixes on this interface should be detached as well */
	ND6_WLOCK();
	nd6_pfxlist_onlink_check();
	ND6_UNLOCK();

	bound = curlwp_bind();
	s = pserialize_read_enter();
	IFADDR_READER_FOREACH(ifa, ifp) {
		struct psref psref;

		if (ifa->ifa_addr->sa_family != AF_INET6)
			continue;

		ifa_acquire(ifa, &psref);
		pserialize_read_exit(s);
		ia = (struct in6_ifaddr *)ifa;

		/* Stop DAD processing */
		nd6_dad_stop(ifa);

		/*
		 * Mark the address as detached.
		 * This satisfies RFC4862 Section 5.3, but we should apply
		 * this logic to all addresses to be a good citizen and
		 * avoid potential duplicated addresses.
		 * When the interface comes up again, detached addresses
		 * are marked tentative and DAD commences.
		 */
		if (!(ia->ia6_flags & IN6_IFF_DETACHED)) {
			nd6log(LOG_DEBUG, "%s marked detached\n",
			    IN6_PRINT(ip6buf, &ia->ia_addr.sin6_addr));
			ia->ia6_flags |= IN6_IFF_DETACHED;
			ia->ia6_flags &=
			    ~(IN6_IFF_TENTATIVE | IN6_IFF_DUPLICATED);
			rt_newaddrmsg(RTM_NEWADDR, ifa, 0, NULL);
		}

		s = pserialize_read_enter();
		ifa_release(ifa, &psref);
	}
	pserialize_read_exit(s);
	curlwp_bindx(bound);
}

void
in6_if_down(struct ifnet *ifp)
{

	in6_if_link_down(ifp);
	lltable_purge_entries(LLTABLE6(ifp));
}

void
in6_if_link_state_change(struct ifnet *ifp, int link_state)
{

	switch (link_state) {
	case LINK_STATE_DOWN:
		in6_if_link_down(ifp);
		break;
	case LINK_STATE_UP:
		in6_if_link_up(ifp);
		break;
	}
}

/*
 * Calculate max IPv6 MTU through all the interfaces and store it
 * to in6_maxmtu.
 */
void
in6_setmaxmtu(void)
{
	unsigned long maxmtu = 0;
	struct ifnet *ifp;
	int s;

	s = pserialize_read_enter();
	IFNET_READER_FOREACH(ifp) {
		/* this function can be called during ifnet initialization */
		if (!ifp->if_afdata[AF_INET6])
			continue;
		if ((ifp->if_flags & IFF_LOOPBACK) == 0 &&
		    IN6_LINKMTU(ifp) > maxmtu)
			maxmtu = IN6_LINKMTU(ifp);
	}
	pserialize_read_exit(s);
	if (maxmtu)	     /* update only when maxmtu is positive */
		in6_maxmtu = maxmtu;
}

/*
 * Provide the length of interface identifiers to be used for the link attached
 * to the given interface.  The length should be defined in "IPv6 over
 * xxx-link" document.  Note that address architecture might also define
 * the length for a particular set of address prefixes, regardless of the
 * link type.  As clarified in rfc2462bis, those two definitions should be
 * consistent, and those really are as of August 2004.
 */
int
in6_if2idlen(struct ifnet *ifp)
{
	switch (ifp->if_type) {
	case IFT_ETHER:		/* RFC2464 */
	case IFT_PROPVIRTUAL:	/* XXX: no RFC. treat it as ether */
	case IFT_L2VLAN:	/* ditto */
	case IFT_IEEE80211:	/* ditto */
	case IFT_FDDI:		/* RFC2467 */
	case IFT_ISO88025:	/* RFC2470 (IPv6 over Token Ring) */
	case IFT_PPP:		/* RFC2472 */
	case IFT_ARCNET:	/* RFC2497 */
	case IFT_FRELAY:	/* RFC2590 */
	case IFT_IEEE1394:	/* RFC3146 */
	case IFT_GIF:		/* draft-ietf-v6ops-mech-v2-07 */
	case IFT_LOOP:		/* XXX: is this really correct? */
		return 64;
	default:
		/*
		 * Unknown link type:
		 * It might be controversial to use the today's common constant
		 * of 64 for these cases unconditionally.  For full compliance,
		 * we should return an error in this case.  On the other hand,
		 * if we simply miss the standard for the link type or a new
		 * standard is defined for a new link type, the IFID length
		 * is very likely to be the common constant.  As a compromise,
		 * we always use the constant, but make an explicit notice
		 * indicating the "unknown" case.
		 */
		printf("in6_if2idlen: unknown link type (%d)\n", ifp->if_type);
		return 64;
	}
}

struct in6_llentry {
	struct llentry		base;
};

#define	IN6_LLTBL_DEFAULT_HSIZE	32
#define	IN6_LLTBL_HASH(k, h) \
	(((((((k >> 8) ^ k) >> 8) ^ k) >> 8) ^ k) & ((h) - 1))

/*
 * Do actual deallocation of @lle.
 * Called by LLE_FREE_LOCKED when number of references
 * drops to zero.
 */
static void
in6_lltable_destroy_lle(struct llentry *lle)
{

	LLE_WUNLOCK(lle);
	LLE_LOCK_DESTROY(lle);
	kmem_intr_free(lle, sizeof(struct in6_llentry));
}

static struct llentry *
in6_lltable_new(const struct in6_addr *addr6, u_int flags)
{
	struct in6_llentry *lle;

	lle = kmem_intr_zalloc(sizeof(struct in6_llentry), KM_NOSLEEP);
	if (lle == NULL)		/* NB: caller generates msg */
		return NULL;

	lle->base.r_l3addr.addr6 = *addr6;
	lle->base.lle_refcnt = 1;
	lle->base.lle_free = in6_lltable_destroy_lle;
	LLE_LOCK_INIT(&lle->base);
	callout_init(&lle->base.lle_timer, CALLOUT_MPSAFE);

	return &lle->base;
}

static int
in6_lltable_match_prefix(const struct sockaddr *prefix,
    const struct sockaddr *mask, u_int flags, struct llentry *lle)
{
	const struct sockaddr_in6 *pfx = (const struct sockaddr_in6 *)prefix;
	const struct sockaddr_in6 *msk = (const struct sockaddr_in6 *)mask;

	if (IN6_ARE_MASKED_ADDR_EQUAL(&lle->r_l3addr.addr6,
	    &pfx->sin6_addr, &msk->sin6_addr) &&
	    ((flags & LLE_STATIC) || !(lle->la_flags & LLE_STATIC)))
		return 1;

	return 0;
}

static void
in6_lltable_free_entry(struct lltable *llt, struct llentry *lle)
{
	struct ifnet *ifp = llt->llt_ifp;
	bool locked = false;

	LLE_WLOCK_ASSERT(lle);

	/* Unlink entry from table */
	if ((lle->la_flags & LLE_LINKED) != 0) {
		IF_AFDATA_WLOCK_ASSERT(ifp);
		lltable_unlink_entry(llt, lle);
		KASSERT((lle->la_flags & LLE_LINKED) == 0);
		locked = true;
	}
	/*
	 * We need to release the lock here to lle_timer proceeds;
	 * lle_timer should stop immediately if LLE_LINKED isn't set.
	 * Note that we cannot pass lle->lle_lock to callout_halt
	 * because it's a rwlock.
	 */
	LLE_ADDREF(lle);
	LLE_WUNLOCK(lle);
	if (locked)
		IF_AFDATA_WUNLOCK(ifp);

#ifdef NET_MPSAFE
	callout_halt(&lle->lle_timer, NULL);
#else
	if (mutex_owned(softnet_lock))
		callout_halt(&lle->lle_timer, softnet_lock);
	else
		callout_halt(&lle->lle_timer, NULL);
#endif
	LLE_WLOCK(lle);
	LLE_REMREF(lle);

	lltable_drop_entry_queue(lle);
	LLE_FREE_LOCKED(lle);

	if (locked)
		IF_AFDATA_WLOCK(ifp);
}

static int
in6_lltable_rtcheck(struct ifnet *ifp, u_int flags,
    const struct sockaddr *l3addr, const struct rtentry *rt)
{
	char ip6buf[INET6_ADDRSTRLEN];

	if (rt == NULL || (rt->rt_flags & RTF_GATEWAY) || rt->rt_ifp != ifp) {
		int s;
		struct ifaddr *ifa;
		/*
		 * Create an ND6 cache for an IPv6 neighbor
		 * that is not covered by our own prefix.
		 */
		/* XXX ifaof_ifpforaddr should take a const param */
		s = pserialize_read_enter();
		ifa = ifaof_ifpforaddr(l3addr, ifp);
		if (ifa != NULL) {
			pserialize_read_exit(s);
			return 0;
		}
		pserialize_read_exit(s);
		log(LOG_INFO, "IPv6 address: \"%s\" is not on the network\n",
		    IN6_PRINT(ip6buf,
		    &((const struct sockaddr_in6 *)l3addr)->sin6_addr));
		return EINVAL;
	}
	return 0;
}

static inline uint32_t
in6_lltable_hash_dst(const struct in6_addr *dst, uint32_t hsize)
{

	return IN6_LLTBL_HASH(dst->s6_addr32[3], hsize);
}

static uint32_t
in6_lltable_hash(const struct llentry *lle, uint32_t hsize)
{

	return in6_lltable_hash_dst(&lle->r_l3addr.addr6, hsize);
}

static void
in6_lltable_fill_sa_entry(const struct llentry *lle, struct sockaddr *sa)
{
	struct sockaddr_in6 *sin6;

	sin6 = (struct sockaddr_in6 *)sa;
	bzero(sin6, sizeof(*sin6));
	sin6->sin6_family = AF_INET6;
	sin6->sin6_len = sizeof(*sin6);
	sin6->sin6_addr = lle->r_l3addr.addr6;
}

static inline struct llentry *
in6_lltable_find_dst(struct lltable *llt, const struct in6_addr *dst)
{
	struct llentry *lle;
	struct llentries *lleh;
	u_int hashidx;

	hashidx = in6_lltable_hash_dst(dst, llt->llt_hsize);
	lleh = &llt->lle_head[hashidx];
	LIST_FOREACH(lle, lleh, lle_next) {
		if (lle->la_flags & LLE_DELETED)
			continue;
		if (IN6_ARE_ADDR_EQUAL(&lle->r_l3addr.addr6, dst))
			break;
	}

	return lle;
}

static int
in6_lltable_delete(struct lltable *llt, u_int flags,
	const struct sockaddr *l3addr)
{
	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)l3addr;
	struct llentry *lle;

	IF_AFDATA_WLOCK_ASSERT(llt->llt_ifp);
	KASSERTMSG(l3addr->sa_family == AF_INET6,
	    "sin_family %d", l3addr->sa_family);

	lle = in6_lltable_find_dst(llt, &sin6->sin6_addr);

	if (lle == NULL) {
#ifdef DEBUG
		char buf[64];
		sockaddr_format(l3addr, buf, sizeof(buf));
		log(LOG_INFO, "%s: cache for %s is not found\n",
		    __func__, buf);
#endif
		return ENOENT;
	}

	LLE_WLOCK(lle);
	lle->la_flags |= LLE_DELETED;
#ifdef DEBUG
	{
		char buf[64];
		sockaddr_format(l3addr, buf, sizeof(buf));
		log(LOG_INFO, "%s: cache for %s (%p) is deleted\n",
		    __func__, buf, lle);
	}
#endif
	if ((lle->la_flags & (LLE_STATIC | LLE_IFADDR)) == LLE_STATIC)
		llentry_free(lle);
	else
		LLE_WUNLOCK(lle);

	return 0;
}

static struct llentry *
in6_lltable_create(struct lltable *llt, u_int flags,
    const struct sockaddr *l3addr, const struct rtentry *rt)
{
	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)l3addr;
	struct ifnet *ifp = llt->llt_ifp;
	struct llentry *lle;

	IF_AFDATA_WLOCK_ASSERT(ifp);
	KASSERTMSG(l3addr->sa_family == AF_INET6,
	    "sin_family %d", l3addr->sa_family);

	lle = in6_lltable_find_dst(llt, &sin6->sin6_addr);

	if (lle != NULL) {
		LLE_WLOCK(lle);
		return lle;
	}

	/*
	 * A route that covers the given address must have
	 * been installed 1st because we are doing a resolution,
	 * verify this.
	 */
	if (!(flags & LLE_IFADDR) &&
	    in6_lltable_rtcheck(ifp, flags, l3addr, rt) != 0)
		return NULL;

	lle = in6_lltable_new(&sin6->sin6_addr, flags);
	if (lle == NULL) {
		log(LOG_INFO, "lla_lookup: new lle malloc failed\n");
		return NULL;
	}
	lle->la_flags = flags;
	if ((flags & LLE_IFADDR) == LLE_IFADDR) {
		memcpy(&lle->ll_addr, CLLADDR(ifp->if_sadl), ifp->if_addrlen);
		lle->la_flags |= LLE_VALID;
	}

	lltable_link_entry(llt, lle);
	LLE_WLOCK(lle);

	return lle;
}

static struct llentry *
in6_lltable_lookup(struct lltable *llt, u_int flags,
	const struct sockaddr *l3addr)
{
	const struct sockaddr_in6 *sin6 = (const struct sockaddr_in6 *)l3addr;
	struct llentry *lle;

	IF_AFDATA_LOCK_ASSERT(llt->llt_ifp);
	KASSERTMSG(l3addr->sa_family == AF_INET6,
	    "sin_family %d", l3addr->sa_family);

	lle = in6_lltable_find_dst(llt, &sin6->sin6_addr);

	if (lle == NULL)
		return NULL;

	if (flags & LLE_EXCLUSIVE)
		LLE_WLOCK(lle);
	else
		LLE_RLOCK(lle);
	return lle;
}

static int
in6_lltable_dump_entry(struct lltable *llt, struct llentry *lle,
    struct rt_walkarg *w)
{
	struct sockaddr_in6 sin6;

	LLTABLE_LOCK_ASSERT();

	/* skip deleted entries */
	if (lle->la_flags & LLE_DELETED)
		return 0;

	sockaddr_in6_init(&sin6, &lle->r_l3addr.addr6, 0, 0, 0);

	return lltable_dump_entry(llt, lle, w, sin6tosa(&sin6));
}

static struct lltable *
in6_lltattach(struct ifnet *ifp)
{
	struct lltable *llt;

	llt = lltable_allocate_htbl(IN6_LLTBL_DEFAULT_HSIZE);
	llt->llt_af = AF_INET6;
	llt->llt_ifp = ifp;

	llt->llt_lookup = in6_lltable_lookup;
	llt->llt_create = in6_lltable_create;
	llt->llt_delete = in6_lltable_delete;
	llt->llt_dump_entry = in6_lltable_dump_entry;
	llt->llt_hash = in6_lltable_hash;
	llt->llt_fill_sa_entry = in6_lltable_fill_sa_entry;
	llt->llt_free_entry = in6_lltable_free_entry;
	llt->llt_match_prefix = in6_lltable_match_prefix;
	lltable_link(llt);

	return llt;
}

void *
in6_domifattach(struct ifnet *ifp)
{
	struct in6_ifextra *ext;

	ext = malloc(sizeof(*ext), M_IFADDR, M_WAITOK|M_ZERO);

	ext->in6_ifstat = malloc(sizeof(struct in6_ifstat),
	    M_IFADDR, M_WAITOK|M_ZERO);

	ext->icmp6_ifstat = malloc(sizeof(struct icmp6_ifstat),
	    M_IFADDR, M_WAITOK|M_ZERO);

	ext->nd_ifinfo = nd6_ifattach(ifp);
	ext->scope6_id = scope6_ifattach(ifp);
	ext->nprefixes = 0;
	ext->ndefrouters = 0;

	ext->lltable = in6_lltattach(ifp);

	return ext;
}

void
in6_domifdetach(struct ifnet *ifp, void *aux)
{
	struct in6_ifextra *ext = (struct in6_ifextra *)aux;

	lltable_free(ext->lltable);
	ext->lltable = NULL;
	SOFTNET_LOCK_UNLESS_NET_MPSAFE();
	nd6_ifdetach(ifp, ext);
	SOFTNET_UNLOCK_UNLESS_NET_MPSAFE();
	free(ext->in6_ifstat, M_IFADDR);
	free(ext->icmp6_ifstat, M_IFADDR);
	scope6_ifdetach(ext->scope6_id);
	free(ext, M_IFADDR);
}

/*
 * Convert IPv4 address stored in struct in_addr to IPv4-Mapped IPv6 address
 * stored in struct in6_addr as defined in RFC 4921 section 2.5.5.2.
 */
void
in6_in_2_v4mapin6(const struct in_addr *in, struct in6_addr *in6)
{
	in6->s6_addr32[0] = 0;
	in6->s6_addr32[1] = 0;
	in6->s6_addr32[2] = IPV6_ADDR_INT32_SMP;
	in6->s6_addr32[3] = in->s_addr;
}

/*
 * Convert sockaddr_in6 to sockaddr_in.  Original sockaddr_in6 must be
 * v4 mapped addr or v4 compat addr
 */
void
in6_sin6_2_sin(struct sockaddr_in *sin, struct sockaddr_in6 *sin6)
{
	memset(sin, 0, sizeof(*sin));
	sin->sin_len = sizeof(struct sockaddr_in);
	sin->sin_family = AF_INET;
	sin->sin_port = sin6->sin6_port;
	sin->sin_addr.s_addr = sin6->sin6_addr.s6_addr32[3];
}

/* Convert sockaddr_in to sockaddr_in6 in v4 mapped addr format. */
void
in6_sin_2_v4mapsin6(const struct sockaddr_in *sin, struct sockaddr_in6 *sin6)
{
	memset(sin6, 0, sizeof(*sin6));
	sin6->sin6_len = sizeof(struct sockaddr_in6);
	sin6->sin6_family = AF_INET6;
	sin6->sin6_port = sin->sin_port;
	in6_in_2_v4mapin6(&sin->sin_addr, &sin6->sin6_addr);
}

/* Convert sockaddr_in6 into sockaddr_in. */
void
in6_sin6_2_sin_in_sock(struct sockaddr *nam)
{
	struct sockaddr_in *sin_p;
	struct sockaddr_in6 sin6;

	/*
	 * Save original sockaddr_in6 addr and convert it
	 * to sockaddr_in.
	 */
	sin6 = *(struct sockaddr_in6 *)nam;
	sin_p = (struct sockaddr_in *)nam;
	in6_sin6_2_sin(sin_p, &sin6);
}

/* Convert sockaddr_in into sockaddr_in6 in v4 mapped addr format. */
void
in6_sin_2_v4mapsin6_in_sock(struct sockaddr **nam)
{
	struct sockaddr_in *sin_p;
	struct sockaddr_in6 *sin6_p;

	sin6_p = malloc(sizeof(*sin6_p), M_SONAME, M_WAITOK);
	sin_p = (struct sockaddr_in *)*nam;
	in6_sin_2_v4mapsin6(sin_p, sin6_p);
	free(*nam, M_SONAME);
	*nam = sin6tosa(sin6_p);
}
