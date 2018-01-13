/*
 * Solaris interface driver for dhcpcd
 * Copyright (c) 2016-2018 Roy Marples <roy@marples.name>
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

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <libdlpi.h>
#include <stddef.h>
#include <stdlib.h>
#include <stropts.h>
#include <string.h>
#include <unistd.h>

#include <inet/ip.h>

#include <net/if_dl.h>
#include <net/if_types.h>

#include <netinet/if_ether.h>
#include <netinet/udp.h>

#include <sys/ioctl.h>
#include <sys/pfmod.h>
#include <sys/tihdr.h>
#include <sys/utsname.h>

/* Private libsocket interface we can hook into to get
 * a better getifaddrs(3).
 * From libsocket_priv.h, which is not always distributed so is here. */
extern int getallifaddrs(sa_family_t, struct ifaddrs **, int64_t);

#include "config.h"
#include "bpf.h"
#include "common.h"
#include "dhcp.h"
#include "if.h"
#include "if-options.h"
#include "ipv4.h"
#include "ipv6.h"
#include "ipv6nd.h"
#include "route.h"
#include "sa.h"

#ifndef ARP_MOD_NAME
#  define ARP_MOD_NAME        "arp"
#endif

#ifndef RT_ROUNDUP
#define RT_ROUNDUP(a)							      \
	((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))
#define RT_ADVANCE(x, n) (x += RT_ROUNDUP(salen(n)))
#endif

#define COPYOUT(sin, sa) do {						      \
	if ((sa) && ((sa)->sa_family == AF_INET))			      \
		(sin) = ((const struct sockaddr_in *)(const void *)	      \
		    (sa))->sin_addr;					      \
	} while (0)

#define COPYOUT6(sin, sa) do {						      \
	if ((sa) && ((sa)->sa_family == AF_INET6))			      \
		(sin) = ((const struct sockaddr_in6 *)(const void *)	      \
		    (sa))->sin6_addr;					      \
	} while (0)

#define COPYSA(dst, src) memcpy((dst), (src), salen((src)))

struct priv {
#ifdef INET6
	int pf_inet6_fd;
#endif
};

int
if_init(__unused struct interface *ifp)
{

	return 0;
}

int
if_conf(__unused struct interface *ifp)
{

	return 0;
}

int
if_opensockets_os(struct dhcpcd_ctx *ctx)
{
	struct priv		*priv;

	if ((priv = malloc(sizeof(*priv))) == NULL)
		return -1;
	ctx->priv = priv;

#ifdef INET6
	priv->pf_inet6_fd = xsocket(PF_INET6, SOCK_DGRAM | SOCK_CLOEXEC, 0);
	/* Don't return an error so we at least work on kernels witout INET6
	 * even though we expect INET6 support.
	 * We will fail noisily elsewhere anyway. */
#endif

	ctx->link_fd = socket(PF_ROUTE,
	    SOCK_RAW | SOCK_CLOEXEC | SOCK_NONBLOCK, 0);
#ifdef INET
	if (ctx->link_fd == -1)
		free(ctx->priv);
#endif
	return ctx->link_fd == -1 ? -1 : 0;
}

void
if_closesockets_os(struct dhcpcd_ctx *ctx)
{
#ifdef INET6
	struct priv		*priv;

	priv = (struct priv *)ctx->priv;
	if (priv->pf_inet6_fd != -1)
		close(priv->pf_inet6_fd);
#endif

	/* each interface should have closed itself */
	free(ctx->priv);
}

int
if_getssid(struct interface *ifp)
{

	UNUSED(ifp);
	errno = ENOTSUP;
	return -1;
}

unsigned short
if_vlanid(__unused const struct interface *ifp)
{

	return 0;
}

int
if_vimaster(__unused const struct dhcpcd_ctx *ctx, __unused const char *ifname)
{

	return 0;
}

int
if_machinearch(__unused char *str, __unused size_t len)
{

	/* There is no extra data really.
	 * isainfo -v does return amd64, but also i386. */
	return 0;
}

struct linkwalk {
	struct ifaddrs		*lw_ifa;
	int			lw_error;
};

static boolean_t
if_newaddr(const char *ifname, void *arg)
{
	struct linkwalk		*lw = arg;
	struct ifaddrs		*ifa;
	dlpi_handle_t		dh;
	dlpi_info_t		dlinfo;
	uint8_t			pa[DLPI_PHYSADDR_MAX];
	size_t			pa_len;
	struct sockaddr_dl	*sdl;

	ifa = NULL;
	if (dlpi_open(ifname, &dh, 0) != DLPI_SUCCESS)
		goto failed1;
	if (dlpi_info(dh, &dlinfo, 0) != DLPI_SUCCESS)
		goto failed;

	/* For some reason, dlpi_info won't return the
	 * physical address, it's all zero's.
	 * So cal dlpi_get_physaddr. */
	pa_len = DLPI_PHYSADDR_MAX;
	if (dlpi_get_physaddr(dh, DL_CURR_PHYS_ADDR,
	    pa, &pa_len) != DLPI_SUCCESS)
		goto failed;

	if ((ifa = calloc(1, sizeof(*ifa))) == NULL)
		goto failed;
	if ((ifa->ifa_name = strdup(ifname)) == NULL)
		goto failed;
	if ((sdl = calloc(1, sizeof(*sdl))) == NULL)
		goto failed;

	ifa->ifa_addr = (struct sockaddr *)sdl;
	sdl->sdl_index = if_nametoindex(ifname);
	sdl->sdl_family = AF_LINK;
	switch (dlinfo.di_mactype) {
	case DL_ETHER:
		sdl->sdl_type = IFT_ETHER;
		break;
	case DL_IB:
		sdl->sdl_type = IFT_IB;
		break;
	default:
		sdl->sdl_type = IFT_OTHER;
		break;
	}

	sdl->sdl_alen = pa_len;
	memcpy(sdl->sdl_data, pa, pa_len);

	ifa->ifa_next = lw->lw_ifa;
	lw->lw_ifa = ifa;
	dlpi_close(dh);
	return (B_FALSE);

failed:
	dlpi_close(dh);
	if (ifa != NULL) {
		free(ifa->ifa_name);
		free(ifa->ifa_addr);
		free(ifa);
	}
failed1:
	lw->lw_error = errno;
	return (B_TRUE);
}

