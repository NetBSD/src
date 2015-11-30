/*	$NetBSD: main.c,v 1.1 2015/11/30 19:59:34 christos Exp $	*/

/*-
 * Copyright (c) 2002 Marcel Moolenaar
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
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
 *
 * CRC32 code derived from work by Gary S. Brown.
 */

#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include <sys/cdefs.h>
#ifdef __RCSID
__RCSID("$NetBSD: main.c,v 1.1 2015/11/30 19:59:34 christos Exp $");
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

#include "map.h"
#include "gpt.h"

static struct {
	int (*fptr)(int, char *[]);
	const char *name;
} cmdsw[] = {
	{ cmd_add, "add" },
#ifndef HAVE_NBTOOL_CONFIG_H
	{ cmd_backup, "backup" },
#endif
	{ cmd_biosboot, "biosboot" },
	{ cmd_create, "create" },
	{ cmd_destroy, "destroy" },
	{ cmd_header, "header" },
	{ NULL, "help" },
	{ cmd_label, "label" },
	{ cmd_migrate, "migrate" },
	{ cmd_recover, "recover" },
	{ cmd_remove, "remove" },
	{ NULL, "rename" },
	{ cmd_resize, "resize" },
	{ cmd_resizedisk, "resizedisk" },
#ifndef HAVE_NBTOOL_CONFIG_H
	{ cmd_restore, "restore" },
#endif
	{ cmd_set, "set" },
	{ cmd_show, "show" },
	{ cmd_type, "type" },
	{ cmd_unset, "unset" },
	{ NULL, "verify" },
	{ NULL, NULL }
};

__dead static void
usage(void)
{
	extern const char addmsg1[], addmsg2[], biosbootmsg[];
	extern const char createmsg[], destroymsg[], headermsg[], labelmsg1[];
	extern const char labelmsg2[], labelmsg3[], migratemsg[], recovermsg[];
	extern const char removemsg1[], removemsg2[], resizemsg[];
	extern const char resizediskmsg[], setmsg[], showmsg[], typemsg1[];
	extern const char typemsg2[], typemsg3[], unsetmsg[];
#ifndef HAVE_NBTOOL_CONFIG_H
	extern const char backupmsg[], restoremsg[];
#endif
	const char *p = getprogname();
	const char *f =
	    "[-nrqv] [-m <mediasize>] [-p <partitionnum>] [-s <sectorsize>]";

	fprintf(stderr,
	    "Usage: %s %s <command> [<args>]\n", p, f);
	fprintf(stderr, 
	    "Commands:\n"
#ifndef HAVE_NBTOOL_CONFIG_H
	    "       %s\n"
	    "       %s\n"
#endif
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n"
	    "       %s\n",
	    addmsg1, addmsg2,
#ifndef HAVE_NBTOOL_CONFIG_H
	    backupmsg,
#endif
	    biosbootmsg, createmsg, destroymsg,
	    headermsg, labelmsg1, labelmsg2, labelmsg3,
	    migratemsg, recovermsg,
	    removemsg1, removemsg2,
	    resizemsg, resizediskmsg,
#ifndef HAVE_NBTOOL_CONFIG_H
	    restoremsg,
#endif
	    setmsg, showmsg,
	    typemsg1, typemsg2, typemsg3,
	    unsetmsg);
	exit(1);
}

static void
prefix(const char *cmd)
{
	char *pfx;

	if (asprintf(&pfx, "%s %s", getprogname(), cmd) < 0)
		pfx = NULL;
	else
		setprogname(pfx);
}

int
main(int argc, char *argv[])
{
	char *cmd, *p;
	int ch, i;

	/* Get the generic options */
	while ((ch = getopt(argc, argv, "m:np:qrs:v")) != -1) {
		switch(ch) {
		case 'm':
			if (mediasz > 0)
				usage();
			mediasz = strtoul(optarg, &p, 10);
			if (*p != 0 || mediasz < 1)
				usage();
			break;
		case 'n':
			nosync = 1;
			break;
		case 'p':
			if (parts > 0)
				usage();
			parts = strtoul(optarg, &p, 10);
			if (*p != 0 || parts < 1)
				usage();
			break;
		case 'r':
			readonly = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 's':
			if (secsz > 0)
				usage();
			secsz = strtoul(optarg, &p, 10);
			if (*p != 0 || secsz < 1)
				usage();
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
		}
	}
	if (!parts)
		parts = 128;

	if (argc == optind)
		usage();

	cmd = argv[optind++];
	for (i = 0; cmdsw[i].name != NULL && strcmp(cmd, cmdsw[i].name); i++);

	if (cmdsw[i].fptr == NULL)
		errx(1, "unknown command: %s", cmd);

	prefix(cmd);
	return ((*cmdsw[i].fptr)(argc, argv));
}
