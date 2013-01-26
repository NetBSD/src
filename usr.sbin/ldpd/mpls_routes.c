/* $NetBSD: mpls_routes.c,v 1.10 2013/01/26 17:29:55 kefren Exp $ */

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

extern int      route_socket;
int             rt_seq = 0;
int		dont_catch = 0;
extern int	no_default_route;
extern int	debug_f, warn_f;

struct rt_msg   replay_rt[REPLAY_MAX];
int             replay_index = 0;

static int read_route_socket(char *, int);
void	mask_addr(union sockunion *);
int	compare_sockunion(union sockunion *, union sockunion *);
char *	mpls_ntoa(union mpls_shim);

extern struct sockaddr mplssockaddr;

/* Many lines inspired or shamelessly stolen from sbin/route/route.c */

#define NEXTADDR(u) \
	do { l = RT_ROUNDUP(u->sa.sa_len); memcpy(cp, u, l); cp += l; } while(0);
#define NEXTADDR2(u) \
	do { l = RT_ROUNDUP(u.sa_len); memcpy(cp, &u, l); cp += l; } while(0);
#define GETNEXT(sunion) \
	(union sockunion *) ((char *) (sunion)  + RT_ROUNDUP((sunion)->sa.sa_len))

static int 
read_route_socket(char *s, int max)
{
	int             rv, to_read;
	fd_set          fs;
	struct timeval  tv;
	struct rt_msghdr *rhdr;

	tv.tv_sec = 0;
	tv.tv_usec = 5000;

	FD_ZERO(&fs);
	FD_SET(route_socket, &fs);

	errno = 0;

	do {
		rv = select(route_socket + 1, &fs, NULL, &fs, &tv);
	} while ((rv == -1) && (errno == EINTR));

	if (rv < 1) {
		if (rv == 0) {
			fatalp("read_route_socket: select timeout\n");
		} else
			fatalp("read_route_socket: select: %s",
			    strerror(errno));
		return 0;
	}

	do {
		rv = recv(route_socket, s, max, MSG_PEEK);
	} while((rv == -1) && (errno == EINTR));

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
compare_sockunion(union sockunion * __restrict a,
    union sockunion * __restrict b)
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
from_mask_to_cidr(char *mask)
{
	/* LoL (although I don't think about something faster right now) */
	char            mtest[20];
	uint8_t        i;

	for (i = 1; i < 32; i++) {
		from_cidr_to_mask(i, mtest);
		if (!strcmp(mask, mtest))
			break;
	}
	return i;
}

uint8_t
from_union_to_cidr(union sockunion *so_pref)
{
	struct sockaddr_in *sin = (struct sockaddr_in*)so_pref;
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
	uint32_t       a = 0, p = prefixlen;
	if (prefixlen > 32) {
		strlcpy(mask, "255.255.255.255", 16);
		return;
	}
	for (; p > 0; p--) {
		a = a >> (p - 1);
		a += 1;
		a = a << (p - 1);
	}
	/* is this OK ? */
#if _BYTE_ORDER == _LITTLE_ENDIAN
	a = a << (32 - prefixlen);
#endif

	snprintf(mask, 16, "%d.%d.%d.%d", a >> 24, (a << 8) >> 24,
	    (a << 16) >> 24, (a << 24) >> 24);
}

char *
mpls_ntoa(union mpls_shim ms)
{
	static char     ret[255];
	union mpls_shim ms2;

	ms2.s_addr = ntohl(ms.s_addr);
	snprintf(ret, sizeof(ret), "%d", ms2.shim.label);
	return ret;
}

char           *
union_ntoa(union sockunion * so)
{
	static char     defret[] = "Unknown family address";
	switch (so->sa.sa_family) {
	case AF_INET:
		return inet_ntoa(so->sin.sin_addr);
	case AF_LINK:
		return link_ntoa(&so->sdl);
	case AF_MPLS:
		return mpls_ntoa(so->smpls.smpls_addr);
	}
	fatalp("Unknown family address in union_ntoa: %d\n",
	       so->sa.sa_family);
	return defret;
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
    union sockunion *so_gate, union sockunion *so_ifa, union sockunion *so_tag,
    int fr, int optype)
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
		mask_addr(so_prefix);
		NEXTADDR(so_prefix);
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
		warnp("Destination was: %s\n", union_ntoa(so_dest));
		if (so_prefix)
			warnp("Prefix was: %s\n", union_ntoa(so_prefix));
		if (so_gate)
			warnp("Gateway was: %s\n", union_ntoa(so_gate));
		rv = LDP_E_ROUTE_ERROR;
	}
	if (fr) {
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
	else
		rm.m_rtm.rtm_addrs = RTA_DST;

	/* destination, gateway, netmask, genmask, ifp, ifa */

	NEXTADDR(so_dest);

	if (so_pref) {
		mask_addr(so_pref);
		NEXTADDR(so_pref);
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
		    route_strerror(errno), union_ntoa(so_dest),
		    spreftmp);
	    } else
		warnp("Error deleting route(%s) : %s",
		    route_strerror(errno), union_ntoa(so_dest));
	    return LDP_E_NO_SUCH_ROUTE;
	}
	return LDP_E_OK;
}

