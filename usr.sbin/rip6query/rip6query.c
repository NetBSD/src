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

#include <stdio.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <net/if.h>
#if defined(__FreeBSD__) && __FreeBSD__ >= 3
#include <net/if_var.h>
#endif /* __FreeBSD__ >= 3 */
#include <netinet/in.h>
#include <netinet/in_var.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "route6d.h"

int	s;
extern int errno;
struct sockaddr_in6 sin6;
struct rip6	*ripbuf;

#define	RIPSIZE(n)	(sizeof(struct rip6) + (n-1) * sizeof(struct netinfo6))

int main __P((int, char **));
void fatal __P((char *));
const char *inet6_n2a __P((struct in6_addr *));

int main(argc, argv)
	int argc;
	char **argv;
{
	struct netinfo6 *np;
	struct sockaddr_in6 fsock;
	struct hostent *hp;
	char *hostname;
	int i, n, len, flen;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s address\n", *argv);
		exit(-1);
	}

	if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0)
		fatal("socket");

	hp = (struct hostent *)gethostbyname2(argv[1], AF_INET6);
	if (hp == NULL) {
		if (inet_pton(AF_INET6, argv[1], (u_int32_t *)&sin6.sin6_addr)
			!= 1) {
			fprintf(stderr, "%s: unknown host %s\n",
				argv[0], argv[1]);
			exit(-1);
		}
	} else {
		bcopy(hp->h_addr, (caddr_t)&sin6.sin6_addr, hp->h_length);
		hostname = strdup(hp->h_name);
	}

	sin6.sin6_len = sizeof(struct sockaddr_in6);
	sin6.sin6_family = AF_INET6;
	sin6.sin6_port = htons(RIP6_PORT);

	if ((ripbuf = (struct rip6 *)malloc(BUFSIZ)) == NULL)
		fatal("malloc");
	ripbuf->rip6_cmd = RIP6_REQUEST;
	ripbuf->rip6_vers = RIP6_VERSION;
	ripbuf->rip6_res1[0] = 0;
	ripbuf->rip6_res1[1] = 0;
	np = ripbuf->rip6_nets;
	bzero(&np->rip6_dest, sizeof(struct in6_addr));
	np->rip6_tag = 0;
	np->rip6_plen = 0;
	np->rip6_metric = HOPCNT_INFINITY6;
	if (sendto(s, ripbuf, RIPSIZE(1), 0,
		(struct sockaddr *)&sin6, sizeof(struct sockaddr_in6)) < 0)
		fatal("send");
	do {
		flen = sizeof(struct sockaddr_in6);
		if ((len = recvfrom(s, ripbuf, BUFSIZ, 0,
			(struct sockaddr *)&fsock, &flen)) < 0)
			fatal("recvfrom");
		printf("Response from %s len %d\n",
			inet6_n2a(&fsock.sin6_addr), len);
		n = (len - sizeof(struct rip6) + sizeof(struct netinfo6)) /
			sizeof(struct netinfo6);
		np = ripbuf->rip6_nets;
		for (i = 0; i < n; i++, np++) {
			printf("\t%s/%d [%d]", inet6_n2a(&np->rip6_dest),
				np->rip6_plen, np->rip6_metric);
			if (np->rip6_tag)
				printf(" tag=0x%x", ntohs(np->rip6_tag));
			printf("\n");
		}
	} while (len == RIPSIZE(24));

	exit(0);
}

void fatal(p)
	char *p;
{
	fprintf(stderr, "%s: %s", p, strerror(errno));
	exit(-1);
}

const char *inet6_n2a(p)
	struct in6_addr *p;
{
	static char buf[BUFSIZ];

	return inet_ntop(AF_INET6, (u_int32_t *)p, buf, sizeof(buf));
}