/* Creates an empty sockaddr_dl for lo0. */
static struct ifaddrs *
if_ifa_lo0(void)
{
	struct ifaddrs		*ifa;
	struct sockaddr_dl	*sdl;

	if ((ifa = calloc(1, sizeof(*ifa))) == NULL)
		return NULL;
	if ((sdl = calloc(1, sizeof(*sdl))) == NULL) {
		free(ifa);
		return NULL;
	}
	if ((ifa->ifa_name = strdup("lo0")) == NULL) {
		free(ifa);
		free(sdl);
		return NULL;
	}

	ifa->ifa_addr = (struct sockaddr *)sdl;
	ifa->ifa_flags = IFF_LOOPBACK;
	sdl->sdl_family = AF_LINK;
	sdl->sdl_index = if_nametoindex("lo0");

	return ifa;
}

/* getifaddrs(3) does not support AF_LINK, strips aliases and won't
 * report addresses that are not UP.
 * As such it's just totally useless, so we need to roll our own. */
int
if_getifaddrs(struct ifaddrs **ifap)
{
	struct linkwalk		lw;
	struct ifaddrs		*ifa;

	/* Private libc function which we should not have to call
	 * to get non UP addresses. */
	if (getallifaddrs(AF_UNSPEC, &lw.lw_ifa, 0) == -1)
		return -1;

	/* Start with some AF_LINK addresses. */
	lw.lw_error = 0;
	dlpi_walk(if_newaddr, &lw, 0);
	if (lw.lw_error != 0) {
		freeifaddrs(lw.lw_ifa);
		errno = lw.lw_error;
		return -1;
	}

	/* lo0 doesn't appear in dlpi_walk, so fudge it. */
	if ((ifa = if_ifa_lo0()) == NULL) {
		freeifaddrs(lw.lw_ifa);
		return -1;
	}
	ifa->ifa_next = lw.lw_ifa;

	*ifap = ifa;
	return 0;
}

static int
salen(const struct sockaddr *sa)
{

	switch (sa->sa_family) {
	case AF_LINK:
		return sizeof(struct sockaddr_dl);
	case AF_INET:
		return sizeof(struct sockaddr_in);
	case AF_INET6:
		return sizeof(struct sockaddr_in6);
	default:
		return sizeof(struct sockaddr);
	}
}

static void
if_linkaddr(struct sockaddr_dl *sdl, const struct interface *ifp)
{

	memset(sdl, 0, sizeof(*sdl));
	sdl->sdl_family = AF_LINK;
	sdl->sdl_nlen = sdl->sdl_alen = sdl->sdl_slen = 0;
	sdl->sdl_index = (unsigned short)ifp->index;
}

static int
get_addrs(int type, const void *data, const struct sockaddr **sa)
{
	const char *cp;
	int i;

	cp = data;
	for (i = 0; i < RTAX_MAX; i++) {
		if (type & (1 << i)) {
			sa[i] = (const struct sockaddr *)cp;
			RT_ADVANCE(cp, sa[i]);
		} else
			sa[i] = NULL;
	}
	return 0;
}

static struct interface *
if_findsdl(struct dhcpcd_ctx *ctx, const struct sockaddr_dl *sdl)
{

	if (sdl->sdl_index)
		return if_findindex(ctx->ifaces, sdl->sdl_index);

	if (sdl->sdl_nlen) {
		char ifname[IF_NAMESIZE];

		memcpy(ifname, sdl->sdl_data, sdl->sdl_nlen);
		ifname[sdl->sdl_nlen] = '\0';
		return if_find(ctx->ifaces, ifname);
	}
	if (sdl->sdl_alen) {
		struct interface *ifp;

		TAILQ_FOREACH(ifp, ctx->ifaces, next) {
			if (ifp->hwlen == sdl->sdl_alen &&
			    memcmp(ifp->hwaddr,
			    sdl->sdl_data, sdl->sdl_alen) == 0)
				return ifp;
		}
	}

	errno = ENOENT;
	return NULL;
}

static struct interface *
if_findsa(struct dhcpcd_ctx *ctx, const struct sockaddr *sa)
{
	if (sa == NULL) {
		errno = EINVAL;
		return NULL;
	}

	switch (sa->sa_family) {
	case AF_LINK:
	{
		const struct sockaddr_dl *sdl;

		sdl = (const void *)sa;
		return if_findsdl(ctx, sdl);
	}
#ifdef INET
	case AF_INET:
	{
		const struct sockaddr_in *sin;
		struct ipv4_addr *ia;

		sin = (const void *)sa;
		if ((ia = ipv4_findmaskaddr(ctx, &sin->sin_addr)))
			return ia->iface;
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		const struct sockaddr_in6 *sin;
		struct ipv6_addr *ia;

		sin = (const void *)sa;
		if ((ia = ipv6_findmaskaddr(ctx, &sin->sin6_addr)))
			return ia->iface;
		break;
	}
#endif
	default:
		errno = EAFNOSUPPORT;
		return NULL;
	}

	errno = ENOENT;
	return NULL;
}

