/*	$NetBSD: if.c,v 1.47.6.1 1998/12/11 04:53:04 kenh Exp $	*/

/*
 * Copyright (c) 1980, 1986, 1993
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
 *	@(#)if.c	8.5 (Berkeley) 1/9/95
 */

#include "opt_compat_linux.h"
#include "opt_compat_svr4.h"

#include <sys/param.h>
#include <sys/mbuf.h>
#include <sys/systm.h>
#include <sys/proc.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/protosw.h>
#include <sys/kernel.h>
#include <sys/ioctl.h>

#include <net/if.h>
#include <net/if_dl.h>
#include <net/if_types.h>
#include <net/radix.h>
#include <net/route.h>

int	ifqmaxlen = IFQ_MAXLEN;
void	if_slowtimo __P((void *arg));

/*
 * Network interface utility routines.
 *
 * Routines with ifa_ifwith* names take sockaddr *'s as
 * parameters.
 */
void
ifinit()
{
	if_slowtimo(NULL);
}

#define	M_IFNET	M_TEMP
/*
 * Allocate an interface. Used so that interfaces can be
 * added & deleted on the fly.
 *
 * Note: calling routines should delete an ifnet by calling
 * if_delref when the refcount is one. It will automatically
 * call if_free on the ifnet.
 */
struct ifnet *
if_alloc(void)
{
	struct ifnet *ifp;

	ifp = (struct ifnet *)malloc( sizeof(struct ifnet),
					M_IFNET, M_WAITOK);
	if (ifp != NULL) {
		ifp->if_refcount = 1;
		ifp->if_iffree = if_free;
		memset(ifp, 0, sizeof(struct ifnet));
	}
	return ifp;
}
#if 0
/*
 * Set up refcount for an ifnet which isn't removable (fixed in place)
 */
void
if_fixedinit(ifp)
	struct ifnet *ifp;
{
	ifp->if_refcount = 1;
	ifp->if_iffree = if_nullfree;
}
#endif
/*
 * Free an interface. It must have no references at this point!
 */
void
if_free(ifp)
	struct ifnet *ifp;
{
	if (ifp->if_refcount != 0)
		panic("Trying to free an ifnet with refcount != 0");

	free(ifp, M_IFNET);
}
#if 0
/*
 * Null free routine for ifnets which aren't de-allocatable.
 */
void
if_nullfree(ifp)
	struct ifnet *ifp;
{
}
#endif
/*
 * Incriment the reference count on an ifnet
 */
void
if_addref(ifp)
	struct ifnet *ifp;
{
	ifp->if_refcount++;
}
/*
 * Decrement the reference count on an ifnet. Delete it if the
 * count goes to 0.
 */
void
if_delref(ifp)
	struct ifnet *ifp;
{
	if (ifp == 0)
		panic("Null ifp for delref!\n");
	if ((--ifp->if_refcount) <= 0)
		if (ifp->if_iffree)
			(*ifp->if_iffree)(ifp);
}
#ifdef _DEBUG_IFA_REF
static int lastadd = 0;
static char lastfile[64];
#include "opt_ddb.h"
#ifdef DDB
static int ifa_refcnt_debug = 0;
#endif
#endif

void
#ifdef _DEBUG_IFA_REF
ifa_addref1(ifa, f)
	char *f;
#else
ifa_addref(ifa)
#endif
	struct ifaddr *ifa;
{
	if (ifa == 0)
		panic("Null ifa for addref!\n");
#ifdef _DEBUG_IFA_REF
	if (lastadd == 1) {
		printf("Unbalanced add in %s\n     and in %s\n", lastfile, f);
#ifdef DDB
		if (ifa_refcnt_debug)
			Debugger();
#endif
	}
	lastadd = 1;
	strncpy(lastfile, f, 63);
#endif
	ifa->ifa_refcnt++;
}
void
#ifdef _DEBUG_IFA_REF
ifa_delref1(ifa, f)
	char *f;
