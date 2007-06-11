/*
 * Copyright © 2007 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <sys/cdefs.h>

#ifndef lint
__COPYRIGHT("@(#) Copyright © (c) 2007 \
                The NetBSD Foundation, Inc.  All rights reserved.");
__RCSID("$NetBSD: fusermount.c,v 1.2 2007/06/11 21:16:23 agc Exp $");
#endif

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef FUSERMOUNT_VERSION
#define FUSERMOUNT_VERSION	"2.6.0"
#endif

enum {
	FlagCheckPerm = 1,
	FlagKernelCache,
	FlagNonrootUsers,

	ActionMount,
	ActionUnmount
};

/* unmount mount point(s) */
static int
refuse_unmount(int argc, char **argv)
{
	int	ret;
	int	i;

	for (ret = 1, i = 0 ; i < argc ; i++) {
		if (unmount(argv[i], 0) < 0) {
			warn("can't unmount `%s'", argv[i]);
			ret = 0;
		}
	}
	return ret;
}

/* print the usage meessage */
static void
usage(const char *prog)
{
	(void) fprintf(stderr,
		"Usage: %s [-c] [-d name] [-h] [-p] [-u] [-x] mountpoint...\n",
		prog);
	(void) fprintf(stderr, "\t-c\tuse kernel cache\n");
	(void) fprintf(stderr, "\t-d name\tuse name in mount information\n");
	(void) fprintf(stderr, "\t-h\tprint help information\n");
	(void) fprintf(stderr, "\t-p\tcheck file permissions\n");
	(void) fprintf(stderr, "\t-u\tunmount mount point(s)\n");
	(void) fprintf(stderr,
		"\t-x\tallow access to mortal (non-root) users\n");
}

int
main(int argc, char **argv)
{
	char	*progname;
	char	*execme;
	int	 flags;
	int	 action;
	int	 i;

	progname = NULL;
	flags = 0;
	action = ActionMount;
	while ((i = getopt(argc, argv, "Vcd:hpux")) != -1) {
		switch(i) {
		case 'V':
			printf("fusermount version: %s\n", FUSERMOUNT_VERSION);
			exit(EXIT_SUCCESS);
			/* NOTREACHED */
		case 'c':
			flags |= FlagKernelCache;
			break;
		case 'd':
			progname = optarg;
			break;
		case 'h':
			usage(*argv);
			exit(EXIT_SUCCESS);
		case 'p':
			flags |= FlagCheckPerm;
			break;
		case 'u':
			action = ActionUnmount;
			break;
		case 'x':
			if (geteuid() != 0) {
				err(EXIT_FAILURE,
				"-x option is only allowed for use by root");
			}
			flags |= FlagNonrootUsers;
			break;
		default:
			warnx("Unrecognised argument `%c'", i);
			usage(*argv);
			exit(EXIT_FAILURE);
		}
	}
	if (optind >= argc - 2) {
		warnx("Not enough command line arguments");
		usage(*argv);
		exit(EXIT_FAILURE);
	}
	execme = argv[optind + 1];
	if (progname) {
		argv[optind + 1] = progname;
	}
	/* mountpoint = argv[optind]; */
	switch(action) {
	case ActionMount:
		execvp(execme, &argv[optind + 1]);
		break;
	case ActionUnmount:
		if (!refuse_unmount(argc - optind, argv + optind)) {
			exit(EXIT_FAILURE);
		}
		break;
	}
	exit(EXIT_SUCCESS);
}
