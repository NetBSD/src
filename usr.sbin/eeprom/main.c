/*	$NetBSD: main.c,v 1.21.12.2 2014/08/20 00:05:07 tls Exp $	*/

/*-
 * Copyright (c) 1996 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1996\
 The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: main.c,v 1.21.12.2 2014/08/20 00:05:07 tls Exp $");
#endif

#include <sys/param.h>
#include <err.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(USE_EEPROM) || defined(USE_OPENPROM)
#include <machine/eeprom.h>
#endif

#include "defs.h"
#include "pathnames.h"

#ifdef USE_OPENPROM
# ifndef USE_EEPROM
#  define ee_action(a,b)
#  define ee_dump()
#  define ee_updatechecksums() (void)0
#  define check_for_openprom() 1
# endif
#endif

static	void action(char *);
static	void dump_prom(void);
static	void usage(void) __dead;

const char *path_eeprom = _PATH_EEPROM;
const char *path_openprom = _PATH_OPENPROM;
const char *path_openfirm = _PATH_OPENFIRM;
const char *path_prepnvram = _PATH_PREPNVRAM;
int	fix_checksum = 0;
int	ignore_checksum = 0;
int	update_checksums = 0;
int	cksumfail = 0;
u_short	writecount;
int	eval = 0;
#ifdef USE_OPENPROM
int	verbose = 0;
int	use_openprom;
#endif
#if defined(USE_OPENFIRM) || defined (USE_PREPNVRAM)
int	verbose=0;
#endif

int
main(int argc, char *argv[])
{
	int ch, do_stdin = 0;
	char *cp, line[BUFSIZE];
#if defined(USE_OPENPROM) || defined(USE_OPENFIRM) || defined(USE_PREPNVRAM)
	const char *optstring = "-cf:iv";
#else
	const char *optstring = "-cf:i";
#endif /* USE_OPENPROM */

	while ((ch = getopt(argc, argv, optstring)) != -1)
		switch (ch) {
		case '-':
			do_stdin = 1;
			break;

		case 'c':
			fix_checksum = 1;
			break;

		case 'f':
			path_eeprom = path_openprom = optarg;
			break;

		case 'i':
			ignore_checksum = 1;
			break;

#if defined(USE_OPENPROM) || defined(USE_OPENFIRM) || defined(USE_PREPNVRAM)
		case 'v':
			verbose = 1;
			break;
#endif /* USE_OPENPROM */

		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

#ifdef USE_OPENPROM
	use_openprom = check_for_openprom();

	if (use_openprom == 0) {
#endif /* USE_OPENPROM */
#if !defined(USE_OPENFIRM) && !defined(USE_PREPNVRAM)
		ee_verifychecksums();
		if (fix_checksum || cksumfail)
			exit(cksumfail);
#endif
#ifdef USE_OPENPROM
	}
#endif /* USE_OPENPROM */

	if (do_stdin) {
		while (fgets(line, BUFSIZE, stdin) != NULL) {
			if (line[0] == '\n')
				continue;
			if ((cp = strrchr(line, '\n')) != NULL)
				*cp = '\0';
			action(line);
		}
		if (ferror(stdin))
			err(++eval, "stdin");
	} else {
		if (argc == 0) {
			dump_prom();
			exit(eval + cksumfail);
		}

		while (argc) {
			action(*argv);
			++argv;
			--argc;
		}
	}

#ifdef USE_OPENPROM
	if (use_openprom == 0)
#endif /* USE_OPENPROM */
#if !defined(USE_OPENFIRM) && !defined(USE_PREPNVRAM)
		if (update_checksums) {
			++writecount;
			ee_updatechecksums();
		}

	exit(eval + cksumfail);
#endif
	return 0;
}

/*
 * Separate the keyword from the argument (if any), find the keyword in
 * the table, and call the corresponding handler function.
 */
static void
action(char *line)
{
	char *keyword, *arg;

	keyword = strdup(line);
	if ((arg = strrchr(keyword, '=')) != NULL)
		*arg++ = '\0';

#ifdef USE_PREPNVRAM
	prep_action(keyword, arg);
#else
#ifdef USE_OPENFIRM
	of_action(keyword, arg);
#else
#ifdef USE_OPENPROM
	if (use_openprom)
		op_action(keyword, arg);
	else {
#endif /* USE_OPENPROM */
		ee_action(keyword, arg);
#ifdef USE_OPENPROM
	}
#endif /* USE_OPENPROM */
#endif /* USE_OPENFIRM */
#endif /* USE_PREPNVRAM */
}

/*
 * Dump the contents of the prom corresponding to all known keywords.
 */
static void
dump_prom(void)
{

#ifdef USE_PREPNVRAM
	prep_dump();
#else
#ifdef USE_OPENFIRM
	of_dump();
#else
#ifdef USE_OPENPROM
	if (use_openprom)
		/*
		 * We have a special dump routine for this.
		 */
		op_dump();
	else {
#endif /* USE_OPENPROM */
		ee_dump();
#ifdef USE_OPENPROM
	}
#endif /* USE_OPENPROM */
#endif /* USE_OPENFIRM */
#endif /* USE_PREPNVRAM */
}

__dead static void
usage(void)
{

#if defined(USE_OPENPROM) || defined(USE_OPENFIRM) || defined(USE_PREPNVRAM)
	fprintf(stderr, "usage: %s %s\n", getprogname(),
	    "[-] [-c] [-f device] [-i] [-v] [field[=value] ...]");
#else
	fprintf(stderr, "usage: %s %s\n", getprogname(),
	    "[-] [-c] [-f device] [-i] [field[=value] ...]");
#endif /* __us */
	exit(1);
}
