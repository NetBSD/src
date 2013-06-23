/* $NetBSD: ipv6.h,v 1.1.1.1.2.2 2013/06/23 06:26:31 tls Exp $ */

/*
 * dhcpcd - DHCP client daemon
 * Copyright (c) 2006-2013 Roy Marples <roy@marples.name>
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

#include <netinet/in.h>

#define ALLROUTERS "ff02::2"
#define HOPLIMIT 255

#define ROUNDUP8(a) (1 + (((a) - 1) | 7))

#ifndef ND6_INFINITE_LIFETIME
#  define ND6_INFINITE_LIFETIME		((uint32_t)~0)
#endif

/*
 * BSD kernels don't inform userland of DAD results.
 * Also, for RTM_NEWADDR messages the address flags could be
 * undefined leading to false positive duplicate address errors.
 * As such we listen for duplicate addresses on the wire and
 * wait the maxium possible length of time as dictated by the DAD transmission
 * counter and RFC timings.
 * See the discussion here:
 *    http://mail-index.netbsd.org/tech-net/2013/03/15/msg004019.html
 */
#ifndef __linux__
/* We guard here to avoid breaking a compile on linux ppc-64 headers */
#  include <sys/param.h>
#endif
#ifdef BSD
#  define LISTEN_DAD
#endif

/* This was fixed in NetBSD */
#ifdef __NetBSD_Prereq__
#  if __NetBSD_Prereq__(6, 99, 20)
#    undef LISTEN_DAD
#  endif
#endif

#ifdef LISTEN_DAD
#  warning kernel does not report DAD results to userland
#  warning listening to duplicated addresses on the wire
#endif

struct ipv6_addr {
	TAILQ_ENTRY(ipv6_addr) next;
	struct interface *iface;
	struct in6_addr prefix;
	int prefix_len;
	uint32_t prefix_vltime;
	uint32_t prefix_pltime;
	struct in6_addr addr;
	short flags;
	char saddr[INET6_ADDRSTRLEN];
	uint8_t iaid[4];
	struct interface *delegating_iface;

	void (*dadcallback)(void *);
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

struct rt6 {
	TAILQ_ENTRY(rt6) next;
	struct in6_addr dest;
	struct in6_addr net;
	struct in6_addr gate;
	const struct interface *iface;
	const struct ra *ra;
	int metric;
	unsigned int mtu;
};
TAILQ_HEAD(rt6head, rt6);

struct ipv6_addr_l {
	TAILQ_ENTRY(ipv6_addr_l) next;
	struct in6_addr addr;
};

TAILQ_HEAD(ipv6_addr_l_head, ipv6_addr_l);

struct ll_callback {
	TAILQ_ENTRY(ll_callback) next;
	void (*callback)(void *);
	void *arg;
};
TAILQ_HEAD(ll_callback_head, ll_callback);

struct ipv6_state {
	struct ipv6_addr_l_head addrs;
	struct ll_callback_head ll_callbacks;
};

#define IPV6_STATE(ifp)							       \
	((struct ipv6_state *)(ifp)->if_data[IF_DATA_IPV6])
#define IPV6_CSTATE(ifp)						       \
	((const struct ipv6_state *)(ifp)->if_data[IF_DATA_IPV6])

#ifdef INET6
int ipv6_init(void);
ssize_t ipv6_printaddr(char *, ssize_t, const uint8_t *, const char *);
int ipv6_makeaddr(struct in6_addr *, const struct interface *,
    const struct in6_addr *, int);
int ipv6_makeprefix(struct in6_addr *, const struct in6_addr *, int);
int ipv6_mask(struct in6_addr *, int);
int ipv6_prefixlen(const struct in6_addr *);
int ipv6_userprefix( const struct in6_addr *, short prefix_len,
    uint64_t user_number, struct in6_addr *result, short result_len);
int ipv6_addaddr(struct ipv6_addr *);
void ipv6_freedrop_addrs(struct ipv6_addrhead *, int,
    const struct interface *);
void ipv6_handleifa(int, struct if_head *,
    const char *, const struct in6_addr *, int);
int ipv6_handleifa_addrs(int, struct ipv6_addrhead *,
    const struct in6_addr *, int);
const struct ipv6_addr_l *ipv6_linklocal(const struct interface *);
const struct ipv6_addr_l *ipv6_findaddr(const struct interface *,
    const struct in6_addr *);
int ipv6_addlinklocalcallback(struct interface *, void (*)(void *), void *);
void ipv6_free_ll_callbacks(struct interface *);
void ipv6_free(struct interface *);
int ipv6_removesubnet(const struct interface *, struct ipv6_addr *);
void ipv6_buildroutes(void);

int if_address6(const struct ipv6_addr *, int);
#define add_address6(a) if_address6(a, 1)
#define del_address6(a) if_address6(a, -1)
int in6_addr_flags(const char *, const struct in6_addr *);

int if_route6(const struct rt6 *rt, int);
#define add_route6(rt) if_route6(rt, 1)
#define change_route6(rt) if_route6(rt, 0)
#define del_route6(rt) if_route6(rt, -1)
#define del_src_route6(rt) if_route6(rt, -2);

#else
#define ipv6_init() -1
#define ipv6_free_ll_callbacks(a)
#define ipv6_free(a)
#endif

#endif