static void
if_finishrt(struct rt *rt)
{

	/* Solaris has a subnet route with the gateway
	 * of the owning address.
	 * dhcpcd has a blank gateway here to indicate a
	 * subnet route. */
	if (!sa_is_unspecified(&rt->rt_gateway)) {
		switch(rt->rt_gateway.sa_family) {
#ifdef INET
		case AF_INET:
		{
			struct in_addr *in;

			in = &satosin(&rt->rt_gateway)->sin_addr;
			if (ipv4_iffindaddr(rt->rt_ifp, in, NULL))
				in->s_addr = INADDR_ANY;
			break;
		}
#endif
#ifdef INET6
		case AF_INET6:
		{
			struct in6_addr *in6;

			in6 = &satosin6(&rt->rt_gateway)->sin6_addr;
			if (ipv6_iffindaddr(rt->rt_ifp, in6, 0))
				*in6 = in6addr_any;
			break;
		}
#endif
		}
	}

	/* Solaris likes to set route MTU to match
	 * interface MTU when adding routes.
	 * This confuses dhcpcd as it expects MTU to be 0
	 * when no explicit MTU has been set. */
	if (rt->rt_mtu == (unsigned int)if_getmtu(rt->rt_ifp))
		rt->rt_mtu = 0;
}

static int
if_copyrt(struct dhcpcd_ctx *ctx, struct rt *rt, const struct rt_msghdr *rtm)
{
	const struct sockaddr *rti_info[RTAX_MAX];

	if (~rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY))
		return -1;

	get_addrs(rtm->rtm_addrs, rtm + 1, rti_info);
	memset(rt, 0, sizeof(*rt));

	rt->rt_flags = (unsigned int)rtm->rtm_flags;
	COPYSA(&rt->rt_dest, rti_info[RTAX_DST]);
	if (rtm->rtm_addrs & RTA_NETMASK)
		COPYSA(&rt->rt_netmask, rti_info[RTAX_NETMASK]);
	/* dhcpcd likes an unspecified gateway to indicate via the link. */
	if (rt->rt_flags & RTF_GATEWAY &&
	    rti_info[RTAX_GATEWAY]->sa_family != AF_LINK)
		COPYSA(&rt->rt_gateway, rti_info[RTAX_GATEWAY]);
	COPYSA(&rt->rt_ifa, rti_info[RTAX_SRC]);
	rt->rt_mtu = (unsigned int)rtm->rtm_rmx.rmx_mtu;

	if (rtm->rtm_index)
		rt->rt_ifp = if_findindex(ctx->ifaces, rtm->rtm_index);
	else if (rtm->rtm_addrs & RTA_IFP)
		rt->rt_ifp = if_findsa(ctx, rti_info[RTAX_IFP]);
	else if (rtm->rtm_addrs & RTA_GATEWAY)
		rt->rt_ifp = if_findsa(ctx, rti_info[RTAX_GATEWAY]);
	else
		rt->rt_ifp = if_findsa(ctx, rti_info[RTAX_DST]);

	if (rt->rt_ifp == NULL) {
		errno = ESRCH;
		return -1;
	}

	return 0;
}

static int
if_addrflags0(int fd, const char *ifname)
{
	struct lifreq		lifr;

	memset(&lifr, 0, sizeof(lifr));
	strlcpy(lifr.lifr_name, ifname, sizeof(lifr.lifr_name));
	if (ioctl(fd, SIOCGLIFFLAGS, &lifr) == -1)
		return -1;

	return lifr.lifr_flags;
}

static void
if_rtm(struct dhcpcd_ctx *ctx, const struct rt_msghdr *rtm)
{
	const struct sockaddr *sa;
	struct rt rt;

	/* Ignore messages generated by us */
	if (rtm->rtm_pid == getpid()) {
		ctx->options &= ~DHCPCD_RTM_PPID;
		return;
	}

	/* Ignore messages sent by the parent after forking */
	if ((ctx->options &
	    (DHCPCD_RTM_PPID | DHCPCD_DAEMONISED)) ==
	    (DHCPCD_RTM_PPID | DHCPCD_DAEMONISED) &&
	    rtm->rtm_pid == ctx->ppid)
	{
		/* If this is the last successful message sent,
		 * clear the check flag as it's possible another
		 * process could re-use the same pid and also
		 * manipulate therouting table. */
		if (rtm->rtm_seq == ctx->pseq)
			ctx->options &= ~DHCPCD_RTM_PPID;
		return;
	}

	sa = (const void *)(rtm + 1);
	switch (sa->sa_family) {
#ifdef INET6
	case AF_INET6:
		if (~rtm->rtm_addrs & (RTA_DST | RTA_GATEWAY))
			break;
		/*
		 * BSD announces host routes.
		 * But does this work on Solaris?
		 * As such, we should be notified of reachability by its
		 * existance with a hardware address.
		 */
		if (rtm->rtm_flags & (RTF_HOST)) {
			const struct sockaddr *rti_info[RTAX_MAX];
			struct in6_addr dst6;
			struct sockaddr_dl sdl;

			get_addrs(rtm->rtm_addrs, sa, rti_info);
			COPYOUT6(dst6, rti_info[RTAX_DST]);
			if (rti_info[RTAX_GATEWAY]->sa_family == AF_LINK)
				memcpy(&sdl, rti_info[RTAX_GATEWAY],
				    sizeof(sdl));
			else
				sdl.sdl_alen = 0;
			ipv6nd_neighbour(ctx, &dst6,
			    rtm->rtm_type != RTM_DELETE && sdl.sdl_alen ?
			    IPV6ND_REACHABLE : 0);
		}
		break;
	}
#endif

	if (if_copyrt(ctx, &rt, rtm) == 0) {
		if_finishrt(&rt);
		rt_recvrt(rtm->rtm_type, &rt);
	}
}

