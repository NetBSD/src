/*	$NetBSD: gcore.c,v 1.5 2003/01/23 18:18:49 jdolecek Exp $	*/

/*-
 * Copyright (c) 2003 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Christos Zoulas.
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
 *        This product includes software developed by the NetBSD
 *        Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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
__RCSID("$NetBSD: gcore.c,v 1.5 2003/01/23 18:18:49 jdolecek Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ptrace.h>
#include <sys/proc.h>

#include <stdio.h>
#include <err.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

static void usage(void) __attribute__((__noreturn__));

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s <pid> [<pid> ...]\n", getprogname());
	exit(1);
}


int
main(int argc, char **argv)
{
	int c;

	while ((c = getopt(argc, argv, "o:")) != -1)
		switch (c) {
		case 'o':
			errx(1, "-o is not supported yet.");
			/*NOTREACHED*/
		case '?':
		default:
			usage();
			/*NOTREACHED*/
		}

	if (optind == argc)
		usage();

	for (c = optind; c < argc; c++) {
		char *ep;
		long lval = strtol(argv[c], &ep, 0);

		if (argv[c] == '\0' || *ep)
			errx(1, "`%s' is not a number.", argv[c]);
		if (errno == ERANGE && (lval == LONG_MAX || lval == LONG_MIN))
			err(1, "Pid `%s'", argv[c]);
		if (ptrace(PT_DUMPCORE, (pid_t)lval, NULL, 0) == -1)
			err(1, "ptrace(PT_DUMPCORE) failed");
	}
	return 0;
}
