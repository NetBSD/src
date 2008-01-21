/* 	$NetBSD: tcpdrop.c,v 1.3.8.2 2008/01/21 20:17:50 bouyer Exp $	 */

/*
 * Copyright (c) 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Herb Hasler and Rick Macklem at The University of Guelph.
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

/* $OpenBSD: tcpdrop.c,v 1.5 2006/01/03 01:46:27 stevesk Exp $ */

/*
 * Copyright (c) 2004 Markus Friedl <markus@openbsd.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/sysctl.h>

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netinet/ip_var.h>
#include <netinet/tcp_timer.h>
#include <netinet/tcp_var.h>

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <netdb.h>

struct hpinfo {
	char host[NI_MAXHOST];
	char serv[NI_MAXSERV];
};

static struct addrinfo *
egetaddrinfo(const char *host, const char *serv)
{
	static const struct addrinfo hints = {
		.ai_family = AF_UNSPEC,
		.ai_socktype = SOCK_STREAM,
	};
	struct addrinfo *ai;
	int gaierr;

	if ((gaierr = getaddrinfo(host, serv, &hints, &ai)) != 0)
		errx(1, "%s port %s: %s", host, serv, gai_strerror(gaierr));
	return ai;
}

static void
egetnameinfo(const struct addrinfo *ai, struct hpinfo *hp)
{
	int gaierr;

	if ((gaierr = getnameinfo(ai->ai_addr, ai->ai_addrlen,
	    hp->host, sizeof(hp->host), hp->serv, sizeof(hp->serv),
	    NI_NUMERICHOST | NI_NUMERICSERV)) != 0)
		errx(1, "getnameinfo: %s", gai_strerror(gaierr));
}

/*
 * Drop a tcp connection.
 */
int
main(int argc, char **argv)
{
	int mib[] = { CTL_NET, 0, IPPROTO_TCP, TCPCTL_DROP };
	struct addrinfo *ail, *aif, *laddr, *faddr;
	struct sockaddr_storage sa[2];
	struct hpinfo fhp, lhp;
	int rval = 0;

	setprogname(argv[0]);

	if (argc != 5) {
		(void)fprintf(stderr, "Usage: %s laddr lport faddr fport\n",
		    getprogname());
		return 1;
	}

	laddr = egetaddrinfo(argv[1], argv[2]);
	faddr = egetaddrinfo(argv[3], argv[4]);

	for (ail = laddr; ail; ail = ail->ai_next) {
		for (aif = faddr; aif; aif = aif->ai_next) {

			if (ail->ai_family != aif->ai_family)
				continue;

			egetnameinfo(ail, &lhp);
			egetnameinfo(aif, &fhp);

			(void)memset(sa, 0, sizeof(sa));

			assert(aif->ai_addrlen <= sizeof(*sa));
			assert(ail->ai_addrlen <= sizeof(*sa));

			(void)memcpy(&sa[0], aif->ai_addr, aif->ai_addrlen);
			(void)memcpy(&sa[1], ail->ai_addr, ail->ai_addrlen);

			switch (ail->ai_family) {
			case AF_INET:
				mib[1] = PF_INET;
				break;
			case AF_INET6:
				mib[1] = PF_INET6;
				break;
			default:
				warnx("Unsupported socket address family %d",
				    ail->ai_family);
				continue;
			}

			if (sysctl(mib, sizeof(mib) / sizeof(int), NULL,
			    NULL, sa, sizeof(sa)) == -1) {
				rval = 1;
				warn("%s:%s, %s:%s", 
				    lhp.host, lhp.serv, fhp.host, fhp.serv);
			} else
				(void)printf("%s:%s %s:%s dropped\n",
				    lhp.host, lhp.serv, fhp.host, fhp.serv);

		}
	}
	freeaddrinfo(laddr);
	freeaddrinfo(faddr);
	return rval;
}
