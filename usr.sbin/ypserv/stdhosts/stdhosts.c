/*	$NetBSD: stdhosts.c,v 1.15 2002/07/06 00:55:29 wiz Exp $	 */

/*
 * Copyright (c) 1994 Mats O Jansson <moj@stacken.kth.se>
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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by Mats O Jansson
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: stdhosts.c,v 1.15 2002/07/06 00:55:29 wiz Exp $");
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>
#include <unistd.h>
#include <netdb.h>

#include "protos.h"

int	main(int, char *[]);
void	usage(void);

int
main(int argc, char *argv[])
{
	struct in_addr host_addr;
	FILE	*data_file;
	size_t	 line_no;
	size_t	 len;
	char	*line, *k, *v, *addr_string, *fname;
	int	 ch;
	int	 af = 1 << 4;	/*IPv4*/
	struct addrinfo hints, *res;

	addr_string = NULL;		/* XXX gcc -Wuninitialized */

	while ((ch = getopt(argc, argv, "n")) != -1) {
		switch (ch) {
		case 'n':
			af |= 1 << 6;	/*IPv6*/
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc > 1)
		usage();

	if (argc == 1) {
		fname = argv[0];
		data_file = fopen(fname, "r");
		if (data_file == NULL)
			err(1, "%s", fname); 
	} else {
		fname = "<stdin>";
		data_file = stdin;
	}

	line_no = 0;
	for (;
	    (line = fparseln(data_file, &len, &line_no, NULL,
		FPARSELN_UNESCALL));
	    free(line)) {
		if (len == 0)
			continue;

		v = line;
		for (k = v; *v && !isspace(*v); v++)
			;
		while (*v && isspace(*v))
			*v++ = '\0';

		memset(&hints, 0, sizeof(hints));
		hints.ai_socktype = SOCK_DGRAM;		/*dummy*/
		hints.ai_flags = AI_NUMERICHOST;

		if ((af & (1 << 4)) != 0 && inet_aton(k, &host_addr) == 1 &&
		    (addr_string = inet_ntoa(host_addr)) != NULL) {
			/* IPv4 */
			printf("%s %s\n", addr_string, v);
		} else if ((af & (1 << 6)) != 0 &&
			   getaddrinfo(k, "0", &hints, &res) == 0) {
			/* IPv6, with scope extension permitted */
			freeaddrinfo(res);
			printf("%s %s\n", k, v);
		} else
			warnx("%s line %lu: syntax error", fname,
			    (unsigned long)line_no);
	}

	exit(0);
}

void
usage(void)
{

	fprintf(stderr, "usage: %s [-n] [file]\n", getprogname());
	exit(1);
}
