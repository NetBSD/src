/* $NetBSD: mpls_routes.c,v 1.9.2.2 2014/08/20 00:05:09 tls Exp $ */

/*-
 * Copyright (c) 2010 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Mihai Chelaru <kefren@NetBSD.org>
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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/sysctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <netmpls/mpls.h>

#include <arpa/inet.h>

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <poll.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "ldp.h"
#include "ldp_errors.h"
#include "ldp_peer.h"
#include "mpls_interface.h"
#include "tlv_stack.h"
#include "label.h"
#include "mpls_routes.h"
#include "socketops.h"

extern int      route_socket;
int             rt_seq = 200;
int		dont_catch = 0;
extern int	no_default_route;
extern int	debug_f, warn_f;
static int	my_pid = 0;

struct rt_msg   replay_rt[REPLAY_MAX];
int             replay_index = 0;

#if 0
static int read_route_socket(char *, int);
#endif
void	mask_addr(union sockunion *);
int	compare_sockunion(const union sockunion *, const union sockunion *);
static int check_if_addr_updown(struct rt_msg *, uint);

extern struct sockaddr mplssockaddr;

/* Many lines inspired or shamelessly stolen from sbin/route/route.c */

#define NEXTADDR(u) \
	do { l = RT_ROUNDUP(u->sa.sa_len); memcpy(cp, u, l); cp += l;} while(0);
#define NEXTADDR2(u) \
	do { l = RT_ROUNDUP(u.sa_len); memcpy(cp, &u, l); cp += l; } while(0);

#define CHECK_LEN(sunion) \
	if (size_cp + sunion->sa.sa_len > rlen) \
		return LDP_E_ROUTE_ERROR; \
	else \
		size_cp += sunion->sa.sa_len;

#define CHECK_MINSA \
	if (size_cp + sizeof(sa_family_t) + sizeof(uint8_t) > rlen) \
		return LDP_E_ROUTE_ERROR;

#define GETNEXT(dstunion, origunion) \
	do { \
	CHECK_MINSA \
	dstunion = (union sockunion *) ((char *) (origunion)  + \
	RT_ROUNDUP((origunion)->sa.sa_len)); \
	CHECK_LEN(dstunion) \
	} while (0);

#if 0
static int 
read_route_socket(char *s, int max)
{
	int             rv, to_read;
	struct rt_msghdr *rhdr;
	struct pollfd pfd;

	pfd.fd = route_socket;
	pfd.events = POLLRDNORM;
	pfd.revents = 0;

	errno = 0;

	do {
		rv = poll(&pfd, 1, 100);
	} while (rv == -1 && errno == EINTR);

	if (rv < 1) {
		if (rv == 0) {
			fatalp("read_route_socket: poll timeout\n");
		} else
			fatalp("read_route_socket: poll: %s",
			    strerror(errno));
		return 0;
	}

	do {
		rv = recv(route_socket, s, max, MSG_PEEK);
	} while(rv == -1 && errno == EINTR);

	if (rv < 1) {
		debugp("read_route_socket: recv error\n");
		return 0;
	}
	if (rv > max) {
		rv = max;
		debugp("read_route_socket: rv > max\n");
	}

	rhdr = (struct rt_msghdr *)s;
	to_read = rhdr->rtm_msglen > max ? max : rhdr->rtm_msglen;
	rv = 0;

	do {
		rv += recv(route_socket, s, to_read - rv, 0);
	} while (rv != to_read);

	return rv;
}
#endif	/* 0 */

/* Recalculate length */
void 
mask_addr(union sockunion * su)
{
/*
	int             olen = su->sa.sa_len;
	char           *cp1 = olen + (char *) su;

	for (su->sa.sa_len = 0; cp1 > (char *) su;)
		if (*--cp1 != 0) {
			su->sa.sa_len = 1 + cp1 - (char *) su;
			break;
		}
*/
/* Let's use INET only version for the moment */
su->sa.sa_len = 4 + from_union_to_cidr(su) / 8 +
    ( from_union_to_cidr(su) % 8 ? 1 : 0 );
}

