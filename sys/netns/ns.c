/*	$NetBSD: ns.c,v 1.28.6.1 2006/06/01 22:39:12 kardel Exp $	*/

/*
 * Copyright (c) 1984, 1985, 1986, 1987, 1993
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
 *	@(#)ns.c	8.5 (Berkeley) 2/9/95
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: ns.c,v 1.28.6.1 2006/06/01 22:39:12 kardel Exp $");

#include <sys/param.h>
#include <sys/systm.h>
#include <sys/mbuf.h>
#include <sys/ioctl.h>
#include <sys/protosw.h>
#include <sys/errno.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/proc.h>
#include <sys/kauth.h>

#include <net/if.h>
#include <net/route.h>

#include <netns/ns.h>
#include <netns/ns_if.h>
#include <netns/ns_var.h>

struct ns_ifaddrhead ns_ifaddr;
int ns_interfaces;

/*
 * Generic internet control operations (ioctl's).
 */
/* ARGSUSED */
int
ns_control(struct socket *so, u_long cmd, caddr_t data, struct ifnet *ifp,
	struct proc *p)
{
	struct ifreq *ifr = (struct ifreq *)data;
	struct ns_ifaddr *ia = 0;
	struct ns_aliasreq *ifra = (struct ns_aliasreq *)data;
	struct sockaddr_ns oldaddr;
	int error = 0, dstIsNew, hostIsNew;

	/*
	 * Find address for this interface, if it exists.
	 */
	if (ifp)
		for (ia = ns_ifaddr.tqh_first; ia != 0; ia = ia->ia_list.tqe_next)
			if (ia->ia_ifp == ifp)
				break;

