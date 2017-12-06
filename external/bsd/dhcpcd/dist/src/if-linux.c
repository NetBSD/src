/*
 * Linux interface driver for dhcpcd
 * Copyright (c) 2006-2017 Roy Marples <roy@marples.name>
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

#include <asm/types.h> /* Needed for 2.4 kernels */

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/param.h>

#include <linux/if_addr.h>
#include <linux/if_link.h>
#include <linux/if_packet.h>
#include <linux/if_vlan.h>
#include <linux/filter.h>
#include <linux/netlink.h>
#include <linux/sockios.h>
#include <linux/rtnetlink.h>

#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <net/route.h>

#include <errno.h>
#include <fcntl.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "config.h"
#include "bpf.h"
#include "common.h"
#include "dev.h"
#include "dhcp.h"
#include "if.h"
#include "ipv4.h"
#include "ipv4ll.h"
#include "ipv6.h"
#include "ipv6nd.h"
#include "logerr.h"
#include "route.h"
#include "sa.h"

#ifdef HAVE_NL80211_H
#include <linux/genetlink.h>
#include <linux/nl80211.h>
#else
int if_getssid_wext(const char *ifname, uint8_t *ssid);
#endif

/* Support older kernels */
#ifndef IFLA_WIRELESS
#define IFLA_WIRELESS (IFLA_MASTER + 1)
#endif

/* For some reason, glibc doesn't include newer flags from linux/if.h
 * However, we cannot include linux/if.h directly as it conflicts
 * with the glibc version. D'oh! */
#ifndef IFF_LOWER_UP
#define IFF_LOWER_UP	0x10000		/* driver signals L1 up		*/
#endif

#define bpf_insn		sock_filter
#define BPF_SKIPTYPE
#define BPF_ETHCOOK		-ETH_HLEN
#define BPF_WHOLEPACKET	0x0fffffff /* work around buggy LPF filters */

struct priv {
	int route_fd;
	uint32_t route_pid;
	struct iovec sndrcv_iov[1];
};

/* We need this to send a broadcast for InfiniBand.
 * Our old code used sendto, but our new code writes to a raw BPF socket.
 * What header structure does IPoIB use? */
#if 0
/* Broadcast address for IPoIB */
static const uint8_t ipv4_bcast_addr[] = {
	0x00, 0xff, 0xff, 0xff,
	0xff, 0x12, 0x40, 0x1b, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff
};
#endif

#define PROC_INET6	"/proc/net/if_inet6"
#define PROC_PROMOTE	"/proc/sys/net/ipv4/conf/%s/promote_secondaries"
#define SYS_LAYER2	"/sys/class/net/%s/device/layer2"

static const char *mproc =
#if defined(__alpha__)
	"system type"
#elif defined(__arm__)
	"Hardware"
#elif defined(__avr32__)
	"cpu family"
#elif defined(__bfin__)
	"BOARD Name"
#elif defined(__cris__)
	"cpu model"
#elif defined(__frv__)
	"System"
#elif defined(__i386__) || defined(__x86_64__)
	"vendor_id"
#elif defined(__ia64__)
	"vendor"
#elif defined(__hppa__)
	"model"
#elif defined(__m68k__)
	"MMU"
#elif defined(__mips__)
	"system type"
#elif defined(__powerpc__) || defined(__powerpc64__)
	"machine"
#elif defined(__s390__) || defined(__s390x__)
	"Manufacturer"
#elif defined(__sh__)
	"machine"
#elif defined(sparc) || defined(__sparc__)
	"cpu"
#elif defined(__vax__)
	"cpu"
#else
	NULL
#endif
	;

int
if_machinearch(char *str, size_t len)
{
	FILE *fp;
	char buf[256];

	if (mproc == NULL) {
		errno = EINVAL;
		return -1;
	}

	fp = fopen("/proc/cpuinfo", "r");
	if (fp == NULL)
		return -1;

	while (fscanf(fp, "%255s : ", buf) != EOF) {
		if (strncmp(buf, mproc, strlen(mproc)) == 0 &&
		    fscanf(fp, "%255s", buf) == 1)
		{
		        fclose(fp);
			return snprintf(str, len, ":%s", buf);
		}
	}
	fclose(fp);
	errno = ESRCH;
	return -1;
}

static int
check_proc_int(const char *path)
{
	FILE *fp;
	int i;

	fp = fopen(path, "r");
	if (fp == NULL)
		return -1;
	if (fscanf(fp, "%d", &i) != 1)
		i = -1;
	fclose(fp);
	return i;
}

static ssize_t
write_path(const char *path, const char *val)
{
	FILE *fp;
	ssize_t r;

	fp = fopen(path, "w");
	if (fp == NULL)
		return -1;
	r = fprintf(fp, "%s\n", val);
	fclose(fp);
	return r;
}

int
if_init(struct interface *ifp)
{
	char path[sizeof(PROC_PROMOTE) + IF_NAMESIZE];
	int n;

	/* We enable promote_secondaries so that we can do this
	 * add 192.168.1.2/24
	 * add 192.168.1.3/24
	 * del 192.168.1.2/24
	 * and the subnet mask moves onto 192.168.1.3/24
	 * This matches the behaviour of BSD which makes coding dhcpcd
	 * a little easier as there's just one behaviour. */
	snprintf(path, sizeof(path), PROC_PROMOTE, ifp->name);
	n = check_proc_int(path);
	if (n == -1)
		return errno == ENOENT ? 0 : -1;
	if (n == 1)
		return 0;
	return write_path(path, "1") == -1 ? -1 : 0;
}

int
if_conf(struct interface *ifp)
{
	char path[sizeof(SYS_LAYER2) + IF_NAMESIZE];
	int n;

	/* Some qeth setups require the use of the broadcast flag. */
	snprintf(path, sizeof(path), SYS_LAYER2, ifp->name);
	n = check_proc_int(path);
	if (n == -1)
		return errno == ENOENT ? 0 : -1;
	if (n == 0)
		ifp->options->options |= DHCPCD_BROADCAST;
	return 0;
}

/* XXX work out Virtal Interface Masters */
int
if_vimaster(__unused const struct dhcpcd_ctx *ctx, __unused const char *ifname)
{

	return 0;
}

unsigned short
if_vlanid(const struct interface *ifp)
{
	struct vlan_ioctl_args v;

	memset(&v, 0, sizeof(v));
	strlcpy(v.device1, ifp->name, sizeof(v.device1));
	v.cmd = GET_VLAN_VID_CMD;
	if (ioctl(ifp->ctx->pf_inet_fd, SIOCSIFVLAN, &v) != 0)
		return 0; /* 0 means no VLANID */
	return (unsigned short)v.u.VID;
}

static int
_open_link_socket(struct sockaddr_nl *nl, int protocol)
{
	int fd;

	if ((fd = xsocket(AF_NETLINK, SOCK_RAW | SOCK_CLOEXEC, protocol)) == -1)
		return -1;
	nl->nl_family = AF_NETLINK;
	if (bind(fd, (struct sockaddr *)nl, sizeof(*nl)) == -1) {
		close(fd);
		return -1;
	}
	return fd;
}