static void
if_ifa(struct dhcpcd_ctx *ctx, const struct ifa_msghdr *ifam)
{
	struct interface	*ifp;
	const struct sockaddr	*sa, *rti_info[RTAX_MAX];
	int			flags;
	const char		*ifalias;

	/* XXX We have no way of knowing who generated these
	 * messages wich truely sucks because we want to
	 * avoid listening to our own delete messages. */
	if ((ifp = if_findindex(ctx->ifaces, ifam->ifam_index)) == NULL)
		return;
	sa = (const void *)(ifam + 1);
	get_addrs(ifam->ifam_addrs, sa, rti_info);
	if ((sa = rti_info[RTAX_IFA]) == NULL)
		return;

	/*
	 * ifa_msghdr does not supply the alias, just the interface index.
	 * This is very bad, because it means we have to call getifaddrs
	 * and trawl the list of addresses to find the added address.
	 * To make life worse, you can have the same address on the same
	 * interface with different aliases.
	 * So this hack is not entirely accurate.
	 *
	 * IF anyone is going to fix Solaris, plesse consider adding the
	 * following fields to extend ifa_msghdr:
	 *   ifam_alias
	 *   ifam_pid
	 */

	ifalias = ifp->name;
	if (ifam->ifam_type != RTM_DELADDR && sa->sa_family != AF_LINK) {
		struct ifaddrs	*ifaddrs, *ifa;

		ifaddrs = NULL;
		if (getallifaddrs(sa->sa_family, &ifaddrs, 0) == -1)
			return;
		for (ifa = ifaddrs; ifa != NULL; ifa = ifa->ifa_next) {
			if (ifa->ifa_addr != NULL) {
				if (sa_cmp(sa, ifa->ifa_addr) == 0) {
					/* Check it's for the right interace. */
					struct interface	*ifpx;

					ifpx = if_find(ctx->ifaces,
					    ifa->ifa_name);
					if (ifp == ifpx) {
						ifalias = ifa->ifa_name;
						break;
					}
				}
			}
		}
		freeifaddrs(ifaddrs);
	}

	switch (sa->sa_family) {
	case AF_LINK:
	{
		struct sockaddr_dl sdl;

		if (ifam->ifam_type != RTM_CHGADDR &&
		    ifam->ifam_type != RTM_NEWADDR)
			break;
		memcpy(&sdl, rti_info[RTAX_IFA], sizeof(sdl));
		dhcpcd_handlehwaddr(ctx, ifp->name, CLLADDR(&sdl),sdl.sdl_alen);
		break;
	}
#ifdef INET
	case AF_INET:
	{
		struct in_addr addr, mask, bcast;

		COPYOUT(addr, rti_info[RTAX_IFA]);
		COPYOUT(mask, rti_info[RTAX_NETMASK]);
		COPYOUT(bcast, rti_info[RTAX_BRD]);

		if (ifam->ifam_type != RTM_DELADDR) {
			flags = if_addrflags0(ctx->pf_inet_fd, ifalias);
			if (flags == -1)
				break;
		} else
			flags = 0;

		ipv4_handleifa(ctx,
		    ifam->ifam_type == RTM_CHGADDR ?
		    RTM_NEWADDR : ifam->ifam_type,
		    NULL, ifalias, &addr, &mask, &bcast, flags);
		break;
	}
#endif
#ifdef INET6
	case AF_INET6:
	{
		struct in6_addr			addr6, mask6;
		const struct sockaddr_in6	*sin6;

		sin6 = (const void *)rti_info[RTAX_IFA];
		addr6 = sin6->sin6_addr;
		sin6 = (const void *)rti_info[RTAX_NETMASK];
		mask6 = sin6->sin6_addr;

		if (ifam->ifam_type != RTM_DELADDR) {
			const struct priv	 *priv;

			priv = (struct priv *)ctx->priv;
			flags = if_addrflags0(priv->pf_inet6_fd, ifalias);
			if (flags == -1)
				break;
		} else
			flags = 0;

		ipv6_handleifa(ctx,
		    ifam->ifam_type == RTM_CHGADDR ?
		    RTM_NEWADDR : ifam->ifam_type,
		    NULL, ifalias, &addr6, ipv6_prefixlen(&mask6), flags);
		break;
	}
#endif
	}
}

static void
if_ifinfo(struct dhcpcd_ctx *ctx, const struct if_msghdr *ifm)
{
	struct interface *ifp;
	int state;

	if ((ifp = if_findindex(ctx->ifaces, ifm->ifm_index)) == NULL)
		return;
	if (ifm->ifm_flags & IFF_OFFLINE || !(ifm->ifm_flags & IFF_UP))
		state = LINK_DOWN;
	else
		state = LINK_UP;
	dhcpcd_handlecarrier(ctx, state,
	    (unsigned int)ifm->ifm_flags, ifp->name);
}

static void
if_dispatch(struct dhcpcd_ctx *ctx, const struct rt_msghdr *rtm)
{

	if (rtm->rtm_version != RTM_VERSION)
		return;

	switch(rtm->rtm_type) {
	case RTM_IFINFO:
		if_ifinfo(ctx, (const void *)rtm);
		break;
	case RTM_ADD:		/* FALLTHROUGH */
	case RTM_CHANGE:	/* FALLTHROUGH */
	case RTM_DELETE:
		if_rtm(ctx, (const void *)rtm);
		break;
	case RTM_CHGADDR:	/* FALLTHROUGH */
	case RTM_DELADDR:	/* FALLTHROUGH */
	case RTM_NEWADDR:
		if_ifa(ctx, (const void *)rtm);
		break;
	}
}

