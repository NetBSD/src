/*	$NetBSD: modstat.c,v 1.16 2000/12/10 11:52:09 jdolecek Exp $	*/

/*
 * Copyright (c) 1993 Terrence R. Lambert.
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
 *      This product includes software developed by Terrence R. Lambert.
 * 4. The name Terrence R. Lambert may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TERRENCE R. LAMBERT ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE TERRENCE R. LAMBERT BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: modstat.c,v 1.16 2000/12/10 11:52:09 jdolecek Exp $");
#endif

#include <sys/param.h>
#include <sys/conf.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/lkm.h>
#include <sys/mount.h>

#include <a.out.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pathnames.h"

void	cleanup __P((void));
int	dostat __P((int, int, char *));
int	main __P((int, char **));
void	usage __P((void));

void
usage()
{

	fprintf(stderr, "usage:\n");
	fprintf(stderr, "modstat [-i <module id>] [-n <module name>]\n");
	exit(1);
}

static const char *type_names[] = {
	"SYSCALL",
	"VFS",
	"DEV",
	"STRMOD",
	"EXEC",
	"COMPAT",
	"MISC"
};
static int tn_nentries = sizeof(type_names) / sizeof(char *);

#define POINTERSIZE ((int)(2 * sizeof(void*)))

int
dostat(devfd, modnum, modname)
	int devfd;
	int modnum;
	char *modname;
{
	struct lmc_stat	sbuf;

	if (modname != NULL)
		strncpy(sbuf.name, modname, sizeof sbuf.name);
	sbuf.name[sizeof(sbuf.name) - 1] = '\0';

	sbuf.id = modnum;

	if (ioctl(devfd, LMSTAT, &sbuf) == -1) {
		switch (errno) {
		case EINVAL:		/* out of range */
			return 2;
		case ENOENT:		/* no such entry */
			return 1;
		default:		/* other error (EFAULT, etc) */
			warn("LMSTAT");
			return 4;
		}
	}

	/*
	 * Decode this stat buffer...
	 */
	printf("%-7s %3d %3ld %0*lx %04lx %0*lx %3ld %s\n",
	    (sbuf.type < tn_nentries) ? type_names[sbuf.type] : "(UNKNOWN)", 
	    sbuf.id,		/* module id */
	    (long)sbuf.offset,	/* offset into modtype struct */
	    POINTERSIZE,
	    (long)sbuf.area,	/* address module loaded at */
	    (long)sbuf.size,	/* size in pages(K) */
	    POINTERSIZE,
	    (long)sbuf.private,	/* kernel address of private area */
	    (long)sbuf.ver,	/* Version; always 1 for now */
	    sbuf.name		/* name from private area */
	);

	/*
	 * Done (success).
	 */
	return 0;
}

int devfd;

void
cleanup()
{

	close(devfd);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c;
	int modnum = -1;
	char *modname = NULL;
	gid_t egid = getegid();

	setegid(getgid());
	while ((c = getopt(argc, argv, "i:n:")) != -1) {
		switch (c) {
		case 'i':
			modnum = atoi(optarg);
			break;	/* number */
		case 'n':
			modname = optarg;
			break;	/* name */
		case '?':
			usage();
		default:
			printf("default!\n");
			break;
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 0)
		usage();

	/*
	 * Open the virtual device device driver for exclusive use (needed
	 * to ioctl() to retrive the loaded module(s) status).
	 */
	(void)setegid(egid);
	if ((devfd = open(_PATH_LKM, O_RDONLY, 0)) == -1)
		err(2, "%s", _PATH_LKM);

	/* get rid of our privileges now */
	setgid(getgid());

	atexit(cleanup);

	printf("Type    Id  Off %-*s Size %-*s Rev Module Name\n", 
	    POINTERSIZE, "Loadaddr", 
	    POINTERSIZE, "Info");

	/*
	 * Oneshot?
	 */
	if (modnum != -1 || modname != NULL) {
		if (dostat(devfd, modnum, modname))
			exit(3);
		exit(0);
	}

	/*
	 * Start at 0 and work up until "EINVAL".
	 */
 	for (modnum = 0; dostat(devfd, modnum, NULL) < 2; modnum++)
 		;

	exit(0);
}