int
if_opensockets_os(struct dhcpcd_ctx *ctx)
{
	struct priv *priv;
	struct sockaddr_nl snl;
	socklen_t len;

	/* Open the link socket first so it gets pid() for the socket.
	 * Then open our persistent route socket so we get a unique
	 * pid that doesn't clash with a process id for after we fork. */
	memset(&snl, 0, sizeof(snl));
	snl.nl_groups = RTMGRP_LINK;

#ifdef INET
	snl.nl_groups |= RTMGRP_IPV4_ROUTE | RTMGRP_IPV4_IFADDR;
#endif
#ifdef INET6
	snl.nl_groups |= RTMGRP_IPV6_ROUTE | RTMGRP_IPV6_IFADDR | RTMGRP_NEIGH;
#endif

	ctx->link_fd = _open_link_socket(&snl, NETLINK_ROUTE);
	if (ctx->link_fd == -1)
		return -1;

	if ((priv = calloc(1, sizeof(*priv))) == NULL)
		return -1;

	ctx->priv = priv;
	memset(&snl, 0, sizeof(snl));
	priv->route_fd = _open_link_socket(&snl, NETLINK_ROUTE);
	if (priv->route_fd == -1)
		return -1;
	len = sizeof(snl);
	if (getsockname(priv->route_fd, (struct sockaddr *)&snl, &len) == -1)
		return -1;
	priv->route_pid = snl.nl_pid;
	return 0;
}

void
if_closesockets_os(struct dhcpcd_ctx *ctx)
{
	struct priv *priv;

	if (ctx->priv != NULL) {
		priv = (struct priv *)ctx->priv;
		free(priv->sndrcv_iov[0].iov_base);
		close(priv->route_fd);
	}
}

static int
get_netlink(struct dhcpcd_ctx *ctx, struct iovec *iov,
    struct interface *ifp, int fd, int flags,
    int (*callback)(struct dhcpcd_ctx *, struct interface *, struct nlmsghdr *))
{
	struct msghdr msg;
	struct sockaddr_nl nladdr;
	ssize_t len;
	struct nlmsghdr *nlm;
	int r;
	unsigned int again;

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &nladdr;
	msg.msg_namelen = sizeof(nladdr);
	memset(&nladdr, 0, sizeof(nladdr));
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
recv_again:
	if ((len = recvmsg_realloc(fd, &msg, flags)) == -1)
		return -1;
	if (len == 0)
		return 0;

	/* Check sender */
	if (msg.msg_namelen != sizeof(nladdr)) {
		errno = EINVAL;
		return -1;
	}
	/* Ignore message if it is not from kernel */
	if (nladdr.nl_pid != 0)
		return 0;

	r = 0;
	again = 0;	/* Appease static analysis */
	for (nlm = iov->iov_base;
	     nlm && NLMSG_OK(nlm, (size_t)len);
	     nlm = NLMSG_NEXT(nlm, len))
	{
		again = (nlm->nlmsg_flags & NLM_F_MULTI);
		if (nlm->nlmsg_type == NLMSG_NOOP)
			continue;
		if (nlm->nlmsg_type == NLMSG_ERROR) {
			struct nlmsgerr *err;

			if (nlm->nlmsg_len - sizeof(*nlm) < sizeof(*err)) {
				errno = EBADMSG;
				return -1;
			}
			err = (struct nlmsgerr *)NLMSG_DATA(nlm);
			if (err->error != 0) {
				errno = -err->error;
				return -1;
			}
			break;
		}
		if (nlm->nlmsg_type == NLMSG_DONE) {
			again = 0;
			break;
		}
		if (callback && (r = callback(ctx, ifp, nlm)) != 0)
			break;
	}

	if (r == 0 && again)
		goto recv_again;

	return r;
}

static int
if_copyrt(struct dhcpcd_ctx *ctx, struct rt *rt, struct nlmsghdr *nlm)
{
	size_t len;
	struct rtmsg *rtm;
	struct rtattr *rta;
	unsigned int ifindex;
	struct sockaddr *sa;

	len = nlm->nlmsg_len - sizeof(*nlm);
	if (len < sizeof(*rtm)) {
		errno = EBADMSG;
		return -1;
	}
	rtm = (struct rtmsg *)NLMSG_DATA(nlm);
	if (rtm->rtm_table != RT_TABLE_MAIN)
		return -1;

	memset(rt, 0, sizeof(*rt));
	if (rtm->rtm_type == RTN_UNREACHABLE)
		rt->rt_flags |= RTF_REJECT;
	if (rtm->rtm_scope == RT_SCOPE_HOST)
		rt->rt_flags |= RTF_HOST;

	rta = (struct rtattr *)RTM_RTA(rtm);
	len = RTM_PAYLOAD(nlm);
	while (RTA_OK(rta, len)) {
		sa = NULL;
		switch (rta->rta_type) {
		case RTA_DST:
			sa = &rt->rt_dest;
			break;
		case RTA_GATEWAY:
			sa = &rt->rt_gateway;
			break;
		case RTA_PREFSRC:
			sa = &rt->rt_ifa;
			break;
		case RTA_OIF:
			ifindex = *(unsigned int *)RTA_DATA(rta);
			rt->rt_ifp = if_findindex(ctx->ifaces, ifindex);
			break;
		case RTA_PRIORITY:
			rt->rt_metric = *(unsigned int *)RTA_DATA(rta);
			break;
		case RTA_METRICS:
		{
			struct rtattr *r2;
			size_t l2;

			l2 = rta->rta_len;
			r2 = (struct rtattr *)RTA_DATA(rta);
			while (RTA_OK(r2, l2)) {
				switch (r2->rta_type) {
				case RTAX_MTU:
					rt->rt_mtu = *(unsigned int *)RTA_DATA(r2);
					break;
				}
				r2 = RTA_NEXT(r2, l2);
			}
			break;
		}
		}

		if (sa != NULL) {
			socklen_t salen;

			sa->sa_family = rtm->rtm_family;
			salen = sa_addrlen(sa);
			memcpy((char *)sa + sa_addroffset(sa), RTA_DATA(rta),
			    MIN(salen, RTA_PAYLOAD(rta)));
		}

		rta = RTA_NEXT(rta, len);
	}

	/* If no RTA_DST set the unspecified address for the family. */
	if (rt->rt_dest.sa_family == AF_UNSPEC)
		rt->rt_dest.sa_family = rtm->rtm_family;

	rt->rt_netmask.sa_family = rtm->rtm_family;
	sa_fromprefix(&rt->rt_netmask, rtm->rtm_dst_len);

	#if 0
	if (rt->rtp_ifp == NULL && rt->src.s_addr != INADDR_ANY) {
		struct ipv4_addr *ap;

		/* For some reason the default route comes back with the
		 * loopback interface in RTA_OIF? Lets find it by
		 * preferred source address */
		if ((ap = ipv4_findaddr(ctx, &rt->src)))
			rt->iface = ap->iface;
	}
	#endif

	if (rt->rt_ifp == NULL) {
		errno = ESRCH;
		return -1;
	}
	return 0;
}

