/* 	$NetBSD: get_net.c,v 1.3 2020/04/23 00:22:01 joerg Exp $	 */

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

#include <sys/cdefs.h>
__RCSID("$NetBSD: get_net.c,v 1.3 2020/04/23 00:22:01 joerg Exp $");

#include <sys/types.h>
#include <netdb.h>
#include <stdio.h>
#include <limits.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <util.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "mountd.h"

#ifdef TEST
int      opt_flags;
const int ninumeric = NI_NUMERICHOST;
int      mountd_debug = 1;
#endif

static int 
isdottedquad(const char *cp)
{
	for (;;)
		switch (*cp++) {
		case '\0':
			return 1;
		case '.':
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			continue;
		default:
			return 0;
		}
}

static int
countones(struct sockaddr *sa)
{
	void *mask;
	int i, bits = 0, bytelen;
	u_int8_t *p;

	switch (sa->sa_family) {
	case AF_INET:
		mask = (u_int8_t *)&((struct sockaddr_in *)sa)->sin_addr;
		bytelen = 4;
		break;
	case AF_INET6:
		mask = (u_int8_t *)&((struct sockaddr_in6 *)sa)->sin6_addr;
		bytelen = 16;
		break;
	default:
		return 0;
	}

	p = mask;

	for (i = 0; i < bytelen; i++, p++) {
		if (*p != 0xff) {
			for (bits = 0; bits < 8; bits++) {
				if (!(*p & (1 << (7 - bits))))
					break;
			}
			break;
		}
	}

	return (i * 8 + bits);
}

/*
 * Translate a net address.
 */
int
get_net(char *cp, struct netmsk *net, int maskflg)
{
	struct netent *np;
	char *nname, *p, *prefp;
	struct sockaddr_in sin, *sinp;
	struct sockaddr *sa;
	struct addrinfo hints, *ai = NULL;
	char netname[NI_MAXHOST];
	long preflen;
	int ecode;

	(void)memset(&sin, 0, sizeof(sin));
	if ((opt_flags & OP_MASKLEN) && !maskflg) {
		p = strchr(cp, '/');
		*p = '\0';
		prefp = p + 1;
	} else {
		p = NULL;	/* XXXGCC -Wuninitialized */
		prefp = NULL;	/* XXXGCC -Wuninitialized */
	}

	if ((np = getnetbyname(cp)) != NULL) {
		sin.sin_family = AF_INET;
		sin.sin_len = sizeof sin;
		sin.sin_addr = inet_makeaddr(np->n_net, 0);
		sa = (struct sockaddr *)&sin;
	} else if (isdottedquad(cp)) {
		/*
		 * Handle dotted quad [or less than quad] notation specially
		 * because getaddrinfo() will parse them in the old style:
		 * i.e. 1.2.3 -> 1.2.0.3 not 1.2.3.0
		 */
		sin.sin_family = AF_INET;
		sin.sin_len = sizeof sin;
		sin.sin_addr = inet_makeaddr(inet_network(cp), 0);
		if (mountd_debug)
			fprintf(stderr, "get_net: '%s' v4 addr %x\n",
			    cp, sin.sin_addr.s_addr);
		sa = (struct sockaddr *)&sin;
	} else {
		memset(&hints, 0, sizeof hints);
		hints.ai_family = AF_UNSPEC;
		hints.ai_flags = AI_NUMERICHOST;
		if (getaddrinfo(cp, NULL, &hints, &ai) == 0)
			sa = ai->ai_addr;
		else
			goto fail;
	}

	/*
	 * Disallow v6 addresses without a specific mask or masklen
	 */
	if (sa->sa_family == AF_INET6 && (!(opt_flags & OP_MASKLEN) || maskflg))
		return 1;

	ecode = getnameinfo(sa, sa->sa_len, netname, sizeof netname,
	    NULL, 0, ninumeric);
	if (ecode != 0)
		goto fail;

	if (maskflg)
		net->nt_len = countones(sa);
	else {
		if (opt_flags & OP_MASKLEN) {
			errno = 0;
			preflen = strtol(prefp, NULL, 10);
			if (preflen == LONG_MIN && errno == ERANGE)
				goto fail;
			net->nt_len = (int)preflen;
			*p = '/';
		}

		if (np)
			nname = np->n_name;
		else {
			if (getnameinfo(sa, sa->sa_len, netname, sizeof netname,
			    NULL, 0, ninumeric) != 0)
				strlcpy(netname, "?", sizeof(netname));
			nname = netname;
		}
		net->nt_name = estrdup(nname);
		memcpy(&net->nt_net, sa, sa->sa_len);
	}

	if (!maskflg && sa->sa_family == AF_INET &&
	    !(opt_flags & (OP_MASK|OP_MASKLEN))) {
		sinp = (struct sockaddr_in *)sa;
		if (IN_CLASSA(sinp->sin_addr.s_addr))
			net->nt_len = 8;
		else if (IN_CLASSB(sinp->sin_addr.s_addr))
			net->nt_len = 16;
		else if (IN_CLASSC(sinp->sin_addr.s_addr))
			net->nt_len = 24;
		else if (IN_CLASSD(sinp->sin_addr.s_addr))
			net->nt_len = 28;
		else
			net->nt_len = 32;	/* XXX */
	}

	if (ai)
		freeaddrinfo(ai);
	return 0;

fail:
	if (ai)
		freeaddrinfo(ai);
	return 1;
}

#ifdef TEST
int
main(int argc, char *argv[])
{
	char buf[1024];
	struct netmsk nm;

	if (get_net(argv[1], &nm, 0))
		errx(EXIT_FAILURE, "get_net failed");

	sockaddr_snprintf(buf, sizeof(buf), "%a:%p", (void *)&nm.nt_net);
	printf("%s %d %s\n", buf, nm.nt_len, nm.nt_name);
	return 0;
}
#endif
