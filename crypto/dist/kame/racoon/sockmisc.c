/*	$KAME: sockmisc.c,v 1.38 2003/05/09 05:31:11 itojun Exp $	*/

/*
 * Copyright (C) 1995, 1996, 1997, and 1998 WIDE Project.
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <netinet/in.h>
#ifdef IPV6_INRIA_VERSION
#include <netinet/ipsec.h>
#define IPV6_RECVDSTADDR IP_RECVDSTADDR
#else
#include <netinet6/ipsec.h>
#endif
#include <netkey/key_var.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "var.h"
#include "misc.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"
#include "gcmalloc.h"

const int niflags = 0;

/*
 * compare two sockaddr without port number.
 * OUT:	0: equal.
 *	1: not equal.
 */
int
cmpsaddrwop(addr1, addr2)
	struct sockaddr *addr1;
	struct sockaddr *addr2;
{
	caddr_t sa1, sa2;

	if (addr1 == 0 && addr2 == 0)
		return 0;
	if (addr1 == 0 || addr2 == 0)
		return 1;

	if (addr1->sa_len != addr2->sa_len
	 || addr1->sa_family != addr2->sa_family)
		return 1;

	switch (addr1->sa_family) {
	case AF_INET:
		sa1 = (caddr_t)&((struct sockaddr_in *)addr1)->sin_addr;
		sa2 = (caddr_t)&((struct sockaddr_in *)addr2)->sin_addr;
		if (memcmp(sa1, sa2, sizeof(struct in_addr)) != 0)
			return 1;
		break;
#ifdef INET6
	case AF_INET6:
		sa1 = (caddr_t)&((struct sockaddr_in6 *)addr1)->sin6_addr;
		sa2 = (caddr_t)&((struct sockaddr_in6 *)addr2)->sin6_addr;
		if (memcmp(sa1, sa2, sizeof(struct in6_addr)) != 0)
			return 1;
		if (((struct sockaddr_in6 *)addr1)->sin6_scope_id !=
		    ((struct sockaddr_in6 *)addr2)->sin6_scope_id)
			return 1;
		break;
#endif
	default:
		return 1;
	}

	return 0;
}

/*
 * compare two sockaddr with port, taking care wildcard.
 * addr1 is a subject address, addr2 is in a database entry.
 * OUT:	0: equal.
 *	1: not equal.
 */
int
cmpsaddrwild(addr1, addr2)
	struct sockaddr *addr1;
	struct sockaddr *addr2;
{
	caddr_t sa1, sa2;
	u_short port1, port2;

	if (addr1 == 0 && addr2 == 0)
		return 0;
	if (addr1 == 0 || addr2 == 0)
		return 1;

	if (addr1->sa_len != addr2->sa_len
	 || addr1->sa_family != addr2->sa_family)
		return 1;

	switch (addr1->sa_family) {
	case AF_INET:
		sa1 = (caddr_t)&((struct sockaddr_in *)addr1)->sin_addr;
		sa2 = (caddr_t)&((struct sockaddr_in *)addr2)->sin_addr;
		port1 = ((struct sockaddr_in *)addr1)->sin_port;
		port2 = ((struct sockaddr_in *)addr2)->sin_port;
		if (!(port1 == IPSEC_PORT_ANY ||
		      port2 == IPSEC_PORT_ANY ||
		      port1 == port2))
			return 1;
		if (memcmp(sa1, sa2, sizeof(struct in_addr)) != 0)
			return 1;
		break;
#ifdef INET6
	case AF_INET6:
		sa1 = (caddr_t)&((struct sockaddr_in6 *)addr1)->sin6_addr;
		sa2 = (caddr_t)&((struct sockaddr_in6 *)addr2)->sin6_addr;
		port1 = ((struct sockaddr_in6 *)addr1)->sin6_port;
		port2 = ((struct sockaddr_in6 *)addr2)->sin6_port;
		if (!(port1 == IPSEC_PORT_ANY ||
		      port2 == IPSEC_PORT_ANY ||
		      port1 == port2))
			return 1;
		if (memcmp(sa1, sa2, sizeof(struct in6_addr)) != 0)
			return 1;
		if (((struct sockaddr_in6 *)addr1)->sin6_scope_id !=
		    ((struct sockaddr_in6 *)addr2)->sin6_scope_id)
			return 1;
		break;
#endif
	default:
		return 1;
	}