/* creates a sockunion from an IP address */
union sockunion *
make_inet_union(const char *s)
{
	union sockunion *so_inet;

	so_inet = calloc(1, sizeof(*so_inet));

	if (!so_inet) {
		fatalp("make_inet_union: malloc problem\n");
		return NULL;
	}

	so_inet->sin.sin_len = sizeof(struct sockaddr_in);
	so_inet->sin.sin_family = AF_INET;
	inet_aton(s, &so_inet->sin.sin_addr);

	return so_inet;
}

/* creates a sockunion from a label */
union sockunion *
make_mpls_union(uint32_t label)
{
	union sockunion *so_mpls;

	so_mpls = calloc(1, sizeof(*so_mpls));

	if (!so_mpls) {
		fatalp("make_mpls_union: malloc problem\n");
		return NULL;
	}

	so_mpls->smpls.smpls_len = sizeof(struct sockaddr_mpls);
	so_mpls->smpls.smpls_family = AF_MPLS;
	so_mpls->smpls.smpls_addr.shim.label = label;

	so_mpls->smpls.smpls_addr.s_addr =
		htonl(so_mpls->smpls.smpls_addr.s_addr);

	return so_mpls;
}

int
compare_sockunion(const union sockunion * __restrict a,
    const union sockunion * __restrict b)
{
	if (a->sa.sa_len != b->sa.sa_len)
		return 1;
	return memcmp(a, b, a->sa.sa_len);
}

union sockunion *
from_cidr_to_union(uint8_t prefixlen)
{
	union sockunion *u;
	uint32_t m = 0xFFFFFFFF;

	u = calloc(1, sizeof(*u));

	if (!u) {
		fatalp("from_cidr_to_union: malloc problem\n");
		return NULL;
	}
	u->sin.sin_len = sizeof(struct sockaddr_in);
	u->sin.sin_family = AF_INET;
	if (prefixlen != 0) {
		m = (m >> (32 - prefixlen) ) << (32 - prefixlen);
		m = ntohl(m);
		u->sin.sin_addr.s_addr = m;
	}
	return u;
}

uint8_t 
from_mask_to_cidr(const char *mask)
{
	struct in_addr addr;
	uint8_t plen = 0;

	if (inet_aton(mask, &addr) != 0)
		for (; addr.s_addr; plen++)
			addr.s_addr &= addr.s_addr - 1;
	return plen;
}

uint8_t
from_union_to_cidr(const union sockunion *so_pref)
{
	const struct sockaddr_in *sin = (const struct sockaddr_in*) so_pref;
	uint32_t a;
	uint8_t r;

	a = ntohl(sin->sin_addr.s_addr);
	for (r=0; a ; a = a << 1, r++);

	return r;
}

/* returns in mask the netmask created from CIDR prefixlen */
void
from_cidr_to_mask(uint8_t prefixlen, char *mask)
{
	uint32_t a = 0;
	uint8_t plen = prefixlen < 32 ? prefixlen : 32;

	if (plen != 0)
		a = (0xffffffff >> (32 - plen)) << (32 - plen);
	snprintf(mask, 16, "%d.%d.%d.%d", a >> 24, (a << 8) >> 24,
	    (a << 16) >> 24, (a << 24) >> 24);  
}

/* From src/sbin/route/route.c */
static const char *
route_strerror(int error)
{

	switch (error) {
	case ESRCH:
		return "not in table";
	case EBUSY:
		return "entry in use";
	case ENOBUFS:
		return "routing table overflow";
	default:
		return strerror(error);
	}
}


/* Adds a route. Or changes it. */
int
add_route(union sockunion *so_dest, union sockunion *so_prefix,
    union sockunion *so_gate, union sockunion *so_ifa,
    union sockunion *so_tag, int fr, int optype)
{
	int             l, rlen, rv = LDP_E_OK;
	struct rt_msg   rm;
	char           *cp;

	if(dont_catch)
		return LDP_E_OK;

	memset(&rm, 0, sizeof(rm));
	cp = rm.m_space;

	rm.m_rtm.rtm_type = (optype == RTM_READD) ? RTM_ADD : optype;
	rm.m_rtm.rtm_flags = RTF_UP | RTF_GATEWAY | RTF_STATIC;