/*
 * Check for a route and returns it in rg
 * If exact_match is set it compares also the so_dest and so_pref
 * with the returned result
 */
int
get_route(struct rt_msg * rg, union sockunion * so_dest,
    union sockunion * so_pref, int exact_match)
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
		rm.m_rtm.rtm_addrs |= RTA_NETMASK;
		mask_addr(so_pref);
		NEXTADDR(so_pref);
	}
	rm.m_rtm.rtm_msglen = l = cp - (char *) &rm;

	if ((rlen = write(route_socket, (char *) &rm, l)) < l) {
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
			debugp("Added to replay PID: %d, SEQ: %d\n",
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
			debugp("Dest %s ", union_ntoa(so_dest));
			debugp("not like %s\n", union_ntoa(su));
			return LDP_E_NO_SUCH_ROUTE;
		}
	}

	return LDP_E_OK;
}


/* triggered when a route event occurs */
int 
check_route(struct rt_msg * rg, uint rlen)
{
	union sockunion *so_dest = NULL, *so_gate = NULL, *so_pref = NULL;
	int             so_pref_allocated = 0;
	int             prefixlen;
	struct peer_map *pm;
	struct label	*lab;
	char            dest[50], gate[50], pref[50], oper[50];
	dest[0] = 0;
	gate[0] = 0;
	pref[0] = 0;

	if (rlen <= sizeof(struct rt_msghdr))
		return LDP_E_ROUTE_ERROR;

	if (rg->m_rtm.rtm_version != RTM_VERSION)
		return LDP_E_ROUTE_ERROR;

	if ((rg->m_rtm.rtm_flags & RTF_DONE) == 0)
		return LDP_E_OK;

	if (rg->m_rtm.rtm_pid == getpid())	/* We did it.. */
		return LDP_E_OK;
	else
		debugp("Check route triggered by PID: %d\n", rg->m_rtm.rtm_pid);

	so_dest = (union sockunion *) rg->m_space;

	if (so_dest->sa.sa_family != AF_INET)
		return LDP_E_OK;/* We don't care about non-IP changes */

	if (rg->m_rtm.rtm_addrs & RTA_GATEWAY) {
		so_gate = GETNEXT(so_dest);
		if ((so_gate->sa.sa_family != AF_INET) &&
		    (so_gate->sa.sa_family != AF_MPLS))
			return LDP_E_OK;
	}
	if (rg->m_rtm.rtm_addrs & RTA_NETMASK) {
		if (so_gate)
			so_pref = so_gate;
		else
			so_pref = so_dest;
		so_pref = GETNEXT(so_pref);
	}
	if (!(rg->m_rtm.rtm_flags & RTF_GATEWAY)) {
		if (rg->m_rtm.rtm_addrs & RTA_GENMASK) {
			debugp("Used GENMASK\n");
		} else
			debugp("No GENMASK to use\n");
	}
	/* Calculate prefixlen */
	if (so_pref)
		prefixlen = from_mask_to_cidr(inet_ntoa(so_pref->sin.sin_addr));
	else {
		prefixlen = 32;
		if ((so_pref = from_cidr_to_union(32)) == NULL)
			return LDP_E_MEMORY;
		so_pref_allocated = 1;
	}

	so_pref->sa.sa_family = AF_INET;
	so_pref->sa.sa_len = sizeof(struct sockaddr_in);

	switch (rg->m_rtm.rtm_type) {
	case RTM_CHANGE:
		lab = label_get(so_dest, so_pref);
		if (lab) {
			send_withdraw_tlv_to_all(&so_dest->sa,
			    prefixlen);
			label_reattach_route(lab, LDP_READD_NODEL);
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
			if (!(rg->m_rtm.rtm_flags & RTF_GATEWAY))
				label_add(so_dest, so_pref, NULL,
					MPLS_LABEL_IMPLNULL, NULL, 0);
			else {
				pm = ldp_test_mapping(&so_dest->sa,
					 prefixlen, &so_gate->sa);
				if (pm) {
					label_add(so_dest, so_pref,
						so_gate, 0, NULL, 0);
					mpls_add_label(pm->peer, rg,
					  &so_dest->sa, prefixlen,
					  pm->lm->label, ROUTE_LOOKUP_LOOP);
					free(pm);
				} else
					label_add(so_dest, so_pref, so_gate,
					    MPLS_LABEL_IMPLNULL, NULL, 0);
			}
		} else	/* We already know about this prefix */
			debugp("Binding already there for prefix %s/%d !\n",
			      union_ntoa(so_dest), prefixlen);
		break;
	case RTM_DELETE:
		if (!so_gate)
			break;	/* Non-existent route  XXX ?! */
		/*
		 * Send withdraw check the binding, delete the route, delete
		 * the binding
		 */
		lab = label_get(so_dest, so_pref);
		if (!lab)
			break;
		send_withdraw_tlv_to_all(&so_dest->sa, prefixlen);
		/* No readd or delete IP route. Just delete the MPLS route */
		label_reattach_route(lab, LDP_READD_NODEL);
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
		strlcpy(dest, union_ntoa(so_dest), 16);
	if (so_pref)
		snprintf(pref, 3, "%d", prefixlen);
	if (so_gate)
		strlcpy(gate, union_ntoa(so_gate), 16);

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
	case RTM_NEWADDR:
		strlcpy(oper, "new address", 20);
		break;
	case RTM_DELADDR:
		strlcpy(oper, "del address", 20);
		break;
	default:
		snprintf(oper, 50, "unknown 0x%X operation",
		    rg->m_rtm.rtm_type);
	}

	warnp("[check_route] Route %s: %s / %s -> %s by PID:%d\n", oper, dest,
		pref, gate, rg->m_rtm.rtm_pid);

	if(so_pref_allocated)
		free(so_pref);
	return LDP_E_OK;
}