	return 0;
}

/*
 * compare two sockaddr with strict match on port.
 * OUT:	0: equal.
 *	1: not equal.
 */
int
cmpsaddrstrict(addr1, addr2)
	struct sockaddr *addr1;
	struct sockaddr *addr2;
{
	caddr_t sa1, sa2;
	u_short port1, port2;

	if (addr1 == 0 && addr2 == 0)
		return 0;
	if (addr1 == 0 || addr2 == 0)
		return 1;

	if (addr1->sa_len != addr2->sa_len
	 || addr1->sa_family != addr2->sa_family)
		return 1;

	switch (addr1->sa_family) {
	case AF_INET:
		sa1 = (caddr_t)&((struct sockaddr_in *)addr1)->sin_addr;
		sa2 = (caddr_t)&((struct sockaddr_in *)addr2)->sin_addr;
		port1 = ((struct sockaddr_in *)addr1)->sin_port;
		port2 = ((struct sockaddr_in *)addr2)->sin_port;
		if (port1 != port2)
			return 1;
		if (memcmp(sa1, sa2, sizeof(struct in_addr)) != 0)
			return 1;
		break;
#ifdef INET6
	case AF_INET6:
		sa1 = (caddr_t)&((struct sockaddr_in6 *)addr1)->sin6_addr;
		sa2 = (caddr_t)&((struct sockaddr_in6 *)addr2)->sin6_addr;
		port1 = ((struct sockaddr_in6 *)addr1)->sin6_port;
		port2 = ((struct sockaddr_in6 *)addr2)->sin6_port;
		if (port1 != port2)
			return 1;
		if (memcmp(sa1, sa2, sizeof(struct in6_addr)) != 0)
			return 1;
		if (((struct sockaddr_in6 *)addr1)->sin6_scope_id !=
		    ((struct sockaddr_in6 *)addr2)->sin6_scope_id)
			return 1;
		break;
#endif
	default:
		return 1;
	}

	return 0;
}

/* get local address against the destination. */
struct sockaddr *
getlocaladdr(remote)
	struct sockaddr *remote;
{
	struct sockaddr *local;
	int local_len = sizeof(struct sockaddr_storage);
	int s;	/* for dummy connection */

	/* allocate buffer */
	if ((local = racoon_calloc(1, local_len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to get address buffer.\n");
		goto err;
	}
	
	/* get real interface received packet */
	if ((s = socket(remote->sa_family, SOCK_DGRAM, 0)) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket (%s)\n", strerror(errno));
		goto err;
	}
	
	if (connect(s, remote, remote->sa_len) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"connect (%s)\n", strerror(errno));
		close(s);
		goto err;
	}

	if (getsockname(s, local, &local_len) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"getsockname (%s)\n", strerror(errno));
		close(s);
		return NULL;
	}

	close(s);
	return local;

    err:
	if (local != NULL)
		racoon_free(local);
	return NULL;
}

/*
 * Receive packet, with src/dst information.  It is assumed that necessary
 * setsockopt() have already performed on socket.
 */