	rm.m_rtm.rtm_version = RTM_VERSION;
	rm.m_rtm.rtm_seq = ++rt_seq;
	rm.m_rtm.rtm_addrs = RTA_DST;
	if (so_gate)
		rm.m_rtm.rtm_addrs |= RTA_GATEWAY;

	assert(so_dest);

	/* Order is: destination, gateway, netmask, genmask, ifp, ifa, tag */
	NEXTADDR(so_dest);
	if (so_gate)
		NEXTADDR(so_gate);

	if (so_prefix) {
		union sockunion *so_prefix_temp = so_prefix;

		if (fr != FREESO) {
			/* don't modify so_prefix */
			so_prefix_temp = calloc(1, so_prefix->sa.sa_len);
			if (so_prefix_temp == NULL)
				return LDP_E_MEMORY;
			memcpy(so_prefix_temp, so_prefix, so_prefix->sa.sa_len);
		}
		mask_addr(so_prefix_temp);
		NEXTADDR(so_prefix_temp);
		if (fr != FREESO)
			free(so_prefix_temp);
		/* XXX: looks like nobody cares about this */
		rm.m_rtm.rtm_flags |= RTF_MASK;
		rm.m_rtm.rtm_addrs |= RTA_NETMASK;
	} else
		rm.m_rtm.rtm_flags |= RTF_HOST;

	/* route to mpls interface */
	if (optype != RTM_READD && so_dest->sa.sa_family != AF_MPLS) {
		NEXTADDR2(mplssockaddr);
		rm.m_rtm.rtm_addrs |= RTA_IFP;
	}

	if (so_ifa != NULL) {
		NEXTADDR(so_ifa);
		rm.m_rtm.rtm_addrs |= RTA_IFA;
	}

	if (so_tag) {
		NEXTADDR(so_tag);
		rm.m_rtm.rtm_addrs |= RTA_TAG;
	}

	rm.m_rtm.rtm_msglen = l = cp - (char *) &rm;

	if ((rlen = write(route_socket, (char *) &rm, l)) < l) {
		warnp("Error adding a route: %s\n", route_strerror(errno));
		warnp("Destination was: %s\n", satos(&so_dest->sa));
		if (so_prefix)
			warnp("Prefix was: %s\n", satos(&so_prefix->sa));
		if (so_gate)
			warnp("Gateway was: %s\n", satos(&so_gate->sa));
		rv = LDP_E_ROUTE_ERROR;
	}
	if (fr == FREESO) {
		free(so_dest);
		if (so_prefix)
			free(so_prefix);
		if (so_gate)
			free(so_gate);
		if (so_ifa)
			free(so_ifa);
		if (so_tag)
			free(so_tag);
	}

	return rv;
}

/* Deletes a route */
int
delete_route(union sockunion * so_dest, union sockunion * so_pref, int freeso)
{
	int             l, rlen;
	struct rt_msg   rm;
	char           *cp;

	if(dont_catch)
		return LDP_E_OK;

	memset(&rm, 0, sizeof(struct rt_msg));
	cp = rm.m_space;

	rm.m_rtm.rtm_type = RTM_DELETE;
	rm.m_rtm.rtm_version = RTM_VERSION;
	rm.m_rtm.rtm_seq = ++rt_seq;
	if (so_pref)
		rm.m_rtm.rtm_addrs = RTA_DST | RTA_NETMASK;
	else {
		rm.m_rtm.rtm_addrs = RTA_DST;
		rm.m_rtm.rtm_flags |= RTF_HOST;
	}

	/* destination, gateway, netmask, genmask, ifp, ifa */

	NEXTADDR(so_dest);

	if (so_pref) {
		union sockunion *so_pref_temp = so_pref; 
		if (freeso != FREESO) {
			/* don't modify the original prefix */
			so_pref_temp = calloc(1, so_pref->sa.sa_len);
			if (so_pref_temp == NULL)
				return LDP_E_MEMORY;
			memcpy(so_pref_temp, so_pref, so_pref->sa.sa_len);
		}
		mask_addr(so_pref_temp);
		NEXTADDR(so_pref_temp);
		if (freeso != FREESO)
			free(so_pref_temp);
	}
	rm.m_rtm.rtm_msglen = l = cp - (char *) &rm;

	if (freeso == FREESO) {
		free(so_dest);
		if (so_pref)
			free(so_pref);
	}
	if ((rlen = write(route_socket, (char *) &rm, l)) < l) {
	    if(so_pref) {
		char spreftmp[INET_ADDRSTRLEN];
		strlcpy(spreftmp, inet_ntoa(so_pref->sin.sin_addr),
		    INET_ADDRSTRLEN);
		warnp("Error deleting route(%s): %s/%s",
		    route_strerror(errno), satos(&so_dest->sa),
		    spreftmp);
	    } else
		warnp("Error deleting route(%s) : %s",
		    route_strerror(errno), satos(&so_dest->sa));
	    return LDP_E_NO_SUCH_ROUTE;
	}
	return LDP_E_OK;
}

