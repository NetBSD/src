/*	$NetBSD: grabmyaddr.c,v 1.29.6.1 2013/02/25 00:24:02 tls Exp $	*/
/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
 * Copyright (C) 2008 Timo Teras <timo.teras@iki.fi>.
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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <sys/socket.h>

#ifdef __linux__
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#define USE_NETLINK
#else
#include <net/route.h>
#include <net/if.h>
#include <net/if_dl.h>
#include <sys/sysctl.h>
#define USE_ROUTE
#endif

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "session.h"
#include "debug.h"

#include "localconf.h"
#include "handler.h"
#include "grabmyaddr.h"
#include "sockmisc.h"
#include "isakmp_var.h"
#include "gcmalloc.h"
#include "nattraversal.h"

static int kernel_receive __P((void *ctx, int fd));
static int kernel_open_socket __P((void));
static void kernel_sync __P((void));

struct myaddr {
	LIST_ENTRY(myaddr) chain;
	struct sockaddr_storage addr;
	int fd;
	int udp_encap;
};

static LIST_HEAD(_myaddr_list_, myaddr) configured, opened;

static void
myaddr_delete(my)
	struct myaddr *my;
{
	if (my->fd != -1)
		isakmp_close(my->fd);
	LIST_REMOVE(my, chain);
	racoon_free(my);
}

static int
myaddr_configured(addr)
	struct sockaddr *addr;
{
	struct myaddr *cfg;

	if (LIST_EMPTY(&configured))
		return TRUE;

	LIST_FOREACH(cfg, &configured, chain) {
		if (cmpsaddr(addr, (struct sockaddr *) &cfg->addr) <= CMPSADDR_WILDPORT_MATCH)
			return TRUE;
	}

	return FALSE;
}

static int
myaddr_open(addr, udp_encap)
	struct sockaddr *addr;
	int udp_encap;
{
	struct myaddr *my;

	/* Already open? */
	LIST_FOREACH(my, &opened, chain) {
		if (cmpsaddr(addr, (struct sockaddr *) &my->addr) <= CMPSADDR_WILDPORT_MATCH)
			return TRUE;
	}

	my = racoon_calloc(1, sizeof(struct myaddr));
	if (my == NULL)
		return FALSE;

	memcpy(&my->addr, addr, sysdep_sa_len(addr));
	my->fd = isakmp_open(addr, udp_encap);
	if (my->fd < 0) {
		racoon_free(my);
		return FALSE;
	}
	my->udp_encap = udp_encap;
	LIST_INSERT_HEAD(&opened, my, chain);
	return TRUE;
}

static int
myaddr_open_all_configured(addr)
	struct sockaddr *addr;
{
	/* create all configured, not already opened addresses */
	struct myaddr *cfg, *my;

	if (addr != NULL) {
		switch (addr->sa_family) {
		case AF_INET:
#ifdef INET6
		case AF_INET6:
#endif
			break;
		default:
			return FALSE;
		}
	}

	LIST_FOREACH(cfg, &configured, chain) {
		if (addr != NULL &&
		    cmpsaddr(addr, (struct sockaddr *) &cfg->addr) > CMPSADDR_WILDPORT_MATCH)
			continue;
		if (!myaddr_open((struct sockaddr *) &cfg->addr, cfg->udp_encap))
			return FALSE;
	}
	if (LIST_EMPTY(&configured)) {
#ifdef ENABLE_HYBRID
		/* Exclude any address we got through ISAKMP mode config */
		if (exclude_cfg_addr(addr) == 0)
			return FALSE;
#endif
		set_port(addr, lcconf->port_isakmp);
		myaddr_open(addr, FALSE);
#ifdef ENABLE_NATT
		set_port(addr, lcconf->port_isakmp_natt);
		myaddr_open(addr, TRUE);
#endif
	}
	return TRUE;
}

static void
myaddr_close_all_open(addr)
	struct sockaddr *addr;
{
	/* delete all matching open sockets */
	struct myaddr *my, *next;

