/*	$NetBSD: ypmatch.c,v 1.19 2009/06/21 14:59:53 wiz Exp $	*/

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
__RCSID("$NetBSD: ypmatch.c,v 1.19 2009/06/21 14:59:53 wiz Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ctype.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <rpc/rpc.h>
#include <rpc/xdr.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

#include "ypalias_init.h"

static void	usage(void) __attribute__((__noreturn__));

int
main(int argc, char *argv[])
{
	char *domainname;
	char *inkey, *outbuf;
	const char *inmap;
	int outbuflen, key, null, notrans;
	int c, r, len;
	size_t i;
	int rval;
	const struct ypalias *ypaliases;

	setprogname(*argv);
	domainname = NULL;
	notrans = key = null = 0;
	ypaliases = ypalias_init();
	while ((c = getopt(argc, argv, "xd:ktz")) != -1) {
		switch (c) {
		case 'x':
			for(i = 0; ypaliases[i].alias; i++)
				printf("Use \"%s\" for \"%s\"\n",
					ypaliases[i].alias,
					ypaliases[i].name);
			return 0;

		case 'd':
			domainname = optarg;
			break;

		case 't':
			notrans++;
			break;

		case 'k':
			key++;
			break;

		case 'z':
			null++;
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
		for (i = 0; ypaliases[i].alias; i++)
			if (strcmp(inmap, ypaliases[i].alias) == 0)
				inmap = ypaliases[i].name;
	}

	rval = 0;
	for(c = 0; c < (argc - 1); c++) {
		inkey = argv[c];

		len = strlen(inkey);
		if (null)
			len++;
		r = yp_match(domainname, inmap, inkey, len,
		    &outbuf, &outbuflen);
		switch (r) {
		case 0:
			if (key)
				printf("%s: ", inkey);
			fwrite(outbuf, outbuflen, 1, stdout);
			putc('\n', stdout);
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

	return rval;
}

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-ktz] [-d domain] key ... "
	    "mapname\n", getprogname());
	(void)fprintf(stderr, "       %s -x\n", getprogname());
	exit(1);
}
