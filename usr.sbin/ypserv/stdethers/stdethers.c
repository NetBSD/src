/*	$NetBSD: stdethers.c,v 1.3 1997/07/18 21:57:08 thorpej Exp $	*/

/*
 * Copyright (c) 1995 Mats O Jansson <moj@stacken.kth.se>
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

#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <net/if_ether.h>
#include <netinet/in.h>
#ifndef NTOA_FIX	/* XXX the below is wrong with old ARP code */
#include <net/if_ether.h> 
#endif
#include <ctype.h>
#include <err.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

#include "protos.h"

int	main __P((int, char *[]));
void	usage __P((void));

extern	char *__progname;		/* from crt0.o */

#ifndef NTOA_FIX
#define	NTOA(x) ether_ntoa(x)
#else
char	*working_ntoa __P((struct ether_addr *));

#define NTOA(x) working_ntoa(x)

/*
 * As of 1995-12-02 NetBSD has an SunOS 4 incompatible ether_ntoa.
 * The code in usr/lib/libc/net/ethers seems to do the correct thing
 * when asking YP but not when returning string from ether_ntoa.
 */

char *
working_ntoa(e)
	struct ether_addr *e;
{
	static char a[] = "xx:xx:xx:xx:xx:xx";

	sprintf(a, "%x:%x:%x:%x:%x:%x",
	    e->ether_addr_octet[0], e->ether_addr_octet[1],
	    e->ether_addr_octet[2], e->ether_addr_octet[3],
	    e->ether_addr_octet[4], e->ether_addr_octet[5]);
	return a;
}
#endif

int
main(argc, argv)
	int argc;
	char *argv[];
{
	FILE *data_file;
	char data_line[_POSIX2_LINE_MAX];
	int line_no = 0, len;
	char *fname;
	struct ether_addr eth_addr;
	char hostname[MAXHOSTNAMELEN];

	if (argc > 2)
		usage();

	if (argc == 2) {
		fname = argv[1];
		data_file = fopen(fname, "r");
		if (data_file == NULL)
			err(1, "%s", fname);
	} else {
		fname = "<stdin>";
		data_file = stdin;
	}

	while (read_line(data_file, data_line, sizeof(data_line))) {
		line_no++;
		len = strlen(data_line);

		if (len > 1) {
			if (data_line[0] == '#') {
				continue;
			}
		} else
			continue;

		/*
		 * Check if we have the whole line
		 */
		if (data_line[len - 1] != '\n') {
			warnx("%s line %d: line to long, skipping",
			    fname, line_no);
			continue;
		} else
			data_line[len - 1] = '\0';

		if (ether_line(data_line, &eth_addr, hostname) == 0)
			printf("%s\t%s\n", NTOA(&eth_addr), hostname);
		else
			warnx("ignoring line %d: `%s'", line_no, data_line);
	}

	exit(0);
}

void
usage()
{

	fprintf(stderr, "usage: %s [file]\n", __progname);
	exit(1);
}
