/*	$KAME: grabmyaddr.c,v 1.29 2002/03/11 07:20:53 sakane Exp $	*/

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
#include <sys/ioctl.h>

#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif
#include <net/route.h>
#include <netkey/key_var.h>
#include <netinet/in.h>
#include <netinet6/in6_var.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <netdb.h>
#ifdef HAVE_GETIFADDRS
#include <ifaddrs.h>
#endif 

#include "var.h"
#include "misc.h"
#include "vmbuf.h"
#include "plog.h"
#include "sockmisc.h"
#include "debug.h"

#include "localconf.h"
#include "grabmyaddr.h"
#include "sockmisc.h"
#include "isakmp_var.h"
#include "gcmalloc.h"

#ifndef HAVE_GETIFADDRS
static unsigned int if_maxindex __P((void));
#endif
static struct myaddrs *find_myaddr __P((struct myaddrs *, struct myaddrs *));
static int suitable_ifaddr __P((const char *, const struct sockaddr *));
#ifdef INET6
static int suitable_ifaddr6 __P((const char *, const struct sockaddr *));
#endif

#ifndef HAVE_GETIFADDRS
static unsigned int
if_maxindex()
{
	struct if_nameindex *p, *p0;
	unsigned int max = 0;

	p0 = if_nameindex();
	for (p = p0; p && p->if_index && p->if_name; p++) {
		if (max < p->if_index)
			max = p->if_index;
	}
	if_freenameindex(p0);
	return max;
}
#endif

void
clear_myaddr(db)
	struct myaddrs **db;
{
	struct myaddrs *p;

	while (*db) {
		p = (*db)->next;
		delmyaddr(*db);
		*db = p;
	}
}
  
static struct myaddrs *
find_myaddr(db, p)
	struct myaddrs *db;
	struct myaddrs *p;
{
	struct myaddrs *q;
	char h1[NI_MAXHOST], h2[NI_MAXHOST];

	if (getnameinfo(p->addr, p->addr->sa_len, h1, sizeof(h1), NULL, 0,
	    NI_NUMERICHOST | niflags) != 0)
		return NULL;

	for (q = db; q; q = q->next) {
		if (p->addr->sa_len != q->addr->sa_len)
			continue;
		if (getnameinfo(q->addr, q->addr->sa_len, h2, sizeof(h2),
		    NULL, 0, NI_NUMERICHOST | niflags) != 0)
			return NULL;
		if (strcmp(h1, h2) == 0)
			return q;
	}

	return NULL;
}

void
grab_myaddrs()
{
#ifdef HAVE_GETIFADDRS
	struct myaddrs *p, *q, *old;
	struct ifaddrs *ifa0, *ifap;
#ifdef INET6
#ifdef __KAME__
	struct sockaddr_in6 *sin6;
#endif
#endif

#if defined(YIPS_DEBUG)
	char _addr1_[NI_MAXHOST];
#endif

	if (getifaddrs(&ifa0)) {
		plog(LLV_ERROR, LOCATION, NULL,
			"getifaddrs failed: %s\n", strerror(errno));
		exit(1);
		/*NOTREACHED*/
	}

	old = lcconf->myaddrs;

	for (ifap = ifa0; ifap; ifap = ifap->ifa_next) {

		if (ifap->ifa_addr->sa_family != AF_INET
#ifdef INET6
		 && ifap->ifa_addr->sa_family != AF_INET6
#endif
		)
			continue;

		if (!suitable_ifaddr(ifap->ifa_name, ifap->ifa_addr)) {
			plog(LLV_ERROR, LOCATION, NULL,
				"unsuitable ifaddr: %s\n",
				saddr2str(ifap->ifa_addr));
			continue;
		}

		p = newmyaddr();
		if (p == NULL) {
			exit(1);
			/*NOTREACHED*/
		}
		p->addr = dupsaddr(ifap->ifa_addr);
		if (p->addr == NULL) {
			exit(1);
			/*NOTREACHED*/
		}
#ifdef INET6
#ifdef __KAME__
		if (ifap->ifa_addr->sa_family == AF_INET6) {
			sin6 = (struct sockaddr_in6 *)p->addr;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)
			 || IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr)) {
				sin6->sin6_scope_id =
					ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
				sin6->sin6_addr.s6_addr[2] = 0;
				sin6->sin6_addr.s6_addr[3] = 0;
			}
		}