#else
ifa_delref(ifa)
#endif
	struct ifaddr *ifa;
{
	if (ifa == 0)
		panic("Null ifa for delref!\n");
#ifdef _DEBUG_IFA_REF
	if ((lastadd != 1) || strcmp(lastfile, f)) {
		printf("Unbalancing del in %s\n     and %s\n", lastfile, f);
#ifdef DDB
		if (ifa_refcnt_debug)
			Debugger();
#endif
	}
	lastadd = 2;
	strncpy(lastfile, f, 63);
#endif
	if ((ifa->ifa_refcnt) <= 0)
		ifafree(ifa);
	else
		ifa->ifa_refcnt--;
}

/*
 * Null routines used while an interface is going away. These routines
 * just return an error.
 */
int  if_nulloutput	__P((struct ifnet *, struct mbuf *, \
				struct sockaddr *, struct rtentry *));
void if_nullstart	__P((struct ifnet *));
int  if_nullioctl	__P((struct ifnet *, u_long, caddr_t));
int  if_nullreset	__P((struct ifnet *));
void if_nullwatchdog	__P((struct ifnet *));

int
if_nulloutput(ifp, m, so, rt)
	struct ifnet *ifp;
	struct mbuf *m;
	struct sockaddr *so;
	struct rtentry *rt;
{
	return (ENXIO);
}
void
if_nullstart(ifp)
	struct ifnet *ifp;
{
	return;
}
int
if_nullioctl(ifp, cmd, data)
	struct ifnet *ifp;
	u_long cmd;
	caddr_t	data;
{
	return (ENXIO);
}
int
if_nullreset(ifp)
	struct ifnet *ifp;
{
	return (ENXIO);
}
void
if_nullwatchdog(ifp)
	struct ifnet *ifp;
{
	return;
}

int if_index = 0;
struct ifaddr **ifnet_addrs;

/*
 * Attach an interface to the
 * list of "active" interfaces.
 */
void
if_attach(ifp)
	struct ifnet *ifp;
{
	unsigned socksize, ifasize;
	int namelen, masklen;
	register struct sockaddr_dl *sdl;
	register struct ifaddr *ifa;
	static int if_indexlim = 8, s;

	s = splimp();
	if (if_index == 0)
		TAILQ_INIT(&ifnet);
	TAILQ_INIT(&ifp->if_addrlist);
	TAILQ_INSERT_TAIL(&ifnet, ifp, if_list);
	ifp->if_index = ++if_index;
	if (ifnet_addrs == 0 || if_index >= if_indexlim) {
		unsigned n = (if_indexlim <<= 1) * sizeof(ifa);
		struct ifaddr **q = (struct ifaddr **)
					malloc(n, M_IFADDR, M_WAITOK);
		if (ifnet_addrs) {
			bcopy((caddr_t)ifnet_addrs, (caddr_t)q, n/2);
			free((caddr_t)ifnet_addrs, M_IFADDR);
		}
		ifnet_addrs = q;
	}
	/*
	 * create a Link Level name for this device
	 */
	namelen = strlen(ifp->if_xname);
	masklen = offsetof(struct sockaddr_dl, sdl_data[0]) + namelen;
	socksize = masklen + ifp->if_addrlen;
#define ROUNDUP(a) (1 + (((a) - 1) | (sizeof(long) - 1)))
	if (socksize < sizeof(*sdl))
		socksize = sizeof(*sdl);
	socksize = ROUNDUP(socksize);
	ifasize = sizeof(*ifa) + 2 * socksize;
	ifa = (struct ifaddr *)malloc(ifasize, M_IFADDR, M_WAITOK);
	bzero((caddr_t)ifa, ifasize);
	sdl = (struct sockaddr_dl *)(ifa + 1);
	sdl->sdl_len = socksize;
	sdl->sdl_family = AF_LINK;
	bcopy(ifp->if_xname, sdl->sdl_data, namelen);
	sdl->sdl_nlen = namelen;
	sdl->sdl_index = ifp->if_index;
	sdl->sdl_type = ifp->if_type;
	ifnet_addrs[if_index - 1] = ifa;
	ifa->ifa_ifp = ifp;
	if_addref(ifp);
	ifa->ifa_rtrequest = link_rtrequest;
	TAILQ_INSERT_HEAD(&ifp->if_addrlist, ifa, ifa_list);
	ifa_addref(ifa); /* we add a ref for the copy in the linked list.
			  * We will ifa_delref twice in if_detach (once for
			  * a link level address, once for ifnet_addrs),
			  * which will free the ifa */
	ifa->ifa_addr = (struct sockaddr *)sdl;
	ifp->if_sadl = sdl;
	sdl = (struct sockaddr_dl *)(socksize + (caddr_t)sdl);
	ifa->ifa_netmask = (struct sockaddr *)sdl;
	sdl->sdl_len = masklen;
	while (namelen != 0)
		sdl->sdl_data[--namelen] = 0xff;
	if (ifp->if_snd.ifq_maxlen == 0)
	    ifp->if_snd.ifq_maxlen = ifqmaxlen;
	ifp->if_broadcastaddr = 0; /* reliably crash if used uninitialized */
	splx(s);
}
/*
 * if_rt_walktree is the callback for a radix tree walk
 * to delete all references to an ifnet
 */
