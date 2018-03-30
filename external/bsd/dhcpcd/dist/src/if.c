/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2018 Roy Marples <roy@marples.name>
 * All rights reserved

 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>

#include "config.h"

#include <net/if.h>
#include <net/if_arp.h>
#include <netinet/in.h>
#ifdef AF_LINK
#  include <net/if_dl.h>
#  include <net/if_types.h>
#  include <netinet/in_var.h>
#  undef AF_PACKET	/* Newer Illumos defines this */
#endif
#ifdef AF_PACKET
#  include <netpacket/packet.h>
#endif
#ifdef SIOCGIFMEDIA
#  include <net/if_media.h>
#endif
#include <net/route.h>

#include <ctype.h>
#include <errno.h>
#include <ifaddrs.h>
#include <inttypes.h>
#include <fnmatch.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>

#include "common.h"
#include "dev.h"
#include "dhcp.h"
#include "dhcp6.h"
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "ipv6nd.h"
#include "logerr.h"

void
if_free(struct interface *ifp)
{

	if (ifp == NULL)
		return;
	ipv4ll_free(ifp);
	dhcp_free(ifp);
	ipv4_free(ifp);
	dhcp6_free(ifp);
	ipv6nd_free(ifp);
	ipv6_free(ifp);
	rt_freeif(ifp);
	free_options(ifp->options);
	free(ifp);
}