	for (my = LIST_FIRST(&opened); my; my = next) {
		next = LIST_NEXT(my, chain);

		if (cmpsaddr((struct sockaddr *) addr,
			     (struct sockaddr *) &my->addr)
		    <= CMPSADDR_WOP_MATCH)
			myaddr_delete(my);
	}
}

static void
myaddr_flush_list(list)
	struct _myaddr_list_ *list;
{
	struct myaddr *my, *next;

	for (my = LIST_FIRST(list); my; my = next) {
		next = LIST_NEXT(my, chain);
		myaddr_delete(my);
	}
}

void
myaddr_flush()
{
	myaddr_flush_list(&configured);
}

int
myaddr_listen(addr, udp_encap)
	struct sockaddr *addr;
	int udp_encap;
{
	struct myaddr *my;

	if (sysdep_sa_len(addr) > sizeof(my->addr)) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "sockaddr size larger than sockaddr_storage\n");
		return -1;
	}

	my = racoon_calloc(1, sizeof(struct myaddr));
	if (my == NULL)
		return -1;

	memcpy(&my->addr, addr, sysdep_sa_len(addr));
	my->udp_encap = udp_encap;
	my->fd = -1;
	LIST_INSERT_HEAD(&configured, my, chain);

	return 0;
}

void
myaddr_sync()
{
	struct myaddr *my, *next;

	if (!lcconf->strict_address) {
		kernel_sync();

		/* delete all existing listeners which are not configured */
		for (my = LIST_FIRST(&opened); my; my = next) {
			next = LIST_NEXT(my, chain);

			if (!myaddr_configured((struct sockaddr *) &my->addr))
				myaddr_delete(my);
		}
	}
}

int
myaddr_getfd(addr)
        struct sockaddr *addr;
{
	struct myaddr *my;

	LIST_FOREACH(my, &opened, chain) {
		if (cmpsaddr((struct sockaddr *) &my->addr, addr) <= CMPSADDR_WILDPORT_MATCH)
			return my->fd;
	}

	return -1;
}

int
myaddr_getsport(addr)
	struct sockaddr *addr;
{
	struct myaddr *my;
	int port = 0, wport;

	LIST_FOREACH(my, &opened, chain) {
		switch (cmpsaddr((struct sockaddr *) &my->addr, addr)) {
		case CMPSADDR_MATCH:
			return extract_port((struct sockaddr *) &my->addr);
		case CMPSADDR_WILDPORT_MATCH:
			wport = extract_port((struct sockaddr *) &my->addr);
			if (port == 0 || wport < port)
				port = wport;
			break;
		}
	}

	if (port == 0)
		port = PORT_ISAKMP;

	return port;
}

void
myaddr_init_lists()
{
	LIST_INIT(&configured);
	LIST_INIT(&opened);
}

int
myaddr_init()
{
        if (!lcconf->strict_address) {
		lcconf->rtsock = kernel_open_socket();
		if (lcconf->rtsock < 0)
			return -1;
		monitor_fd(lcconf->rtsock, kernel_receive, NULL, 0);
	} else {
		lcconf->rtsock = -1;
		if (!myaddr_open_all_configured(NULL))
			return -1;
	}
	return 0;
}

void
myaddr_close()
{
	myaddr_flush_list(&configured);
	myaddr_flush_list(&opened);
	if (lcconf->rtsock != -1) {
		unmonitor_fd(lcconf->rtsock);
		close(lcconf->rtsock);
	}
}

#if defined(USE_NETLINK)

static int netlink_fd = -1;

#define NLMSG_TAIL(nmsg) \
	((struct rtattr *) (((void *) (nmsg)) + NLMSG_ALIGN((nmsg)->nlmsg_len)))

static void
parse_rtattr(struct rtattr *tb[], int max, struct rtattr *rta, int len)
{
	memset(tb, 0, sizeof(struct rtattr *) * (max + 1));
	while (RTA_OK(rta, len)) {
		if (rta->rta_type <= max)
			tb[rta->rta_type] = rta;
		rta = RTA_NEXT(rta,len);
	}
}