static int
link_route(struct dhcpcd_ctx *ctx, __unused struct interface *ifp,
    struct nlmsghdr *nlm)
{
	size_t len;
	int cmd;
	struct priv *priv;
	struct rt rt;

	switch (nlm->nlmsg_type) {
	case RTM_NEWROUTE:
		cmd = RTM_ADD;
		break;
	case RTM_DELROUTE:
		cmd = RTM_DELETE;
		break;
	default:
		return 0;
	}

	len = nlm->nlmsg_len - sizeof(*nlm);
	if (len < sizeof(struct rtmsg)) {
		errno = EBADMSG;
		return -1;
	}

	/* Ignore messages we sent. */
	priv = (struct priv *)ctx->priv;
	if (nlm->nlmsg_pid == priv->route_pid)
		return 0;

	if (if_copyrt(ctx, &rt, nlm) == 0)
		rt_recvrt(cmd, &rt);

	return 0;
}

static int
link_addr(struct dhcpcd_ctx *ctx, struct interface *ifp, struct nlmsghdr *nlm)
{
	size_t len;
	struct rtattr *rta;
	struct ifaddrmsg *ifa;
	struct priv *priv;
#ifdef INET
	struct in_addr addr, net, brd;
#endif
#ifdef INET6
	struct in6_addr addr6;
#endif

	if (nlm->nlmsg_type != RTM_DELADDR && nlm->nlmsg_type != RTM_NEWADDR)
		return 0;

	len = nlm->nlmsg_len - sizeof(*nlm);
	if (len < sizeof(*ifa)) {
		errno = EBADMSG;
		return -1;
	}

	/* Ignore messages we sent. */
	priv = (struct priv*)ctx->priv;
	if (nlm->nlmsg_pid == priv->route_pid)
		return 0;

	ifa = NLMSG_DATA(nlm);
	if ((ifp = if_findindex(ctx->ifaces, ifa->ifa_index)) == NULL) {
		/* We don't know about the interface the address is for
		 * so it's not really an error */
		return 1;
	}
	rta = (struct rtattr *)IFA_RTA(ifa);
	len = NLMSG_PAYLOAD(nlm, sizeof(*ifa));
	switch (ifa->ifa_family) {
#ifdef INET
	case AF_INET:
		addr.s_addr = brd.s_addr = INADDR_ANY;
		inet_cidrtoaddr(ifa->ifa_prefixlen, &net);
		while (RTA_OK(rta, len)) {
			switch (rta->rta_type) {
			case IFA_ADDRESS:
				if (ifp->flags & IFF_POINTOPOINT) {
					memcpy(&brd.s_addr, RTA_DATA(rta),
					    sizeof(brd.s_addr));
				}
				break;
			case IFA_BROADCAST:
				memcpy(&brd.s_addr, RTA_DATA(rta),
				    sizeof(brd.s_addr));
				break;
			case IFA_LOCAL:
				memcpy(&addr.s_addr, RTA_DATA(rta),
				    sizeof(addr.s_addr));
				break;
			}
			rta = RTA_NEXT(rta, len);
		}
		ipv4_handleifa(ctx, nlm->nlmsg_type, NULL, ifp->name,
		    &addr, &net, &brd, ifa->ifa_flags);
		break;
#endif
#ifdef INET6
	case AF_INET6:
		memset(&addr6, 0, sizeof(addr6));
		while (RTA_OK(rta, len)) {
			switch (rta->rta_type) {
			case IFA_ADDRESS:
				memcpy(&addr6.s6_addr, RTA_DATA(rta),
				       sizeof(addr6.s6_addr));
				break;
			}
			rta = RTA_NEXT(rta, len);
		}
		ipv6_handleifa(ctx, nlm->nlmsg_type, NULL, ifp->name,
		    &addr6, ifa->ifa_prefixlen, ifa->ifa_flags);
		break;
#endif
	}
	return 0;
}

static uint8_t
l2addr_len(unsigned short if_type)
{

	switch (if_type) {
	case ARPHRD_ETHER: /* FALLTHROUGH */
	case ARPHRD_IEEE802: /*FALLTHROUGH */
	case ARPHRD_IEEE80211:
		return 6;
	case ARPHRD_IEEE1394:
		return 8;
	case ARPHRD_INFINIBAND:
		return 20;
	}

	/* Impossible */
	return 0;
}

static int
handle_rename(struct dhcpcd_ctx *ctx, unsigned int ifindex, const char *ifname)
{
	struct interface *ifp;

	TAILQ_FOREACH(ifp, ctx->ifaces, next) {
		if (ifp->index == ifindex && strcmp(ifp->name, ifname)) {
			dhcpcd_handleinterface(ctx, -1, ifp->name);
			/* Let dev announce the interface for renaming */
			if (!dev_listening(ctx))
				dhcpcd_handleinterface(ctx, 1, ifname);
			return 1;
		}
	}
	return 0;
}

#ifdef INET6
static int
link_neigh(struct dhcpcd_ctx *ctx, __unused struct interface *ifp,
    struct nlmsghdr *nlm)
{
	struct ndmsg *r;
	struct rtattr *rta;
	size_t len;
	struct in6_addr addr6;
	int flags;

	if (nlm->nlmsg_type != RTM_NEWNEIGH && nlm->nlmsg_type != RTM_DELNEIGH)
		return 0;
	if (nlm->nlmsg_len < sizeof(*r))
		return -1;

	r = NLMSG_DATA(nlm);
	rta = (struct rtattr *)RTM_RTA(r);
	len = RTM_PAYLOAD(nlm);
        if (r->ndm_family == AF_INET6) {
		flags = 0;
		if (r->ndm_flags & NTF_ROUTER)
			flags |= IPV6ND_ROUTER;
		if (nlm->nlmsg_type == RTM_NEWNEIGH &&
		    r->ndm_state &
		    (NUD_REACHABLE | NUD_STALE | NUD_DELAY | NUD_PROBE |
		     NUD_PERMANENT))
		        flags |= IPV6ND_REACHABLE;
		memset(&addr6, 0, sizeof(addr6));
		while (RTA_OK(rta, len)) {
			switch (rta->rta_type) {
			case NDA_DST:
				memcpy(&addr6.s6_addr, RTA_DATA(rta),
				       sizeof(addr6.s6_addr));
				break;
			}
			rta = RTA_NEXT(rta, len);
		}
		ipv6nd_neighbour(ctx, &addr6, flags);
	}

	return 0;
}
#endif

static int
link_netlink(struct dhcpcd_ctx *ctx, struct interface *ifp,
    struct nlmsghdr *nlm)
{
	int r;
	size_t len;
	struct rtattr *rta, *hwaddr;
	struct ifinfomsg *ifi;
	char ifn[IF_NAMESIZE + 1];

	r = link_route(ctx, ifp, nlm);
	if (r != 0)
		return r;
	r = link_addr(ctx, ifp, nlm);
	if (r != 0)
		return r;
#ifdef INET6
	r = link_neigh(ctx, ifp, nlm);
	if (r != 0)
		return r;
#endif