int
recvfromto(s, buf, buflen, flags, from, fromlen, to, tolen)
	int s;
	void *buf;
	size_t buflen;
	int flags;
	struct sockaddr *from;
	int *fromlen;
	struct sockaddr *to;
	int *tolen;
{
	int otolen;
	int len;
	struct sockaddr_storage ss;
	struct msghdr m;
	struct cmsghdr *cm;
	struct iovec iov[2];
	u_char cmsgbuf[256];
#if defined(INET6) && defined(ADVAPI)
	struct in6_pktinfo *pi;
#endif /*ADVAPI*/
	struct sockaddr_in *sin;
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif

	len = sizeof(ss);
	if (getsockname(s, (struct sockaddr *)&ss, &len) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"getsockname (%s)\n", strerror(errno));
		return -1;
	}

	m.msg_name = (caddr_t)from;
	m.msg_namelen = *fromlen;
	iov[0].iov_base = (caddr_t)buf;
	iov[0].iov_len = buflen;
	m.msg_iov = iov;
	m.msg_iovlen = 1;
	memset(cmsgbuf, 0, sizeof(cmsgbuf));
	cm = (struct cmsghdr *)cmsgbuf;
	m.msg_control = (caddr_t)cm;
	m.msg_controllen = sizeof(cmsgbuf);
	if ((len = recvmsg(s, &m, flags)) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"recvmsg (%s)\n", strerror(errno));
		return -1;
	}
	*fromlen = m.msg_namelen;

	otolen = *tolen;
	*tolen = 0;
	for (cm = (struct cmsghdr *)CMSG_FIRSTHDR(&m);
	     m.msg_controllen != 0 && cm;
	     cm = (struct cmsghdr *)CMSG_NXTHDR(&m, cm)) {
#if 0
		plog(LLV_ERROR, LOCATION, NULL,
			"cmsg %d %d\n", cm->cmsg_level, cm->cmsg_type);)
#endif
#if defined(INET6) && defined(ADVAPI)
		if (ss.ss_family == AF_INET6
		 && cm->cmsg_level == IPPROTO_IPV6
		 && cm->cmsg_type == IPV6_PKTINFO
		 && otolen >= sizeof(*sin6)) {
			pi = (struct in6_pktinfo *)(CMSG_DATA(cm));
			*tolen = sizeof(*sin6);
			sin6 = (struct sockaddr_in6 *)to;
			memset(sin6, 0, sizeof(*sin6));
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(*sin6);
			memcpy(&sin6->sin6_addr, &pi->ipi6_addr,
				sizeof(sin6->sin6_addr));
			/* XXX other cases, such as site-local? */
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr))
				sin6->sin6_scope_id = pi->ipi6_ifindex;
			else
				sin6->sin6_scope_id = 0;
			sin6->sin6_port =
				((struct sockaddr_in6 *)&ss)->sin6_port;
			otolen = -1;	/* "to" already set */
			continue;
		}
#endif
#if defined(INET6) && defined(IPV6_RECVDSTADDR)
		if (ss.ss_family == AF_INET6
		      && cm->cmsg_level == IPPROTO_IPV6
		      && cm->cmsg_type == IPV6_RECVDSTADDR
		      && otolen >= sizeof(*sin6)) {
			*tolen = sizeof(*sin6);
			sin6 = (struct sockaddr_in6 *)to;
			memset(sin6, 0, sizeof(*sin6));
			sin6->sin6_family = AF_INET6;
			sin6->sin6_len = sizeof(*sin6);
			memcpy(&sin6->sin6_addr, CMSG_DATA(cm),
				sizeof(sin6->sin6_addr));
			sin6->sin6_port =
				((struct sockaddr_in6 *)&ss)->sin6_port;
			otolen = -1;	/* "to" already set */
			continue;
		}
#endif
		if (ss.ss_family == AF_INET
		 && cm->cmsg_level == IPPROTO_IP
		 && cm->cmsg_type == IP_RECVDSTADDR
		 && otolen >= sizeof(*sin)) {
			*tolen = sizeof(*sin);
			sin = (struct sockaddr_in *)to;
			memset(sin, 0, sizeof(*sin));
			sin->sin_family = AF_INET;
			sin->sin_len = sizeof(*sin);
			memcpy(&sin->sin_addr, CMSG_DATA(cm),
				sizeof(sin->sin_addr));
			sin->sin_port = ((struct sockaddr_in *)&ss)->sin_port;
			otolen = -1;	/* "to" already set */
			continue;
		}
	}

	return len;
}