int	if_rt_walktree __P((struct radix_node *, void *));
int
if_rt_walktree(rn, v)
	struct radix_node *rn;
	void	*v;
{
	struct ifnet *ifp = v;
	struct rtentry *rt = (struct rtentry *)rn;
	if (rt->rt_ifp == ifp) {
		/* actually delete rt */
		rtrequest(RTM_DELETE, (struct sockaddr *) rt_key(rt),
			  0, rt_mask(rt), 0, 0);
	}
	return 0;
}

/*
 * Deactivate an interface.  Right now, just point all of the network
 * functions to null.  Can be called in interrupt context.
 */

void
if_deactivate(ifp)
	struct ifnet *ifp;
{
	int s;

	s = splimp();

	ifp->if_output   = if_nulloutput;
	ifp->if_start    = if_nullstart;
	ifp->if_ioctl    = if_nullioctl;
	ifp->if_reset    = if_nullreset;
	ifp->if_watchdog = if_nullwatchdog;
	ifp->if_flags   |= IFF_DEAD;
	ifp->if_snd.ifq_maxlen = 0;	/* now the queue's always full */

	splx(s);
}

/*
 * Detach an interface from the
 * list of "active" interfaces. Free any space 
 * malloc'd above.
 */
void
if_detach(ifp)
	struct ifnet *ifp;
{
	/*
	 * Here's the idea. We tear as many things down as
	 * can. We start with the most dangling-of things.
	 *
	 * We do an if_down to try to get all the protocols
	 * to do something, but many protocols ignore the
	 * if_down. Oh well.
	 * Then we rip all of the addresses off. Hopefully that
	 * will kill all the routes. :-)
	 *
	 * The only last thing to do is kill any incoming packets pointing
	 * to this interface.
	 */
	int s, i;
	register struct ifaddr *ifa;
	struct protosw *pr;
	struct socket so;
	struct radix_node_head *rnh;

	so.so_type = SOCK_DGRAM;
	so.so_options = 0;		/* is all this needed? */
	so.so_linger = 0;
	so.so_state = SS_NOFDREF | SS_CANTSENDMORE | SS_CANTRCVMORE;
	so.so_pcb = 0;
	so.so_head = 0;
	TAILQ_INIT(&so.so_q0);
	TAILQ_INIT(&so.so_q);

	so.so_q0len = so.so_qlen = so.so_qlimit = 0;
	so.so_timeo = so.so_oobmark = 0;

	s = splimp();

	if_down(ifp);