static int
netlink_add_rtattr_l(struct nlmsghdr *n, int maxlen, int type,
		     const void *data, int alen)
{
	int len = RTA_LENGTH(alen);
	struct rtattr *rta;

	if (NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len) > maxlen)
		return FALSE;

	rta = NLMSG_TAIL(n);
	rta->rta_type = type;
	rta->rta_len = len;
	memcpy(RTA_DATA(rta), data, alen);
	n->nlmsg_len = NLMSG_ALIGN(n->nlmsg_len) + RTA_ALIGN(len);
	return TRUE;
}

static int
netlink_enumerate(fd, family, type)
	int fd;
	int family;
	int type;
{
	struct {
		struct nlmsghdr nlh;
		struct rtgenmsg g;
	} req;
	struct sockaddr_nl addr;
	static __u32 seq = 0;

	memset(&addr, 0, sizeof(addr));
	addr.nl_family = AF_NETLINK;

	memset(&req, 0, sizeof(req));
	req.nlh.nlmsg_len = sizeof(req);
	req.nlh.nlmsg_type = type;
	req.nlh.nlmsg_flags = NLM_F_ROOT | NLM_F_MATCH | NLM_F_REQUEST;
	req.nlh.nlmsg_pid = 0;
	req.nlh.nlmsg_seq = ++seq;
	req.g.rtgen_family = family;

	return sendto(fd, (void *) &req, sizeof(req), 0,
		      (struct sockaddr *) &addr, sizeof(addr)) >= 0;
}

static void
netlink_add_del_address(int add, struct sockaddr *saddr)
{
	plog(LLV_DEBUG, LOCATION, NULL,
	     "Netlink: address %s %s\n",
	     saddrwop2str((struct sockaddr *) saddr),
	     add ? "added" : "deleted");

	if (add)
		myaddr_open_all_configured(saddr);
	else
		myaddr_close_all_open(saddr);
}

#ifdef INET6
static int
netlink_process_addr(struct nlmsghdr *h)
{
	struct sockaddr_storage addr;
	struct ifaddrmsg *ifa;
	struct rtattr *rta[IFA_MAX+1];
	struct sockaddr_in6 *sin6;

	ifa = NLMSG_DATA(h);
	parse_rtattr(rta, IFA_MAX, IFA_RTA(ifa), IFA_PAYLOAD(h));

	if (ifa->ifa_family != AF_INET6)
		return 0;
	if (ifa->ifa_flags & IFA_F_TENTATIVE)
		return 0;
	if (rta[IFA_LOCAL] == NULL)
		rta[IFA_LOCAL] = rta[IFA_ADDRESS];
	if (rta[IFA_LOCAL] == NULL)
		return 0;

	memset(&addr, 0, sizeof(addr));
	addr.ss_family = ifa->ifa_family;
	sin6 = (struct sockaddr_in6 *) &addr;
	memcpy(&sin6->sin6_addr, RTA_DATA(rta[IFA_LOCAL]),
		sizeof(sin6->sin6_addr));
	if (!IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
		return 0;
	sin6->sin6_scope_id = ifa->ifa_index;

	netlink_add_del_address(h->nlmsg_type == RTM_NEWADDR,
				(struct sockaddr *) &addr);

	return 0;
}
#endif

static int
netlink_route_is_local(int family, const unsigned char *addr, size_t addr_len)
{
	struct {
		struct nlmsghdr n;
		struct rtmsg    r;
		char            buf[1024];
	} req;
	struct rtmsg *r = NLMSG_DATA(&req.n);
	struct rtattr *rta[RTA_MAX+1];
	struct sockaddr_nl nladdr;
	ssize_t rlen;

	memset(&req, 0, sizeof(req));
	req.n.nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg));
	req.n.nlmsg_flags = NLM_F_REQUEST;
	req.n.nlmsg_type = RTM_GETROUTE;
	req.r.rtm_family = family;
	netlink_add_rtattr_l(&req.n, sizeof(req), RTA_DST,
			     addr, addr_len);
	req.r.rtm_dst_len = addr_len * 8;

	memset(&nladdr, 0, sizeof(nladdr));
	nladdr.nl_family = AF_NETLINK;

	if (sendto(netlink_fd, &req, sizeof(req), 0,
		   (struct sockaddr *) &nladdr, sizeof(nladdr)) < 0)
		return 0;
	rlen = recv(netlink_fd, &req, sizeof(req), 0);
	if (rlen < 0)
		return 0;

	return  req.n.nlmsg_type == RTM_NEWROUTE &&
		req.r.rtm_type == RTN_LOCAL;
}