#endif
#endif
		if (getnameinfo(p->addr, p->addr->sa_len,
				_addr1_, sizeof(_addr1_),
				NULL, 0,
				NI_NUMERICHOST | niflags))
		strcpy(_addr1_, "(invalid)");
		plog(LLV_DEBUG, LOCATION, NULL,
			"my interface: %s (%s)\n",
			_addr1_, ifap->ifa_name);
		q = find_myaddr(old, p);
		if (q)
			p->sock = q->sock;
		else
			p->sock = -1;
		p->next = lcconf->myaddrs;
		lcconf->myaddrs = p;
	}

	freeifaddrs(ifa0);

	clear_myaddr(&old);

#else /*!HAVE_GETIFADDRS*/
	int s;
	unsigned int maxif;
	int len;
	struct ifreq *iflist;
	struct ifconf ifconf;
	struct ifreq *ifr, *ifr_end;
	struct myaddrs *p, *q, *old;
#ifdef INET6
#ifdef __KAME__
	struct sockaddr_in6 *sin6;
#endif
#endif

#if defined(YIPS_DEBUG)
	char _addr1_[NI_MAXHOST];
#endif

	maxif = if_maxindex() + 1;
	len = maxif * sizeof(struct sockaddr_storage) * 4; /* guess guess */

	iflist = (struct ifreq *)racoon_malloc(len);
	if (!iflist) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer\n");
		exit(1);
		/*NOTREACHED*/
	}

	if ((s = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket(SOCK_DGRAM) failed: %s\n",
			strerror(errno));
		exit(1);
		/*NOTREACHED*/
	}
	memset(&ifconf, 0, sizeof(ifconf));
	ifconf.ifc_req = iflist;
	ifconf.ifc_len = len;
	if (ioctl(s, SIOCGIFCONF, &ifconf) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ioctl(SIOCGIFCONF) failed: %s\n",
			strerror(errno));
		exit(1);
		/*NOTREACHED*/
	}
	close(s);

	old = lcconf->myaddrs;

	/* Look for this interface in the list */
	ifr_end = (struct ifreq *) (ifconf.ifc_buf + ifconf.ifc_len);

#define _IFREQ_LEN(p) \
  (sizeof((p)->ifr_name) + (p)->ifr_addr.sa_len > sizeof(struct ifreq) \
    ? sizeof((p)->ifr_name) + (p)->ifr_addr.sa_len : sizeof(struct ifreq))

	for (ifr = ifconf.ifc_req;
	     ifr < ifr_end;
	     ifr = (struct ifreq *)((caddr_t)ifr + _IFREQ_LEN(ifr))) {

		switch (ifr->ifr_addr.sa_family) {
		case AF_INET:
#ifdef INET6
		case AF_INET6:
#endif
			if (!suitable_ifaddr(ifr->ifr_name, &ifr->ifr_addr)) {
				plog(LLV_DEBUG, LOCATION, NULL,
					"unsuitable ifaddr %s\n");
				continue;
			}

			p = newmyaddr();
			if (p == NULL) {
				exit(1);
				/*NOTREACHED*/
			}
			p->addr = dupsaddr(&ifr->ifr_addr);
			if (p->addr == NULL) {
				exit(1);
				/*NOTREACHED*/
			}
#ifdef INET6
#ifdef __KAME__
			sin6 = (struct sockaddr_in6 *)p->addr;
			if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)
			 || IN6_IS_ADDR_SITELOCAL(&sin6->sin6_addr)) {
				sin6->sin6_scope_id =
					ntohs(*(u_int16_t *)&sin6->sin6_addr.s6_addr[2]);
				sin6->sin6_addr.s6_addr[2] = 0;
				sin6->sin6_addr.s6_addr[3] = 0;
			}