int
if_handlelink(struct dhcpcd_ctx *ctx)
{
	struct msghdr msg;
	ssize_t len;

	memset(&msg, 0, sizeof(msg));
	msg.msg_iov = ctx->iov;
	msg.msg_iovlen = 1;

	if ((len = recvmsg_realloc(ctx->link_fd, &msg, 0)) == -1)
		return -1;
	if (len != 0)
		if_dispatch(ctx, ctx->iov[0].iov_base);
	return 0;
}

static void
if_octetstr(char *buf, const Octet_t *o, ssize_t len)
{
	int			i;
	char			*p;

	p = buf;
	for (i = 0; i < o->o_length; i++) {
		if ((p + 1) - buf < len)
			*p++ = o->o_bytes[i];
		else
			break;
	}
	*p = '\0';
}

static int
if_addaddr(int fd, const char *ifname,
    struct sockaddr_storage *addr, struct sockaddr_storage *mask)
{
	struct lifreq		lifr;

	memset(&lifr, 0, sizeof(lifr));
	strlcpy(lifr.lifr_name, ifname, sizeof(lifr.lifr_name));

	/* First assign the netmask. */
	lifr.lifr_addr = *mask;
	if (ioctl(fd, SIOCSLIFNETMASK, &lifr) == -1)
		return -1;

	/* Then assign the address. */
	lifr.lifr_addr = *addr;
	if (ioctl(fd, SIOCSLIFADDR, &lifr) == -1)
		return -1;

	/* Now bring it up. */
	if (ioctl(fd, SIOCGLIFFLAGS, &lifr) == -1)
		return -1;
	if (!(lifr.lifr_flags & IFF_UP)) {
		lifr.lifr_flags |= IFF_UP;
		if (ioctl(fd, SIOCSLIFFLAGS, &lifr) == -1)
			return -1;
	}
	return 0;
}

static int
if_plumblif(int cmd, const struct dhcpcd_ctx *ctx, int af, const char *ifname)
{
	struct lifreq		lifr;
	int			s;

	memset(&lifr, 0, sizeof(lifr));
	strlcpy(lifr.lifr_name, ifname, sizeof(lifr.lifr_name));
	lifr.lifr_addr.ss_family = af;
	if (af == AF_INET)
		s = ctx->pf_inet_fd;
	else {
		struct priv	*priv;

		priv = (struct priv *)ctx->priv;
		s = priv->pf_inet6_fd;
	}
	return ioctl(s,
	    cmd == RTM_NEWADDR ? SIOCLIFADDIF : SIOCLIFREMOVEIF,
	    &lifr) == -1 && errno != EEXIST ? -1 : 0;
}

static int
if_plumbif(const struct dhcpcd_ctx *ctx, int af, const char *ifname)
{
	dlpi_handle_t		dh;
	int			fd, af_fd, mux_fd, retval;
	struct lifreq		lifr;
	const char		*udp_dev;

	memset(&lifr, 0, sizeof(lifr));
	switch (af) {
	case AF_INET:
		lifr.lifr_flags = IFF_IPV4;
		af_fd = ctx->pf_inet_fd;
		udp_dev = UDP_DEV_NAME;
		break;
	case AF_INET6:
	{
		struct priv *priv;

		/* We will take care of setting the link local address. */
		lifr.lifr_flags = IFF_IPV6 | IFF_NOLINKLOCAL;
		priv = (struct priv *)ctx->priv;
		af_fd = priv->pf_inet6_fd;
		udp_dev = UDP6_DEV_NAME;
		break;
	}
	default:
		errno = EPROTONOSUPPORT;
		return -1;
	}

	if (dlpi_open(ifname, &dh, DLPI_NOATTACH) != DLPI_SUCCESS) {
		errno = EINVAL;
		return -1;
	}

	fd = dlpi_fd(dh);
	retval = -1;
	mux_fd = -1;
	if (ioctl(fd, I_PUSH, IP_MOD_NAME) == -1)
		goto out;
	strlcpy(lifr.lifr_name, ifname, sizeof(lifr.lifr_name));
	if (ioctl(fd, SIOCSLIFNAME, &lifr) == -1)
		goto out;

	/* Get full flags. */
	if (ioctl(af_fd, SIOCGLIFFLAGS, &lifr) == -1)
		goto out;

	/* Open UDP as a multiplexor to PLINK the interface stream.
	 * UDP is used because STREAMS will not let you PLINK a driver
	 * under itself and IP is generally  at the bottom of the stream. */
	if ((mux_fd = open(udp_dev, O_RDWR)) == -1)
		goto out;
	/* POP off all undesired modules. */
	while (ioctl(mux_fd, I_POP, 0) != -1)
		;
	if (errno != EINVAL)
		goto out;

	if (lifr.lifr_flags & IFF_IPV4 && !(lifr.lifr_flags & IFF_NOARP)) {
		if (ioctl(mux_fd, I_PUSH, ARP_MOD_NAME) == -1)
			goto out;
	}

	/* PLINK the interface stream so it persists. */
	if (ioctl(mux_fd, I_PLINK, fd) == -1)
		goto out;

	retval = 0;

out:
	dlpi_close(dh);
	if (mux_fd != -1)
		close(mux_fd);
	return retval;
}