static int
netlink_process_route(struct nlmsghdr *h)
{
	struct sockaddr_storage addr;
	struct rtmsg *rtm;
	struct rtattr *rta[RTA_MAX+1];
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif

	rtm = NLMSG_DATA(h);

	/* local IP addresses get local route in the local table */
	if (rtm->rtm_type != RTN_LOCAL ||
	    rtm->rtm_table != RT_TABLE_LOCAL)
		return 0;

	parse_rtattr(rta, IFA_MAX, RTM_RTA(rtm), IFA_PAYLOAD(h));
	if (rta[RTA_DST] == NULL)
 		return 0;

	/* setup the socket address */
	memset(&addr, 0, sizeof(addr));
	addr.ss_family = rtm->rtm_family;
	switch (rtm->rtm_family) {
	case AF_INET:
		sin = (struct sockaddr_in *) &addr;
		memcpy(&sin->sin_addr, RTA_DATA(rta[RTA_DST]),
			sizeof(sin->sin_addr));
		break;
#ifdef INET6
	case AF_INET6:
		sin6 = (struct sockaddr_in6 *) &addr;
		memcpy(&sin6->sin6_addr, RTA_DATA(rta[RTA_DST]),
			sizeof(sin6->sin6_addr));
		/* Link-local addresses are handled with RTM_NEWADDR
		 * notifications */
		if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
			return 0;
		break;
#endif
	default:
		return 0;
	}

	/* If local route was deleted, check if there is still local
	 * route for the same IP on another interface */
	if (h->nlmsg_type == RTM_DELROUTE &&
	    netlink_route_is_local(rtm->rtm_family,
				   RTA_DATA(rta[RTA_DST]),
				   RTA_PAYLOAD(rta[RTA_DST]))) {
		plog(LLV_DEBUG, LOCATION, NULL,
			"Netlink: not deleting %s yet, it exists still\n",
			saddrwop2str((struct sockaddr *) &addr));
		return 0;
	}

	netlink_add_del_address(h->nlmsg_type == RTM_NEWROUTE,
				(struct sockaddr *) &addr);
	return 0;
}

static int
netlink_process(struct nlmsghdr *h)
{
	switch (h->nlmsg_type) {
#ifdef INET6
	case RTM_NEWADDR:
	case RTM_DELADDR:
		return netlink_process_addr(h);
#endif
	case RTM_NEWROUTE:
	case RTM_DELROUTE:
		return netlink_process_route(h);
	}
	return 0;
}

static int
kernel_receive(ctx, fd)
	void *ctx;
	int fd;
{
	struct sockaddr_nl nladdr;
	struct iovec iov;
	struct msghdr msg = {
		.msg_name = &nladdr,
		.msg_namelen = sizeof(nladdr),
		.msg_iov = &iov,
		.msg_iovlen = 1,
	};
	struct nlmsghdr *h;
	int len, status;
	char buf[16*1024];

	iov.iov_base = buf;
	while (1) {
		iov.iov_len = sizeof(buf);
		status = recvmsg(fd, &msg, MSG_DONTWAIT);
		if (status < 0) {
			if (errno == EINTR)
				continue;
			if (errno == EAGAIN)
				return FALSE;
			continue;
		}
		if (status == 0)
			return FALSE;

		h = (struct nlmsghdr *) buf;
		while (NLMSG_OK(h, status)) {
			netlink_process(h);
			h = NLMSG_NEXT(h, status);
		}
	}

	return TRUE;
}