#if 0
/*
 * Check for a route and returns it in rg
 * If exact_match is set it compares also the so_dest and so_pref
 * with the returned result
 */
int
get_route(struct rt_msg * rg, const union sockunion * so_dest,
    const union sockunion * so_pref, int exact_match)
{
	int             l, rlen, myseq;
	struct rt_msg   rm;
	char           *cp;
	union sockunion *su;

	memset(&rm, 0, sizeof(struct rt_msg));
	cp = rm.m_space;

	myseq = ++rt_seq;

	rm.m_rtm.rtm_type = RTM_GET;
	rm.m_rtm.rtm_version = RTM_VERSION;
	rm.m_rtm.rtm_seq = myseq;

	/*
	 * rtm_addrs should contain what we provide into this message but
	 * RTA_DST | RTA_IFP trick is allowed in order to find out the
	 * interface.
	 */

	rm.m_rtm.rtm_addrs = RTA_DST | RTA_IFP;

	/*
	 * ORDER of fields is: destination, gateway, netmask, genmask, ifp,
	 * ifa
	 */

	NEXTADDR(so_dest);
	if (so_pref) {
		union sockunion *so_pref_temp = calloc(1, so_pref->sa.sa_len);

		if (so_pref_temp == NULL)
			return LDP_E_MEMORY;
		rm.m_rtm.rtm_addrs |= RTA_NETMASK;
		memcpy(so_pref_temp, so_pref, so_pref->sa.sa_len);
		mask_addr(so_pref_temp);
		NEXTADDR(so_pref_temp);
		free(so_pref_temp);
	}
	rm.m_rtm.rtm_msglen = l = cp - (char *) &rm;

	setsockopt(route_socket, SOL_SOCKET, SO_USELOOPBACK, &(int){1},
	    sizeof(int));
	rlen = write(route_socket, (char *) &rm, l);
	setsockopt(route_socket, SOL_SOCKET, SO_USELOOPBACK, &(int){0},
	    sizeof(int));

	if (rlen < l) {
		debugp("Cannot get a route !(rlen=%d instead of %d) - %s\n",
		    rlen, l, strerror(errno));
		return LDP_E_NO_SUCH_ROUTE;
	} else
		for ( ; ; ) {
			rlen = read_route_socket((char *) rg,
			    sizeof(struct rt_msg));
			if (rlen < 1)
				break;
			/*
			 * We might lose important messages here. WORKAROUND:
			 * For now I just try to save this messages and replay
			 * them later
			 */
			if (rg->m_rtm.rtm_pid == getpid() &&
			    rg->m_rtm.rtm_seq == myseq)
				break;
			/* Fast skip */
			if (rg->m_rtm.rtm_type != RTM_ADD &&
			    rg->m_rtm.rtm_type != RTM_DELETE &&
			    rg->m_rtm.rtm_type != RTM_CHANGE &&
			    rg->m_rtm.rtm_type != RTM_NEWADDR &&
			    rg->m_rtm.rtm_type != RTM_DELADDR)
				continue;
			warnp("Added to replay PID: %d, SEQ: %d\n",
			    rg->m_rtm.rtm_pid, rg->m_rtm.rtm_seq);
			memcpy(&replay_rt[replay_index], rg,
			    sizeof(struct rt_msg));
			if (replay_index < REPLAY_MAX - 1)
				replay_index++;
			else
				fatalp("Replay index is full\n");
		}

	if (rlen <= (int)sizeof(struct rt_msghdr)) {
		debugp("Got only %d bytes, expecting at least %zu\n", rlen,
		    sizeof(struct rt_msghdr));
		return LDP_E_ROUTE_ERROR;
	}

	/* Check if we don't have a less specific route */
	if (exact_match) {
		su = (union sockunion*)(rg->m_space);
		if (compare_sockunion(so_dest, su)) {
			debugp("Dest %s ", satos(&so_dest->sa));
			debugp("not like %s\n", satos(&su->sa));
			return LDP_E_NO_SUCH_ROUTE;
		}
	}

	return LDP_E_OK;
}