static int
if_unplumbif(const struct dhcpcd_ctx *ctx, int af, const char *ifname)
{
	struct sockaddr_storage		addr, mask;
	int				fd;

	/* For the time being, don't unplumb the interface, just
	 * set the address to zero. */
	memset(&addr, 0, sizeof(addr));
	addr.ss_family = af;
	memset(&mask, 0, sizeof(mask));
	mask.ss_family = af;
	switch (af) {
#ifdef INET
	case AF_INET:
		fd = ctx->pf_inet_fd;
		break;
#endif
#ifdef INET6
	case AF_INET6:
	{
		struct priv		*priv;

		priv = (struct priv *)ctx->priv;
		fd = priv->pf_inet6_fd;
		break;
	}
#endif
	default:
		errno = EAFNOSUPPORT;
		return -1;
	}
	return if_addaddr(fd, ifname, &addr, &mask);
}

static int
if_plumb(int cmd, const struct dhcpcd_ctx *ctx, int af, const char *ifname)
{
	struct if_spec		spec;

	if (if_nametospec(ifname, &spec) == -1)
		return -1;
	if (spec.lun != -1)
		return if_plumblif(cmd, ctx, af, ifname);
	if (cmd == RTM_NEWADDR)
		return if_plumbif(ctx, af, ifname);
	else
		return if_unplumbif(ctx, af, ifname);
}

int
if_route(unsigned char cmd, const struct rt *rt)
{
	struct dhcpcd_ctx *ctx;
	struct rtm
	{
		struct rt_msghdr hdr;
		char buffer[sizeof(struct sockaddr_storage) * RTAX_MAX];
	} rtmsg;
	struct rt_msghdr *rtm;
	char *bp = rtmsg.buffer;
	size_t l;

	/* WARNING: Solaris will not allow you to delete RTF_KERNEL routes.
	 * This includes subnet/prefix routes. */

	ctx = rt->rt_ifp->ctx;
	if ((cmd == RTM_ADD || cmd == RTM_DELETE || cmd == RTM_CHANGE) &&
	    ctx->options & DHCPCD_DAEMONISE &&
	    !(ctx->options & DHCPCD_DAEMONISED))
		ctx->options |= DHCPCD_RTM_PPID;

#define ADDSA(sa) do {							      \
		l = RT_ROUNDUP(salen((sa)));				      \
		memcpy(bp, (sa), l);					      \
		bp += l;						      \
	} while (/* CONSTCOND */ 0)

	memset(&rtmsg, 0, sizeof(rtmsg));
	rtm = &rtmsg.hdr;
	rtm->rtm_version = RTM_VERSION;
	rtm->rtm_type = cmd;
	rtm->rtm_seq = ++ctx->seq;
	rtm->rtm_flags = rt->rt_flags;
	rtm->rtm_addrs = RTA_DST | RTA_GATEWAY;

	if (cmd == RTM_ADD || cmd == RTM_CHANGE) {
		bool netmask_bcast = sa_is_allones(&rt->rt_netmask);

		rtm->rtm_flags |= RTF_UP;
		if (!(rtm->rtm_flags & RTF_REJECT) &&
		    !sa_is_loopback(&rt->rt_gateway))
		{
			rtm->rtm_addrs |= RTA_IFP;
			if (!sa_is_unspecified(&rt->rt_ifa))
				rtm->rtm_addrs |= RTA_IFA;
		}
		if (netmask_bcast)
			rtm->rtm_flags |= RTF_HOST;
		else
			rtm->rtm_flags |= RTF_GATEWAY;

		/* Emulate the kernel by marking address generated
		 * network routes non-static. */
		if (!(rt->rt_dflags & RTDF_IFA_ROUTE))
			rtm->rtm_flags |= RTF_STATIC;

		if (rt->rt_mtu != 0) {
			rtm->rtm_inits |= RTV_MTU;
			rtm->rtm_rmx.rmx_mtu = rt->rt_mtu;
		}
	}

	ADDSA(&rt->rt_dest);

	if (sa_is_unspecified(&rt->rt_gateway))
		ADDSA(&rt->rt_ifa);
	else
		ADDSA(&rt->rt_gateway);

	if (rtm->rtm_addrs & RTA_NETMASK)
		ADDSA(&rt->rt_netmask);

	if (rtm->rtm_addrs & RTA_IFP) {
		struct sockaddr_dl sdl;

		if_linkaddr(&sdl, rt->rt_ifp);
		ADDSA((struct sockaddr *)&sdl);
	}

/* This no workie :/ */
#if 1
	/* route(1M) says RTA_IFA is accepted but ignored
	 * it's unclear how RTA_SRC is different. */
	if (rtm->rtm_addrs & RTA_IFA) {
		rtm->rtm_addrs &= ~RTA_IFA;
		rtm->rtm_addrs |= RTA_SRC;
	}
	if (rtm->rtm_addrs & RTA_SRC)
		ADDSA(&rt->rt_ifa);
#endif

#undef ADDSA

	rtm->rtm_msglen = (unsigned short)(bp - (char *)rtm);
	if (write(ctx->link_fd, rtm, rtm->rtm_msglen) == -1)
		return -1;
	ctx->sseq = ctx->seq;
	return 0;
}