#endif
#endif
			if (getnameinfo(p->addr, p->addr->sa_len,
					_addr1_, sizeof(_addr1_),
					NULL, 0,
					NI_NUMERICHOST | niflags))
			strcpy(_addr1_, "(invalid)");
			plog(LLV_DEBUG, LOCATION, NULL,
				"my interface: %s (%s)\n",
				_addr1_, ifr->ifr_name);
			q = find_myaddr(old, p);
			if (q)
				p->sock = q->sock;
			else
				p->sock = -1;
			p->next = lcconf->myaddrs;
			lcconf->myaddrs = p;
			break;
		default:
			break;
		}
	}

	clear_myaddr(&old);

	racoon_free(iflist);
#endif /*HAVE_GETIFADDRS*/
}

/*
 * check the interface is suitable or not
 */
static int
suitable_ifaddr(ifname, ifaddr)
	const char *ifname;
	const struct sockaddr *ifaddr;
{
	switch(ifaddr->sa_family) {
	case AF_INET:
		return 1;
#ifdef INET6
	case AF_INET6:
		return suitable_ifaddr6(ifname, ifaddr);
#endif
	default:
		return 0;
	}
	/*NOTREACHED*/
}

#ifdef INET6
static int
suitable_ifaddr6(ifname, ifaddr)
	const char *ifname;
	const struct sockaddr *ifaddr;
{
	struct in6_ifreq ifr6;
	int s;

	if (ifaddr->sa_family != AF_INET6)
		return 0;

	s = socket(PF_INET6, SOCK_DGRAM, 0);
	if (s == -1) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket(SOCK_DGRAM) failed:%s\n", strerror(errno));
		return 0;
	}

	memset(&ifr6, 0, sizeof(ifr6));
	strncpy(ifr6.ifr_name, ifname, strlen(ifname));

	ifr6.ifr_addr = *(const struct sockaddr_in6 *)ifaddr;

	if (ioctl(s, SIOCGIFAFLAG_IN6, &ifr6) < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"ioctl(SIOCGIFAFLAG_IN6) failed:%s\n", strerror(errno));
		close(s);
		return 0;
	}

	close(s);

	if (ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DUPLICATED
	 || ifr6.ifr_ifru.ifru_flags6 & IN6_IFF_DETACHED)
		return 0;

	/* suitable */
	return 1;
}
#endif

int
update_myaddrs()
{
	char msg[BUFSIZ];
	int len;
	struct rt_msghdr *rtm;

	len = read(lcconf->rtsock, msg, sizeof(msg));
	if (len < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"read(PF_ROUTE) failed: %s\n",
			strerror(errno));
		return 0;
	}
	rtm = (struct rt_msghdr *)msg;
	if (len < rtm->rtm_msglen) {
		plog(LLV_ERROR, LOCATION, NULL,
			"read(PF_ROUTE) short read\n");
		return 0;
	}
	if (rtm->rtm_version != RTM_VERSION) {
		plog(LLV_ERROR, LOCATION, NULL,
			"routing socket version mismatch\n");
		close(lcconf->rtsock);
		lcconf->rtsock = 0;
		return 0;
	}
	switch (rtm->rtm_type) {
	case RTM_NEWADDR:
	case RTM_DELADDR:
	case RTM_DELETE:
	case RTM_IFINFO:
		break;
	case RTM_MISS:
		/* ignore this message silently */
		return 0;
	default:
		plog(LLV_DEBUG, LOCATION, NULL,
			"msg %d not interesting\n", rtm->rtm_type);
		return 0;
	}
	/* XXX more filters here? */

	plog(LLV_DEBUG, LOCATION, NULL,
		"caught rtm:%d, need update interface address list\n",
		rtm->rtm_type);
	return 1;
}