	for( ; ifp->if_addrlist.tqh_first; ) {
		ifa = ifp->if_addrlist.tqh_first;
		if (ifa->ifa_addr->sa_family == AF_LINK) {
			rtinit(ifa, RTM_DELETE, 0);
			TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
			ifa_delref(ifa);
		} else {
			pr = pffindtype(ifa->ifa_addr->sa_family, SOCK_DGRAM);
			so.so_proto = pr;
			if (pr->pr_usrreq) {
				(*pr->pr_usrreq)(&so, PRU_CONTROL,
					(struct mbuf *) SIOCDIFADDR,
					(struct mbuf *) ifa,
					(struct mbuf *) ifp, &proc0);
			} else {
				rtinit(ifa, RTM_DELETE, 0);
				TAILQ_REMOVE(&ifp->if_addrlist, ifa, ifa_list);
				ifa_delref(ifa);
			}
		}
	}

	/* walk the routing table to look for straglers */
	for (i = 1; i <= AF_MAX; i++)
		if ((rnh = rt_tables[i]) && 
		    ((*rnh->rnh_walktree)(rnh, if_rt_walktree,
		    (void *)ifp)))
			break;

	ifa_delref(ifnet_addrs[ifp->if_index - 1]);
	ifnet_addrs[ifp->if_index - 1] = 0;
	if_delref(ifp);
	TAILQ_REMOVE(&ifnet, ifp, if_list);
	if_delref(ifp);

	splx(s);

	/*
	 * At this point, the only possable references are mbufs of
	 * packets that have recently come in. They will automatically
	 * drain out as the packets are processed, and the interface
	 * will finally be deleted when the last one's gone.
	 */
}
/*
 * Locate an interface based on a complete address.
 */
/*ARGSUSED*/
struct ifaddr *
#ifdef _DEBUG_IFA_REF
ifa_ifwithaddr1(addr, f)
	char *f;
#else
ifa_ifwithaddr(addr)
#endif
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register int s;

	s = splimp();
#define	equal(a1, a2) \
  (bcmp((caddr_t)(a1), (caddr_t)(a2), ((struct sockaddr *)(a1))->sa_len) == 0)
	for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next)
	    for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next) {
		if (ifa->ifa_addr->sa_family != addr->sa_family)
			continue;
		if (equal(addr, ifa->ifa_addr)) {
#ifdef _DEBUG_IFA_REF
			ifa_addref1(ifa, f);
#else
			ifa_addref(ifa);
#endif
			splx(s);
			return (ifa);
		}
		if ((ifp->if_flags & IFF_BROADCAST) && ifa->ifa_broadaddr &&
		    equal(ifa->ifa_broadaddr, addr)) {
#ifdef _DEBUG_IFA_REF
			ifa_addref1(ifa, f);
#else
			ifa_addref(ifa);
#endif
			splx(s);
			return (ifa);
		}
	}
	splx(s);
	return ((struct ifaddr *)0);
}
/*
 * Locate the point to point interface with a given destination address.
 */
/*ARGSUSED*/
struct ifaddr *
#ifdef _DEBUG_IFA_REF
ifa_ifwithdstaddr1(addr, f)
	char *f;
#else
ifa_ifwithdstaddr(addr)
#endif
	register struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register int s;

	s = splimp();
	for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next)
	    if (ifp->if_flags & IFF_POINTOPOINT)
		for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next) {
			if (ifa->ifa_addr->sa_family != addr->sa_family ||
			    ifa->ifa_dstaddr == NULL)
				continue;
			if (equal(addr, ifa->ifa_dstaddr)) {
#ifdef _DEBUG_IFA_REF
				ifa_addref1(ifa, f);
#else
				ifa_addref(ifa);
#endif
				splx(s);
				return (ifa);
			}
	}
	splx(s);
	return ((struct ifaddr *)0);
}

/*
 * Find an interface on a specific network.  If many, choice
 * is most specific found.
 */
struct ifaddr *
#ifdef _DEBUG_IFA_REF
ifa_ifwithnet1(addr, f)
	char *f;
