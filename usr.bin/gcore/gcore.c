/*	$NetBSD: gcore.c,v 1.10 2007/12/15 19:44:50 perry Exp $	*/

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
__RCSID("$NetBSD: gcore.c,v 1.10 2007/12/15 19:44:50 perry Exp $");

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <sys/ptrace.h>
#include <sys/proc.h>
#include <sys/wait.h>

#include <stdio.h>
#include <string.h>
#include <err.h>
#include <limits.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

static void usage(void) __dead;

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-c <corename>] <pid> [<pid> ...]\n",
	    getprogname());
	exit(1);
}


int
main(int argc, char **argv)
{
	int c;
	char *corename = NULL;
	int corelen = 0;

	while ((c = getopt(argc, argv, "c:")) != -1)
		switch (c) {
		case 'c':
			corename = optarg;
			corelen = strlen(corename);
			break;
		case '?':
		default:
			usage();
			/*NOTREACHED*/
		}

	if (optind == argc)
		usage();

	for (c = optind; c < argc; c++) {
		char *ep;
		u_long lval;
		int status, serrno;
		pid_t pid, cpid;

		errno = 0;
		lval = strtoul(argv[c], &ep, 0);
		if (argv[c] == '\0' || *ep)
			errx(1, "`%s' is not a number.", argv[c]);

		if (errno == ERANGE && lval == ULONG_MAX)
			err(1, "Pid `%s'", argv[c]);

		pid = (pid_t)lval;

		if (ptrace(PT_ATTACH, pid, 0, 0) == -1)
			err(1, "ptrace(PT_ATTACH) to %lu failed", lval);

		if ((cpid = waitpid(pid, &status, 0)) != pid) {
			serrno = errno;
			(void)ptrace(PT_DETACH, pid, (void *)1, 0);
			errno = serrno;
			if (cpid == -1)
				err(1, "Cannot wait for %lu", lval);
			else
				errx(1, "Unexpected process %lu waited",
				    (unsigned long)cpid);
		}

		if (ptrace(PT_DUMPCORE, pid, corename, corelen) == -1) {
			serrno = errno;
			(void)ptrace(PT_DETACH, pid, (void *)1, 0);
			errno = serrno;
			err(1, "ptrace(PT_DUMPCORE) to %lu failed", lval);
		}
		if (ptrace(PT_DETACH, pid, (void *)1, 0) == -1)
			err(1, "ptrace(PT_DETACH) to %lu failed", lval);
	}
	return 0;
}