/* send packet, with fixing src/dst address pair. */
int
sendfromto(s, buf, buflen, src, dst, cnt)
	int s, cnt;
	const void *buf;
	size_t buflen;
	struct sockaddr *src;
	struct sockaddr *dst;
{
	struct sockaddr_storage ss;
	int len;
	int i;

	if (src->sa_family != dst->sa_family) {
		plog(LLV_ERROR, LOCATION, NULL,
			"address family mismatch\n");
		return -1;
	}

	len = sizeof(ss);
	if (getsockname(s, (struct sockaddr *)&ss, &len) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"getsockname (%s)\n", strerror(errno));
		return -1;
	}

	plog(LLV_DEBUG, LOCATION, NULL,
		"sockname %s\n", saddr2str((struct sockaddr *)&ss));
	plog(LLV_DEBUG, LOCATION, NULL,
		"send packet from %s\n", saddr2str(src));
	plog(LLV_DEBUG, LOCATION, NULL,
		"send packet to %s\n", saddr2str(dst));

	if (src->sa_family != ss.ss_family) {
		plog(LLV_ERROR, LOCATION, NULL,
			"address family mismatch\n");
		return -1;
	}

	switch (src->sa_family) {
#if defined(INET6) && defined(ADVAPI) && !defined(IPV6_INRIA_VERSION)
	case AF_INET6:
	    {
		struct msghdr m;
		struct cmsghdr *cm;
		struct iovec iov[2];
		u_char cmsgbuf[256];
		struct in6_pktinfo *pi;
		int ifindex;
		struct sockaddr_in6 src6, dst6;

		memcpy(&src6, src, sizeof(src6));
		memcpy(&dst6, dst, sizeof(dst6));

		/* XXX take care of other cases, such as site-local */
		ifindex = 0;
		if (IN6_IS_ADDR_LINKLOCAL(&src6.sin6_addr)
		 || IN6_IS_ADDR_MULTICAST(&src6.sin6_addr)) {
			ifindex = src6.sin6_scope_id;	/*???*/
		}

		/* XXX some sanity check on dst6.sin6_scope_id */

		/* flowinfo for IKE?  mmm, maybe useful but for now make it 0 */
		src6.sin6_flowinfo = dst6.sin6_flowinfo = 0;

		memset(&m, 0, sizeof(m));
		m.msg_name = (caddr_t)&dst6;
		m.msg_namelen = sizeof(dst6);
		iov[0].iov_base = (char *)buf;
		iov[0].iov_len = buflen;
		m.msg_iov = iov;
		m.msg_iovlen = 1;

		memset(cmsgbuf, 0, sizeof(cmsgbuf));
		cm = (struct cmsghdr *)cmsgbuf;
		m.msg_control = (caddr_t)cm;
		m.msg_controllen = CMSG_SPACE(sizeof(struct in6_pktinfo));

		cm->cmsg_len = CMSG_LEN(sizeof(struct in6_pktinfo));
		cm->cmsg_level = IPPROTO_IPV6;
		cm->cmsg_type = IPV6_PKTINFO;
		pi = (struct in6_pktinfo *)CMSG_DATA(cm);
		memcpy(&pi->ipi6_addr, &src6.sin6_addr, sizeof(src6.sin6_addr));
		pi->ipi6_ifindex = ifindex;

		plog(LLV_DEBUG, LOCATION, NULL,
			"src6 %s %d\n",
			saddr2str((struct sockaddr *)&src6),
			src6.sin6_scope_id);
		plog(LLV_DEBUG, LOCATION, NULL,
			"dst6 %s %d\n",
			saddr2str((struct sockaddr *)&dst6),
			dst6.sin6_scope_id);

		for (i = 0; i < cnt; i++) {
			len = sendmsg(s, &m, 0 /*MSG_DONTROUTE*/);
			if (len < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"sendmsg (%s)\n", strerror(errno));
				return -1;
			}
			plog(LLV_DEBUG, LOCATION, NULL,
				"%d times of %d bytes message will be sent "
				"to %s\n",
				i + 1, len, saddr2str(dst));
		}
		plogdump(LLV_DEBUG, (char *)buf, buflen);

		return len;
	    }
