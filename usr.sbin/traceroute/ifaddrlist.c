/*	$NetBSD: ifaddrlist.c,v 1.5 2002/07/06 21:51:30 wiz Exp $	*/

/*
 * Copyright (c) 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char rcsid[] =
    "@(#) Header: ifaddrlist.c,v 1.2 97/04/22 13:31:05 leres Exp  (LBL)";
#else
__RCSID("$NetBSD: ifaddrlist.c,v 1.5 2002/07/06 21:51:30 wiz Exp $");
#endif
#endif

#include <sys/param.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/time.h>				/* concession to AIX */

struct mbuf;
struct rtentry;

#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <ctype.h>
#include <errno.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

#include "gnuc.h"
#ifdef HAVE_OS_PROTO_H
#include "os-proto.h"
#endif

#include "ifaddrlist.h"
#include "savestr.h"


/* Not all systems have IFF_LOOPBACK */
#ifdef HAVE_IFADDRS_H
#ifdef IFF_LOOPBACK
#define ISLOOPBACK(p) ((p)->ifa_flags & IFF_LOOPBACK)
#else
#define ISLOOPBACK(p) (strcmp((p)->ifa_name, "lo0") == 0)
#endif
#else
#ifdef IFF_LOOPBACK
#define ISLOOPBACK(p) ((p)->ifr_flags & IFF_LOOPBACK)
#else
#define ISLOOPBACK(p) (strcmp((p)->ifr_name, "lo0") == 0)
#endif
#endif

#define MAX_IPADDR 256

/*
 * Return the interface list
 */
int
ifaddrlist(struct ifaddrlist **ipaddrp, char *errbuf, int buflen)
{
#ifdef HAVE_IFADDRS_H
	int nipaddr;
	struct sockaddr_in *sin;
	struct ifaddrs *ifap, *ifa;
	struct ifaddrlist *al;
	static struct ifaddrlist ifaddrlist[MAX_IPADDR];

	al = ifaddrlist;
	nipaddr = 0;

	if (getifaddrs(&ifap) != 0) {
		(void)snprintf(errbuf, buflen, "getifaddrs: %s",
		    strerror(errno));
		return (-1);
	}

	for (ifa = ifap; ifa; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr->sa_family != AF_INET)
			continue;

		/* Must be up */
		if ((ifa->ifa_flags & IFF_UP) == 0)
			continue;

		/*
		 * Must not be a loopback address (127/8)
		 */
		sin = (struct sockaddr_in *)ifa->ifa_addr;
		if (ISLOOPBACK(ifa))
			if (ntohl(sin->sin_addr.s_addr) == INADDR_LOOPBACK)
				continue;

		al->addr = sin->sin_addr.s_addr;
		al->device = savestr(ifa->ifa_name);
		++al;
		++nipaddr;
	}
	*ipaddrp = ifaddrlist;
	freeifaddrs(ifap);
	return (nipaddr);
#else
	int fd, nipaddr;
#ifdef HAVE_SOCKADDR_SA_LEN
	int n;
#endif
	struct ifreq *ifrp, *ifend, *ifnext, *mp;
	struct sockaddr_in *sin;
	struct ifaddrlist *al;
	struct ifconf ifc;
	struct ifreq ibuf[MAX_IPADDR], ifr;
	char device[sizeof(ifr.ifr_name) + 1];
	static struct ifaddrlist ifaddrlist[MAX_IPADDR];

	fd = socket(AF_INET, SOCK_DGRAM, 0);
	if (fd < 0) {
		(void)snprintf(errbuf, buflen, "socket: %s", strerror(errno));
		return (-1);
	}
	ifc.ifc_len = sizeof(ibuf);
	ifc.ifc_buf = (caddr_t)ibuf;

	if (ioctl(fd, SIOCGIFCONF, (char *)&ifc) < 0 ||
	    ifc.ifc_len < sizeof(struct ifreq)) {
		(void)snprintf(errbuf, buflen, "SIOCGIFCONF: %s", strerror(errno));
		(void)close(fd);
		return (-1);
	}
	ifrp = ibuf;
	ifend = (struct ifreq *)((char *)ibuf + ifc.ifc_len);

	al = ifaddrlist;
	mp = NULL;
	nipaddr = 0;
	for (; ifrp < ifend; ifrp = ifnext) {
#ifdef HAVE_SOCKADDR_SA_LEN
		n = ifrp->ifr_addr.sa_len + sizeof(ifrp->ifr_name);
		if (n < sizeof(*ifrp))
			ifnext = ifrp + 1;
		else
			ifnext = (struct ifreq *)((char *)ifrp + n);
		if (ifrp->ifr_addr.sa_family != AF_INET)
			continue;
#else
		ifnext = ifrp + 1;
#endif
		/*
		 * Need a template to preserve address info that is
		 * used below to locate the next entry.  (Otherwise,
		 * SIOCGIFFLAGS stomps over it because the requests
		 * are returned in a union.)
		 */
		strncpy(ifr.ifr_name, ifrp->ifr_name, sizeof(ifr.ifr_name));
		if (ioctl(fd, SIOCGIFFLAGS, (char *)&ifr) < 0) {
			if (errno == ENXIO)
				continue;
			(void)snprintf(errbuf, buflen, "SIOCGIFFLAGS: %.*s: %s",
			    (int)sizeof(ifr.ifr_name), ifr.ifr_name,
			    strerror(errno));
			(void)close(fd);
			return (-1);
		}

		/* Must be up */
		if ((ifr.ifr_flags & IFF_UP) == 0)
			continue;

		/*
		 * Must not be a loopback address (127/8)
		 */
		sin = (struct sockaddr_in *)&ifrp->ifr_addr;
		if (ISLOOPBACK(&ifr))
			if (ntohl(sin->sin_addr.s_addr) == INADDR_LOOPBACK)
				continue;

		(void)strncpy(device, ifrp->ifr_name, sizeof(ifrp->ifr_name));
		device[sizeof(device) - 1] = '\0';

		al->addr = sin->sin_addr.s_addr;
		al->device = savestr(device);
		++al;
		++nipaddr;
	}
	(void)close(fd);

	*ipaddrp = ifaddrlist;
	return (nipaddr);
#endif
}
