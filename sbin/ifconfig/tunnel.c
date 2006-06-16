/*	$NetBSD: tunnel.c,v 1.6 2006/06/16 23:48:35 elad Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
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
__RCSID("$NetBSD: tunnel.c,v 1.6 2006/06/16 23:48:35 elad Exp $");
#endif /* not lint */

#include <sys/param.h> 
#include <sys/ioctl.h> 
#include <sys/socket.h>

#include <net/if.h> 

#ifdef INET6
#include <netinet/in.h>
#endif

#include <ctype.h>
#include <err.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "extern.h"
#include "tunnel.h"

#ifdef INET6
#include "af_inet6.h"
#endif

void
settunnel(const char *src, const char *dst)
{
	struct addrinfo hints, *srcres, *dstres;
	int ecode;
	struct if_laddrreq req;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = afp->af_af;
	hints.ai_socktype = SOCK_DGRAM; /*dummy*/

	if ((ecode = getaddrinfo(src, NULL, &hints, &srcres)) != 0)
		errx(EXIT_FAILURE, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if ((ecode = getaddrinfo(dst, NULL, &hints, &dstres)) != 0)
		errx(EXIT_FAILURE, "error in parsing address string: %s",
		    gai_strerror(ecode));

	if (srcres->ai_addr->sa_family != dstres->ai_addr->sa_family)
		errx(EXIT_FAILURE,
		    "source and destination address families do not match");

	if (srcres->ai_addrlen > sizeof(req.addr) ||
	    dstres->ai_addrlen > sizeof(req.dstaddr))
		errx(EXIT_FAILURE, "invalid sockaddr");

	memset(&req, 0, sizeof(req));
	estrlcpy(req.iflr_name, name, sizeof(req.iflr_name));
	memcpy(&req.addr, srcres->ai_addr, srcres->ai_addrlen);
	memcpy(&req.dstaddr, dstres->ai_addr, dstres->ai_addrlen);

#ifdef INET6
	if (req.addr.ss_family == AF_INET6) {
		struct sockaddr_in6 *s6, *d;

		s6 = (struct sockaddr_in6 *)&req.addr;
		d = (struct sockaddr_in6 *)&req.dstaddr;
		if (s6->sin6_scope_id != d->sin6_scope_id) {
			errx(EXIT_FAILURE, "scope mismatch");
			/* NOTREACHED */
		}
#ifdef __KAME__
		/* embed scopeid */
		if (s6->sin6_scope_id &&
		    (IN6_IS_ADDR_LINKLOCAL(&s6->sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&s6->sin6_addr))) {
			*(u_int16_t *)&s6->sin6_addr.s6_addr[2] =
			    htons(s6->sin6_scope_id);
		}
		if (d->sin6_scope_id &&
		    (IN6_IS_ADDR_LINKLOCAL(&d->sin6_addr) ||
		     IN6_IS_ADDR_MC_LINKLOCAL(&d->sin6_addr))) {
			*(u_int16_t *)&d->sin6_addr.s6_addr[2] =
			    htons(d->sin6_scope_id);
		}
#endif /* __KAME__ */
	}
#endif /* INET6 */

	if (ioctl(s, SIOCSLIFPHYADDR, &req) == -1)
		warn("SIOCSLIFPHYADDR");

	freeaddrinfo(srcres);
	freeaddrinfo(dstres);
}

/*ARGSUSED*/
void
deletetunnel(const char *ifname, int param)
{

	if (ioctl(s, SIOCDIFPHYADDR, &ifr) == -1)
		err(EXIT_FAILURE, "SIOCDIFPHYADDR");
}

void
tunnel_status(void)
{
	char psrcaddr[NI_MAXHOST];
	char pdstaddr[NI_MAXHOST];
	const int niflag = NI_NUMERICHOST;
	struct if_laddrreq req;
	const struct afswtch *lafp;

	psrcaddr[0] = pdstaddr[0] = '\0';

	memset(&req, 0, sizeof(req));
	estrlcpy(req.iflr_name, name, IFNAMSIZ);
	if (ioctl(s, SIOCGLIFPHYADDR, &req) == -1)
		return;
	lafp = lookup_af_bynum(req.addr.ss_family);
#ifdef INET6
	if (req.addr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.addr);
#endif /* INET6 */
	getnameinfo((struct sockaddr *)&req.addr, req.addr.ss_len,
	    psrcaddr, sizeof(psrcaddr), 0, 0, niflag);

#ifdef INET6
	if (req.dstaddr.ss_family == AF_INET6)
		in6_fillscopeid((struct sockaddr_in6 *)&req.dstaddr);
#endif
	getnameinfo((struct sockaddr *)&req.dstaddr, req.dstaddr.ss_len,
	    pdstaddr, sizeof(pdstaddr), 0, 0, niflag);

	printf("\ttunnel %s %s --> %s\n", lafp ? lafp->af_name : "???",
	    psrcaddr, pdstaddr);
}