/*
 * initialize default port for ISAKMP to send, if no "listen"
 * directive is specified in config file.
 *
 * DO NOT listen to wildcard addresses.  if you receive packets to
 * wildcard address, you'll be in trouble (DoS attack possible by
 * broadcast storm).
 */
int
autoconf_myaddrsport()
{
	struct myaddrs *p;
	struct sockaddr_in *sin4;
#ifdef INET6
	struct sockaddr_in6 *sin6;
#endif
	int n;

	plog(LLV_DEBUG, LOCATION, NULL,
		"configuring default isakmp port.\n");
	n = 0;
	for (p = lcconf->myaddrs; p; p = p->next) {
		switch (p->addr->sa_family) {
		case AF_INET:
			sin4 = (struct sockaddr_in *)p->addr;
			sin4->sin_port = htons(lcconf->port_isakmp);
			break;
#ifdef INET6
		case AF_INET6:
			sin6 = (struct sockaddr_in6 *)p->addr;
			sin6->sin6_port = htons(lcconf->port_isakmp);
			break;
#endif
		default:
			plog(LLV_ERROR, LOCATION, NULL,
				"unsupported AF %d\n", p->addr->sa_family);
			goto err;
		}
		n++;
	}
	plog(LLV_DEBUG, LOCATION, NULL,
		"%d addrs are configured successfully\n", n);

	return 0;
err:
	plog(LLV_ERROR, LOCATION, NULL, "address autoconfiguration failed\n");
	return -1;
}

/*
 * get a port number to which racoon binded.
 * NOTE: network byte order returned.
 */
u_short
getmyaddrsport(local)
	struct sockaddr *local;
{
	struct myaddrs *p;

	/* get a relative port */
	for (p = lcconf->myaddrs; p; p = p->next) {
		if (!p->addr)
			continue;
		if (!cmpsaddrwop(local, p->addr)) {
			switch (p->addr->sa_family) {
			case AF_INET:
				return ((struct sockaddr_in *)p->addr)->sin_port;
#ifdef INET6
			case AF_INET6:
				return ((struct sockaddr_in6 *)p->addr)->sin6_port;
#endif
			default:
				plog(LLV_ERROR, LOCATION, NULL,
					"invalid family: %d\n",
					p->addr->sa_family);
				return -1;
			}
		}
		continue;
	}

	return htons(PORT_ISAKMP);
}

struct myaddrs *
newmyaddr()
{
	struct myaddrs *new;

	new = racoon_calloc(1, sizeof(*new));
	if (new == NULL) {
		plog(LLV_ERROR, LOCATION, NULL,
			"failed to allocate buffer for myaddrs.\n");
		return NULL;
	}

	new->next = NULL;
	new->addr = NULL;

	return new;
}

void
insmyaddr(new, head)
	struct myaddrs *new;
	struct myaddrs **head;
{
	new->next = *head;
	*head = new;
}

void
delmyaddr(myaddr)
	struct myaddrs *myaddr;
{
	if (myaddr->addr)
		racoon_free(myaddr->addr);
	racoon_free(myaddr);
}

int
initmyaddr()
{
	/* initialize routing socket */
	lcconf->rtsock = socket(PF_ROUTE, SOCK_RAW, PF_UNSPEC);
	if (lcconf->rtsock < 0) {
		plog(LLV_ERROR, LOCATION, NULL,
			"socket(PF_ROUTE) failed: %s",
			strerror(errno));
		return -1;
	}

	if (lcconf->myaddrs == NULL && lcconf->autograbaddr == 1) {
		grab_myaddrs();

		if (autoconf_myaddrsport() < 0)
			return -1;
	}

	return 0;
}