#else
ifa_ifwithnet(addr)
#endif
	struct sockaddr *addr;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;
	char *addr_data = addr->sa_data, *cplim;
	register int s;

	s = splimp();
	if (af == AF_LINK) {
	    register struct sockaddr_dl *sdl = (struct sockaddr_dl *)addr;
	    if (sdl->sdl_index && sdl->sdl_index <= if_index) {
		ifa = ifnet_addrs[sdl->sdl_index - 1];
#ifdef _DEBUG_IFA_REF
		ifa_addref1(ifa, f);
#else
		ifa_addref(ifa);
#endif
		splx(s);
		return (ifa);
	    }
	}
	for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next)
		for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next) {
			register char *cp, *cp2, *cp3;

			if (ifa->ifa_addr->sa_family != af ||
			    ifa->ifa_netmask == 0)
				next: continue;
			cp = addr_data;
			cp2 = ifa->ifa_addr->sa_data;
			cp3 = ifa->ifa_netmask->sa_data;
			cplim = (char *)ifa->ifa_netmask +
				ifa->ifa_netmask->sa_len;
			while (cp3 < cplim)
				if ((*cp++ ^ *cp2++) & *cp3++)
				    /* want to continue for() loop */
					goto next;
			if (ifa_maybe == 0 ||
			    rn_refines((caddr_t)ifa->ifa_netmask,
			    (caddr_t)ifa_maybe->ifa_netmask))
				ifa_maybe = ifa;
		}
	if (ifa_maybe)
#ifdef _DEBUG_IFA_REF
		ifa_addref1(ifa_maybe, f);
#else
		ifa_addref(ifa_maybe);
#endif
	splx(s);
	return (ifa_maybe);
}
/*
 * Find the interface of the addresss.
 */
struct ifaddr *
#ifdef _DEBUG_IFA_REF
ifa_ifwithladdr1(addr, f)
	char *f;
#else
ifa_ifwithladdr(addr)
#endif
	struct sockaddr *addr;
{
	struct ifaddr *ia;

#ifdef _DEBUG_IFA_REF
	if ((ia = ifa_ifwithaddr1(addr, f)) || (ia = ifa_ifwithdstaddr1(addr, f))
	    || (ia = ifa_ifwithnet1(addr, f))) {
#else
	if ((ia = ifa_ifwithaddr(addr)) || (ia = ifa_ifwithdstaddr(addr))
	    || (ia = ifa_ifwithnet(addr))) {
#endif
		return (ia);
	}
	return (NULL);
}

/*
 * Find an interface using a specific address family
 */
struct ifaddr *
#ifdef _DEBUG_IFA_REF
ifa_ifwithaf1(af, f)
	char *f;
#else
ifa_ifwithaf(af)
#endif
	register int af;
{
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	register int s;

	s = splimp();
	for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next)
		for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next)
			if (ifa->ifa_addr->sa_family == af) {
#ifdef _DEBUG_IFA_REF
				ifa_addref1(ifa, f);
#else
				ifa_addref(ifa);
#endif
				splx(s);
				return (ifa);
			}
	splx(s);
	return ((struct ifaddr *)0);
}

/*
 * Find an interface address specific to an interface best matching
 * a given address.
 */
struct ifaddr *
#ifdef _DEBUG_IFA_REF
ifaof_ifpforaddr1(addr, ifp, f)
	char *f;
