/* $NetBSD: ypcat.c,v 1.16.6.1 2012/04/17 00:09:44 yamt Exp $	*/

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
__RCSID("$NetBSD: ypcat.c,v 1.16.6.1 2012/04/17 00:09:44 yamt Exp $");
#endif

#include <sys/param.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
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

static int	printit(int, char *, int, char *, int, char *);
static void	usage(void) __attribute__((__noreturn__));

static int	compressspace;


int
main(int argc, char *argv[])
{
	char *domainname, *b_retry_cnt;
	struct ypall_callback ypcb;
	const char *inmap;
	int notrans;
	int c, r;
	size_t i;
	const struct ypalias *ypaliases;
	int key;

	setprogname(*argv);
	domainname = b_retry_cnt = NULL;
	notrans = key = 0;
	ypaliases = ypalias_init();
	while((c = getopt(argc, argv, "bd:kstx")) != -1) {
		switch (c) {
		case 'b':
			b_retry_cnt = optarg;
			break;

		case 'd':
			domainname = optarg;
			break;

		case 'k':
			key++;
			break;

		case 's':
			compressspace++;
			break;

		case 'x':
			for (i = 0; ypaliases[i].alias; i++)
				printf("Use \"%s\" for \"%s\"\n",
				    ypaliases[i].alias,
				    ypaliases[i].name);
			return 0;

		case 't':
			notrans++;
			break;

		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc != 1)
		usage();

	if (b_retry_cnt != NULL) {
		char *s;
		unsigned long l;

		l = strtoul(b_retry_cnt, &s, 10);
		if (*s != '\0' || l > 0xffff) usage();
		yp_setbindtries((int)l);
	}

	if (domainname == NULL)
		yp_get_default_domain(&domainname);

	inmap = argv[0];
	if (notrans == 0) {
		for (i = 0; ypaliases[i].alias; i++)
			if (strcmp(inmap, ypaliases[i].alias) == 0)
				inmap = ypaliases[i].name;
	}

	ypcb.foreach = printit;
	ypcb.data = key ? (void *)&key : NULL;

	r = yp_all(domainname, inmap, &ypcb);
	switch (r) {
	case 0:
		break;

	case YPERR_YPBIND:
		errx(1, "not running ypbind");

	default:
		errx(1, "no such map %s.  Reason: %s", inmap, yperr_string(r));
	}
	return 0;
}

static int
printit(int instatus, char *inkey, int inkeylen, char *inval,
    int invallen, char *indata)
{

	if (instatus != YP_TRUE)
		return instatus;
	if (indata)
		(void)printf("%*.*s", inkeylen, inkeylen, inkey);
	if (invallen) {
		if (indata)
			(void)putc(' ', stdout);
		if (compressspace) {
			int i;
			int hadspace = 0;

			for (i = 0; i < invallen; i++) {
				if (isspace((unsigned char)inval[i])) {
					if (hadspace)
						continue;
					hadspace = 1;
					(void)putc(' ', stdout);
				} else {
					hadspace = 0;
					(void)putc(inval[i], stdout);
				}
			}
		} else
			(void)printf("%*.*s", invallen, invallen, inval);
	}
	(void)putc('\n', stdout);
	return 0;
}

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-kst] [-b <num-retry> "
	    "[-d <domainname>] <mapname>\n", getprogname());
	(void)fprintf(stderr, "       %s -x\n", getprogname());
	exit(1);
}