	if (nlm->nlmsg_type != RTM_NEWLINK && nlm->nlmsg_type != RTM_DELLINK)
		return 0;
	len = nlm->nlmsg_len - sizeof(*nlm);
	if ((size_t)len < sizeof(*ifi)) {
		errno = EBADMSG;
		return -1;
	}
	ifi = NLMSG_DATA(nlm);
	if (ifi->ifi_flags & IFF_LOOPBACK)
		return 0;
	rta = (void *)((char *)ifi + NLMSG_ALIGN(sizeof(*ifi)));
	len = NLMSG_PAYLOAD(nlm, sizeof(*ifi));
	*ifn = '\0';
	hwaddr = NULL;

	while (RTA_OK(rta, len)) {
		switch (rta->rta_type) {
		case IFLA_WIRELESS:
			/* Ignore wireless messages */
			if (nlm->nlmsg_type == RTM_NEWLINK &&
			    ifi->ifi_change == 0)
				return 0;
			break;
		case IFLA_IFNAME:
			strlcpy(ifn, (char *)RTA_DATA(rta), sizeof(ifn));
			break;
		case IFLA_ADDRESS:
			hwaddr = rta;
			break;
		}
		rta = RTA_NEXT(rta, len);
	}

	if (nlm->nlmsg_type == RTM_DELLINK) {
		dhcpcd_handleinterface(ctx, -1, ifn);
		return 0;
	}

	/* Virtual interfaces may not get a valid hardware address
	 * at this point.
	 * To trigger a valid hardware address pickup we need to pretend
	 * that that don't exist until they have one. */
	if (ifi->ifi_flags & IFF_MASTER && !hwaddr) {
		dhcpcd_handleinterface(ctx, -1, ifn);
		return 0;
	}

	/* Check for interface name change */
	if (handle_rename(ctx, (unsigned int)ifi->ifi_index, ifn))
		return 0;

	/* Check for a new interface */
	if ((ifp = if_find(ctx->ifaces, ifn)) == NULL) {
		/* If are listening to a dev manager, let that announce
		 * the interface rather than the kernel. */
		if (dev_listening(ctx) < 1)
			dhcpcd_handleinterface(ctx, 1, ifn);
		return 0;
	}

	/* Re-read hardware address and friends */
	if (!(ifi->ifi_flags & IFF_UP) && hwaddr) {
		uint8_t l;

		l = l2addr_len(ifi->ifi_type);
		if (hwaddr->rta_len == RTA_LENGTH(l))
			dhcpcd_handlehwaddr(ctx, ifn, RTA_DATA(hwaddr), l);
	}

	dhcpcd_handlecarrier(ctx,
	    ifi->ifi_flags & IFF_RUNNING ? LINK_UP : LINK_DOWN,
	    ifi->ifi_flags, ifn);
	return 0;
}

int
if_handlelink(struct dhcpcd_ctx *ctx)
{

	return get_netlink(ctx, ctx->iov, NULL,
	    ctx->link_fd, MSG_DONTWAIT, &link_netlink);
}

static int
send_netlink(struct dhcpcd_ctx *ctx, struct interface *ifp,
    int protocol, struct nlmsghdr *hdr,
    int (*callback)(struct dhcpcd_ctx *, struct interface *, struct nlmsghdr *))
{
	int s, r;
	struct sockaddr_nl snl;
	struct iovec iov[1];
	struct msghdr msg;

	memset(&snl, 0, sizeof(snl));
	snl.nl_family = AF_NETLINK;

	if (protocol == NETLINK_ROUTE) {
		struct priv *priv;

		priv = (struct priv *)ctx->priv;
		s = priv->route_fd;
	} else {
		if ((s = _open_link_socket(&snl, protocol)) == -1)
			return -1;
	}

	memset(&msg, 0, sizeof(msg));
	msg.msg_name = &snl;
	msg.msg_namelen = sizeof(snl);
	memset(&iov, 0, sizeof(iov));
	iov[0].iov_base = hdr;
	iov[0].iov_len = hdr->nlmsg_len;
	msg.msg_iov = iov;
	msg.msg_iovlen = 1;
	/* Request a reply */
	hdr->nlmsg_flags |= NLM_F_ACK;
	hdr->nlmsg_seq = (uint32_t)++ctx->seq;
	if (sendmsg(s, &msg, 0) != -1) {
		struct priv *priv;

		priv = (struct priv *)ctx->priv;
		ctx->sseq = ctx->seq;
		r = get_netlink(ctx, priv->sndrcv_iov, ifp, s, 0, callback);
	} else
		r = -1;
	if (protocol != NETLINK_ROUTE)
		close(s);
	return r;
}

#define NLMSG_TAIL(nmsg)						\
	((struct rtattr *)(((ptrdiff_t)(nmsg))+NLMSG_ALIGN((nmsg)->nlmsg_len)))

static int
add_attr_l(struct nlmsghdr *n, unsigned short maxlen, unsigned short type,
    const void *data, unsigned short alen)
{
	unsigned short len = (unsigned short)RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen) {
		errno = ENOBUFS;
		return -1;
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	if (alen)
		memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);

	return 0;
}

static int
add_attr_32(struct nlmsghdr *n, unsigned short maxlen, unsigned short type,
    uint32_t data)
{
	unsigned short len = RTA_LENGTH(sizeof(data));
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		errno = ENOBUFS;
		return -1;
	}

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), &data, sizeof(data));
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

	return 0;
}

static int
rta_add_attr_32(struct rtattr *rta, unsigned short maxlen,
    unsigned short type, uint32_t data)
{
	unsigned short len = RTA_LENGTH(sizeof(data));
	struct rtattr *subrta;

	if (RTA_ALIGN(rta->rta_len) + len > maxlen) {
		errno = ENOBUFS;
		return -1;
	}

	subrta = (void *)((char*)rta + RTA_ALIGN(rta->rta_len));
	subrta->rta_type = type;
	subrta->rta_len = len;
	memcpy(RTA_DATA(subrta), &data, sizeof(data));
	rta->rta_len = (unsigned short)(NLMSG_ALIGN(rta->rta_len) + len);
	return 0;
}

#ifdef HAVE_NL80211_H
static struct nlattr *
nla_next(struct nlattr *nla, size_t *rem)
{

	*rem -= (size_t)NLA_ALIGN(nla->nla_len);
	return (void *)((char *)nla + NLA_ALIGN(nla->nla_len));
}

#define NLA_TYPE(nla) ((nla)->nla_type & NLA_TYPE_MASK)
#define NLA_LEN(nla) (unsigned int)((nla)->nla_len - NLA_HDRLEN)
#define NLA_OK(nla, rem) \
	((rem) >= sizeof(struct nlattr) && \
	(nla)->nla_len >= sizeof(struct nlattr) && \
	(nla)->nla_len <= rem)
#define NLA_DATA(nla) (void *)((char *)(nla) + NLA_HDRLEN)
#define NLA_FOR_EACH_ATTR(pos, head, len, rem) \
	for (pos = head, rem = len; \
	     NLA_OK(pos, rem); \
	     pos = nla_next(pos, &(rem)))

