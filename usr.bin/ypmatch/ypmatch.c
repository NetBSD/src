/*	$NetBSD: ypmatch.c,v 1.10 1997/07/18 07:05:36 thorpej Exp $	*/

/*
 * Copyright (c) 1992, 1993 Theo de Raadt <deraadt@fsa.ca>
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
 *	This product includes software developed by Theo de Raadt.
 * 4. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
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
__RCSID("$NetBSD: ypmatch.c,v 1.10 1997/07/18 07:05:36 thorpej Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

const struct ypalias {
	char *alias, *name;
} ypaliases[] = {
	{ "passwd", "passwd.byname" },
	{ "group", "group.byname" },
	{ "networks", "networks.byaddr" },
	{ "hosts", "hosts.byname" },
	{ "protocols", "protocols.bynumber" },
	{ "services", "services.byname" },
	{ "aliases", "mail.aliases" },
	{ "ethers", "ethers.byname" },
};

int	main __P((int, char *[]));
void	usage __P((void));

extern	char *__progname;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	char *domainname;
	char *inkey, *inmap, *outbuf;
	extern char *optarg;
	extern int optind;
	int outbuflen, key, notrans;
	int c, r, i;
	int rval;

	domainname = NULL;
	notrans = key = 0;
	while ((c = getopt(argc, argv, "xd:kt")) != -1) {
		switch (c) {
		case 'x':
			for(i = 0;
			    i < sizeof(ypaliases)/sizeof(ypaliases[0]); i++)
				printf("Use \"%s\" for \"%s\"\n",
					ypaliases[i].alias,
					ypaliases[i].name);
			exit(0);

		case 'd':
			domainname = optarg;
			break;

		case 't':
			notrans++;
			break;

		case 'k':
			key++;
			break;

		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc < 2)
		usage();

	if (domainname == NULL)
		yp_get_default_domain(&domainname);

	inmap = argv[argc - 1];
	if (notrans == 0) {
		for (i = 0 ; i < sizeof(ypaliases)/sizeof(ypaliases[0]); i++)
			if (strcmp(inmap, ypaliases[i].alias) == 0)
				inmap = ypaliases[i].name;
	}

	rval = 0;
	for(i = 0; i < (argc - 1); i++) {
		inkey = argv[i];

		r = yp_match(domainname, inmap, inkey, strlen(inkey),
		    &outbuf, &outbuflen);
		switch (r) {
		case 0:
			if (key)
				printf("%s: ", inkey);
			printf("%*.*s\n", outbuflen, outbuflen, outbuf);
			break;

		case YPERR_YPBIND:
			errx(1, "not running ypbind");

		default:
			warnx("can't match key %s in map %s.  Reason: %s",
			    inkey, inmap, yperr_string(r));
			rval = 1;
			break;
		}
	}

	exit(rval);
}

void
usage()
{

	fprintf(stderr, "usage: %s [-d domain] [-t] [-k] key [key ...] "
	    "mapname\n", __progname);
	fprintf(stderr, "       %s -x\n", __progname);
	exit(1);
}