static int
netlink_open_socket()
{
	int fd;

	fd = socket(PF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
	if (fd < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket(PF_NETLINK) failed: %s",
			strerror(errno));
		return -1;
	}
	close_on_exec(fd);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		plog(LLV_WARNING, LOCATION, NULL,
		     "failed to put socket in non-blocking mode\n");

	return fd;
}

static int
kernel_open_socket()
{
	struct sockaddr_nl nl;
	int fd;

	if (netlink_fd < 0) {
		netlink_fd = netlink_open_socket();
		if (netlink_fd < 0)
			return -1;
	}

	fd = netlink_open_socket();
	if (fd < 0)
		return fd;

	/* We monitor IPv4 addresses using RTMGRP_IPV4_ROUTE group
	 * the get the RTN_LOCAL routes which are automatically added
	 * by kernel. This is because:
	 *  - Linux kernel has a bug that calling bind() immediately
	 *    after IPv4 RTM_NEWADDR event can fail
	 *  - if IP is configured in multiple interfaces, we get
	 *    RTM_DELADDR for each of them. RTN_LOCAL gets deleted only
	 *    after the last IP address is deconfigured.
	 * The latter reason is also why I chose to use route
	 * notifications for IPv6. However, we do need to use RTM_NEWADDR
	 * for the link-local IPv6 addresses to get the interface index
	 * that is needed in bind().
	 */
	memset(&nl, 0, sizeof(nl));
	nl.nl_family = AF_NETLINK;
	nl.nl_groups = RTMGRP_IPV4_ROUTE 
#ifdef INET6
			| RTMGRP_IPV6_IFADDR | RTMGRP_IPV6_ROUTE
#endif
			;
	if (bind(fd, (struct sockaddr*) &nl, sizeof(nl)) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "bind(PF_NETLINK) failed: %s\n",
		     strerror(errno));
		close(fd);
		return -1;
	}
	return fd;
}

static void
kernel_sync()
{
	int fd = lcconf->rtsock;

	/* refresh addresses */
	if (!netlink_enumerate(fd, PF_UNSPEC, RTM_GETROUTE)) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "unable to enumerate addresses: %s\n",
		     strerror(errno));
	}
	while (kernel_receive(NULL, fd) == TRUE);

#ifdef INET6
	if (!netlink_enumerate(fd, PF_INET6, RTM_GETADDR)) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "unable to enumerate addresses: %s\n",
		     strerror(errno));
	}
	while (kernel_receive(NULL, fd) == TRUE);
#endif
}

#elif defined(USE_ROUTE)

#define ROUNDUP(a) \
  ((a) > 0 ? (1 + (((a) - 1) | (sizeof(long) - 1))) : sizeof(long))

#define SAROUNDUP(X)   ROUNDUP(((struct sockaddr *)(X))->sa_len)

static size_t
parse_address(start, end, dest)
	caddr_t start;
	caddr_t end;
	struct sockaddr_storage *dest;
{
	int len;

	if (start >= end)
		return 0;

	len = SAROUNDUP(start);
	if (start + len > end)
		return end - start;

	if (dest != NULL && len <= sizeof(struct sockaddr_storage))
		memcpy(dest, start, len);

	return len;
}

static void
parse_addresses(start, end, flags, addr)
	caddr_t start;
	caddr_t end;
	int flags;
	struct sockaddr_storage *addr;
{
	memset(addr, 0, sizeof(*addr));
	if (flags & RTA_DST)
		start += parse_address(start, end, NULL);
	if (flags & RTA_GATEWAY)
		start += parse_address(start, end, NULL);
	if (flags & RTA_NETMASK)
		start += parse_address(start, end, NULL);
	if (flags & RTA_GENMASK)
		start += parse_address(start, end, NULL);
	if (flags & RTA_IFP)
		start += parse_address(start, end, NULL);
	if (flags & RTA_IFA)
		start += parse_address(start, end, addr);
	if (flags & RTA_AUTHOR)
		start += parse_address(start, end, NULL);
	if (flags & RTA_BRD)
		start += parse_address(start, end, NULL);
}