struct nlmg
{
	struct nlmsghdr hdr;
	struct genlmsghdr ghdr;
	char buffer[64];
};

static int
nla_put_32(struct nlmsghdr *n, unsigned short maxlen,
    unsigned short type, uint32_t data)
{
	unsigned short len;
	struct nlattr *nla;

	len = NLA_ALIGN(NLA_HDRLEN + sizeof(data));
	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		errno = ENOBUFS;
		return -1;
	}

	nla = (struct nlattr *)NLMSG_TAIL(n);
	nla->nla_type = type;
	nla->nla_len = len;
	memcpy(NLA_DATA(nla), &data, sizeof(data));
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + len;

	return 0;
}

static int
nla_put_string(struct nlmsghdr *n, unsigned short maxlen,
    unsigned short type, const char *data)
{
	struct nlattr *nla;
	size_t len, sl;

	sl = strlen(data) + 1;
	len = NLA_ALIGN(NLA_HDRLEN + sl);
	if (NLMSG_ALIGN(n->nlmsg_len) + len > maxlen) {
		errno = ENOBUFS;
		return -1;
	}

	nla = (struct nlattr *)NLMSG_TAIL(n);
	nla->nla_type = type;
	nla->nla_len = (unsigned short)len;
	memcpy(NLA_DATA(nla), data, sl);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + (unsigned short)len;
	return 0;
}

static int
nla_parse(struct nlattr *tb[], struct nlattr *head, size_t len, int maxtype)
{
	struct nlattr *nla;
	size_t rem;
	int type;

	memset(tb, 0, sizeof(*tb) * ((unsigned int)maxtype + 1));
	NLA_FOR_EACH_ATTR(nla, head, len, rem) {
		type = NLA_TYPE(nla);
		if (type > maxtype)
			continue;
		tb[type] = nla;
	}
	return 0;
}

static int
genl_parse(struct nlmsghdr *nlm, struct nlattr *tb[], int maxtype)
{
	struct genlmsghdr *ghdr;
	struct nlattr *head;
	size_t len;

	ghdr = NLMSG_DATA(nlm);
	head = (void *)((char *)ghdr + GENL_HDRLEN);
	len = nlm->nlmsg_len - GENL_HDRLEN - NLMSG_HDRLEN;
	return nla_parse(tb, head, len, maxtype);
}

static int
_gnl_getfamily(__unused struct dhcpcd_ctx *ctx, __unused struct interface *ifp,
    struct nlmsghdr *nlm)
{
	struct nlattr *tb[CTRL_ATTR_FAMILY_ID + 1];
	uint16_t family;

	if (genl_parse(nlm, tb, CTRL_ATTR_FAMILY_ID) == -1)
		return -1;
	if (tb[CTRL_ATTR_FAMILY_ID] == NULL) {
		errno = ENOENT;
		return -1;
	}
	memcpy(&family, NLA_DATA(tb[CTRL_ATTR_FAMILY_ID]), sizeof(family));
	return (int)family;
}

static int
gnl_getfamily(struct dhcpcd_ctx *ctx, const char *name)
{
	struct nlmg nlm;

	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
	nlm.hdr.nlmsg_type = GENL_ID_CTRL;
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST;
	nlm.ghdr.cmd = CTRL_CMD_GETFAMILY;
	nlm.ghdr.version = 1;
	if (nla_put_string(&nlm.hdr, sizeof(nlm),
	    CTRL_ATTR_FAMILY_NAME, name) == -1)
		return -1;
	return send_netlink(ctx, NULL, NETLINK_GENERIC, &nlm.hdr,
	    &_gnl_getfamily);
}

static int
_if_getssid_nl80211(__unused struct dhcpcd_ctx *ctx, struct interface *ifp,
    struct nlmsghdr *nlm)
{
	struct nlattr *tb[NL80211_ATTR_BSS + 1];
	struct nlattr *bss[NL80211_BSS_STATUS + 1];
	uint32_t status;
	unsigned char *ie;
	int ie_len;

	if (genl_parse(nlm, tb, NL80211_ATTR_BSS) == -1)
		return 0;

	if (tb[NL80211_ATTR_BSS] == NULL)
		return 0;

	if (nla_parse(bss,
	    NLA_DATA(tb[NL80211_ATTR_BSS]),
	    NLA_LEN(tb[NL80211_ATTR_BSS]),
	    NL80211_BSS_STATUS) == -1)
		return 0;

	if (bss[NL80211_BSS_BSSID] == NULL || bss[NL80211_BSS_STATUS] == NULL)
		return 0;

	memcpy(&status, NLA_DATA(bss[NL80211_BSS_STATUS]), sizeof(status));
	if (status != NL80211_BSS_STATUS_ASSOCIATED)
		return 0;

	if (bss[NL80211_BSS_INFORMATION_ELEMENTS] == NULL)
		return 0;

	ie = NLA_DATA(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
	ie_len = (int)NLA_LEN(bss[NL80211_BSS_INFORMATION_ELEMENTS]);
	/* ie[0] is type, ie[1] is lenth, ie[2..] is data */
	while (ie_len >= 2 && ie_len >= ie[1]) {
		if (ie[0] == 0) {
			/* SSID */
			if (ie[1] > IF_SSIDLEN) {
				errno = ENOBUFS;
				return -1;
			}
			ifp->ssid_len = ie[1];
			memcpy(ifp->ssid, ie + 2, ifp->ssid_len);
			return (int)ifp->ssid_len;
		}
		ie_len -= ie[1] + 2;
		ie += ie[1] + 2;
	}

	return 0;
}

static int
if_getssid_nl80211(struct interface *ifp)
{
	int family;
	struct nlmg nlm;

	errno = 0;
	family = gnl_getfamily(ifp->ctx, "nl80211");
	if (family == -1)
		return -1;

	/* Is this a wireless interface? */
	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
	nlm.hdr.nlmsg_type = (unsigned short)family;
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST;
	nlm.ghdr.cmd = NL80211_CMD_GET_WIPHY;
	nla_put_32(&nlm.hdr, sizeof(nlm), NL80211_ATTR_IFINDEX, ifp->index);
	if (send_netlink(ifp->ctx, ifp, NETLINK_GENERIC, &nlm.hdr, NULL) == -1)
		return -1;

	/* We need to parse out the list of scan results and find the one
	 * we are connected to. */
	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct genlmsghdr));
	nlm.hdr.nlmsg_type = (unsigned short)family;
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_DUMP;
	nlm.ghdr.cmd = NL80211_CMD_GET_SCAN;
	nla_put_32(&nlm.hdr, sizeof(nlm), NL80211_ATTR_IFINDEX, ifp->index);

	return send_netlink(ifp->ctx, ifp,
	    NETLINK_GENERIC, &nlm.hdr, &_if_getssid_nl80211);
}
#endif

