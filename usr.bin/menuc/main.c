/*	$NetBSD: main.c,v 1.10.50.1 2012/04/17 00:09:36 yamt Exp $	*/

/*
 * Copyright 1997 Piermont Information Systems Inc.
 * All rights reserved.
 *
 * Written by Philip A. Nelson for Piermont Information Systems Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of Piermont Information Systems Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY PIERMONT INFORMATION SYSTEMS INC. ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL PIERMONT INFORMATION SYSTEMS INC. BE 
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF 
 * THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

/* main.c - main program for menu compiler. */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>

#if defined(__RCSID) && !defined(lint)
__RCSID("$NetBSD: main.c,v 1.10.50.1 2012/04/17 00:09:36 yamt Exp $");
#endif


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define MAIN
#include "defs.h"

/* Local prototypes */
__dead void usage(void);

int
main(int argc, char **argv)
{
	int ch;

	/* Process the arguments. */
	while ((ch = getopt(argc, argv, "o:")) != -1 ) {
		switch (ch) {
		case 'o': /* output file name */
			out_name = optarg;
			break;
		default:
			usage();
		}
	}

	if (optind != argc - 1)
		usage();

	src_name = argv[optind];

	yyin = fopen(src_name, "r");
	if (yyin == NULL) {
		(void)fprintf(stderr, "%s: could not open %s.\n", prog_name,
		    src_name);
		exit(1);
	}

	/* The default menu */
	default_menu.info = &default_info;
	default_info.title = "\"\"";
	default_info.helpstr = NULL;
	default_info.exitstr = NULL;
	default_info.mopt = 0;
	default_info.x = -1;
	default_info.y = 0;
	default_info.h = 0;
	default_info.w = 0;
	default_info.numopt = 0;
	default_info.optns = NULL;
	default_info.postact.code = NULL;
	default_info.postact.endwin = FALSE;
	default_info.exitact.code = NULL;
	default_info.exitact.endwin = FALSE;

	/* Do the parse */
	(void)yyparse();

	return had_errors ? 1 : 0;
}


__dead void
usage(void)
{
	(void)fprintf (stderr, "%s [-o name] file\n", getprogname());
	exit(1);
}
