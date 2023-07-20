/*	$NetBSD: main2.c,v 1.34 2023/07/13 08:40:38 rillig Exp $	*/

/*
 * Copyright (c) 1994, 1995 Jochen Pohl
 * All Rights Reserved.
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
 *	This product includes software developed by Jochen Pohl for
 *	The NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#if defined(__RCSID)
__RCSID("$NetBSD: main2.c,v 1.34 2023/07/13 08:40:38 rillig Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "lint2.h"

bool	Cflag;
bool	Fflag;
bool	Hflag;
bool	hflag;
bool	sflag;
bool	tflag;
bool	uflag;
bool	xflag;
const char *libname;

/*
 * List of libraries (from -l flag). These libraries are read after all
 * other input files has been read and, for Cflag, after the new lint library
 * has been written.
 */
static const char **libs;

static	void	usage(void) __attribute__((noreturn));

static void
check_name_non_const(hte_t *hte)
{
	check_name(hte);
}

int
main(int argc, char *argv[])
{
	int c, i;
	size_t len;
	char *lname;

	libs = xcalloc(1, sizeof(*libs));

	opterr = 0;
	while ((c = getopt(argc, argv, "hl:stuxC:HF")) != -1) {
		switch (c) {
		case 's':
			sflag = true;
			break;
		case 't':
			tflag = true;
			break;
		case 'u':
			uflag = true;
			break;
		case 'x':
			xflag = true;
			break;
		case 'C':
			len = strlen(optarg);
			lname = xmalloc(len + 10);
			(void)snprintf(lname, len + 10, "llib-l%s.ln", optarg);
			libname = lname;
			Cflag = true;
			uflag = true;
			break;
		case 'H':
			Hflag = true;
			break;
		case 'h':
			hflag = true;
			break;
		case 'F':
			Fflag = true;
			break;
		case 'l':
			for (i = 0; libs[i] != NULL; i++)
				continue;
			libs = xrealloc(libs, (i + 2) * sizeof(*libs));
			libs[i] = xstrdup(optarg);
			libs[i + 1] = NULL;
			break;
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc == 0)
		usage();

	symtab_init();

	for (i = 0; i < argc; i++)
		readfile(argv[i]);

	/* write the lint library */
	if (Cflag) {
		symtab_forall(mkstatic);
		outlib(libname);
	}

	/* read additional libraries */
	for (i = 0; libs[i] != NULL; i++)
		readfile(libs[i]);

	symtab_forall(mkstatic);

	mark_main_as_used();

	/* perform all tests */
	symtab_forall_sorted(check_name_non_const);

	return 0;
}

static void
usage(void)
{
	(void)fprintf(stderr,
	    "usage: %s [-hstuxHF] -Clib -l lib ... src1 ...\n", getprogname());
	exit(1);
}