int
if_getssid(struct interface *ifp)
{
	int r;

#ifdef HAVE_NL80211_H
	r = if_getssid_nl80211(ifp);
	if (r == -1)
		ifp->ssid_len = 0;
#else
	r = if_getssid_wext(ifp->name, ifp->ssid);
	if (r != -1)
		ifp->ssid_len = (unsigned int)r;
#endif

	ifp->ssid[ifp->ssid_len] = '\0';
	return r;
}

struct nlma
{
	struct nlmsghdr hdr;
	struct ifaddrmsg ifa;
	char buffer[64];
};

struct nlmr
{
	struct nlmsghdr hdr;
	struct rtmsg rt;
	char buffer[256];
};

int
if_route(unsigned char cmd, const struct rt *rt)
{
	struct nlmr nlm;
	bool gateway_unspec;

	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	switch (cmd) {
	case RTM_CHANGE:
		nlm.hdr.nlmsg_type = RTM_NEWROUTE;
		nlm.hdr.nlmsg_flags = NLM_F_CREATE | NLM_F_REPLACE;
		break;
	case RTM_ADD:
		nlm.hdr.nlmsg_type = RTM_NEWROUTE;
		nlm.hdr.nlmsg_flags = NLM_F_CREATE | NLM_F_EXCL;
		break;
	case RTM_DELETE:
		nlm.hdr.nlmsg_type = RTM_DELROUTE;
		break;
	}
	nlm.hdr.nlmsg_flags |= NLM_F_REQUEST;
	nlm.rt.rtm_family = (unsigned char)rt->rt_dest.sa_family;
	nlm.rt.rtm_table = RT_TABLE_MAIN;

	gateway_unspec = sa_is_unspecified(&rt->rt_gateway);

	if (cmd == RTM_DELETE) {
		nlm.rt.rtm_scope = RT_SCOPE_NOWHERE;
	} else {
		/* Address generated routes are RTPROT_KERNEL,
		 * otherwise RTPROT_BOOT */
#ifdef RTPROT_RA
		if (rt->rt_dflags & RTDF_RA)
			nlm.rt.rtm_protocol = RTPROT_RA;
		else
#endif
#ifdef RTPROT_DHCP
		if (rt->rt_dflags & RTDF_DHCP)
			nlm.rt.rtm_protocol = RTPROT_DHCP;
		else
#endif
		if (rt->rt_dflags & RTDF_IFA_ROUTE)
			nlm.rt.rtm_protocol = RTPROT_KERNEL;
		else
			nlm.rt.rtm_protocol = RTPROT_BOOT;
		if (rt->rt_ifp->flags & IFF_LOOPBACK)
			nlm.rt.rtm_scope = RT_SCOPE_HOST;
		else if (gateway_unspec || sa_is_allones(&rt->rt_netmask))
			nlm.rt.rtm_scope = RT_SCOPE_LINK;
		else
			nlm.rt.rtm_scope = RT_SCOPE_UNIVERSE;
		if (rt->rt_flags & RTF_REJECT)
			nlm.rt.rtm_type = RTN_UNREACHABLE;
		else
			nlm.rt.rtm_type = RTN_UNICAST;
	}

#define ADDSA(type, sa)							\
	add_attr_l(&nlm.hdr, sizeof(nlm), (type),			\
	    (const char *)(sa) + sa_addroffset((sa)),			\
	    (unsigned short)sa_addrlen((sa)));
	nlm.rt.rtm_dst_len = (unsigned char)sa_toprefix(&rt->rt_netmask);
	ADDSA(RTA_DST, &rt->rt_dest);
	if (cmd == RTM_ADD || cmd == RTM_CHANGE) {
		if (!gateway_unspec)
			ADDSA(RTA_GATEWAY, &rt->rt_gateway);
		/* Cannot add tentative source addresses.
		 * We don't know this here, so just skip INET6 ifa's.*/
		if (!sa_is_unspecified(&rt->rt_ifa) &&
		    rt->rt_ifa.sa_family != AF_INET6)
			ADDSA(RTA_PREFSRC, &rt->rt_ifa);
		if (rt->rt_mtu) {
			char metricsbuf[32];
			struct rtattr *metrics = (void *)metricsbuf;

			metrics->rta_type = RTA_METRICS;
			metrics->rta_len = RTA_LENGTH(0);
			rta_add_attr_32(metrics, sizeof(metricsbuf),
			    RTAX_MTU, rt->rt_mtu);
			add_attr_l(&nlm.hdr, sizeof(nlm), RTA_METRICS,
			    RTA_DATA(metrics),
			    (unsigned short)RTA_PAYLOAD(metrics));
		}
	}

	if (!sa_is_loopback(&rt->rt_gateway))
		add_attr_32(&nlm.hdr, sizeof(nlm), RTA_OIF, rt->rt_ifp->index);

	if (rt->rt_metric != 0)
		add_attr_32(&nlm.hdr, sizeof(nlm), RTA_PRIORITY,
		    rt->rt_metric);

	return send_netlink(rt->rt_ifp->ctx, NULL,
	    NETLINK_ROUTE, &nlm.hdr, NULL);
}

static int
_if_initrt(struct dhcpcd_ctx *ctx, __unused struct interface *ifp,
    struct nlmsghdr *nlm)
{
	struct rt rt;

	if (if_copyrt(ctx, &rt, nlm) == 0) {
		rt.rt_dflags |= RTDF_INIT;
		rt_recvrt(RTM_ADD, &rt);
	}
	return 0;
}

int
if_initrt(struct dhcpcd_ctx *ctx, int af)
{
	struct nlmr nlm;

	rt_headclear(&ctx->kroutes, af);

	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	nlm.hdr.nlmsg_type = RTM_GETROUTE;
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST | NLM_F_MATCH;
	nlm.rt.rtm_table = RT_TABLE_MAIN;
	nlm.rt.rtm_family = (unsigned char)af;

	return send_netlink(ctx, NULL, NETLINK_ROUTE, &nlm.hdr, &_if_initrt);
}


#ifdef INET
/* Linux is a special snowflake when it comes to BPF. */
const char *bpf_name = "Packet Socket";
#define	BPF_BUFFER_LEN		1500

int
bpf_open(struct interface *ifp, int (*filter)(struct interface *, int))
{
	struct ipv4_state *state;
/* Linux is a special snowflake for opening BPF. */
	int s;
	union sockunion {
		struct sockaddr sa;
		struct sockaddr_ll sll;
		struct sockaddr_storage ss;
	} su;
#ifdef PACKET_AUXDATA
	int n;
#endif

#define SF	SOCK_CLOEXEC | SOCK_NONBLOCK
	if ((s = xsocket(PF_PACKET, SOCK_RAW | SF, htons(ETH_P_ALL))) == -1)
		return -1;
#undef SF

	/* Allocate a suitably large buffer for a single packet. */
	state = ipv4_getstate(ifp);
	if (state == NULL)
		goto eexit;
	if (state->buffer_size < ETH_DATA_LEN) {
		void *nb;

		if ((nb = realloc(state->buffer, ETH_DATA_LEN)) == NULL)
			goto eexit;
		state->buffer = nb;
		state->buffer_size = ETH_DATA_LEN;
		state->buffer_len = state->buffer_pos = 0;
	}

	if (filter(ifp, s) != 0)
		goto eexit;

#ifdef PACKET_AUXDATA
	n = 1;
	if (setsockopt(s, SOL_PACKET, PACKET_AUXDATA, &n, sizeof(n)) != 0) {
		if (errno != ENOPROTOOPT)
			goto eexit;
	}
#endif

	memset(&su, 0, sizeof(su));
	su.sll.sll_family = PF_PACKET;
	su.sll.sll_protocol = htons(ETH_P_ALL);
	su.sll.sll_ifindex = (int)ifp->index;
	if (bind(s, &su.sa, sizeof(su.sll)) == -1)
		goto eexit;
	return s;

eexit:
	if (state != NULL) {
		free(state->buffer);
		state->buffer = NULL;
	}
	close(s);
	return -1;
}