#endif	/* 0 */

/* triggered when a route event occurs */
int 
check_route(struct rt_msg * rg, uint rlen)
{
	union sockunion *so_dest = NULL, *so_gate = NULL, *so_pref = NULL;
	int             so_pref_allocated = 0;
	int             prefixlen;
	size_t		size_cp;
	struct peer_map *pm;
	struct label	*lab;
	char            dest[50], gate[50], pref[50], oper[50];
	dest[0] = 0;
	gate[0] = 0;
	pref[0] = 0;

	if (rlen <= offsetof(struct rt_msghdr, rtm_type) ||
	    rg->m_rtm.rtm_version != RTM_VERSION)
		return LDP_E_ROUTE_ERROR;

	if (rg->m_rtm.rtm_type == RTM_NEWADDR ||
	    rg->m_rtm.rtm_type == RTM_DELADDR)
		return check_if_addr_updown(rg, rlen);

	size_cp = sizeof(struct rt_msghdr);
	CHECK_MINSA;

	if (my_pid == 0)
		my_pid = getpid();
	assert(my_pid != 0);

	if (rg->m_rtm.rtm_pid == my_pid ||
	    (rg->m_rtm.rtm_pid != 0 && (rg->m_rtm.rtm_flags & RTF_DONE) == 0) ||
	    (rg->m_rtm.rtm_type != RTM_ADD &&
	     rg->m_rtm.rtm_type != RTM_DELETE &&
	     rg->m_rtm.rtm_type != RTM_CHANGE))
		return LDP_E_OK;

	debugp("Check route triggered by PID: %d\n", rg->m_rtm.rtm_pid);

	if (rg->m_rtm.rtm_addrs & RTA_DST)
		so_dest = (union sockunion *) rg->m_space;
	else
		return LDP_E_ROUTE_ERROR;

	if (so_dest->sa.sa_family != AF_INET)
		return LDP_E_OK;/* We don't care about non-IP changes */

	CHECK_LEN(so_dest);

	if (rg->m_rtm.rtm_addrs & RTA_GATEWAY) {
		GETNEXT(so_gate, so_dest);
		if ((rg->m_rtm.rtm_flags & RTF_CLONING) == 0 &&
		    so_gate->sa.sa_family != AF_INET &&
		    so_gate->sa.sa_family != AF_MPLS)
			return LDP_E_OK;
	}
	if (rg->m_rtm.rtm_addrs & RTA_NETMASK) {
		if (so_gate != NULL) {
			GETNEXT(so_pref, so_gate);
		} else
			GETNEXT(so_pref, so_dest);
	}
	/* Calculate prefixlen */
	if (so_pref)
		prefixlen = from_union_to_cidr(so_pref);
	else {
		prefixlen = 32;
		if ((so_pref = from_cidr_to_union(32)) == NULL)
			return LDP_E_MEMORY;
		so_pref_allocated = 1;
	}

	so_pref->sa.sa_family = AF_INET;
	so_pref->sa.sa_len = sizeof(struct sockaddr_in);
	so_pref->sin.sin_port = 0;
	memset(&so_pref->sin.sin_zero, 0, sizeof(so_pref->sin.sin_zero));

	if (rg->m_rtm.rtm_flags & RTF_CLONING)
		so_gate = NULL;

	switch (rg->m_rtm.rtm_type) {
	case RTM_CHANGE:
		lab = label_get(so_dest, so_pref);
		if (lab) {
			send_withdraw_tlv_to_all(&so_dest->sa,
			    prefixlen);
			label_reattach_route(lab, REATT_INET_DEL);
			label_del(lab);
		}
	/* Fallthrough */
	case RTM_ADD:
		/*
		 * Check if the route is connected. If so, bind it to
		 * POP_LABEL and send announce. If not, check if the prefix
		 * was announced by a LDP neighbour and route it there
		 */

		/* First of all check if we already know this one */
		if (label_get(so_dest, so_pref) == NULL) {
			/* Just add an IMPLNULL label */
			if (so_gate == NULL)
				label_add(so_dest, so_pref, NULL,
					MPLS_LABEL_IMPLNULL, NULL, 0,
					rg->m_rtm.rtm_flags & RTF_HOST);
			else {
				pm = ldp_test_mapping(&so_dest->sa,
					 prefixlen, &so_gate->sa);
				if (pm) {
					/* create an implnull label as it
					 * gets rewritten in mpls_add_label */
					lab = label_add(so_dest, so_pref,
					    so_gate, MPLS_LABEL_IMPLNULL,
					    pm->peer, pm->lm->label,
					    rg->m_rtm.rtm_flags & RTF_HOST);
					if (lab != NULL)
						mpls_add_label(lab);
					free(pm);
				} else
					label_add(so_dest, so_pref, so_gate,
					    MPLS_LABEL_IMPLNULL, NULL, 0,
					    rg->m_rtm.rtm_flags & RTF_HOST);
			}
		} else	/* We already know about this prefix */
			fatalp("Binding already there for prefix %s/%d !\n",
			      satos(&so_dest->sa), prefixlen);
		break;
	case RTM_DELETE:
		/*
		 * Send withdraw check the binding, delete the route, delete
		 * the binding
		 */
		lab = label_get(so_dest, so_pref);
		if (!lab)
			break;
		send_withdraw_tlv_to_all(&so_dest->sa, prefixlen);
		/* No readd or delete IP route. Just delete the MPLS route */
		label_reattach_route(lab, REATT_INET_NODEL);
		label_del(lab);
		break;
	}

	if (!debug_f && !warn_f) {
		if(so_pref_allocated)
			free(so_pref);
		return LDP_E_OK;
	}

	/* Rest is just for debug */

	if (so_dest)
		strlcpy(dest, satos(&so_dest->sa), sizeof(dest));
	if (so_pref)
		snprintf(pref, sizeof(pref), "%d", prefixlen);
	if (so_gate)
		strlcpy(gate, satos(&so_gate->sa), sizeof(gate));

	switch (rg->m_rtm.rtm_type) {
	case RTM_ADD:
		strlcpy(oper, "added", 20);
		break;
	case RTM_DELETE:
		strlcpy(oper, "delete", 20);
		break;
	case RTM_GET:
		strlcpy(oper, "get", 20);
		break;
	case RTM_CHANGE:
		strlcpy(oper, "change", 20);
		break;
	case RTM_LOSING:
		strlcpy(oper, "losing", 20);
		break;
	case RTM_REDIRECT:
		strlcpy(oper, "redirect", 20);
		break;
	case RTM_NEWADDR:
		strlcpy(oper, "new address", 20);
		break;
	case RTM_DELADDR:
		strlcpy(oper, "del address", 20);
		break;
	default:
		snprintf(oper, sizeof(oper), "unknown 0x%X operation",
		    rg->m_rtm.rtm_type);
	}

	debugp("[check_route] Route %s: %s / %s -> %s by PID:%d\n", oper, dest,
		pref, gate, rg->m_rtm.rtm_pid);

	if(so_pref_allocated)
		free(so_pref);
	return LDP_E_OK;
}