	switch (cmd) {

	case SIOCAIFADDR:
	case SIOCDIFADDR:
		if (ifra->ifra_addr.sns_family == AF_NS)
			for (; ia; ia = ia->ia_list.tqe_next) {
				if (ia->ia_ifp == ifp  &&
				    ns_neteq(ia->ia_addr.sns_addr, ifra->ifra_addr.sns_addr))
					break;
			}
		if (cmd == SIOCDIFADDR && ia == 0)
			return (EADDRNOTAVAIL);
		/* FALLTHROUGH */
	case SIOCSIFADDR:
	case SIOCSIFDSTADDR:
		if (p == 0 || (error = kauth_authorize_generic(p->p_cred,
							 KAUTH_GENERIC_ISSUSER,
							 &p->p_acflag)))
			return (EPERM);

		if (ifp == 0)
			panic("ns_control");
		if (ia == 0) {
			MALLOC(ia, struct ns_ifaddr *, sizeof(*ia),
			       M_IFADDR, M_WAITOK|M_ZERO);
			if (ia == 0)
				return (ENOBUFS);
			TAILQ_INSERT_TAIL(&ns_ifaddr, ia, ia_list);
			IFAREF((struct ifaddr *)ia);
			TAILQ_INSERT_TAIL(&ifp->if_addrlist, (struct ifaddr *)ia,
			    ifa_list);
			IFAREF((struct ifaddr *)ia);
			ia->ia_ifa.ifa_addr = snstosa(&ia->ia_addr);
			ia->ia_ifa.ifa_netmask = snstosa(&ns_netmask);
			ia->ia_ifa.ifa_dstaddr = snstosa(&ia->ia_dstaddr);
			if (ifp->if_flags & IFF_BROADCAST) {
				ia->ia_broadaddr.sns_len = sizeof(ia->ia_addr);
				ia->ia_broadaddr.sns_family = AF_NS;
				ia->ia_broadaddr.sns_addr.x_host = ns_broadhost;
			}
			ia->ia_ifp = ifp;
			if ((ifp->if_flags & IFF_LOOPBACK) == 0)
				ns_interfaces++;
		}
		break;

	case SIOCGIFADDR:
	case SIOCGIFDSTADDR:
	case SIOCGIFBRDADDR:
		if (ia == 0)
			return (EADDRNOTAVAIL);
		break;
	}
	switch (cmd) {

	case SIOCGIFADDR:
		*satosns(&ifr->ifr_addr) = ia->ia_addr;
		break;

	case SIOCGIFBRDADDR:
		if ((ifp->if_flags & IFF_BROADCAST) == 0)
			return (EINVAL);
		*satosns(&ifr->ifr_dstaddr) = ia->ia_broadaddr;
		break;

	case SIOCGIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		*satosns(&ifr->ifr_dstaddr) = ia->ia_dstaddr;
		break;

	case SIOCSIFDSTADDR:
		if ((ifp->if_flags & IFF_POINTOPOINT) == 0)
			return (EINVAL);
		oldaddr = ia->ia_dstaddr;
		ia->ia_dstaddr = *satosns(&ifr->ifr_dstaddr);
		if (ifp->if_ioctl && (error = (*ifp->if_ioctl)
					(ifp, SIOCSIFDSTADDR, (caddr_t)ia))) {
			ia->ia_dstaddr = oldaddr;
			return (error);
		}
		if (ia->ia_flags & IFA_ROUTE) {
			ia->ia_ifa.ifa_dstaddr = snstosa(&oldaddr);
			rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
			ia->ia_ifa.ifa_dstaddr = snstosa(&ia->ia_dstaddr);
			rtinit(&(ia->ia_ifa), (int)RTM_ADD, RTF_HOST|RTF_UP);
		}
		break;

	case SIOCSIFADDR:
		return (ns_ifinit(ifp, ia, satosns(&ifr->ifr_addr), 1));

	case SIOCAIFADDR:
		dstIsNew = 0;
		hostIsNew = 1;
		if (ia->ia_addr.sns_family == AF_NS) {
			if (ifra->ifra_addr.sns_len == 0) {
				ifra->ifra_addr = ia->ia_addr;
				hostIsNew = 0;
			} else if (ns_neteq(ifra->ifra_addr.sns_addr, ia->ia_addr.sns_addr))
				hostIsNew = 0;
		}
		if ((ifp->if_flags & IFF_POINTOPOINT) &&
		    (ifra->ifra_dstaddr.sns_family == AF_NS)) {
			ns_ifscrub(ifp, ia);
			ia->ia_dstaddr = ifra->ifra_dstaddr;
			dstIsNew  = 1;
		}
		if (ifra->ifra_addr.sns_family == AF_NS &&
		    (hostIsNew || dstIsNew))
			error = ns_ifinit(ifp, ia, &ifra->ifra_addr, 0);
		return (error);

	case SIOCDIFADDR:
		ns_purgeaddr(&ia->ia_ifa, ifp);
		break;

	default:
		if (ifp == 0 || ifp->if_ioctl == 0)
			return (EOPNOTSUPP);
		return ((*ifp->if_ioctl)(ifp, cmd, data));
	}
	return (0);
}

void
ns_purgeaddr(struct ifaddr *ifa, struct ifnet *ifp)
{
	struct ns_ifaddr *ia = (void *) ifa;

	ns_ifscrub(ifp, ia);
	TAILQ_REMOVE(&ifp->if_addrlist, (struct ifaddr *)ia, ifa_list);
	IFAFREE(&ia->ia_ifa);
	TAILQ_REMOVE(&ns_ifaddr, ia, ia_list);
	IFAFREE((&ia->ia_ifa));
	if (0 == --ns_interfaces) {
		/*
		 * We reset to virginity and start all over again
		 */
		ns_thishost = ns_zerohost;
	}
}

void
ns_purgeif(struct ifnet *ifp)
{
	struct ifaddr *ifa, *nifa;

	for (ifa = TAILQ_FIRST(&ifp->if_addrlist); ifa != NULL; ifa = nifa) {
		nifa = TAILQ_NEXT(ifa, ifa_list);
		if (ifa->ifa_addr->sa_family != AF_NS)
			continue;
		ns_purgeaddr(ifa, ifp);
	}
}

/*
 * Delete any previous route for an old address.
 */