/* BPF requires that we read the entire buffer.
 * So we pass the buffer in the API so we can loop on >1 packet. */
ssize_t
bpf_read(struct interface *ifp, int s, void *data, size_t len,
    unsigned int *flags)
{
	ssize_t bytes;
	struct ipv4_state *state = IPV4_STATE(ifp);

	struct iovec iov = {
		.iov_base = state->buffer,
		.iov_len = state->buffer_size
	};
	struct msghdr msg = { .msg_iov = &iov, .msg_iovlen = 1 };
#ifdef PACKET_AUXDATA
	unsigned char cmsgbuf[CMSG_LEN(sizeof(struct tpacket_auxdata))];
	struct cmsghdr *cmsg;
	struct tpacket_auxdata *aux;
#endif

#ifdef PACKET_AUXDATA
	msg.msg_control = cmsgbuf;
	msg.msg_controllen = sizeof(cmsgbuf);
#endif

	bytes = recvmsg(s, &msg, 0);
	if (bytes == -1)
		return -1;
	*flags |= BPF_EOF; /* We only ever read one packet. */
	*flags &= ~BPF_PARTIALCSUM;
	if (bytes) {
		ssize_t fl = (ssize_t)bpf_frame_header_len(ifp);

		bytes -= fl;
		if ((size_t)bytes > len)
			bytes = (ssize_t)len;
		memcpy(data, state->buffer + fl, (size_t)bytes);
#ifdef PACKET_AUXDATA
		for (cmsg = CMSG_FIRSTHDR(&msg);
		     cmsg;
		     cmsg = CMSG_NXTHDR(&msg, cmsg))
		{
			if (cmsg->cmsg_level == SOL_PACKET &&
			    cmsg->cmsg_type == PACKET_AUXDATA) {
				aux = (void *)CMSG_DATA(cmsg);
				if (aux->tp_status & TP_STATUS_CSUMNOTREADY)
					*flags |= BPF_PARTIALCSUM;
			}
		}
#endif
	}
	return bytes;
}

int
bpf_attach(int s, void *filter, unsigned int filter_len)
{
	struct sock_fprog pf;

	/* Install the filter. */
	memset(&pf, 0, sizeof(pf));
	pf.filter = filter;
	pf.len = (unsigned short)filter_len;
	return setsockopt(s, SOL_SOCKET, SO_ATTACH_FILTER, &pf, sizeof(pf));
}

int
if_address(unsigned char cmd, const struct ipv4_addr *addr)
{
	struct nlma nlm;
	int retval = 0;

	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST;
	nlm.hdr.nlmsg_type = cmd;
	if (cmd == RTM_NEWADDR)
		nlm.hdr.nlmsg_flags |= NLM_F_CREATE | NLM_F_REPLACE;
	nlm.ifa.ifa_index = addr->iface->index;
	nlm.ifa.ifa_family = AF_INET;
	nlm.ifa.ifa_prefixlen = inet_ntocidr(addr->mask);
#if 0
	/* This creates the aliased interface */
	add_attr_l(&nlm.hdr, sizeof(nlm), IFA_LABEL,
	    addr->iface->alias,
	    (unsigned short)(strlen(addr->iface->alias) + 1));
#endif
	add_attr_l(&nlm.hdr, sizeof(nlm), IFA_LOCAL,
	    &addr->addr.s_addr, sizeof(addr->addr.s_addr));
	if (cmd == RTM_NEWADDR)
		add_attr_l(&nlm.hdr, sizeof(nlm), IFA_BROADCAST,
		    &addr->brd.s_addr, sizeof(addr->brd.s_addr));

	if (send_netlink(addr->iface->ctx, NULL,
	    NETLINK_ROUTE, &nlm.hdr, NULL) == -1)
		retval = -1;
	return retval;
}

int
if_addrflags(__unused const struct interface *ifp,
__unused const struct in_addr *addr, __unused const char *alias)
{

	/* Linux has no support for IPv4 address flags */
	return 0;
}
#endif

#ifdef INET6
int
if_address6(unsigned char cmd, const struct ipv6_addr *ia)
{
	struct nlma nlm;
	struct ifa_cacheinfo cinfo;
/* IFA_FLAGS is not a define, but is was added at the same time
 * IFA_F_NOPREFIXROUTE was do use that. */
#if defined(IFA_F_NOPREFIXROUTE) || defined(IFA_F_MANAGETEMPADDR)
	uint32_t flags = 0;
#endif

	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifaddrmsg));
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST;
	nlm.hdr.nlmsg_type = cmd;
	if (cmd == RTM_NEWADDR)
		nlm.hdr.nlmsg_flags |= NLM_F_CREATE | NLM_F_REPLACE;
	nlm.ifa.ifa_index = ia->iface->index;
	nlm.ifa.ifa_family = AF_INET6;
#ifdef IPV6_MANAGETEMPADDR
	if (ia->flags & IPV6_AF_TEMPORARY) {
		/* Currently the kernel filters out these flags */
#ifdef IFA_F_NOPREFIXROUTE
		flags |= IFA_F_TEMPORARY;
#else
		nlm.ifa.ifa_flags |= IFA_F_TEMPORARY;
#endif
	}
#elif IFA_F_MANAGETEMPADDR
	if (ia->flags & IPV6_AF_AUTOCONF)
		flags |= IFA_F_MANAGETEMPADDR;
#endif

	/* Add as /128 if no IFA_F_NOPREFIXROUTE ? */
	nlm.ifa.ifa_prefixlen = ia->prefix_len;
#if 0
	/* This creates the aliased interface */
	add_attr_l(&nlm.hdr, sizeof(nlm), IFA_LABEL,
	    ia->iface->alias, (unsigned short)(strlen(ia->iface->alias) + 1));
#endif
	add_attr_l(&nlm.hdr, sizeof(nlm), IFA_LOCAL,
	    &ia->addr.s6_addr, sizeof(ia->addr.s6_addr));

	if (cmd == RTM_NEWADDR) {
		memset(&cinfo, 0, sizeof(cinfo));
		cinfo.ifa_prefered = ia->prefix_pltime;
		cinfo.ifa_valid = ia->prefix_vltime;
		add_attr_l(&nlm.hdr, sizeof(nlm), IFA_CACHEINFO,
		    &cinfo, sizeof(cinfo));
	}

