/*	$NetBSD: ipsyncs.c,v 1.1.1.1 2004/03/28 08:56:35 martti Exp $	*/

/*
 * Copyright (C) 1993-2001 by Darren Reed.
 *
 * See the IPFILTER.LICENCE file for details on licencing.
 */
#if !defined(lint)
static const char sccsid[] = "@(#)ip_fil.c	2.41 6/5/96 (C) 1993-2000 Darren Reed";
static const char rcsid[] = "@(#)Id: ipsyncs.c,v 1.5 2003/09/04 18:40:43 darrenr Exp";
#endif
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <net/if.h>

#include <arpa/inet.h>

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <strings.h>
#include <unistd.h>

#include "netinet/ip_compat.h"
#include "netinet/ip_fil.h"


int	main __P((int, char *[]));


int main(argc, argv)
int argc;
char *argv[];
{
	int fd, nfd, lfd, i, n, slen;
	struct sockaddr_in sin, san;
	struct in_addr in;
	char buff[1400];

	fd = open(IPSYNC_NAME, O_WRONLY);
	if (fd == -1) {
		perror("open");
		exit(1);
	}

	bzero((char *)&sin, sizeof(sin));
	sin.sin_family = AF_INET;
	if (argc > 1)
		sin.sin_addr.s_addr = inet_addr(argv[1]);
	if (argc > 2)
		sin.sin_port = htons(atoi(argv[2]));
	else
		sin.sin_port = htons(43434);
	if (argc > 3)
		in.s_addr = inet_addr(argv[3]);
	else
		in.s_addr = 0;

	lfd = socket(AF_INET, SOCK_STREAM, 0);
	if (lfd == -1) {
		perror("socket");
		exit(1);
	}

	n = 1;
	setsockopt(lfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

	if (bind(lfd, (struct sockaddr *)&sin, sizeof(sin)) == -1) {
		perror("bind");
		exit(1);
	}

	listen(lfd, 1);

	do {
		slen = sizeof(san);
		nfd = accept(lfd, (struct sockaddr *)&san, &slen);
		if (nfd == -1) {
			perror("accept");
			continue;
		}
		n = 1;
		setsockopt(nfd, SOL_SOCKET, SO_REUSEADDR, &n, sizeof(n));

		printf("Connection from %s\n", inet_ntoa(san.sin_addr));

		if (in.s_addr && (in.s_addr != san.sin_addr.s_addr)) {
			close(nfd);
			continue;
		}

		while ((n = read(nfd, buff, sizeof(buff))) > 0) {
			i = write(fd, buff, n);
			if (i != n) {
				perror("write");
				exit(1);
			}
		}
		close(nfd);
	} while (1);

	close(lfd);

	exit(0);
}