/*
 * Checks NEWADDR and DELADDR messages and sends announcements accordingly
 */
static int
check_if_addr_updown(struct rt_msg * rg, uint rlen)
{
	union sockunion *ifa, *netmask;
	struct ldp_peer *p;
	struct address_list_tlv al_tlv;
	struct ifa_msghdr *msghdr = (struct ifa_msghdr *)&rg->m_rtm;
	size_t size_cp = sizeof(struct ifa_msghdr);

	if (rlen < sizeof(struct ifa_msghdr) ||
	    (msghdr->ifam_addrs & RTA_NETMASK) == 0 ||
	    (msghdr->ifam_addrs & RTA_IFA) == 0)
		return LDP_E_ROUTE_ERROR;

	CHECK_MINSA;

	/* we should have RTA_NETMASK, RTA_IFP, RTA_IFA and RTA_BRD */
	ifa = netmask = (union sockunion *)(msghdr + 1);
	if (netmask->sa.sa_family != AF_INET)
		return LDP_E_OK;
	CHECK_LEN(netmask);

	if (msghdr->ifam_addrs & RTA_IFP)
		GETNEXT(ifa, ifa);

	GETNEXT(ifa, ifa);

	if (ifa->sa.sa_family != AF_INET ||
	    ntohl(ifa->sin.sin_addr.s_addr) >> 24 == IN_LOOPBACKNET)
		return LDP_E_OK;

	memset(&al_tlv, 0, sizeof(al_tlv));
	al_tlv.type = rg->m_rtm.rtm_type == RTM_NEWADDR ? htons(LDP_ADDRESS) :
	    htons(LDP_ADDRESS_WITHDRAW);
	al_tlv.length = htons(sizeof(al_tlv) - TLV_TYPE_LENGTH);
	al_tlv.messageid = htonl(get_message_id());
	al_tlv.a_type = htons(TLV_ADDRESS_LIST);
	al_tlv.a_length = htons(sizeof(al_tlv.a_af) + sizeof(al_tlv.a_address));
	al_tlv.a_af = htons(LDP_AF_INET);
	memcpy(&al_tlv.a_address, &ifa->sin.sin_addr, sizeof(al_tlv.a_address));

	SLIST_FOREACH(p, &ldp_peer_head, peers)
		if (p->state == LDP_PEER_ESTABLISHED)
			send_tlv(p, (struct tlv *)&al_tlv);

	return LDP_E_OK;
}

