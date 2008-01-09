/*	$NetBSD: modstat.c,v 1.22.12.1 2008/01/09 02:00:49 matt Exp $	*/

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
__RCSID("$NetBSD: modstat.c,v 1.22.12.1 2008/01/09 02:00:49 matt Exp $");
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

static void	cleanup(void);
static int	dostat(int, const char *);
static void	usage(void) __dead;


static const char *type_names[] = {
	MODTYPE_NAMES
};

static size_t tn_nentries = sizeof(type_names) / sizeof(type_names[0]);
static int devfd;

#define POINTERSIZE ((int)(2 * sizeof(void*)))

static void
usage(void)
{

	(void)fprintf(stderr, "Usage: %s [-i <module id>] [-n <module name>]\n",
		getprogname());
	exit(1);
}

static int
dostat(int modnum, const char *modname)
{
	struct lmc_stat	sbuf;
	long offset;
	char offset_string[32];

	if (modname != NULL)
		(void)strlcpy(sbuf.name, modname, sizeof sbuf.name);

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
	offset = (long)sbuf.offset;
	if (sbuf.type == LM_DEV)
		(void) snprintf(offset_string, sizeof(offset_string), 
		    "%3d/%-3d", 
		    LKM_BLOCK_MAJOR(offset), 
		    LKM_CHAR_MAJOR(offset));
	else if (offset < 0)
		(void) strlcpy(offset_string, " -", sizeof (offset_string));
	else
		(void) snprintf(offset_string, sizeof (offset_string), " %3ld",
		    offset);

	(void)printf("%-7s %3d %7s %0*lx %04lx %0*lx %3ld %s\n",
	    (sbuf.type < tn_nentries) ? type_names[sbuf.type] : "(UNKNOWN)", 
	    sbuf.id,		/* module id */
	    offset_string,	/* offset into modtype struct */
	    POINTERSIZE,
	    (long)sbuf.area,	/* address module loaded at */
	    (long)sbuf.size,	/* size in KB */
	    POINTERSIZE,
	    (long)sbuf.private,	/* kernel address of private area */
	    (long)sbuf.ver,	/* Version of module interface */
	    sbuf.name		/* name from private area */
	);

	/*
	 * Done (success).
	 */
	return 0;
}

static void
cleanup(void)
{

	(void)close(devfd);
}

int
main(int argc, char *argv[])
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
		default:
			usage();
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
	(void)setgid(getgid());

	(void)atexit(cleanup);

	(void)printf("Type    Id   Offset %-*s Size %-*s Rev Module Name\n", 
	    POINTERSIZE, "Loadaddr", 
	    POINTERSIZE, "Info");

	/*
	 * Oneshot?
	 */
	if (modnum != -1 || modname != NULL) {
		if (dostat(modnum, modname))
			return(3);
		return(0);
	}

	/*
	 * Start at 0 and work up until "EINVAL".
	 */
 	for (modnum = 0; dostat(modnum, NULL) < 2; modnum++)
 		continue;

	return(0);
}