#else
ifaof_ifpforaddr(addr, ifp)
#endif
	struct sockaddr *addr;
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;
	register char *cp, *cp2, *cp3;
	register char *cplim;
	struct ifaddr *ifa_maybe = 0;
	u_int af = addr->sa_family;
	int s;

	if (af >= AF_MAX)
		return (0);
	s = splimp();
	for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next) {
		if (ifa->ifa_addr->sa_family != af)
			continue;
		ifa_maybe = ifa;
		if (ifa->ifa_netmask == 0) {
			if (equal(addr, ifa->ifa_addr) ||
			    (ifa->ifa_dstaddr &&
			    equal(addr, ifa->ifa_dstaddr))) {
#ifdef _DEBUG_IFA_REF
				ifa_addref1(ifa, f);
#else
				ifa_addref(ifa);
#endif
				splx(s);
				return (ifa);
			}
			continue;
		}
		cp = addr->sa_data;
		cp2 = ifa->ifa_addr->sa_data;
		cp3 = ifa->ifa_netmask->sa_data;
		cplim = ifa->ifa_netmask->sa_len + (char *)ifa->ifa_netmask;
		for (; cp3 < cplim; cp3++)
			if ((*cp++ ^ *cp2++) & *cp3)
				break;
		if (cp3 == cplim) {
#ifdef _DEBUG_IFA_REF
			ifa_addref1(ifa, f);
#else
			ifa_addref(ifa);
#endif
			splx(s);
			return (ifa);
		}
	}
	if (ifa_maybe)
#ifdef _DEBUG_IFA_REF
		ifa_addref1(ifa_maybe, f);
#else
		ifa_addref(ifa_maybe);
#endif
	splx(s);
	return (ifa_maybe);
}

/*
 * Default action when installing a route with a Link Level gateway.
 * Lookup an appropriate real ifa to point to.
 * This should be moved to /sys/net/link.c eventually.
 */
void
link_rtrequest(cmd, rt, sa)
	int cmd;
	register struct rtentry *rt;
	struct sockaddr *sa;
{
	register struct ifaddr *ifa;
	struct sockaddr *dst;
	struct ifnet *ifp;
	int s;

	s = splimp();

	if (cmd != RTM_ADD || ((ifa = rt->rt_ifa) == 0) ||
	    ((ifp = ifa->ifa_ifp) == 0) || ((dst = rt_key(rt)) == 0)) {
		splx(s);
		return;
	}
	if ((ifa = ifaof_ifpforaddr(dst, ifp)) != NULL) {
		IFAFREE(rt->rt_ifa);
		rt->rt_ifa = ifa;
		/* ifa->ifa_refcnt++; We implicitly do this by not
		 * ifa_delref'ing ifa */
		if (ifa->ifa_rtrequest && ifa->ifa_rtrequest != link_rtrequest)
			ifa->ifa_rtrequest(cmd, rt, sa);
	}
	splx(s);
}

/*
 * Mark an interface down and notify protocols of
 * the transition.
 * NOTE: must be called at splimp!
 */
void
if_down(ifp)
	register struct ifnet *ifp;
{
	register struct ifaddr *ifa;

	ifp->if_flags &= ~IFF_UP;
	for (ifa = ifp->if_addrlist.tqh_first; ifa != 0; ifa = ifa->ifa_list.tqe_next)
		pfctlinput(PRC_IFDOWN, ifa->ifa_addr);
	if_qflush(&ifp->if_snd);
	rt_ifmsg(ifp);
}

/*
 * Mark an interface up and notify protocols of
 * the transition.
 * NOTE: must be called at splimp!
 */
void
if_up(ifp)
	register struct ifnet *ifp;
{
#ifdef notyet
	register struct ifaddr *ifa;
#endif

	ifp->if_flags |= IFF_UP;
#ifdef notyet
	/* this has no effect on IP, and will kill all ISO connections XXX */
	for (ifa = ifp->if_addrlist.tqh_first; ifa != 0;
	     ifa = ifa->ifa_list.tqe_next)
		pfctlinput(PRC_IFUP, ifa->ifa_addr);
#endif
	rt_ifmsg(ifp);
}

/*
 * Flush an interface queue.
 * Needs to be called at splimp
 */
void
if_qflush(ifq)
	register struct ifqueue *ifq;
{
	register struct mbuf *m, *n;

	n = ifq->ifq_head;
	while ((m = n) != NULL) {
		n = m->m_act;
		m_freem(m);
	}
	ifq->ifq_head = 0;
	ifq->ifq_tail = 0;
	ifq->ifq_len = 0;
}