int 
bind_current_routes()
{
	size_t          needed;
	int             mib[6];
	char           *buf, *next, *lim;
	struct rt_msghdr *rtmes;
	union sockunion *so_dst, *so_pref, *so_gate;

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

	for (next = buf; next < lim; next += rtmes->rtm_msglen) {
		rtmes = (struct rt_msghdr *) next;
		so_pref = NULL;
		so_gate = NULL;
		if (rtmes->rtm_flags & RTF_LLINFO)	/* No need for arps */
			continue;
		if (!(rtmes->rtm_addrs & RTA_DST)) {
			debugp("No dst\n");
			continue;
		}

		so_dst = (union sockunion *) & rtmes[1];

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

		/* XXX: Check if it's loopback */
		if ((ntohl(so_dst->sin.sin_addr.s_addr) >> 24)==IN_LOOPBACKNET)
			continue;

		/* Get Gateway */
		if (rtmes->rtm_addrs & RTA_GATEWAY)
			so_gate = GETNEXT(so_dst);

		/* Get prefix */
		if (rtmes->rtm_flags & RTF_HOST) {
			if ((so_pref = from_cidr_to_union(32)) == NULL)
				return LDP_E_MEMORY;
		} else if (rtmes->rtm_addrs & RTA_GATEWAY)
			so_pref = GETNEXT(so_gate);
		else
			so_pref = GETNEXT(so_dst);

		so_pref->sa.sa_family = AF_INET;
		so_pref->sa.sa_len = sizeof(struct sockaddr_in);

		/* Also deletes when dest is IPv4 and gateway MPLS */
		if ((rtmes->rtm_addrs & RTA_GATEWAY) &&
		    (so_gate->sa.sa_family == AF_MPLS)) {
			debugp("MPLS route to %s deleted.\n",
			    inet_ntoa(so_dst->sin.sin_addr));
			delete_route(so_dst, so_pref, NO_FREESO);
			if (rtmes->rtm_flags & RTF_HOST)
				free(so_pref);
			continue;
		}
		if (so_gate->sa.sa_family == AF_INET)
			label_add(so_dst, so_pref, so_gate,
			    MPLS_LABEL_IMPLNULL, NULL, 0);

		if (rtmes->rtm_flags & RTF_HOST)
			free(so_pref);
	}
	free(buf);
	return LDP_E_OK;
}

int 
flush_mpls_routes()
{
	size_t          needed;
	int             mib[6];
	char           *buf, *next, *lim;
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

	for (next = buf; next < lim; next += rtm->rtm_msglen) {
		rtm = (struct rt_msghdr *) next;
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

		if (rtm->rtm_addrs & RTA_GATEWAY) {
			so_gate = GETNEXT(so_dst);
			so_pref = GETNEXT(so_gate);
		} else
			so_pref = GETNEXT(so_dst);

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