void
ns_ifscrub(struct ifnet *ifp, struct ns_ifaddr *ia)
{

	if ((ia->ia_flags & IFA_ROUTE) == 0)
		return;
	if (ifp->if_flags & (IFF_LOOPBACK|IFF_POINTOPOINT))
		rtinit(&(ia->ia_ifa), (int)RTM_DELETE, RTF_HOST);
	else
		rtinit(&(ia->ia_ifa), (int)RTM_DELETE, 0);
	ia->ia_flags &= ~IFA_ROUTE;
}
/*
 * Initialize an interface's internet address
 * and routing table entry.
 */
int
ns_ifinit(struct ifnet *ifp, struct ns_ifaddr *ia, struct sockaddr_ns *sns,
	int scrub)
{
	struct sockaddr_ns oldaddr;
	union ns_host *h = &ia->ia_addr.sns_addr.x_host;
	int s = splnet(), error;

	/*
	 * Set up new addresses.
	 */
	oldaddr = ia->ia_addr;
	ia->ia_addr = *sns;
	/*
	 * The convention we shall adopt for naming is that
	 * a supplied address of zero means that "we don't care".
	 * if there is a single interface, use the address of that
	 * interface as our 6 byte host address.
	 * if there are multiple interfaces, use any address already
	 * used.
	 *
	 * Give the interface a chance to initialize
	 * if this is its first address,
	 * and to validate the address if necessary.
	 */
	if (ns_hosteqnh(ns_thishost, ns_zerohost)) {
		if (ifp->if_ioctl &&
		     (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR,
		    (caddr_t)ia)))
			goto bad;
		ns_thishost = *h;
	} else if (ns_hosteqnh(sns->sns_addr.x_host, ns_zerohost)
	    || ns_hosteqnh(sns->sns_addr.x_host, ns_thishost)) {
		*h = ns_thishost;
		if (ifp->if_ioctl &&
		     (error = (*ifp->if_ioctl)(ifp, SIOCSIFADDR,
		    (caddr_t)ia)))
			goto bad;
		if (!ns_hosteqnh(ns_thishost, *h)) {
			error = EINVAL;
			goto bad;
		}
	} else {
		error = EINVAL;
		goto bad;
	}
	ia->ia_ifa.ifa_metric = ifp->if_metric;
	/*
	 * Add route for the network.
	 */
	if (scrub) {
		ia->ia_ifa.ifa_addr = snstosa(&oldaddr);
		ns_ifscrub(ifp, ia);
		ia->ia_ifa.ifa_addr = snstosa(&ia->ia_addr);
	}
	if (ifp->if_flags & IFF_POINTOPOINT)
		rtinit(&(ia->ia_ifa), (int)RTM_ADD, RTF_HOST|RTF_UP);
	else {
		ia->ia_broadaddr.sns_addr.x_net = ia->ia_addr.sns_addr.x_net;
		rtinit(&(ia->ia_ifa), (int)RTM_ADD, RTF_UP);
	}
	ia->ia_flags |= IFA_ROUTE;
	splx(s);
	return (0);
bad:
	ia->ia_addr = oldaddr;
	splx(s);
	return (error);
}

/*
 * Return address info for specified internet network.
 */
struct ns_ifaddr *
ns_iaonnetof(struct ns_addr *dst)
{
	struct ns_ifaddr *ia;
	struct ns_addr *compare;
	struct ifnet *ifp;
	struct ns_ifaddr *ia_maybe = 0;
	union ns_net net = dst->x_net;

	for (ia = ns_ifaddr.tqh_first; ia != 0; ia = ia->ia_list.tqe_next) {
		if ((ifp = ia->ia_ifp) != NULL) {
			if (ifp->if_flags & IFF_POINTOPOINT) {
				compare = &satons_addr(ia->ia_dstaddr);
				if (ns_hosteq(*dst, *compare))
					return (ia);
				if (ns_neteqnn(net, ia->ia_addr.sns_addr.x_net))
					ia_maybe = ia;
			} else {
				if (ns_neteqnn(net, ia->ia_addr.sns_addr.x_net))
					return (ia);
			}
		}
	}
	return (ia_maybe);
}