int 
bind_current_routes()
{
	size_t          needed, size_cp;
	int             mib[6];
	char           *buf, *next, *lim;
	struct rt_msghdr *rtmes;
	union sockunion *so_dst, *so_pref, *so_gate;
	uint rlen;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = 0;
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
		fatalp("route-sysctl-estimate: %s",
		    strerror(errno));
		return LDP_E_ROUTE_ERROR;
	}
	if ((buf = malloc(needed)) == 0)
		return LDP_E_ROUTE_ERROR;
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
		free(buf);
		return LDP_E_ROUTE_ERROR;
	}
	lim = buf + needed;

	for (next = buf; next < lim; next += rlen) {
		rtmes = (struct rt_msghdr *) next;
		rlen = rtmes->rtm_msglen;
		size_cp = sizeof(struct rt_msghdr);
		so_gate = so_pref = NULL;
		if (rtmes->rtm_flags & RTF_LLINFO)	/* No need for arps */
			continue;
		if (!(rtmes->rtm_addrs & RTA_DST)) {
			debugp("No dst\n");
			continue;
		}

		CHECK_MINSA;
		so_dst = (union sockunion *) & rtmes[1];
		CHECK_LEN(so_dst);

		/*
		 * This function is called only at startup, so use
		 * this ocassion to delete all MPLS routes
		 */
		if (so_dst->sa.sa_family == AF_MPLS) {
			delete_route(so_dst, NULL, NO_FREESO);
			debugp("MPLS route deleted.\n");
			continue;
		}

		if (so_dst->sa.sa_family != AF_INET) {
			/*debugp("sa_dst is not AF_INET\n");*/
			continue;
		}

		/* Check if it's the default gateway */
		if (so_dst->sin.sin_addr.s_addr == 0 && no_default_route != 0)
			continue;

		/* Check if it's loopback */
		if ((ntohl(so_dst->sin.sin_addr.s_addr) >> 24)==IN_LOOPBACKNET)
			continue;

		/* Get Gateway */
		if (rtmes->rtm_addrs & RTA_GATEWAY)
			GETNEXT(so_gate, so_dst);

		/* Get prefix */
		if (rtmes->rtm_flags & RTF_HOST) {
			if ((so_pref = from_cidr_to_union(32)) == NULL)
				return LDP_E_MEMORY;
		} else if (rtmes->rtm_addrs & RTA_GATEWAY) {
			GETNEXT(so_pref, so_gate);
		} else
			GETNEXT(so_pref, so_dst);

		so_pref->sa.sa_family = AF_INET;
		so_pref->sa.sa_len = sizeof(struct sockaddr_in);
		so_pref->sin.sin_port = 0;
		memset(&so_pref->sin.sin_zero, 0, 8);

		/* Also deletes when dest is IPv4 and gateway MPLS */
		if ((rtmes->rtm_addrs & RTA_GATEWAY) &&
		    (so_gate->sa.sa_family == AF_MPLS)) {
			debugp("MPLS route to %s deleted.\n",
			    inet_ntoa(so_dst->sin.sin_addr));
			delete_route(so_dst,
			    rtmes->rtm_flags & RTF_HOST ? NULL : so_pref,
			    NO_FREESO);
			if (rtmes->rtm_flags & RTF_HOST)
				free(so_pref);
			continue;
		}

		if (so_gate != NULL && so_gate->sa.sa_family == AF_LINK)
			so_gate = NULL;	/* connected route */

		if (so_gate == NULL || so_gate->sa.sa_family == AF_INET)
			label_add(so_dst, so_pref, so_gate,
			    MPLS_LABEL_IMPLNULL, NULL, 0,
			    rtmes->rtm_flags & RTF_HOST);

		if (rtmes->rtm_flags & RTF_HOST)
			free(so_pref);
	}
	free(buf);
	return LDP_E_OK;
}