#endif
	default:
	    {
		int needclose = 0;
		int sendsock;

		if (ss.ss_family == src->sa_family && memcmp(&ss, src, src->sa_len) == 0) {
			sendsock = s;
			needclose = 0;
		} else {
			int yes = 1;
			/*
			 * Use newly opened socket for sending packets.
			 * NOTE: this is unsafe, because if the peer is quick enough
			 * the packet from the peer may be queued into sendsock.
			 * Better approach is to prepare bind'ed udp sockets for
			 * each of the interface addresses.
			 */
			sendsock = socket(src->sa_family, SOCK_DGRAM, 0);
			if (sendsock < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"socket (%s)\n", strerror(errno));
				return -1;
			}
			if (setsockopt(sendsock, SOL_SOCKET, SO_REUSEPORT,
				       (void *)&yes, sizeof(yes)) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"setsockopt (%s)\n", strerror(errno));
				close(sendsock);
				return -1;
			}
#ifdef IPV6_USE_MIN_MTU
			if (src->sa_family == AF_INET6 &&
			    setsockopt(sendsock, IPPROTO_IPV6, IPV6_USE_MIN_MTU,
			    (void *)&yes, sizeof(yes)) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"setsockopt (%s)\n", strerror(errno));
				close(sendsock);
				return -1;
			}
#endif
			if (setsockopt_bypass(sendsock, src->sa_family) < 0) {
				close(sendsock);
				return -1;
			}

			if (bind(sendsock, (struct sockaddr *)src, src->sa_len) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"bind 1 (%s)\n", strerror(errno));
				close(sendsock);
				return -1;
			}
			needclose = 1;
		}

		for (i = 0; i < cnt; i++) {
			len = sendto(sendsock, buf, buflen, 0, dst, dst->sa_len);
			if (len < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"sendto (%s)\n", strerror(errno));
				if (needclose)
					close(sendsock);
				return len;
			}
			plog(LLV_DEBUG, LOCATION, NULL,
				"%d times of %d bytes message will be sent "
				"to %s\n",
				i + 1, len, saddr2str(dst));
		}
		plogdump(LLV_DEBUG, (char *)buf, buflen);

		if (needclose)
			close(sendsock);

		return len;
	    }
	}
}

int
setsockopt_bypass(so, family)
	int so, family;
{
	int level;
	char *buf;
	char *policy;

	switch (family) {
	case AF_INET:
		level = IPPROTO_IP;
		break;
#ifdef INET6
	case AF_INET6:
		level = IPPROTO_IPV6;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"unsupported address family %d\n", family);
		return -1;
	}

	policy = "in bypass";
	buf = ipsec_set_policy(policy, strlen(policy));
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ipsec_set_policy (%s)\n",
			ipsec_strerror());
		return -1;
	}
	if (setsockopt(so, level,
	               (level == IPPROTO_IP ?
	                         IP_IPSEC_POLICY : IPV6_IPSEC_POLICY),
	               buf, ipsec_get_policylen(buf)) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"setsockopt (%s)\n",
			strerror(errno));
		return -1;
	}
	racoon_free(buf);

	policy = "out bypass";
	buf = ipsec_set_policy(policy, strlen(policy));
	if (buf == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ipsec_set_policy (%s)\n",
			ipsec_strerror());
		return -1;
	}
	if (setsockopt(so, level,
	               (level == IPPROTO_IP ?
	                         IP_IPSEC_POLICY : IPV6_IPSEC_POLICY),
	               buf, ipsec_get_policylen(buf)) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"setsockopt (%s)\n",
			strerror(errno));
		return -1;
	}
	racoon_free(buf);

	return 0;
}

