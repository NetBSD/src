/*	$NetBSD: pt_tcp.c,v 1.17 2003/08/07 10:04:31 agc Exp $	*/

/*
 * Copyright (c) 1992, 1993, 1994
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software donated to Berkeley by
 * Jan-Simon Pendry.
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
 *
 *	from: Id: pt_tcp.c,v 1.1 1992/05/25 21:43:09 jsp Exp
 *	@(#)pt_tcp.c	8.5 (Berkeley) 4/28/95
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: pt_tcp.c,v 1.17 2003/08/07 10:04:31 agc Exp $");
#endif /* not lint */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/syslog.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "portald.h"

/*
 * Key will be tcp/host/port[/"priv"]
 * Create a TCP socket connected to the
 * requested host and port.
 * Some trailing suffix values have special meanings.
 * An unrecognised suffix is an error.
 */
int
portal_tcp(pcr, key, v, kso, fdp)
	struct portal_cred *pcr;
	char *key;
	char **v;
	int kso;
	int *fdp;
{
	char host[MAXHOSTNAMELEN];
	char port[MAXHOSTNAMELEN];
	char *p = key + (v[1] ? strlen(v[1]) : 0);
	char *q;
	int priv = 0;
#ifdef INET6
	struct addrinfo hints, *res, *lres;
	int so = -1;
	const char *cause = "unknown";
#else /* ! INET6 */
	struct hostent *hp;
	struct servent *sp;
	struct in_addr **ipp;
	struct in_addr *ip[2];
	struct in_addr ina;
	int s_port;
	struct sockaddr_in sain;
#endif

	q = strchr(p, '/');
	if (q == 0 || q - p >= sizeof(host))
		return (EINVAL);
	*q = '\0';
	if (strlcpy(host, p, sizeof(host)) >= sizeof(host))
		return (EINVAL);
	p = q + 1;

	q = strchr(p, '/');
	if (q)
		*q = '\0';
	if (strlcpy(port, p, sizeof(port)) >= sizeof(port))
		return (EINVAL);
	if (q) {
		p = q + 1;
		if (strcmp(p, "priv") == 0) {
			if (pcr->pcr_uid == 0)
				priv = 1;
			else
				return (EPERM);
		} else {
			return (EINVAL);
		}
	}

#ifdef INET6
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = PF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;
	if (getaddrinfo(host, port, &hints, &res) != 0)
		return(EINVAL);

	for (lres = res; lres; lres = lres->ai_next) {
		if (priv)
			so = rresvport((int *) 0);
		else
			so = socket(lres->ai_family, lres->ai_socktype,
			    lres->ai_protocol);
		if (so < 0) {
			cause = "socket";
			continue;
		}

		if (connect(so, lres->ai_addr, lres->ai_addrlen) != 0) {
			cause = "connect";
			(void)close(so);
			so = -1;
			continue;
		}

		*fdp = so;
		errno = 0;
		break;
	}

	if (so < 0)
		syslog(LOG_WARNING, "%s: %m", cause);
		
	freeaddrinfo(res);
#else /* ! INET6 */
	if (inet_aton(host, &ina) == 0) {
		hp = gethostbyname(host);
		if (hp == 0)
			return (EINVAL);
		ipp = (struct in_addr **) hp->h_addr_list;
	} else {
		ip[0] = &ina;
		ip[1] = 0;
		ipp = ip;
	}

	sp = getservbyname(port, "tcp");
	if (sp != 0)
		s_port = sp->s_port;
	else {
		s_port = strtoul(port, &p, 0);
		if (s_port == 0 || *p != '\0')
			return (EINVAL);
		s_port = htons(s_port);
	}

	memset(&sain, 0, sizeof(sain));
	sain.sin_len = sizeof(sain);
	sain.sin_family = AF_INET;
	sain.sin_port = s_port;

	while (ipp[0]) {
		int so;

		if (priv)
			so = rresvport((int *) 0);
		else
			so = socket(AF_INET, SOCK_STREAM, 0);
		if (so < 0) {
			syslog(LOG_WARNING, "socket: %m");
			return (errno);
		}

		sain.sin_addr = *ipp[0];
		if (connect(so, (struct sockaddr *) &sain,
		    sizeof(sain)) == 0) {
			*fdp = so;
			return (0);
		}
		(void) close(so);

		ipp++;
	}
#endif /* INET6 */

	return (errno);
}