int 
flush_mpls_routes()
{
	size_t needed, size_cp;
	int mib[6];
	uint rlen;
	char *buf, *next, *lim;
	struct rt_msghdr *rtm;
	union sockunion *so_dst, *so_pref, *so_gate;

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[2] = 0;
	mib[3] = 0;
	mib[4] = NET_RT_DUMP;
	mib[5] = 0;
	if (sysctl(mib, 6, NULL, &needed, NULL, 0) < 0) {
		fatalp("route-sysctl-estimate: %s", strerror(errno));
		return LDP_E_ROUTE_ERROR;
	}
	if ((buf = malloc(needed)) == NULL) {
		fatalp("route-sysctl-estimate: %s", strerror(errno));
		return LDP_E_MEMORY;
	}
	if (sysctl(mib, 6, buf, &needed, NULL, 0) < 0) {
		free(buf);
		return LDP_E_ROUTE_ERROR;
	}
	lim = buf + needed;

	for (next = buf; next < lim; next += rlen) {
		rtm = (struct rt_msghdr *) next;
		size_cp = sizeof(struct rt_msghdr);
		rlen = rtm->rtm_msglen;
		so_pref = NULL;
		so_gate = NULL;
		if (rtm->rtm_flags & RTF_LLINFO)	/* No need for arps */
			continue;
		if (!(rtm->rtm_addrs & RTA_DST)) {
			debugp("No dst\n");
			continue;
		}
		so_dst = (union sockunion *) & rtm[1];

		if (so_dst->sa.sa_family == AF_MPLS) {
			delete_route(so_dst, NULL, NO_FREESO);
			debugp("MPLS route deleted.\n");
			continue;
		}

		if ((rtm->rtm_addrs & RTA_GATEWAY) == 0)
			continue;
		GETNEXT(so_gate, so_dst);

		if ((rtm->rtm_flags & RTF_HOST) == 0)
			GETNEXT(so_pref, so_gate);

		if (so_gate->sa.sa_family == AF_MPLS) {
			if (so_dst->sa.sa_family == AF_INET)
				debugp("MPLS route to %s deleted.\n",
				    inet_ntoa(so_dst->sin.sin_addr));
			delete_route(so_dst, so_pref, NO_FREESO);
			continue;
		}

	}
	free(buf);
	return LDP_E_OK;
}