struct sockaddr *
newsaddr(len)
	int len;
{
	struct sockaddr *new;

	new = racoon_calloc(1, len);
	if (new == NULL)
		plog(LLV_ERROR, LOCATION, NULL,
			"%s\n", strerror(errno)); 

	/* initial */
	new->sa_len = len;

	return new;
}

struct sockaddr *
dupsaddr(src)
	struct sockaddr *src;
{
	struct sockaddr *dst;

	dst = racoon_calloc(1, src->sa_len);
	if (dst == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"%s\n", strerror(errno)); 
		return NULL;
	}

	memcpy(dst, src, src->sa_len);

	return dst;
}

char *
saddr2str(saddr)
	const struct sockaddr *saddr;
{
	static char buf[NI_MAXHOST + NI_MAXSERV + 10];
	char addr[NI_MAXHOST], port[NI_MAXSERV];

	if (saddr == NULL)
		return NULL;

	GETNAMEINFO(saddr, addr, port);
	snprintf(buf, sizeof(buf), "%s[%s]", addr, port);

	return buf;
}

char *
saddrwop2str(saddr)
	struct sockaddr *saddr;
{
	static char buf[NI_MAXHOST + NI_MAXSERV + 10];
	char addr[NI_MAXHOST];

	if (saddr == NULL)
		return NULL;

	GETNAMEINFO(saddr, addr, NULL);
	snprintf(buf, sizeof(buf), "%s", addr);

	return buf;
}

struct sockaddr *
str2saddr(host, port)
	char *host;
	char *port;
{
	struct addrinfo hints, *res;
	struct sockaddr *saddr;
	int error;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_NUMERICHOST;
	error = getaddrinfo(host, port, &hints, &res);
	if (error != 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"getaddrinfo(%s%s%s): %s",
			host, port ? "," : "", port ? port : "",
			gai_strerror(error));
		return NULL;
	}
	if (res->ai_next != NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"getaddrinfo(%s%s%s): "
			"resolved to multiple address, "
			"taking the first one",
			host, port ? "," : "", port ? port : "");
	}
	saddr = racoon_malloc(res->ai_addrlen);
	if (saddr == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer.\n");
		freeaddrinfo(res);
		return NULL;
	}
	memcpy(saddr, res->ai_addr, res->ai_addrlen);
	freeaddrinfo(res);

	return saddr;
}

void
mask_sockaddr(a, b, l)
	struct sockaddr *a;
	const struct sockaddr *b;
	size_t l;
{
	size_t i;
	u_int8_t *p, alen;

	switch (b->sa_family) {
	case AF_INET:
		alen = sizeof(struct in_addr);
		p = (u_int8_t *)&((struct sockaddr_in *)a)->sin_addr;
		break;
#ifdef INET6
	case AF_INET6:
		alen = sizeof(struct in6_addr);
		p = (u_int8_t *)&((struct sockaddr_in6 *)a)->sin6_addr;
		break;
#endif
	default:
		plog(LLV_ERROR, LOCATION, NULL,
			"invalid family: %d\n", b->sa_family);
		exit(1);
	}

	if ((alen << 3) < l) {
		plog(LLV_ERROR, LOCATION, NULL,
			"unexpected inconsistency: %d %d\n", b->sa_family, l);
		exit(1);
	}

	memcpy(a, b, b->sa_len);
	p[l / 8] &= (0xff00 >> (l % 8)) & 0xff;
	for (i = l / 8 + 1; i < alen; i++)
		p[i] = 0x00;
}
