/*	$NetBSD: eject.c,v 1.26 2010/06/23 18:07:59 yamt Exp $	*/

/*-
 * Copyright (c) 1999 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Chris Jones.
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
__COPYRIGHT("@(#) Copyright (c) 1999\
 The NetBSD Foundation, Inc.  All rights reserved.");
#endif				/* not lint */

#ifndef lint
__RCSID("$NetBSD: eject.c,v 1.26 2010/06/23 18:07:59 yamt Exp $");
#endif				/* not lint */

#include <sys/types.h>
#include <sys/cdio.h>
#include <sys/disklabel.h>
#include <sys/ioctl.h>
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/mtio.h>

#include <ctype.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

#ifdef AMD_SUPPORT
# include "am_glue.h"
#endif

struct nicknames_s {
	const char *name;	/* The name given on the command line. */
	const char *devname;	/* The base name of the device */
	int type;		/* The type of device, for determining what
				 * ioctl to use. */
# define TAPE 0x10
# define DISK 0x20
	/* OR one of the above with one of the below: */
# define NOTLOADABLE 0x00
# define LOADABLE 0x01
# define FLOPPY 0x2
# define TYPEMASK ((int)~0x01)
} nicknames[] = {
	{ "diskette", "fd",  DISK | FLOPPY | NOTLOADABLE },
	{ "floppy",   "fd",  DISK | FLOPPY | NOTLOADABLE },
	{ "fd",       "fd",  DISK | FLOPPY | NOTLOADABLE },
	{ "sd",       "sd",  DISK | NOTLOADABLE },
	{ "cdrom",    "cd",  DISK | LOADABLE },
	{ "cd",       "cd",  DISK | LOADABLE },
	{ "cdr",      "cd",  DISK | LOADABLE },
	{ "cdrw",     "cd",  DISK | LOADABLE },
	{ "dvdrom",   "cd",  DISK | LOADABLE },
	{ "dvd",      "cd",  DISK | LOADABLE },
	{ "dvdr",     "cd",  DISK | LOADABLE },
	{ "dvdrw",    "cd",  DISK | LOADABLE },
	{ "mcd",      "mcd", DISK | LOADABLE },	/* XXX Is this true? */
	{ "tape",     "st",  TAPE | NOTLOADABLE },
	{ "st",       "st",  TAPE | NOTLOADABLE },
	{ "dat",      "st",  TAPE | NOTLOADABLE },
	{ "exabyte",  "st",  TAPE | NOTLOADABLE }
};
#define MAXNICKLEN 12		/* at least enough room for the longest
				 * nickname */
#define MAXDEVLEN (MAXNICKLEN + 7)	/* "/dev/r" ... "a" */

struct devtypes_s {
	const char *name;
	int type;
} devtypes[] = {
	{ "diskette", DISK | NOTLOADABLE },
	{ "floppy",   DISK | NOTLOADABLE },
	{ "cdrom",    DISK | LOADABLE },
	{ "disk",     DISK | NOTLOADABLE },
	{ "tape",     TAPE | NOTLOADABLE }
};

enum eject_op {
	OP_EJECT, OP_LOAD, OP_LOCK, OP_UNLOCK
};

int verbose_f = 0;
int umount_f = 1;

int main(int, char *[]);
__dead static void usage(void);
static char *nick2dev(const char *);
static char *nick2rdev(const char *);
static int guess_devtype(const char *);
static void eject_disk(const char *, enum eject_op);
static void eject_tape(const char *, enum eject_op);
static void unmount_dev(const char *);

