/* $NetBSD: ipv6.h,v 1.1.1.11 2014/09/18 20:43:58 roy Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2014 Roy Marples <roy@marples.name>
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

#ifndef IPV6_H
#define IPV6_H

#include <sys/queue.h>
#include <sys/uio.h>

#include <netinet/in.h>

#if defined(__linux__) && defined(__GLIBC__)
#  define _LINUX_IN6_H
#  include <linux/ipv6.h>
#endif

#include "dhcpcd.h"

#define ALLROUTERS "ff02::2"

#define ROUNDUP8(a)  (1 + (((a) - 1) |  7))
#define ROUNDUP16(a) (1 + (((a) - 1) | 16))

#define EUI64_GBIT		0x01
#define EUI64_UBIT		0x02
#define EUI64_TO_IFID(in6)	do {(in6)->s6_addr[8] ^= EUI64_UBIT; } while (0)
#define EUI64_GROUP(in6)	((in6)->s6_addr[8] & EUI64_GBIT)

#ifndef ND6_INFINITE_LIFETIME
#  define ND6_INFINITE_LIFETIME		((uint32_t)~0)
#endif

/* RFC7217 constants */
#define IDGEN_RETRIES	3
#define IDGEN_DELAY	1 /* second */

/*
 * BSD kernels don't inform userland of DAD results.
 * See the discussion here:
 *    http://mail-index.netbsd.org/tech-net/2013/03/15/msg004019.html
 */
#ifndef __linux__
/* We guard here to avoid breaking a compile on linux ppc-64 headers */
#  include <sys/param.h>
#endif
#ifdef BSD
#  define IPV6_POLLADDRFLAG
#endif

/* This was fixed in NetBSD */
#if defined(__NetBSD_Version__) && __NetBSD_Version__ >= 699002000
#  undef IPV6_POLLADDRFLAG
#endif

struct ipv6_addr {
	TAILQ_ENTRY(ipv6_addr) next;
	struct interface *iface;
	struct in6_addr prefix;
	uint8_t prefix_len;
	uint32_t prefix_vltime;
	uint32_t prefix_pltime;
	struct in6_addr addr;
	int addr_flags;
	short flags;
	char saddr[INET6_ADDRSTRLEN];
	uint8_t iaid[4];
	uint16_t ia_type;
	struct interface *delegating_iface;
	uint8_t prefix_exclude_len;
	struct in6_addr prefix_exclude;

	void (*dadcallback)(void *);
	int dadcounter;
	uint8_t *ns;
	size_t nslen;
	int nsprobes;
};
TAILQ_HEAD(ipv6_addrhead, ipv6_addr);

#define IPV6_AF_ONLINK		0x0001
#define	IPV6_AF_NEW		0x0002
#define IPV6_AF_STALE		0x0004
#define IPV6_AF_ADDED		0x0008
#define IPV6_AF_AUTOCONF	0x0010
#define IPV6_AF_DUPLICATED	0x0020
#define IPV6_AF_DADCOMPLETED	0x0040
#define IPV6_AF_DELEGATED	0x0080
#define IPV6_AF_DELEGATEDPFX	0x0100
#define IPV6_AF_DELEGATEDZERO	0x0200
#define IPV6_AF_REQUEST		0x0400

struct rt6 {
	TAILQ_ENTRY(rt6) next;
	struct in6_addr dest;
	struct in6_addr net;
	struct in6_addr gate;
	const struct interface *iface;
	unsigned int flags;
	unsigned int metric;
	unsigned int mtu;
};
TAILQ_HEAD(rt6_head, rt6);

struct ll_callback {
	TAILQ_ENTRY(ll_callback) next;
	void (*callback)(void *);
	void *arg;
};
TAILQ_HEAD(ll_callback_head, ll_callback);

struct ipv6_state {
	struct ipv6_addrhead addrs;
	struct ll_callback_head ll_callbacks;
};

#define IPV6_STATE(ifp)							       \
	((struct ipv6_state *)(ifp)->if_data[IF_DATA_IPV6])
#define IPV6_CSTATE(ifp)						       \
	((const struct ipv6_state *)(ifp)->if_data[IF_DATA_IPV6])

#define IP6BUFLEN	(CMSG_SPACE(sizeof(struct in6_pktinfo)) + \
			CMSG_SPACE(sizeof(int)))

#ifdef INET6
struct ipv6_ctx {
	struct sockaddr_in6 from;
	struct msghdr sndhdr;
	struct iovec sndiov[2];
	unsigned char sndbuf[CMSG_SPACE(sizeof(struct in6_pktinfo))];
	struct msghdr rcvhdr;
	struct iovec rcviov[2];
	unsigned char rcvbuf[IP6BUFLEN];
	unsigned char ansbuf[1500];
	char ntopbuf[INET6_ADDRSTRLEN];
	const char *sfrom;

	int nd_fd;
#ifdef IPV6_POLLADDRFLAG
	uint8_t polladdr_warned;
#endif
	struct ra_head *ra_routers;
	struct rt6_head *routes;

	int dhcp_fd;
};
#endif

#ifdef INET6
struct ipv6_ctx *ipv6_init(struct dhcpcd_ctx *);
ssize_t ipv6_printaddr(char *, size_t, const uint8_t *, const char *);
int ipv6_makestableprivate(struct in6_addr *addr,
    const struct in6_addr *prefix, int prefix_len,
    const struct interface *ifp, int *dad_counter);
int ipv6_makeaddr(struct in6_addr *, const struct interface *,
    const struct in6_addr *, int);
int ipv6_makeprefix(struct in6_addr *, const struct in6_addr *, int);
int ipv6_mask(struct in6_addr *, int);
uint8_t ipv6_prefixlen(const struct in6_addr *);
int ipv6_userprefix( const struct in6_addr *, short prefix_len,
    uint64_t user_number, struct in6_addr *result, short result_len);
void ipv6_checkaddrflags(void *);
int ipv6_addaddr(struct ipv6_addr *);
ssize_t ipv6_addaddrs(struct ipv6_addrhead *addrs);
void ipv6_freedrop_addrs(struct ipv6_addrhead *, int,
    const struct interface *);
void ipv6_handleifa(struct dhcpcd_ctx *ctx, int, struct if_head *,
    const char *, const struct in6_addr *, int);
int ipv6_handleifa_addrs(int, struct ipv6_addrhead *,
    const struct in6_addr *, int);
const struct ipv6_addr *ipv6_findaddr(const struct interface *,
    const struct in6_addr *);
#define ipv6_linklocal(ifp) (ipv6_findaddr((ifp), NULL))
int ipv6_addlinklocalcallback(struct interface *, void (*)(void *), void *);
void ipv6_free_ll_callbacks(struct interface *);
int ipv6_start(struct interface *);
void ipv6_free(struct interface *);
void ipv6_ctxfree(struct dhcpcd_ctx *);
int ipv6_removesubnet(struct interface *, struct ipv6_addr *);
void ipv6_buildroutes(struct dhcpcd_ctx *);

#else
#define ipv6_init(a) NULL
#define ipv6_start(a) (-1)
#define ipv6_free_ll_callbacks(a)
#define ipv6_free(a)
#define ipv6_ctxfree(a)
#endif

#endif
