/*	$NetBSD: sockmisc.c,v 1.8.6.1 2007/08/01 11:52:22 vanhu Exp $	*/

/* Id: sockmisc.c,v 1.24 2006/05/07 21:32:59 manubsd Exp */

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

#include "config.h"

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/uio.h>

#include <netinet/in.h>
#include PATH_IPSEC_H

#if defined(INET6) && !defined(INET6_ADVAPI) && \
	defined(IP_RECVDSTADDR) && !defined(IPV6_RECVDSTADDR)
#define IPV6_RECVDSTADDR IP_RECVDSTADDR
#endif

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
#include "debugrm.h"
#include "libpfkey.h"

#ifndef IP_IPSEC_POLICY
#define IP_IPSEC_POLICY 16	/* XXX: from linux/in.h */
#endif

#ifndef IPV6_IPSEC_POLICY
#define IPV6_IPSEC_POLICY 34	/* XXX: from linux/???.h per
				   "Tom Lendacky" <toml@us.ibm.com> */
#endif

const int niflags = 0;

/*
 * compare two sockaddr without port number.
 * OUT:	0: equal.
 *	1: not equal.
 */
int
cmpsaddrwop(addr1, addr2)
	const struct sockaddr *addr1;
	const struct sockaddr *addr2;
{
	caddr_t sa1, sa2;

	if (addr1 == 0 && addr2 == 0)
		return 0;
	if (addr1 == 0 || addr2 == 0)
		return 1;

#ifdef __linux__
	if (addr1->sa_family != addr2->sa_family)
		return 1;
#else
	if (addr1->sa_len != addr2->sa_len
	 || addr1->sa_family != addr2->sa_family)
		return 1;

#endif /* __linux__ */

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
	const struct sockaddr *addr1;
	const struct sockaddr *addr2;
{
	caddr_t sa1, sa2;
	u_short port1, port2;

	if (addr1 == 0 && addr2 == 0)
		return 0;
	if (addr1 == 0 || addr2 == 0)
		return 1;

#ifdef __linux__
	if (addr1->sa_family != addr2->sa_family)
		return 1;
#else
	if (addr1->sa_len != addr2->sa_len
	 || addr1->sa_family != addr2->sa_family)
		return 1;

#endif /* __linux__ */

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
	const struct sockaddr *addr1;
	const struct sockaddr *addr2;
{
	caddr_t sa1, sa2;
	u_short port1, port2;

	if (addr1 == 0 && addr2 == 0)
		return 0;
	if (addr1 == 0 || addr2 == 0)
		return 1;

#ifdef __linux__
	if (addr1->sa_family != addr2->sa_family)
		return 1;
#else
	if (addr1->sa_len != addr2->sa_len
	 || addr1->sa_family != addr2->sa_family)
		return 1;

#endif /* __linux__ */

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
	u_int local_len = sizeof(struct sockaddr_storage);
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

	setsockopt_bypass(s, remote->sa_family);
	
	if (connect(s, remote, sysdep_sa_len(remote)) < 0) {
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
	socklen_t *fromlen;
	struct sockaddr *to;
	u_int *tolen;
{
	int otolen;
	u_int len;
	struct sockaddr_storage ss;
	struct msghdr m;
	struct cmsghdr *cm;
	struct iovec iov[2];
	u_char cmsgbuf[256];
#if defined(INET6) && defined(INET6_ADVAPI)
	struct in6_pktinfo *pi;
#endif /*INET6_ADVAPI*/
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
#if defined(INET6) && defined(INET6_ADVAPI)
		if (ss.ss_family == AF_INET6
		 && cm->cmsg_level == IPPROTO_IPV6
		 && cm->cmsg_type == IPV6_PKTINFO
		 && otolen >= sizeof(*sin6)) {
			pi = (struct in6_pktinfo *)(CMSG_DATA(cm));
			*tolen = sizeof(*sin6);
			sin6 = (struct sockaddr_in6 *)to;
			memset(sin6, 0, sizeof(*sin6));
			sin6->sin6_family = AF_INET6;
#ifndef __linux__
			sin6->sin6_len = sizeof(*sin6);
#endif
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
#ifdef __linux__
		if (ss.ss_family == AF_INET
		 && cm->cmsg_level == IPPROTO_IP
		 && cm->cmsg_type == IP_PKTINFO
		 && otolen >= sizeof(sin)) {
			struct in_pktinfo *pi = (struct in_pktinfo *)(CMSG_DATA(cm));
			*tolen = sizeof(*sin);
			sin = (struct sockaddr_in *)to;
			memset(sin, 0, sizeof(*sin));
			sin->sin_family = AF_INET;
			memcpy(&sin->sin_addr, &pi->ipi_addr,
				sizeof(sin->sin_addr));
			sin->sin_port =
				((struct sockaddr_in *)&ss)->sin_port;
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
#ifndef __linux__
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
#endif
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
	u_int len;
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
#if defined(INET6) && defined(INET6_ADVAPI)
// XXX: This block wasn't compiled on Linux - does it work?
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
#ifdef __linux__
	case AF_INET:
	    {
		struct msghdr m;
		struct cmsghdr *cm;
		struct iovec iov[2];
		u_char cmsgbuf[256];
		struct in_pktinfo *pi;
		int ifindex = 0;
		struct sockaddr_in src6, dst6;

		memcpy(&src6, src, sizeof(src6));
		memcpy(&dst6, dst, sizeof(dst6));

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
		m.msg_controllen = CMSG_SPACE(sizeof(struct in_pktinfo));

		cm->cmsg_len = CMSG_LEN(sizeof(struct in_pktinfo));
		cm->cmsg_level = IPPROTO_IP;
		cm->cmsg_type = IP_PKTINFO;
		pi = (struct in_pktinfo *)CMSG_DATA(cm);
		memcpy(&pi->ipi_spec_dst, &src6.sin_addr, sizeof(src6.sin_addr));
		pi->ipi_ifindex = ifindex;

		plog(LLV_DEBUG, LOCATION, NULL,
			"src4 %s\n",
			saddr2str((struct sockaddr *)&src6));
		plog(LLV_DEBUG, LOCATION, NULL,
			"dst4 %s\n",
			saddr2str((struct sockaddr *)&dst6));

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
#endif /* __linux__ */
	default:
	    {
		int needclose = 0;
		int sendsock;

		if (ss.ss_family == src->sa_family && memcmp(&ss, src, sysdep_sa_len(src)) == 0) {
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
			if (setsockopt(sendsock, SOL_SOCKET,
#ifdef __linux__
				       SO_REUSEADDR,
#else
				       SO_REUSEPORT,
#endif
				       (void *)&yes, sizeof(yes)) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"setsockopt SO_REUSEPORT (%s)\n", 
					strerror(errno));
				close(sendsock);
				return -1;
			}
#ifdef IPV6_USE_MIN_MTU
			if (src->sa_family == AF_INET6 &&
			    setsockopt(sendsock, IPPROTO_IPV6, IPV6_USE_MIN_MTU,
			    (void *)&yes, sizeof(yes)) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"setsockopt IPV6_USE_MIN_MTU (%s)\n", 
					strerror(errno));
				close(sendsock);
				return -1;
			}
#endif
			if (setsockopt_bypass(sendsock, src->sa_family) < 0) {
				close(sendsock);
				return -1;
			}

			if (bind(sendsock, (struct sockaddr *)src, sysdep_sa_len(src)) < 0) {
				plog(LLV_ERROR, LOCATION, NULL,
					"bind 1 (%s)\n", strerror(errno));
				close(sendsock);
				return -1;
			}
			needclose = 1;
		}

		for (i = 0; i < cnt; i++) {
			len = sendto(sendsock, buf, buflen, 0, dst, sysdep_sa_len(dst));
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
			"setsockopt IP_IPSEC_POLICY (%s)\n",
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
			"setsockopt IP_IPSEC_POLICY (%s)\n",
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

	if ((new = racoon_calloc(1, len)) == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"%s\n", strerror(errno)); 
		goto out;
	}

#ifdef __linux__
	if (len == sizeof (struct sockaddr_in6))
		new->sa_family = AF_INET6;
	else
		new->sa_family = AF_INET;
#else
	/* initial */
	new->sa_len = len;
#endif
out:
	return new;
}

struct sockaddr *
dupsaddr(src)
	struct sockaddr *src;
{
	struct sockaddr *dst;

	dst = racoon_calloc(1, sysdep_sa_len(src));
	if (dst == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"%s\n", strerror(errno)); 
		return NULL;
	}

	memcpy(dst, src, sysdep_sa_len(src));

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

	if (saddr->sa_family == AF_UNSPEC)
		snprintf (buf, sizeof(buf), "%s", "anonymous");
	else {
		GETNAMEINFO(saddr, addr, port);
		snprintf(buf, sizeof(buf), "%s[%s]", addr, port);
	}

	return buf;
}

char *
saddrwop2str(saddr)
	const struct sockaddr *saddr;
{
	static char buf[NI_MAXHOST + NI_MAXSERV + 10];
	char addr[NI_MAXHOST];

	if (saddr == NULL)
		return NULL;

	GETNAMEINFO_NULL(saddr, addr);
	snprintf(buf, sizeof(buf), "%s", addr);

	return buf;
}

char *
naddrwop2str(const struct netaddr *naddr)
{
	static char buf[NI_MAXHOST + 10];
	static const struct sockaddr sa_any;	/* this is initialized to all zeros */
	
	if (naddr == NULL)
		return NULL;

	if (memcmp(&naddr->sa, &sa_any, sizeof(sa_any)) == 0)
		snprintf(buf, sizeof(buf), "%s", "any");
	else {
		snprintf(buf, sizeof(buf), "%s", saddrwop2str(&naddr->sa.sa));
		snprintf(&buf[strlen(buf)], sizeof(buf) - strlen(buf), "/%ld", naddr->prefix);
	}
	return buf;
}

char *
naddrwop2str_fromto(const char *format, const struct netaddr *saddr,
		    const struct netaddr *daddr)
{
	static char buf[2*(NI_MAXHOST + NI_MAXSERV + 10) + 100];
	char *src, *dst;

	src = racoon_strdup(naddrwop2str(saddr));
	dst = racoon_strdup(naddrwop2str(daddr));
	STRDUP_FATAL(src);
	STRDUP_FATAL(dst);
	/* WARNING: Be careful about the format string! Don't 
	   ever pass in something that a user can modify!!! */
	snprintf (buf, sizeof(buf), format, src, dst);
	racoon_free (src);
	racoon_free (dst);

	return buf;
}

char *
saddr2str_fromto(format, saddr, daddr)
	const char *format;
	const struct sockaddr *saddr;
	const struct sockaddr *daddr;
{
	static char buf[2*(NI_MAXHOST + NI_MAXSERV + 10) + 100];
	char *src, *dst;

	src = racoon_strdup(saddr2str(saddr));
	dst = racoon_strdup(saddr2str(daddr));
	STRDUP_FATAL(src);
	STRDUP_FATAL(dst);
	/* WARNING: Be careful about the format string! Don't 
	   ever pass in something that a user can modify!!! */
	snprintf (buf, sizeof(buf), format, src, dst);
	racoon_free (src);
	racoon_free (dst);

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
			"getaddrinfo(%s%s%s): %s\n",
			host, port ? "," : "", port ? port : "",
			gai_strerror(error));
		return NULL;
	}
	if (res->ai_next != NULL) {
		plog(LLV_WARNING, LOCATION, NULL,
			"getaddrinfo(%s%s%s): "
			"resolved to multiple address, "
			"taking the first one\n",
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
			"unexpected inconsistency: %d %zu\n", b->sa_family, l);
		exit(1);
	}

	memcpy(a, b, sysdep_sa_len(b));
	p[l / 8] &= (0xff00 >> (l % 8)) & 0xff;
	for (i = l / 8 + 1; i < alen; i++)
		p[i] = 0x00;
}

/* Compute a score describing how "accurate" a netaddr is for a given sockaddr.
 * Examples:
 * 	Return values for address 10.20.30.40 [port 500] and given netaddresses...
 * 		10.10.0.0/16	=> -1	... doesn't match
 * 		0.0.0.0/0	=> 0	... matches, but only 0 bits.
 * 		10.20.0.0/16	=> 16	... 16 bits match
 * 		10.20.30.0/24	=> 24	... guess what ;-)
 * 		10.20.30.40/32	=> 32	... whole address match
 * 		10.20.30.40:500	=> 33	... both address and port match
 * 		10.20.30.40:501	=> -1	... port doesn't match and isn't 0 (=any)
 */
int
naddr_score(const struct netaddr *naddr, const struct sockaddr *saddr)
{
	static const struct netaddr naddr_any;	/* initialized to all-zeros */
	struct sockaddr sa;
	u_int16_t naddr_port, saddr_port;
	int port_score;

	if (!naddr || !saddr) {
		plog(LLV_ERROR, LOCATION, NULL,
		     "Call with null args: naddr=%p, saddr=%p\n",
		     naddr, saddr);
		return -1;
	}

	/* Wildcard address matches, but only 0 bits. */
	if (memcmp(naddr, &naddr_any, sizeof(naddr_any)) == 0)
		return 0;

	/* If families don't match we really can't do much... */
	if (naddr->sa.sa.sa_family != saddr->sa_family)
		return -1;
	
	/* If port check fail don't bother to check addresses. */
	naddr_port = extract_port(&naddr->sa.sa);
	saddr_port = extract_port(saddr);
	if (naddr_port == 0 || saddr_port == 0)	/* wildcard match */
		port_score = 0;
	else if (naddr_port == saddr_port)	/* exact match */
		port_score = 1;
	else					/* mismatch :-) */
		return -1;

	/* Here it comes - compare network addresses. */
	mask_sockaddr(&sa, saddr, naddr->prefix);
	if (loglevel >= LLV_DEBUG) {	/* debug only */
		char *a1, *a2, *a3;
		a1 = racoon_strdup(naddrwop2str(naddr));
		a2 = racoon_strdup(saddrwop2str(saddr));
		a3 = racoon_strdup(saddrwop2str(&sa));
		STRDUP_FATAL(a1);
		STRDUP_FATAL(a2);
		STRDUP_FATAL(a3);
		plog(LLV_DEBUG, LOCATION, NULL,
		     "naddr=%s, saddr=%s (masked=%s)\n",
		     a1, a2, a3);
		free(a1);
		free(a2);
		free(a3);
	}
	if (cmpsaddrwop(&sa, &naddr->sa.sa) == 0)
		return naddr->prefix + port_score;

	return -1;
}

/* Some usefull functions for sockaddr port manipulations. */
u_int16_t
extract_port (const struct sockaddr *addr)
{
  u_int16_t port = 0;
  
  if (!addr)
    return port;

  switch (addr->sa_family) {
    case AF_INET:
      port = ((struct sockaddr_in *)addr)->sin_port;
      break;
    case AF_INET6:
      port = ((struct sockaddr_in6 *)addr)->sin6_port;
      break;
    default:
      plog(LLV_ERROR, LOCATION, NULL, "unknown AF: %u\n", addr->sa_family);
      break;
  }

  return ntohs(port);
}

u_int16_t *
get_port_ptr (struct sockaddr *addr)
{
  u_int16_t *port_ptr;

  if (!addr)
    return NULL;

  switch (addr->sa_family) {
    case AF_INET:
      port_ptr = &(((struct sockaddr_in *)addr)->sin_port);
      break;
    case AF_INET6:
      port_ptr = &(((struct sockaddr_in6 *)addr)->sin6_port);
      break;
    default:
      plog(LLV_ERROR, LOCATION, NULL, "unknown AF: %u\n", addr->sa_family);
      return NULL;
      break;
  }

  return port_ptr;
}

u_int16_t *
set_port (struct sockaddr *addr, u_int16_t new_port)
{
  u_int16_t *port_ptr;

  port_ptr = get_port_ptr (addr);

  if (port_ptr)
    *port_ptr = htons(new_port);

  return port_ptr;
}