int
if_opensockets(struct dhcpcd_ctx *ctx)
{

	if (if_opensockets_os(ctx) == -1)
		return -1;

	/* We use this socket for some operations without INET. */
	ctx->pf_inet_fd = xsocket(PF_INET, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (ctx->pf_inet_fd == -1)
		return -1;

#ifdef IFLR_ACTIVE
	ctx->pf_link_fd = xsocket(PF_LINK, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	if (ctx->pf_link_fd == -1)
		return -1;
#endif

	return 0;
}

void
if_closesockets(struct dhcpcd_ctx *ctx)
{

	if (ctx->pf_inet_fd != -1)
		close(ctx->pf_inet_fd);
#ifdef IFLR_ACTIVE
	if (ctx->pf_link_fd != -1)
		close(ctx->pf_link_fd);
#endif

	if (ctx->priv) {
		if_closesockets_os(ctx);
		free(ctx->priv);
	}
}

int
if_carrier(struct interface *ifp)
{
	int r;
	struct ifreq ifr;
#ifdef SIOCGIFMEDIA
	struct ifmediareq ifmr;
#endif

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	if (ioctl(ifp->ctx->pf_inet_fd, SIOCGIFFLAGS, &ifr) == -1)
		return LINK_UNKNOWN;
	ifp->flags = (unsigned int)ifr.ifr_flags;

#ifdef SIOCGIFMEDIA
	memset(&ifmr, 0, sizeof(ifmr));
	strlcpy(ifmr.ifm_name, ifp->name, sizeof(ifmr.ifm_name));
	if (ioctl(ifp->ctx->pf_inet_fd, SIOCGIFMEDIA, &ifmr) != -1 &&
	    ifmr.ifm_status & IFM_AVALID)
		r = (ifmr.ifm_status & IFM_ACTIVE) ? LINK_UP : LINK_DOWN;
	else
		r = ifr.ifr_flags & IFF_RUNNING ? LINK_UP : LINK_UNKNOWN;
#else
	r = ifr.ifr_flags & IFF_RUNNING ? LINK_UP : LINK_DOWN;
#endif
	return r;
}

int
if_setflag(struct interface *ifp, short flag)
{
	struct ifreq ifr;
	int r;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	r = -1;
	if (ioctl(ifp->ctx->pf_inet_fd, SIOCGIFFLAGS, &ifr) == 0) {
		if (flag == 0 || (ifr.ifr_flags & flag) == flag)
			r = 0;
		else {
			ifr.ifr_flags |= flag;
			if (ioctl(ifp->ctx->pf_inet_fd, SIOCSIFFLAGS, &ifr) ==0)
				r = 0;
		}
		ifp->flags = (unsigned int)ifr.ifr_flags;
	}
	return r;
}

static int
if_hasconf(struct dhcpcd_ctx *ctx, const char *ifname)
{
	int i;

	for (i = 0; i < ctx->ifcc; i++) {
		if (strcmp(ctx->ifcv[i], ifname) == 0)
			return 1;
	}
	return 0;
}

void
if_markaddrsstale(struct if_head *ifs)
{
	struct interface *ifp;

	TAILQ_FOREACH(ifp, ifs, next) {
#ifdef INET
		ipv4_markaddrsstale(ifp);
#endif
#ifdef INET6
		ipv6_markaddrsstale(ifp, 0);
#endif
	}
}

void
if_learnaddrs(struct dhcpcd_ctx *ctx, struct if_head *ifs,
    struct ifaddrs **ifaddrs)
{
	struct ifaddrs *ifa;
	struct interface *ifp;
#ifdef INET
	const struct sockaddr_in *addr, *net, *brd;
#endif
#ifdef INET6
	struct sockaddr_in6 *sin6, *net6;
#endif
	int addrflags;

	for (ifa = *ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;
		if ((ifp = if_find(ifs, ifa->ifa_name)) == NULL)
			continue;
#ifdef HAVE_IFADDRS_ADDRFLAGS
		addrflags = (int)ifa->ifa_addrflags;
#endif
		switch(ifa->ifa_addr->sa_family) {
#ifdef INET
		case AF_INET:
			addr = (void *)ifa->ifa_addr;
			net = (void *)ifa->ifa_netmask;
			if (ifa->ifa_flags & IFF_POINTOPOINT)
				brd = (void *)ifa->ifa_dstaddr;
			else
				brd = (void *)ifa->ifa_broadaddr;
#ifndef HAVE_IFADDRS_ADDRFLAGS
			addrflags = if_addrflags(ifp, &addr->sin_addr,
			    ifa->ifa_name);
			if (addrflags == -1) {
				if (errno != EEXIST)
					logerr("%s: if_addrflags: %s",
					    __func__,
					    inet_ntoa(addr->sin_addr));
				continue;
			}
#endif
			ipv4_handleifa(ctx, RTM_NEWADDR, ifs, ifa->ifa_name,
				&addr->sin_addr, &net->sin_addr,
				brd ? &brd->sin_addr : NULL, addrflags, 0);
			break;
#endif
#ifdef INET6
		case AF_INET6:
			sin6 = (void *)ifa->ifa_addr;
			net6 = (void *)ifa->ifa_netmask;
#ifdef __KAME__
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				/* Remove the scope from the address */
				sin6->sin6_addr.s6_addr[2] =
				    sin6->sin6_addr.s6_addr[3] = '\0';
#endif
#ifndef HAVE_IFADDRS_ADDRFLAGS
			addrflags = if_addrflags6(ifp, &sin6->sin6_addr,
			    ifa->ifa_name);
			if (addrflags == -1) {
				if (errno != EEXIST)
					logerr("%s: if_addrflags6", __func__);
				continue;
			}
#endif
			ipv6_handleifa(ctx, RTM_NEWADDR, ifs,
			    ifa->ifa_name, &sin6->sin6_addr,
			    ipv6_prefixlen(&net6->sin6_addr), addrflags, 0);
			break;
#endif
		}
	}

	freeifaddrs(*ifaddrs);
	*ifaddrs = NULL;
}

void
if_deletestaleaddrs(struct if_head *ifs)
{
	struct interface *ifp;

	TAILQ_FOREACH(ifp, ifs, next) {
#ifdef INET
		ipv4_deletestaleaddrs(ifp);
#endif
#ifdef INET6
		ipv6_deletestaleaddrs(ifp);
#endif
	}
}

bool
if_valid_hwaddr(const uint8_t *hwaddr, size_t hwlen)
{
	size_t i;
	bool all_zeros, all_ones;

	all_zeros = all_ones = true;
	for (i = 0; i < hwlen; i++) {
		if (hwaddr[i] != 0x00)
			all_zeros = false;
		if (hwaddr[i] != 0xff)
			all_ones = false;
		if (!all_zeros && !all_ones)
			return true;
	}
	return false;
}

struct if_head *
if_discover(struct dhcpcd_ctx *ctx, struct ifaddrs **ifaddrs,
    int argc, char * const *argv)
{
	struct ifaddrs *ifa;
	int i;
	unsigned int active;
	struct if_head *ifs;
	struct interface *ifp;
	struct if_spec spec;
#ifdef AF_LINK
	const struct sockaddr_dl *sdl;
#ifdef SIOCGIFPRIORITY
	struct ifreq ifr;
#endif
#ifdef IFLR_ACTIVE
	struct if_laddrreq iflr;
#endif

#ifdef IFLR_ACTIVE
	memset(&iflr, 0, sizeof(iflr));
#endif
#elif AF_PACKET
	const struct sockaddr_ll *sll;
#endif

	if ((ifs = malloc(sizeof(*ifs))) == NULL) {
		logerr(__func__);
		return NULL;
	}
	TAILQ_INIT(ifs);
	if (getifaddrs(ifaddrs) == -1) {
		logerr(__func__);
		goto out;
	}

	for (ifa = *ifaddrs; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr != NULL) {
#ifdef AF_LINK
			if (ifa->ifa_addr->sa_family != AF_LINK)
				continue;
#elif AF_PACKET
			if (ifa->ifa_addr->sa_family != AF_PACKET)
				continue;
#endif
		}
		if (if_nametospec(ifa->ifa_name, &spec) != 0)
			continue;

		/* It's possible for an interface to have >1 AF_LINK.
		 * For our purposes, we use the first one. */
		TAILQ_FOREACH(ifp, ifs, next) {
			if (strcmp(ifp->name, spec.devname) == 0)
				break;
		}
		if (ifp)
			continue;

		if (argc > 0) {
			for (i = 0; i < argc; i++) {
				if (strcmp(argv[i], spec.devname) == 0)
					break;
			}
			active = (i == argc) ? IF_INACTIVE : IF_ACTIVE_USER;
		} else {
			/* -1 means we're discovering against a specific
			 * interface, but we still need the below rules
			 * to apply. */
			if (argc == -1 && strcmp(argv[0], spec.devname) != 0)
				continue;
			active = ctx->options & DHCPCD_INACTIVE ?
			    IF_INACTIVE: IF_ACTIVE_USER;
		}

		for (i = 0; i < ctx->ifdc; i++)
			if (fnmatch(ctx->ifdv[i], spec.devname, 0) == 0)
				break;
		if (i < ctx->ifdc)
			active = IF_INACTIVE;
		for (i = 0; i < ctx->ifc; i++)
			if (fnmatch(ctx->ifv[i], spec.devname, 0) == 0)
				break;
		if (ctx->ifc && i == ctx->ifc)
			active = IF_INACTIVE;
		for (i = 0; i < ctx->ifac; i++)
			if (fnmatch(ctx->ifav[i], spec.devname, 0) == 0)
				break;
		if (ctx->ifac && i == ctx->ifac)
			active = IF_INACTIVE;

#ifdef PLUGIN_DEV
		/* Ensure that the interface name has settled */
		if (!dev_initialized(ctx, spec.devname))
			continue;
#endif

		/* Don't allow loopback or pointopoint unless explicit */
		if (ifa->ifa_flags & (IFF_LOOPBACK | IFF_POINTOPOINT)) {
			if ((argc == 0 || argc == -1) &&
			    ctx->ifac == 0 && !if_hasconf(ctx, spec.devname))
				active = IF_INACTIVE;
		}

		if (if_vimaster(ctx, spec.devname) == 1) {
			logfunc_t *logfunc = argc != 0 ? logerrx : logdebugx;
			logfunc("%s: is a Virtual Interface Master, skipping",
			    spec.devname);
			continue;
		}

		ifp = calloc(1, sizeof(*ifp));
		if (ifp == NULL) {
			logerr(__func__);
			break;
		}
		ifp->ctx = ctx;
		strlcpy(ifp->name, spec.devname, sizeof(ifp->name));
		ifp->flags = ifa->ifa_flags;

		if (ifa->ifa_addr != NULL) {
#ifdef AF_LINK
			sdl = (const void *)ifa->ifa_addr;

#ifdef IFLR_ACTIVE
			/* We need to check for active address */
			strlcpy(iflr.iflr_name, ifp->name,
			    sizeof(iflr.iflr_name));
			memcpy(&iflr.addr, ifa->ifa_addr,
			    MIN(ifa->ifa_addr->sa_len, sizeof(iflr.addr)));
			iflr.flags = IFLR_PREFIX;
			iflr.prefixlen = (unsigned int)sdl->sdl_alen * NBBY;
			if (ioctl(ctx->pf_link_fd, SIOCGLIFADDR, &iflr) == -1 ||
			    !(iflr.flags & IFLR_ACTIVE))
			{
				if_free(ifp);
				continue;
			}
#endif

			ifp->index = sdl->sdl_index;
			switch(sdl->sdl_type) {
#ifdef IFT_BRIDGE
			case IFT_BRIDGE: /* FALLTHROUGH */
#endif
#ifdef IFT_PPP
			case IFT_PPP: /* FALLTHROUGH */
#endif
#ifdef IFT_PROPVIRTUAL
			case IFT_PROPVIRTUAL: /* FALLTHROUGH */
#endif
#if defined(IFT_BRIDGE) || defined(IFT_PPP) || defined(IFT_PROPVIRTUAL)
				/* Don't allow unless explicit */
				if ((argc == 0 || argc == -1) &&
				    ctx->ifac == 0 && active &&
				    !if_hasconf(ctx, ifp->name))
				{
					logdebugx("%s: ignoring due to"
					    " interface type and"
					    " no config",
					    ifp->name);
					active = IF_INACTIVE;
				}
				/* FALLTHROUGH */
#endif
#ifdef IFT_L2VLAN
			case IFT_L2VLAN: /* FALLTHROUGH */
#endif
#ifdef IFT_L3IPVLAN
			case IFT_L3IPVLAN: /* FALLTHROUGH */
#endif
			case IFT_ETHER:
				ifp->family = ARPHRD_ETHER;
				break;
#ifdef IFT_IEEE1394
			case IFT_IEEE1394:
				ifp->family = ARPHRD_IEEE1394;
				break;
#endif
#ifdef IFT_INFINIBAND
			case IFT_INFINIBAND:
				ifp->family = ARPHRD_INFINIBAND;
				break;
#endif
			default:
				/* Don't allow unless explicit */
				if ((argc == 0 || argc == -1) &&
				    ctx->ifac == 0 &&
				    !if_hasconf(ctx, ifp->name))
					active = IF_INACTIVE;
				if (active)
					logwarnx("%s: unsupported"
					    " interface type %.2x",
					    ifp->name, sdl->sdl_type);
				/* Pretend it's ethernet */
				ifp->family = ARPHRD_ETHER;
				break;
			}
			ifp->hwlen = sdl->sdl_alen;
			memcpy(ifp->hwaddr, CLLADDR(sdl), ifp->hwlen);
#elif AF_PACKET
			sll = (const void *)ifa->ifa_addr;
			ifp->index = (unsigned int)sll->sll_ifindex;
			ifp->family = sll->sll_hatype;
			ifp->hwlen = sll->sll_halen;
			if (ifp->hwlen != 0)
				memcpy(ifp->hwaddr, sll->sll_addr, ifp->hwlen);
#endif
		}
#ifdef __linux__
		/* PPP addresses on Linux don't have hardware addresses */
		else
			ifp->index = if_nametoindex(ifp->name);
#endif

		/* Ensure hardware address is valid. */
		if (!if_valid_hwaddr(ifp->hwaddr, ifp->hwlen))
			ifp->hwlen = 0;

		/* We only work on ethernet by default */
		if (ifp->family != ARPHRD_ETHER) {
			if ((argc == 0 || argc == -1) &&
			    ctx->ifac == 0 && !if_hasconf(ctx, ifp->name))
				active = IF_INACTIVE;
			switch (ifp->family) {
			case ARPHRD_IEEE1394:
			case ARPHRD_INFINIBAND:
#ifdef ARPHRD_LOOPBACK
			case ARPHRD_LOOPBACK:
#endif
#ifdef ARPHRD_PPP
			case ARPHRD_PPP:
#endif
				/* We don't warn for supported families */
				break;

/* IFT already checked */
#ifndef AF_LINK
			default:
				if (active)
					logwarnx("%s: unsupported"
					    " interface family %.2x",
					    ifp->name, ifp->family);
				break;
#endif
			}
		}

		if (!(ctx->options & (DHCPCD_DUMPLEASE | DHCPCD_TEST))) {
			/* Handle any platform init for the interface */
			if (active != IF_INACTIVE && if_init(ifp) == -1) {
				logerr("%s: if_init", ifp->name);
				if_free(ifp);
				continue;
			}
		}

		ifp->vlanid = if_vlanid(ifp);

#ifdef SIOCGIFPRIORITY
		/* Respect the interface priority */
		memset(&ifr, 0, sizeof(ifr));
		strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
		if (ioctl(ctx->pf_inet_fd, SIOCGIFPRIORITY, &ifr) == 0)
			ifp->metric = (unsigned int)ifr.ifr_metric;
		if_getssid(ifp);
#else
		/* We reserve the 100 range for virtual interfaces, if and when
		 * we can work them out. */
		ifp->metric = 200 + ifp->index;
		if (if_getssid(ifp) != -1) {
			ifp->wireless = 1;
			ifp->metric += 100;
		}
#endif

		ifp->active = active;
		if (ifp->active)
			ifp->carrier = if_carrier(ifp);
		else
			ifp->carrier = LINK_UNKNOWN;
		TAILQ_INSERT_TAIL(ifs, ifp, next);
	}

out:
	return ifs;
}

/* Decode bge0:1 as dev = bge, ppa = 0 and lun = 1 */
int
if_nametospec(const char *ifname, struct if_spec *spec)
{
	char *ep;
	int e;

	if (ifname == NULL || *ifname == '\0' ||
	    strlcpy(spec->ifname, ifname, sizeof(spec->ifname)) >=
	    sizeof(spec->ifname) ||
	    strlcpy(spec->drvname, ifname, sizeof(spec->drvname)) >=
	    sizeof(spec->drvname))
	{
		errno = EINVAL;
		return -1;
	}
	ep = strchr(spec->drvname, ':');
	if (ep) {
		spec->lun = (int)strtoi(ep + 1, NULL, 10, 0, INT_MAX, &e);
		if (e != 0) {
			errno = e;
			return -1;
		}
		*ep-- = '\0';
	} else {
		spec->lun = -1;
		ep = spec->drvname + strlen(spec->drvname) - 1;
	}
	strlcpy(spec->devname, spec->drvname, sizeof(spec->devname));
	while (ep > spec->drvname && isdigit((int)*ep))
		ep--;
	if (*ep++ == ':') {
		errno = EINVAL;
		return -1;
	}
	spec->ppa = (int)strtoi(ep, NULL, 10, 0, INT_MAX, &e);
	if (e != 0)
		spec->ppa = -1;
	*ep = '\0';

	return 0;
}

static struct interface *
if_findindexname(struct if_head *ifaces, unsigned int idx, const char *name)
{

	if (ifaces != NULL) {
		struct if_spec spec;
		struct interface *ifp;

		if (name && if_nametospec(name, &spec) == -1)
			return NULL;

		TAILQ_FOREACH(ifp, ifaces, next) {
			if ((name && strcmp(ifp->name, spec.devname) == 0) ||
			    (!name && ifp->index == idx))
				return ifp;
		}
	}

	errno = ENXIO;
	return NULL;
}

struct interface *
if_find(struct if_head *ifaces, const char *name)
{

	return if_findindexname(ifaces, 0, name);
}

struct interface *
if_findindex(struct if_head *ifaces, unsigned int idx)
{

	return if_findindexname(ifaces, idx, NULL);
}

struct interface *
if_loopback(struct dhcpcd_ctx *ctx)
{
	struct interface *ifp;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (ifp->flags & IFF_LOOPBACK)
			return ifp;
	}
	return NULL;
}