static void
kernel_handle_message(msg)
	caddr_t msg;
{
	struct rt_msghdr *rtm = (struct rt_msghdr *) msg;
	struct ifa_msghdr *ifa = (struct ifa_msghdr *) msg;
	struct sockaddr_storage addr;

	switch (rtm->rtm_type) {
	case RTM_NEWADDR:
		parse_addresses(ifa + 1, msg + ifa->ifam_msglen,
				ifa->ifam_addrs, &addr);
		myaddr_open_all_configured((struct sockaddr *) &addr);
		break;
	case RTM_DELADDR:
		parse_addresses(ifa + 1, msg + ifa->ifam_msglen,
				ifa->ifam_addrs, &addr);
		myaddr_close_all_open((struct sockaddr *) &addr);
		break;
	case RTM_ADD:
	case RTM_DELETE:
	case RTM_CHANGE:
	case RTM_MISS:
	case RTM_IFINFO:
#ifdef RTM_OIFINFO
	case RTM_OIFINFO:
#endif
#ifdef RTM_NEWMADDR
	case RTM_NEWMADDR:
	case RTM_DELMADDR:
#endif
#ifdef RTM_IFANNOUNCE
	case RTM_IFANNOUNCE:
#endif
		break;
	default:
		plog(LLV_WARNING, LOCATION, NULL,
		     "unrecognized route message with rtm_type: %d",
		     rtm->rtm_type);
		break;
	}
}

static int
kernel_receive(ctx, fd)
	void *ctx;
	int fd;
{
	char buf[16*1024];
	struct rt_msghdr *rtm = (struct rt_msghdr *) buf;
	int len;

	len = read(fd, &buf, sizeof(buf));
	if (len <= 0) {
		if (len < 0 && errno != EWOULDBLOCK && errno != EAGAIN)
			plog(LLV_WARNING, LOCATION, NULL,
			     "routing socket error: %s", strerror(errno));
		return FALSE;
	}

	if (rtm->rtm_msglen != len) {
		plog(LLV_WARNING, LOCATION, NULL,
		     "kernel_receive: rtm->rtm_msglen %d, len %d, type %d\n",
		     rtm->rtm_msglen, len, rtm->rtm_type);
		return FALSE;
	}

	kernel_handle_message(buf);
	return TRUE;
}

static int
kernel_open_socket()
{
	int fd;

	fd = socket(PF_ROUTE, SOCK_RAW, 0);
	if (fd < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket(PF_ROUTE) failed: %s",
			strerror(errno));
		return -1;
	}
	close_on_exec(fd);
	if (fcntl(fd, F_SETFL, O_NONBLOCK) == -1)
		plog(LLV_WARNING, LOCATION, NULL,
		     "failed to put socket in non-blocking mode\n");

	return fd;
}

static void
kernel_sync()
{
	caddr_t ref, buf, end;
	size_t bufsiz;
	struct if_msghdr *ifm;
	struct interface *ifp;

#define MIBSIZ 6
	int mib[MIBSIZ] = {
		CTL_NET,
		PF_ROUTE,
		0,
		0, /*  AF_INET & AF_INET6 */
		NET_RT_IFLIST,
		0
	};

	if (sysctl(mib, MIBSIZ, NULL, &bufsiz, NULL, 0) < 0) {
		plog(LLV_WARNING, LOCATION, NULL,
		     "sysctl() error: %s", strerror(errno));
		return;
	}

	ref = buf = racoon_malloc(bufsiz);

	if (sysctl(mib, MIBSIZ, buf, &bufsiz, NULL, 0) >= 0) {
		/* Parse both interfaces and addresses. */
		for (end = buf + bufsiz; buf < end; buf += ifm->ifm_msglen) {
			ifm = (struct if_msghdr *) buf;
			kernel_handle_message(buf);
		}
	} else {
		plog(LLV_WARNING, LOCATION, NULL,
		     "sysctl() error: %s", strerror(errno));
	}

	racoon_free(ref);
}

#else

#error No supported interface to monitor local addresses.

#endif