#ifdef INET
static int
if_walkrt(struct dhcpcd_ctx *ctx, char *data, size_t len)
{
	mib2_ipRouteEntry_t *re, *e;
	struct rt rt;
	char ifname[IF_NAMESIZE];
	struct in_addr in;

	if (len % sizeof(*re) != 0) {
		errno = EINVAL;
		return -1;
	}

	re = (mib2_ipRouteEntry_t *)data;
	e = (mib2_ipRouteEntry_t *)(data + len);
	do {
		/* Skip route types we don't want. */
		switch (re->ipRouteInfo.re_ire_type) {
		case IRE_IF_CLONE:
		case IRE_BROADCAST:
		case IRE_MULTICAST:
		case IRE_NOROUTE:
		case IRE_LOCAL:
			continue;
		default:
			break;
		}
		memset(&rt, 0, sizeof(rt));
		rt.rt_dflags |= RTDF_INIT;
		in.s_addr = re->ipRouteDest;
		sa_in_init(&rt.rt_dest, &in);
		in.s_addr = re->ipRouteMask;
		sa_in_init(&rt.rt_netmask, &in);
		in.s_addr = re->ipRouteNextHop;
		sa_in_init(&rt.rt_gateway, &in);
		rt.rt_flags = re->ipRouteInfo.re_flags;
		in.s_addr = re->ipRouteInfo.re_src_addr;
		sa_in_init(&rt.rt_ifa, &in);
		rt.rt_mtu = re->ipRouteInfo.re_max_frag;

		if_octetstr(ifname, &re->ipRouteIfIndex, sizeof(ifname));
		rt.rt_ifp = if_find(ctx->ifaces, ifname);
		if (rt.rt_ifp != NULL) {
			if_finishrt(&rt);
			rt_recvrt(RTM_ADD, &rt);
		}
	} while (++re < e);
	return 0;
}
#endif

#ifdef INET6
static int
if_walkrt6(struct dhcpcd_ctx *ctx, char *data, size_t len)
{
	mib2_ipv6RouteEntry_t *re, *e;
	struct rt rt;
	char ifname[IF_NAMESIZE];
	struct in6_addr in6;

	if (len % sizeof(*re) != 0) {
		errno = EINVAL;
		return -1;
	}

	re = (mib2_ipv6RouteEntry_t *)data;
	e = (mib2_ipv6RouteEntry_t *)(data + len);

	do {
		/* Skip route types we don't want. */
		switch (re->ipv6RouteInfo.re_ire_type) {
		case IRE_IF_CLONE:
		case IRE_BROADCAST:
		case IRE_MULTICAST:
		case IRE_NOROUTE:
		case IRE_LOCAL:
			continue;
		default:
			break;
		}
		memset(&rt, 0, sizeof(rt));
		rt.rt_dflags |= RTDF_INIT;
		sa_in6_init(&rt.rt_dest, &re->ipv6RouteDest);
		ipv6_mask(&in6, re->ipv6RoutePfxLength);
		sa_in6_init(&rt.rt_netmask, &in6);
		sa_in6_init(&rt.rt_gateway, &re->ipv6RouteNextHop);
		rt.rt_mtu = re->ipv6RouteInfo.re_max_frag;

		if_octetstr(ifname, &re->ipv6RouteIfIndex, sizeof(ifname));
		rt.rt_ifp = if_find(ctx->ifaces, ifname);
		if (rt.rt_ifp != NULL) {
			if_finishrt(&rt);
			rt_recvrt(RTM_ADD, &rt);
		}
	} while (++re < e);
	return 0;
}
#endif

static int
if_parsert(struct dhcpcd_ctx *ctx, unsigned int level, unsigned int name,
    int (*walkrt)(struct dhcpcd_ctx *, char *, size_t))
{
	int			s, retval, code, flags;
	uintptr_t		buf[512 / sizeof(uintptr_t)];
	struct strbuf		ctlbuf, databuf;
	struct T_optmgmt_req	*tor = (struct T_optmgmt_req *)buf;
	struct T_optmgmt_ack	*toa = (struct T_optmgmt_ack *)buf;
	struct T_error_ack	*tea = (struct T_error_ack *)buf;
	struct opthdr		*req;

	if ((s = open("/dev/arp", O_RDWR)) == -1)
		return -1;

	/* Assume we are erroring. */
	retval = -1;

	tor->PRIM_type = T_SVR4_OPTMGMT_REQ;
	tor->OPT_offset = sizeof (struct T_optmgmt_req);
	tor->OPT_length = sizeof (struct opthdr);
	tor->MGMT_flags = T_CURRENT;

	req = (struct opthdr *)&tor[1];
	req->level = EXPER_IP_AND_ALL_IRES;
	req->name = 0;
	req->len = 1;

	ctlbuf.buf = (char *)buf;
	ctlbuf.len = tor->OPT_length + tor->OPT_offset;
	if (putmsg(s, &ctlbuf, NULL, 0) == 1)
		goto out;

	req = (struct opthdr *)&toa[1];
	ctlbuf.maxlen = sizeof(buf);

	/* Create a reasonable buffer to start with */
	databuf.maxlen = BUFSIZ * 2;
	if ((databuf.buf = malloc(databuf.maxlen)) == NULL)
		goto out;

	for (;;) {
		flags = 0;
		if ((code = getmsg(s, &ctlbuf, 0, &flags)) == -1)
			break;
		if (code == 0 &&
		    toa->PRIM_type == T_OPTMGMT_ACK &&
		    toa->MGMT_flags == T_SUCCESS &&
		    (size_t)ctlbuf.len >= sizeof(struct T_optmgmt_ack))
		{
			/* End of messages, so return success! */
			retval = 0;
			break;
		}
		if (tea->PRIM_type == T_ERROR_ACK) {
			errno = tea->TLI_error == TSYSERR ?
			    tea->UNIX_error : EPROTO;
			break;
		}
		if (code != MOREDATA ||
		    toa->PRIM_type != T_OPTMGMT_ACK ||
		    toa->MGMT_flags != T_SUCCESS)
		{
			errno = ENOMSG;
			break;
		}

		/* Try to ensure out buffer is big enough
		 * for future messages as well. */
		if ((size_t)databuf.maxlen < req->len) {
			size_t newlen;

			free(databuf.buf);
			newlen = roundup(req->len, BUFSIZ);
			if ((databuf.buf = malloc(newlen)) == NULL)
				break;
			databuf.maxlen = newlen;
		}

		flags = 0;
		if (getmsg(s, NULL, &databuf, &flags) == -1)
			break;

		/* We always have to get the data before moving onto
		 * the next item, so don't move this test higher up
		 * to avoid the buffer allocation and getmsg calls. */
		if (req->level == level && req->name == name) {
			if (walkrt(ctx, databuf.buf, req->len) == -1)
				break;
		}
	}

	free(databuf.buf);
out:
	close(s);
	return retval;
}


