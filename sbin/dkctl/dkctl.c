/*	$NetBSD: dkctl.c,v 1.2 2002/07/01 18:49:57 yamt Exp $	*/

/*
 * Copyright 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Jason R. Thorpe for Wasabi Systems, Inc.
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
 *	This product includes software developed for the NetBSD Project by
 *	Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * dkctl(8) -- a program to manipulate disks.
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/dkio.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>     
#include <unistd.h>
#include <util.h>

#define	YES	1
#define	NO	0

/* I don't think nl_langinfo is suitable in this case */
#define	YES_STR	"yes"
#define	NO_STR	"no"
#define YESNO_ARG	YES_STR " | " NO_STR

struct command {
	const char *cmd_name;
	const char *arg_names;
	void (*cmd_func)(int, char *[]);
	int open_flags;
};

int	main(int, char *[]);
void	usage(void);

int	fd;				/* file descriptor for device */
const	char *dvname;			/* device name */
char	dvname_store[MAXPATHLEN];	/* for opendisk(3) */
const	char *cmdname;			/* command user issued */
const	char *argnames;			/* helpstring; expected arguments */

int yesno(const char *);

void	disk_getcache(int, char *[]);
void	disk_setcache(int, char *[]);
void	disk_synccache(int, char *[]);
void	disk_keeplabel(int, char *[]);

struct command commands[] = {
	{ "getcache",
	  "",
	  disk_getcache,
	  O_RDONLY },

	{ "setcache",
	  "none | r | w | rw [save]",
	  disk_setcache,
	  O_RDWR },

	{ "synccache",
	  "[force]",
	  disk_synccache,
	  O_RDWR },

	{ "keeplabel",
	  YESNO_ARG,
	  disk_keeplabel,
	  O_RDWR },

	{ NULL,
	  NULL,
	  NULL,
	  0 },
};

int
main(int argc, char *argv[])
{
	int i;

	/* Must have at least: device command */
	if (argc < 3)
		usage();

	/* Skip program name, get and skip device name and command. */
	dvname = argv[1];
	cmdname = argv[2];
	argv += 3;
	argc -= 3;

	/* Look up and call the command. */
	for (i = 0; commands[i].cmd_name != NULL; i++)
		if (strcmp(cmdname, commands[i].cmd_name) == 0)
			break;
	if (commands[i].cmd_name == NULL)
		errx(1, "unknown command: %s", cmdname);

	argnames = commands[i].arg_names;

	/* Open the device. */
	fd = opendisk(dvname, commands[i].open_flags, dvname_store,
	    sizeof(dvname_store), 0);
	if (fd == -1)
		err(1, "%s", dvname);

	dvname = dvname_store;

	(*commands[i].cmd_func)(argc, argv);
	exit(0);
}

void
usage()
{
	int i;

	fprintf(stderr, "Usage: %s device command [arg [...]]\n",
	    getprogname());

	fprintf(stderr, "   Available commands:\n");
	for (i = 0; commands[i].cmd_name != NULL; i++)
		fprintf(stderr, "\t%s %s\n", commands[i].cmd_name,
		    commands[i].arg_names);

	exit(1);
}

void
disk_getcache(int argc, char *argv[])
{
	int bits;

	if (ioctl(fd, DIOCGCACHE, &bits) == -1)
		err(1, "%s: getcache", dvname);

	if ((bits & (DKCACHE_READ|DKCACHE_WRITE)) == 0)
		printf("%s: No caches enabled\n", dvname);
	else {
		if (bits & DKCACHE_READ)
			printf("%s: read cache enabled\n", dvname);
		if (bits & DKCACHE_WRITE)
			printf("%s: write-back cache enabled\n", dvname);
	}

	printf("%s: read cache enable is %schangeable\n", dvname,
	    (bits & DKCACHE_RCHANGE) ? "" : "not ");
	printf("%s: write cache enable is %schangeable\n", dvname,
	    (bits & DKCACHE_WCHANGE) ? "" : "not ");

	printf("%s: cache parameters are %ssavable\n", dvname,
	    (bits & DKCACHE_SAVE) ? "" : "not ");
}

void
disk_setcache(int argc, char *argv[])
{
	int bits;

	if (argc > 2 || argc == 0)
		usage();

	if (strcmp(argv[0], "none") == 0)
		bits = 0;
	else if (strcmp(argv[0], "r") == 0)
		bits = DKCACHE_READ;
	else if (strcmp(argv[0], "w") == 0)
		bits = DKCACHE_WRITE;
	else if (strcmp(argv[0], "rw") == 0)
		bits = DKCACHE_READ|DKCACHE_WRITE;
	else
		usage();

	if (argc == 2) {
		if (strcmp(argv[1], "save") == 0)
			bits |= DKCACHE_SAVE;
		else
			usage();
	}

	if (ioctl(fd, DIOCSCACHE, &bits) == -1)
		err(1, "%s: setcache", dvname);
}

void
disk_synccache(int argc, char *argv[])
{
	int force;

	switch (argc) {
	case 0:
		force = 0;
		break;

	case 1:
		if (strcmp(argv[0], "force") == 0)
			force = 1;
		else
			usage();
		break;

	default:
		usage();
	}

	if (ioctl(fd, DIOCCACHESYNC, &force) == -1)
		err(1, "%s: sync cache", dvname);
}

void
disk_keeplabel(int argc, char *argv[])
{
	int keep;
	int yn;

	if (argc != 1)
		usage();

	yn = yesno(argv[0]);
	if (yn < 0)
		usage();

	keep = yn == YES;

	if (ioctl(fd, DIOCKLABEL, &keep) == -1)
		err(1, "%s: keep label", dvname);
}

/*
 * return YES, NO or -1.
 */
int
yesno(const char *p)
{

	if (!strcmp(p, YES_STR))
		return YES;
	if (!strcmp(p, NO_STR))
		return NO;
	return -1;
}