/*
 * Handle interface watchdog timer routines.  Called
 * from softclock, we decrement timers (if set) and
 * call the appropriate interface routine on expiration.
 */
void
if_slowtimo(arg)
	void *arg;
{
	register struct ifnet *ifp;
	int s = splimp();

	for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next) {
		if (ifp->if_timer == 0 || --ifp->if_timer)
			continue;
		if (ifp->if_watchdog)
			(*ifp->if_watchdog)(ifp);
	}
	splx(s);
	timeout(if_slowtimo, NULL, hz / IFNET_SLOWHZ);
}

/*
 * Map interface name to
 * interface structure pointer.
 */
struct ifnet *
ifunit(name)
	register char *name;
{
	register struct ifnet *ifp;
	int s;

	s = splimp();
	for (ifp = ifnet.tqh_first; ifp != 0; ifp = ifp->if_list.tqe_next)
		if (strcmp(ifp->if_xname, name) == 0) {
			if_addref(ifp);
			splx(s);
			return (ifp);
		    }

	splx(s);
	return (NULL);
}

/*
 * Interface ioctls.
 */
int
ifioctl(so, cmd, data, p)
	struct socket *so;
	u_long cmd;
	caddr_t data;
	struct proc *p;
{
	register struct ifnet *ifp;
	register struct ifreq *ifr;
	int error = 0;

	switch (cmd) {

	case SIOCGIFCONF:
	case OSIOCGIFCONF:
		return (ifconf(cmd, data));
	}
	ifr = (struct ifreq *)data;
	ifp = ifunit(ifr->ifr_name);
	if (ifp == 0)
		return (ENXIO);
	switch (cmd) {

	case SIOCGIFFLAGS:
		ifr->ifr_flags = ifp->if_flags;
		break;

	case SIOCGIFMETRIC:
		ifr->ifr_metric = ifp->if_metric;
		break;

	case SIOCGIFMTU:
		ifr->ifr_mtu = ifp->if_mtu;
		break;

	case SIOCSIFFLAGS:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
		if (ifp->if_flags & IFF_UP && (ifr->ifr_flags & IFF_UP) == 0) {
			int s = splimp();
			if_down(ifp);
			splx(s);
		}
		if (ifr->ifr_flags & IFF_UP && (ifp->if_flags & IFF_UP) == 0) {
			int s = splimp();
			if_up(ifp);
			splx(s);
		}
		ifp->if_flags = (ifp->if_flags & IFF_CANTCHANGE) |
			(ifr->ifr_flags &~ IFF_CANTCHANGE);
		if (ifp->if_ioctl)
			(void) (*ifp->if_ioctl)(ifp, cmd, data);
		/* XXX wrs shouldn't we return what the if_ioctl says? */
		break;

	case SIOCSIFMETRIC:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
		ifp->if_metric = ifr->ifr_metric;
		break;

	case SIOCSIFMTU:
	case SIOCADDMULTI:
	case SIOCDELMULTI:
	case SIOCSIFMEDIA:
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			break;
		/* FALLTHROUGH */
	case SIOCGIFMEDIA:
		if (ifp->if_ioctl == 0) {
			error =  EOPNOTSUPP;
			break;
		}
		error = (*ifp->if_ioctl)(ifp, cmd, data);

	case SIOCSDRVSPEC:  
		/* XXX:  need to pass proc pointer through to driver... */
		if ((error = suser(p->p_ucred, &p->p_acflag)) != 0)
			return (error);
	/* FALLTHROUGH */
	default:
		if (so->so_proto == 0) {
			error = (EOPNOTSUPP);
			break;
		}
#if !defined(COMPAT_43) && !defined(COMPAT_LINUX) && !defined(COMPAT_SVR4)
		error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
		    (struct mbuf *)cmd, (struct mbuf *)data,
		    (struct mbuf *)ifp, p));