int
if_domtu(const struct interface *ifp, short int mtu)
{
	int r;
	struct ifreq ifr;

	memset(&ifr, 0, sizeof(ifr));
	strlcpy(ifr.ifr_name, ifp->name, sizeof(ifr.ifr_name));
	ifr.ifr_mtu = mtu;
	r = ioctl(ifp->ctx->pf_inet_fd, mtu ? SIOCSIFMTU : SIOCGIFMTU, &ifr);
	if (r == -1)
		return -1;
	return ifr.ifr_mtu;
}

/* Interface comparer for working out ordering. */
static int
if_cmp(const struct interface *si, const struct interface *ti)
{
#ifdef INET
	int r;
#endif

	/* Check active first */
	if (si->active > ti->active)
		return -1;
	if (si->active < ti->active)
		return 1;

	/* Check carrier status next */
	if (si->carrier > ti->carrier)
		return -1;
	if (si->carrier < ti->carrier)
		return 1;

	if (D_STATE_RUNNING(si) && !D_STATE_RUNNING(ti))
		return -1;
	if (!D_STATE_RUNNING(si) && D_STATE_RUNNING(ti))
		return 1;
	if (RS_STATE_RUNNING(si) && !RS_STATE_RUNNING(ti))
		return -1;
	if (!RS_STATE_RUNNING(si) && RS_STATE_RUNNING(ti))
		return 1;
	if (D6_STATE_RUNNING(si) && !D6_STATE_RUNNING(ti))
		return -1;
	if (!D6_STATE_RUNNING(si) && D6_STATE_RUNNING(ti))
		return 1;

#ifdef INET
	/* Special attention needed here due to states and IPv4LL. */
	if ((r = ipv4_ifcmp(si, ti)) != 0)
		return r;
#endif

	/* Finally, metric */
	if (si->metric < ti->metric)
		return -1;
	if (si->metric > ti->metric)
		return 1;
	return 0;
}