int
if_initrt(struct dhcpcd_ctx *ctx, int af)
{

	rt_headclear(&ctx->kroutes, af);
#ifdef INET
	if ((af == AF_UNSPEC || af == AF_INET) &&
	    if_parsert(ctx, MIB2_IP,MIB2_IP_ROUTE, if_walkrt) == -1)
		return -1;
#endif
#ifdef INET6
	if ((af == AF_UNSPEC || af == AF_INET6) &&
	    if_parsert(ctx, MIB2_IP6, MIB2_IP6_ROUTE, if_walkrt6) == -1)
		return -1;
#endif
	return 0;
}


#ifdef INET
/* XXX We should fix this to write via the BPF interface. */
ssize_t
bpf_send(const struct interface *ifp, __unused int fd, uint16_t protocol,
    const void *data, size_t len)
{
	dlpi_handle_t		dh;
	dlpi_info_t		di;
	int			r;

	if (dlpi_open(ifp->name, &dh, 0) != DLPI_SUCCESS)
		return -1;
	if ((r = dlpi_info(dh, &di, 0)) == DLPI_SUCCESS &&
	    (r = dlpi_bind(dh, protocol, NULL)) == DLPI_SUCCESS)
		r = dlpi_send(dh, di.di_bcastaddr, ifp->hwlen, data, len, NULL);
	dlpi_close(dh);
	return r == DLPI_SUCCESS ? (ssize_t)len : -1;
}

int
if_address(unsigned char cmd, const struct ipv4_addr *ia)
{
	struct sockaddr_storage	ss_addr, ss_mask;
	struct sockaddr_in	*sin_addr, *sin_mask;

	/* Either remove the alias or ensure it exists. */
	if (if_plumb(cmd, ia->iface->ctx, AF_INET, ia->alias) == -1)
		return -1;

	if (cmd == RTM_DELADDR)
		return 0;

	if (cmd != RTM_NEWADDR) {
		errno = EINVAL;
		return -1;
	}

	sin_addr = (struct sockaddr_in *)&ss_addr;
	sin_addr->sin_family = AF_INET;
	sin_addr->sin_addr = ia->addr;
	sin_mask = (struct sockaddr_in *)&ss_mask;
	sin_mask->sin_family = AF_INET;
	sin_mask->sin_addr = ia->mask;
	return if_addaddr(ia->iface->ctx->pf_inet_fd,
	    ia->alias, &ss_addr, &ss_mask);
}

int
if_addrflags(const struct interface *ifp, __unused const struct in_addr *addr,
    const char *alias)
{
	int		flags, aflags;

	aflags = if_addrflags0(ifp->ctx->pf_inet_fd, alias);
	if (aflags == -1)
		return -1;
	flags = 0;
	if (aflags & IFF_DUPLICATE)
		flags |= IN_IFF_DUPLICATED;
	return flags;
}

#endif

#ifdef INET6
int
if_address6(unsigned char cmd, const struct ipv6_addr *ia)
{
	struct sockaddr_storage	ss_addr, ss_mask;
	struct sockaddr_in6	*sin6_addr, *sin6_mask;
	struct priv		*priv;
	int			r;

	/* Either remove the alias or ensure it exists. */
	if (if_plumb(cmd, ia->iface->ctx, AF_INET6, ia->alias) == -1)
		return -1;

	if (cmd == RTM_DELADDR)
		return 0;

	if (cmd != RTM_NEWADDR) {
		errno = EINVAL;
		return -1;
	}

	priv = (struct priv *)ia->iface->ctx->priv;
	sin6_addr = (struct sockaddr_in6 *)&ss_addr;
	sin6_addr->sin6_family = AF_INET6;
	sin6_addr->sin6_addr = ia->addr;
	sin6_mask = (struct sockaddr_in6 *)&ss_mask;
	sin6_mask->sin6_family = AF_INET6;
	ipv6_mask(&sin6_mask->sin6_addr, ia->prefix_len);
	r = if_addaddr(priv->pf_inet6_fd,
	    ia->alias, &ss_addr, &ss_mask);
	if (r == -1 && errno == EEXIST)
		return 0;
	return r;
}

int
if_addrflags6(const struct interface *ifp, __unused const struct in6_addr *addr,
    const char *alias)
{
	struct priv		*priv;
	int			aflags, flags;

	priv = (struct priv *)ifp->ctx->priv;
	aflags = if_addrflags0(priv->pf_inet6_fd, alias);
	flags = 0;
	if (aflags & IFF_DUPLICATE)
		flags |= IN6_IFF_DUPLICATED;
	return flags;
}

int
if_getlifetime6(struct ipv6_addr *addr)
{

	UNUSED(addr);
	errno = ENOTSUP;
	return -1;
}

int
if_checkipv6(__unused struct dhcpcd_ctx *ctx,
    __unused const struct interface *ifp)
{

	return 0;
}
#endif