#else
	    {
		int ocmd = cmd;

		switch (cmd) {

		case SIOCSIFADDR:
		case SIOCSIFDSTADDR:
		case SIOCSIFBRDADDR:
		case SIOCSIFNETMASK:
#if BYTE_ORDER != BIG_ENDIAN
			if (ifr->ifr_addr.sa_family == 0 &&
			    ifr->ifr_addr.sa_len < 16) {
				ifr->ifr_addr.sa_family = ifr->ifr_addr.sa_len;
				ifr->ifr_addr.sa_len = 16;
			}
#else
			if (ifr->ifr_addr.sa_len == 0)
				ifr->ifr_addr.sa_len = 16;
#endif
			break;

		case OSIOCGIFADDR:
			cmd = SIOCGIFADDR;
			break;

		case OSIOCGIFDSTADDR:
			cmd = SIOCGIFDSTADDR;
			break;

		case OSIOCGIFBRDADDR:
			cmd = SIOCGIFBRDADDR;
			break;

		case OSIOCGIFNETMASK:
			cmd = SIOCGIFNETMASK;
		}
		error = ((*so->so_proto->pr_usrreq)(so, PRU_CONTROL,
		    (struct mbuf *)cmd, (struct mbuf *)data,
		    (struct mbuf *)ifp, p));
		switch (ocmd) {

		case OSIOCGIFADDR:
		case OSIOCGIFDSTADDR:
		case OSIOCGIFBRDADDR:
		case OSIOCGIFNETMASK:
			*(u_int16_t *)&ifr->ifr_addr = ifr->ifr_addr.sa_family;
		}
	    }
#endif
	}
	return (error);
}

/*
 * Return interface configuration
 * of system.  List may be used
 * in later ioctl's (above) to get
 * other information.
 */
/*ARGSUSED*/
int
ifconf(cmd, data)
	u_long cmd;
	caddr_t data;
{
	register struct ifconf *ifc = (struct ifconf *)data;
	register struct ifnet *ifp;
	register struct ifaddr *ifa;
	struct ifreq ifr, *ifrp;
	int space = ifc->ifc_len, error = 0, s = splimp();

	ifrp = ifc->ifc_req;
	for (ifp = ifnet.tqh_first;
	    space >= sizeof (ifr) && ifp != 0; ifp = ifp->if_list.tqe_next) {
		bcopy(ifp->if_xname, ifr.ifr_name, IFNAMSIZ);
		if ((ifa = ifp->if_addrlist.tqh_first) == 0) {
			bzero((caddr_t)&ifr.ifr_addr, sizeof(ifr.ifr_addr));
			error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
			    sizeof(ifr));
			if (error)
				break;
			space -= sizeof (ifr), ifrp++;
		} else 
		    for (; space >= sizeof (ifr) && ifa != 0; ifa = ifa->ifa_list.tqe_next) {
			register struct sockaddr *sa = ifa->ifa_addr;
#if defined(COMPAT_43) || defined(COMPAT_LINUX) || defined(COMPAT_SVR4)
			if (cmd == OSIOCGIFCONF) {
				struct osockaddr *osa =
					 (struct osockaddr *)&ifr.ifr_addr;
				ifr.ifr_addr = *sa;
				osa->sa_family = sa->sa_family;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr));
				ifrp++;
			} else
#endif
			if (sa->sa_len <= sizeof(*sa)) {
				ifr.ifr_addr = *sa;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr));
				ifrp++;
			} else {
				space -= sa->sa_len - sizeof(*sa);
				if (space < sizeof (ifr))
					break;
				error = copyout((caddr_t)&ifr, (caddr_t)ifrp,
						sizeof (ifr.ifr_name));
				if (error == 0)
				    error = copyout((caddr_t)sa,
				      (caddr_t)&ifrp->ifr_addr, sa->sa_len);
				ifrp = (struct ifreq *)
					(sa->sa_len + (caddr_t)&ifrp->ifr_addr);
			}
			if (error)
				break;
			space -= sizeof (ifr);
		}
	}
	ifc->ifc_len -= space;
	splx(s);
	return (error);
}