int
main(int argc, char *argv[])
{
	int ch;
	int devtype;
	size_t n;
	char *dev_name;		/* XXX - devname is declared in stdlib.h */
	enum eject_op op;

	devtype = -1;
	dev_name = NULL;
	op = OP_EJECT;

	while ((ch = getopt(argc, argv, "d:flLnt:Uv")) != -1) {
		switch (ch) {
		case 'd':
			dev_name = optarg;
			break;
		case 'f':
			umount_f = 0;
			break;
		case 'l':
			if (op != OP_EJECT)
				usage();
			op = OP_LOAD;
			break;
		case 'L':
			if (op != OP_EJECT)
				usage();
			op = OP_LOCK;
			break;
		case 'n':
			for (n = 0; n < __arraycount(nicknames); n++) {
				struct nicknames_s *np = &nicknames[n];

				(void)printf("%s -> %s\n",
				    np->name, nick2dev(np->name));
			}
			return 0;
		case 't':
			for (n = 0; n < __arraycount(devtypes); n++) {
				if (strcasecmp(devtypes[n].name, optarg)
				    == 0) {
					devtype = devtypes[n].type;
					break;
				}
			}
			if (devtype == -1)
				errx(1, "%s: unknown device type", optarg);
			break;
		case 'U':
			if (op != OP_EJECT)
				usage();
			op = OP_UNLOCK;
			break;
		case 'v':
			verbose_f = 1;
			break;
		default:
			usage();
			/* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (dev_name == NULL) {
		if (argc == 0)
			usage();
		else
			dev_name = argv[0];
	}
	if (devtype == -1)
		devtype = guess_devtype(dev_name);
	if (devtype == -1)
		errx(1, "%s: unable to determine type of device", dev_name);
	if (verbose_f) {
		(void)printf("device type == ");
		if ((devtype & TYPEMASK) == TAPE)
			(void)printf("tape\n");
		else
			(void)printf("disk, floppy, or cdrom\n");
	}
	if (umount_f)
		unmount_dev(dev_name);

	/* XXX Tapes and disks have different ioctl's: */
	if ((devtype & TYPEMASK) == TAPE)
		eject_tape(dev_name, op);
	else
		eject_disk(dev_name, op);

	if (verbose_f)
		(void)printf("done.\n");

	return 0;
}

__dead
static void
usage(void)
{

	(void)fprintf(stderr, "usage: eject [-fv] [-l | -L | -U] "
	    "[-t device-type] [-d] device\n");
	(void)fprintf(stderr, "       eject -n\n");
	exit(1);
}

static int
guess_devtype(const char *dev_name)
{
	size_t n;

	/* Nickname match: */
	for (n = 0; n < __arraycount(nicknames); n++) {
		if (strncasecmp(nicknames[n].name, dev_name,
			strlen(nicknames[n].name)) == 0)
			return nicknames[n].type;
	}

	/*
         * If we still don't know it, then try to compare vs. dev and
         * rdev names that we know.
         */
	/* dev first: */
	for (n = 0; n < __arraycount(nicknames); n++) {
		char *name;
		name = nick2dev(nicknames[n].name);
		/*
		 * Assume that the part of the name that distinguishes
		 * the instance of this device begins with a 0.
		 */
		*(strchr(name, '0')) = '\0';
		if (strncmp(name, dev_name, strlen(name)) == 0)
			return nicknames[n].type;
	}

	/* Now rdev: */
	for (n = 0; n < __arraycount(nicknames); n++) {
		char *name = nick2rdev(nicknames[n].name);
		*(strchr(name, '0')) = '\0';
		if (strncmp(name, dev_name, strlen(name)) == 0)
			return nicknames[n].type;
	}

	/* Not found. */
	return -1;
}

/* "floppy5" -> "/dev/fd5a".  Yep, this uses a static buffer. */
static char *
nick2dev(const char *nn)
{
	static char dev_name[MAXDEVLEN];
	size_t n;
	int devnum;

	devnum = 0;
	for (n = 0; n < __arraycount(nicknames); n++) {
		if (strncasecmp(nicknames[n].name, nn,
			strlen(nicknames[n].name)) == 0) {
			(void)sscanf(nn, "%*[^0-9]%d", &devnum);
			(void)sprintf(dev_name, "/dev/%s%d",
			    nicknames[n].devname, devnum);
			if ((nicknames[n].type & TYPEMASK) != TAPE)
				(void)strcat(dev_name, "a");
			return dev_name;
		}
	}
	return NULL;
}

/* "floppy5" -> "/dev/rfd5c".  Static buffer. */
static char *
nick2rdev(const char *nn)
{
	static char dev_name[MAXDEVLEN];
	size_t n;
	int devnum;

	devnum = 0;
	for (n = 0; n < __arraycount(nicknames); n++) {
		if (strncasecmp(nicknames[n].name, nn,
			strlen(nicknames[n].name)) == 0) {
			(void)sscanf(nn, "%*[^0-9]%d", &devnum);
			(void)sprintf(dev_name, "/dev/r%s%d",
			    nicknames[n].devname, devnum);
			if ((nicknames[n].type & TYPEMASK) != TAPE) {
				(void)strcat(dev_name, "a");
				if ((nicknames[n].type & FLOPPY) != FLOPPY)
					dev_name[strlen(dev_name) - 1]
					    += getrawpartition();
			}
			return dev_name;
		}
	}
	return NULL;
}

/* Unmount all filesystems attached to dev. */
static void
unmount_dev(const char *name)
{
	struct statvfs *mounts;
	size_t len;
	int i, nmnts;
	const char *dn;

	nmnts = getmntinfo(&mounts, MNT_NOWAIT);
	if (nmnts == 0)
		err(1, "getmntinfo");

	/* Make sure we have a device name: */
	dn = nick2dev(name);
	if (dn == NULL)
		dn = name;

	/* Set len to strip off the partition name: */
	len = strlen(dn);
	if (!isdigit((unsigned char)dn[len - 1]))
		len--;
	if (!isdigit((unsigned char)dn[len - 1]))
		errx(1, "Can't figure out base name for dev name %s", dn);

	for (i = 0; i < nmnts; i++) {
		if (strncmp(mounts[i].f_mntfromname, dn, len) == 0) {
			if (verbose_f)
				(void)printf("Unmounting %s from %s...\n",
				    mounts[i].f_mntfromname,
				    mounts[i].f_mntonname);

			if (
#ifdef AMD_SUPPORT
			    am_unmount(mounts[i].f_mntonname) != 0 &&
#endif
			    unmount(mounts[i].f_mntonname, 0) == -1)
				err(1, "unmount: %s", mounts[i].f_mntonname);
		}
	}
	return;
}

static void
eject_tape(const char *name, enum eject_op op)
{
	struct mtop m;
	int fd;
	const char *dn;

	dn = nick2rdev(name);
	if (dn == NULL)
		dn = name;	/* Hope for the best. */
	fd = open(dn, O_RDONLY);
	if (fd == -1)
		err(1, "open: %s", dn);
	switch (op) {
	case OP_EJECT:
		if (verbose_f)
			(void)printf("Ejecting %s...\n", dn);

		m.mt_op = MTOFFL;
		m.mt_count = 0;
		if (ioctl(fd, MTIOCTOP, &m) == -1)
			err(1, "ioctl: MTIOCTOP: %s", dn);
		break;
	case OP_LOAD:
		errx(1, "cannot load tapes");
		/* NOTREACHED */
	case OP_LOCK:
		errx(1, "cannot lock tapes");
		/* NOTREACHED */
	case OP_UNLOCK:
		errx(1, "cannot unlock tapes");
		/* NOTREACHED */
	}
	(void)close(fd);
	return;
}

static void
eject_disk(const char *name, enum eject_op op)
{
	int fd;
	const char *dn;
	int arg;

	dn = nick2rdev(name);
	if (dn == NULL)
		dn = name;	/* Hope for the best. */
	fd = open(dn, O_RDONLY);
	if (fd == -1)
		err(1, "open: %s", dn);
	switch (op) {
	case OP_LOAD:
		if (verbose_f)
			(void)printf("Closing %s...\n", dn);

		if (ioctl(fd, CDIOCCLOSE, NULL) == -1)
			err(1, "ioctl: CDIOCCLOSE: %s", dn);
		break;
	case OP_EJECT:
		if (verbose_f)
			(void)printf("Ejecting %s...\n", dn);

		arg = 0;
		if (umount_f == 0) {
			/* force eject, unlock the device first */
			if (ioctl(fd, DIOCLOCK, &arg) == -1)
				err(1, "ioctl: DIOCLOCK: %s", dn);
			arg = 1;
		}
		if (ioctl(fd, DIOCEJECT, &arg) == -1)
			err(1, "ioctl: DIOCEJECT: %s", dn);
		break;
	case OP_LOCK:
		if (verbose_f)
			(void)printf("Locking %s...\n", dn);

		arg = 1;
		if (ioctl(fd, DIOCLOCK, &arg) == -1)
			err(1, "ioctl: DIOCLOCK: %s", dn);
		break;
	case OP_UNLOCK:
		if (verbose_f)
			(void)printf("Unlocking %s...\n", dn);

		arg = 0;
		if (ioctl(fd, DIOCLOCK, &arg) == -1)
			err(1, "ioctl: DIOCLOCK: %s", dn);
		break;
	}

	(void)close(fd);
	return;
}