#ifdef IFA_F_NOPREFIXROUTE
	if (!IN6_IS_ADDR_LINKLOCAL(&ia->addr))
		flags |= IFA_F_NOPREFIXROUTE;
#endif
#if defined(IFA_F_NOPREFIXROUTE) || defined(IFA_F_MANAGETEMPADDR)
	if (flags)
		add_attr_32(&nlm.hdr, sizeof(nlm), IFA_FLAGS, flags);
#endif

	return send_netlink(ia->iface->ctx, NULL,
	    NETLINK_ROUTE, &nlm.hdr, NULL);
}

int
if_addrflags6(const struct interface *ifp, const struct in6_addr *addr,
    __unused const char *alias)
{
	FILE *fp;
	char *p, ifaddress[33], address[33], name[IF_NAMESIZE + 1];
	unsigned int ifindex;
	int prefix, scope, flags, i;

	fp = fopen(PROC_INET6, "r");
	if (fp == NULL)
		return -1;

	p = ifaddress;
	for (i = 0; i < (int)sizeof(addr->s6_addr); i++) {
		p += snprintf(p, 3, "%.2x", addr->s6_addr[i]);
	}
	*p = '\0';

	while (fscanf(fp, "%32[a-f0-9] %x %x %x %x %"TOSTRING(IF_NAMESIZE)"s\n",
	    address, &ifindex, &prefix, &scope, &flags, name) == 6)
	{
		if (strlen(address) != 32) {
			fclose(fp);
			errno = EINVAL;
			return -1;
		}
		if (strcmp(name, ifp->name) == 0 &&
		    strcmp(ifaddress, address) == 0)
		{
			fclose(fp);
			return flags;
		}
	}

	fclose(fp);
	errno = ESRCH;
	return -1;
}

int
if_getlifetime6(__unused struct ipv6_addr *ia)
{

	/* God knows how to work out address lifetimes on Linux */
	errno = ENOTSUP;
	return -1;
}

struct nlml
{
	struct nlmsghdr hdr;
	struct ifinfomsg i;
	char buffer[32];
};

static int
add_attr_8(struct nlmsghdr *n, unsigned short maxlen, unsigned short type,
    uint8_t data)
{

	return add_attr_l(n, maxlen, type, &data, sizeof(data));
}

static struct rtattr *
add_attr_nest(struct nlmsghdr *n, unsigned short maxlen, unsigned short type)
{
	struct rtattr *nest;

	nest = NLMSG_TAIL(n);
	add_attr_l(n, maxlen, type, NULL, 0);
	return nest;
}

static void
add_attr_nest_end(struct nlmsghdr *n, struct rtattr *nest)
{

	nest->rta_len = (unsigned short)((char *)NLMSG_TAIL(n) - (char *)nest);
}

static int
if_disable_autolinklocal(struct dhcpcd_ctx *ctx, unsigned int ifindex)
{
#ifdef HAVE_IN6_ADDR_GEN_MODE_NONE
	struct nlml nlm;
	struct rtattr *afs, *afs6;

	memset(&nlm, 0, sizeof(nlm));
	nlm.hdr.nlmsg_len = NLMSG_LENGTH(sizeof(struct ifinfomsg));
	nlm.hdr.nlmsg_type = RTM_NEWLINK;
	nlm.hdr.nlmsg_flags = NLM_F_REQUEST;
	nlm.i.ifi_family = AF_INET6;
	nlm.i.ifi_index = (int)ifindex;
	afs = add_attr_nest(&nlm.hdr, sizeof(nlm), IFLA_AF_SPEC);
	afs6 = add_attr_nest(&nlm.hdr, sizeof(nlm), AF_INET6);
	add_attr_8(&nlm.hdr, sizeof(nlm), IFLA_INET6_ADDR_GEN_MODE,
	    IN6_ADDR_GEN_MODE_NONE);
	add_attr_nest_end(&nlm.hdr, afs6);
	add_attr_nest_end(&nlm.hdr, afs);

	return send_netlink(ctx, NULL, NETLINK_ROUTE, &nlm.hdr, NULL);
#else
	UNUSED(ctx);
	UNUSED(ifindex);
	errno = ENOTSUP;
	return -1;
#endif
}

static const char *prefix = "/proc/sys/net/ipv6/conf";

int
if_checkipv6(struct dhcpcd_ctx *ctx, const struct interface *ifp)
{
	const char *ifname;
	int ra;
	char path[256];

	if (ifp == NULL)
		ifname = "all";
	else if (!(ctx->options & DHCPCD_TEST)) {
		if (if_disable_autolinklocal(ctx, ifp->index) == -1)
			logdebug("%s: if_disable_autolinklocal",
			    ifp->name);
	}
	if (ifp)
		ifname = ifp->name;

	snprintf(path, sizeof(path), "%s/%s/autoconf", prefix, ifname);
	ra = check_proc_int(path);
	if (ra != 1) {
		if (ctx->options & DHCPCD_TEST)
			logwarnx("%s: IPv6 kernel autoconf disabled", ifname);
	} else if (ra != -1 && !(ctx->options & DHCPCD_TEST)) {
		if (write_path(path, "0") == -1) {
			logerr("%s: %s", __func__, path);
			return -1;
		}
	}

	snprintf(path, sizeof(path), "%s/%s/accept_ra", prefix, ifname);
	ra = check_proc_int(path);
	if (ra == -1) {
		logfunc_t *logfunc = errno == ENOENT? logdebug : logwarn;

		/* The sysctl probably doesn't exist, but this isn't an
		 * error as such so just log it and continue */
		logfunc("%s", path);
	} else if (ra != 0 && !(ctx->options & DHCPCD_TEST)) {
		logdebugx("%s: disabling kernel IPv6 RA support", ifname);
		if (write_path(path, "0") == -1) {
			logerr("%s: %s", __func__, path);
			return ra;
		}
		return 0;
	}

	return ra;
}

#ifdef IPV6_MANAGETEMPADDR
int
ip6_use_tempaddr(const char *ifname)
{
	char path[256];
	int val;

	if (ifname == NULL)
		ifname = "all";
	snprintf(path, sizeof(path), "%s/%s/use_tempaddr", prefix, ifname);
	val = check_proc_int(path);
	return val == -1 ? 0 : val;
}

int
ip6_temp_preferred_lifetime(const char *ifname)
{
	char path[256];
	int val;

	if (ifname == NULL)
		ifname = "all";
	snprintf(path, sizeof(path), "%s/%s/temp_prefered_lft", prefix,
	    ifname);
	val = check_proc_int(path);
	return val < 0 ? TEMP_PREFERRED_LIFETIME : val;
}

int
ip6_temp_valid_lifetime(const char *ifname)
{
	char path[256];
	int val;

	if (ifname == NULL)
		ifname = "all";
	snprintf(path, sizeof(path), "%s/%s/temp_valid_lft", prefix, ifname);
	val = check_proc_int(path);
	return val < 0 ? TEMP_VALID_LIFETIME : val;
}
#endif /* IPV6_MANAGETEMPADDR */
#endif /* INET6 */