/* Sort the interfaces into a preferred order - best first, worst last. */
void
if_sortinterfaces(struct dhcpcd_ctx *ctx)
{
	struct if_head sorted;
	struct interface *ifp, *ift;

	if (ctx->ifaces == NULL ||
	    (ifp = TAILQ_FIRST(ctx->ifaces)) == NULL ||
	    TAILQ_NEXT(ifp, next) == NULL)
		return;

	TAILQ_INIT(&sorted);
	TAILQ_REMOVE(ctx->ifaces, ifp, next);
	TAILQ_INSERT_HEAD(&sorted, ifp, next);
	while ((ifp = TAILQ_FIRST(ctx->ifaces))) {
		TAILQ_REMOVE(ctx->ifaces, ifp, next);
		TAILQ_FOREACH(ift, &sorted, next) {
			if (if_cmp(ifp, ift) == -1) {
				TAILQ_INSERT_BEFORE(ift, ifp, next);
				break;
			}
		}
		if (ift == NULL)
			TAILQ_INSERT_TAIL(&sorted, ifp, next);
	}
	TAILQ_CONCAT(ctx->ifaces, &sorted, next);
}

int
xsocket(int domain, int type, int protocol)
{
	int s;
#if !defined(HAVE_SOCK_CLOEXEC) || !defined(HAVE_SOCK_NONBLOCK)
	int xflags, xtype = type;
#endif

#ifndef HAVE_SOCK_CLOEXEC
	if (xtype & SOCK_CLOEXEC)
		type &= ~SOCK_CLOEXEC;
#endif
#ifndef HAVE_SOCK_NONBLOCK
	if (xtype & SOCK_NONBLOCK)
		type &= ~SOCK_NONBLOCK;
#endif

	if ((s = socket(domain, type, protocol)) == -1)
		return -1;

#ifndef HAVE_SOCK_CLOEXEC
	if ((xtype & SOCK_CLOEXEC) && ((xflags = fcntl(s, F_GETFD)) == -1 ||
	    fcntl(s, F_SETFD, xflags | FD_CLOEXEC) == -1))
		goto out;
#endif
#ifndef HAVE_SOCK_NONBLOCK
	if ((xtype & SOCK_NONBLOCK) && ((xflags = fcntl(s, F_GETFL)) == -1 ||
	    fcntl(s, F_SETFL, xflags | O_NONBLOCK) == -1))
		goto out;
#endif

	return s;

#if !defined(HAVE_SOCK_CLOEXEC) || !defined(HAVE_SOCK_NONBLOCK)
out:
	close(s);
	return -1;
#endif
}
