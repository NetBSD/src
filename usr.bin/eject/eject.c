/*	$NetBSD: eject.c,v 1.7 1999/02/08 16:35:33 bouyer Exp $	*/
/*
 * Copyright (c) 1995
 *	Matthieu Herrb.  All rights reserved.
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
 *	This product includes software developed for the NetBSD Project
 *	by Matthieu Herrb.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: eject.c,v 1.7 1999/02/08 16:35:33 bouyer Exp $");
#endif

/*
 * Eject command
 *
 * It knows to eject floppies, CD-ROMS and tapes
 * and tries to unmount file systems first
 */

#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/cdio.h>
#include <sys/mtio.h>
#include <sys/disklabel.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/cdefs.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <err.h>
#include <string.h>

typedef struct DEVTAB {
	char   *name;
	char   *device;
	char   qualifier;
	u_int   type;
} DEVTAB;

/*
 * known device nicknames and types
 * (used for selecting the proper ioctl to eject them)
 */
#define DISK   0x00000002
#define CDROM  0x00000003
#define TAPE   0x00010000

#define MOUNTABLE(x) ((x) & 0x0000ffff)

#define RPART ('a' + RAW_PART)

static DEVTAB devtab[] = {
	{ "diskette", "/dev/rfd0", 'a', DISK },
	{ "diskette0", "/dev/rfd0", 'a', DISK },
	{ "diskette1", "/dev/rfd1", 'a', DISK },
	{ "floppy", "/dev/rfd0", 'a', DISK },
	{ "floppy0", "/dev/rfd0", 'a', DISK },
	{ "floppy1", "/dev/rfd1", 'a', DISK },
	{ "fd", "/dev/rfd0", 'a', DISK },
	{ "fd0", "/dev/rfd0", 'a', DISK },
	{ "fd1", "/dev/rfd1", RPART, DISK },
	{ "sd0", "/dev/rsd0", RPART, DISK },
	{ "sd1", "/dev/rsd1", RPART, DISK },
	{ "sd2", "/dev/rsd2", RPART, DISK },
	{ "cdrom", "/dev/rcd0", RPART, CDROM },
	{ "cdrom0", "/dev/rcd0", RPART, CDROM },
	{ "cdrom1", "/dev/rcd1", RPART, CDROM },
	{ "cd", "/dev/rcd0", RPART, CDROM },
	{ "cd0", "/dev/rcd0", RPART, CDROM },
	{ "cd1", "/dev/rcd1", RPART, CDROM },
	{ "mcd", "/dev/rmcd0", RPART, CDROM },
	{ "mcd0", "/dev/rmcd0", RPART, CDROM },
	{ "mcd1", "/dev/rmcd1", RPART, CDROM },
	{ "tape", "/dev/rst0", '\0', TAPE },
	{ "tape0", "/dev/rst0", '\0', TAPE },
	{ "tape1", "/dev/rst1", '\0', TAPE },
	{ "st", "/dev/rst0", '\0', TAPE },
	{ "st0", "/dev/rst0", '\0', TAPE },
	{ "st1", "/dev/rst1", '\0', TAPE },
	{ "dat", "/dev/rst0", '\0', TAPE },
	{ "dat0", "/dev/rst0", '\0', TAPE },
	{ "dat1", "/dev/rst1", '\0', TAPE },
	{ "exabyte", "/dev/rst0", '\0', TAPE },
	{ "exabyte0", "/dev/rst0", '\0', TAPE },
	{ "exabyte1", "/dev/rst1", '\0', TAPE },
	{ NULL, NULL }
};

struct types {
	char	*str;
	int	type;
} types[] = {
	{ "diskette", DISK },
	{ "floppy", DISK },
	{ "cdrom", CDROM },
	{ "disk", DISK },
	{ "tape", TAPE },
	{ NULL, 0 }
};

int verbose;

static	void	usage __P((void));
static	char   *device_by_name __P((char *, int *, char *));
static	char   *device_by_nickname __P((char *, int *, char *));
static	void	load_cdrom __P((char *));
static	void	eject_disk __P((char *, int));
static	void	eject_tape __P((char *));
	int	main __P((int, char **));
static	void	umount_mounted __P((char *));

/*
 * remind the syntax of the command to the user
 */
static void
usage()
{
	fprintf(stderr,
	    "usage: eject [-n][-f][-l][-t devtype][[-d] raw device | "
	    "nickname ]\n");
	exit(1);
	/*NOTREACHED*/
}


/*
 * given a device nick name, find its associated raw device and type
 */
static char *
device_by_nickname(name, pdevtype, pqualifier)
	char	*name;
	int	*pdevtype;
	char	*pqualifier;
{
	int     i;

	for (i = 0; devtab[i].name != NULL; i++) {
		if (strcmp(name, devtab[i].name) == 0) {
			*pdevtype = devtab[i].type;
			*pqualifier = devtab[i].qualifier;
			return devtab[i].device;
		}
	}
	*pdevtype = -1;
	return NULL;
}

/*
 * Given a raw device name, find its type and partition tag
 * from the name.
 */
static char *
device_by_name(device, pdevtype, pqualifier)
	char	*device;
	int	*pdevtype;
	char	*pqualifier;
{
	int     i;

	for (i = 0; devtab[i].name != NULL; i++) {
		if (strncmp(devtab[i].device, device,
			    strlen(devtab[i].device)) == 0) {
			*pdevtype = devtab[i].type;
			*pqualifier = devtab[i].qualifier;
			return devtab[i].device;
		}
	}
	*pdevtype = -1;
	return NULL;
}

/*
 * load a disk (cdrom only)
 */
static void
load_cdrom(device)
	char   *device;
{
	int     fd;

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		err(1, "%s: open", device);
	}

	if (ioctl(fd, CDIOCCLOSE, NULL) < 0) {
		err(1, "%s: DIOCEJECT", device);
	}
	if (close(fd) != 0)
		err(1, "%s: close", device);
}

/*
 * eject a disk (including floppy and cdrom)
 */
static void
eject_disk(device, umount_flag)
	char   *device;
	int    umount_flag;
{
	int     fd, arg = 0;

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		err(1, "%s: open", device);
	}
	if (umount_flag == 0) {
		if (ioctl(fd, DIOCLOCK, (char *)&arg) < 0) {
			err(1, "%s: DIOCLOCK", device);
		}
		arg = 1; /* eject without device busy check */
	}
	if (ioctl(fd, DIOCEJECT, (char *)&arg) < 0) {
		err(1, "%s: DIOCEJECT", device);
	}
	if (close(fd) != 0)
		err(1, "%s: close", device);
} /* eject_disk */

/*
 * eject a tape
 */
static void
eject_tape(device)
	char   *device;
{
	int     fd;
	struct mtop mt_com;

	fd = open(device, O_RDONLY);
	if (fd < 0) {
		err(1, "open %s", device);
	}
	mt_com.mt_op = MTOFFL;

	if (ioctl(fd, MTIOCTOP, &mt_com) < 0) {
		err(1, "%s:  MTOFFL", device);
	}
	close(fd);
} /* eject_tape */

/*
 * test if partitions of a device are mounted
 * and unmount them
 */
static void
umount_mounted(device)
	char   *device;
{
	struct statfs *mntbuf;
	int     i, n, l;
	static char blkdev[32];
	struct stat stb;
	const char *dp;

	/* convert path to block device if needed, remove partition letter */
	if (stat(device, &stb) < 0)
		return;
	if (S_ISBLK(stb.st_mode)) {
		strncpy(blkdev, device, 32);
	} else if ((dp = strrchr(device, '/')) != 0 && dp[1] == 'r') {
		snprintf(blkdev, 32, "%.*s/%s", (int)(dp - device),
		    device, dp + 2);
	} else
		return;
	blkdev[strlen(blkdev) - 1] = '\0';
			
	n = getmntinfo(&mntbuf, MNT_NOWAIT);
	if (n == 0) {
		err(1, "getmntinfo");
	}
	l = strlen(blkdev);
	for (i = 0; i < n; i++) {
		if (strncmp(blkdev, mntbuf[i].f_mntfromname, l) == 0) {
			if (verbose)
				printf("Unmounting: %s\n",
					mntbuf[i].f_mntonname);
			if (unmount(mntbuf[i].f_mntonname, 0) < 0) {
				err(1, "umount %s from %s",
					mntbuf[i].f_mntfromname,
					mntbuf[i].f_mntonname);
			}
		}
	}

}

/*
 * Eject - ejects various removable devices, including cdrom, tapes,
 * diskettes, and other removable disks (like ZIP drives)
 */
int
main(argc, argv)
	int     argc;
	char   *argv[];
{
	char    device[MAXPATHLEN];
	char	*devpath;
	char	qualifier;
	int     umount_flag, load_flag, devtype;
	int     i, ch;

	/* Default options values */
	devpath = NULL;
	devtype = -1;
	umount_flag = 1;
	load_flag = 0;

	while ((ch = getopt(argc, argv, "d:flnt:v")) != -1) {
		switch (ch) {
		case 'd':
			devpath = optarg;
			break;
		case 'f':
			umount_flag = 0;
			break;
		case 'l':
			load_flag = 1;
			break;
		case 'n':
			for (i = 0; devtab[i].name != NULL; i++) {
				if (devtab[i].qualifier != '\0') {
					printf("%9s => %s%c\n", devtab[i].name,
					       devtab[i].device,
					       devtab[i].qualifier);
				} else {
					printf("%9s => %s\n", devtab[i].name,
					       devtab[i].device);
				}
			}
			return 0;
		case 't':
			for (i = 0; types[i].str != NULL; i++) {
				if (strcasecmp(optarg, types[i].str) == 0) {	
					devtype = types[i].type;
					break;
				}
			}
			if (devtype == -1)
				errx(1, "%s: unknown device type", optarg);
			break;
		case 'v':
			verbose = 1;
			break;
		case '?':
		default:
			usage();
			/*NOTREACHED*/
		}
	}

	argc -= optind;
	argv += optind;

	if (devpath != NULL) {
		/* device specified with 'd' option */
		if (devtype == -1)
			device_by_name(devpath, &devtype, &qualifier);
		else
			qualifier = '\0';
	} else {
		if (argc <= 0) {
			errx(1, "No device specified");
			/* NOTREACHED */
		}
		if (strncmp(argv[0], "/dev/", 5) == 0) {
			/*
			 * If argument begins with "/dev/", assume
			 * a device name.
			 */
			if (devtype == -1) {
				devpath = device_by_name(argv[0],
							 &devtype, &qualifier);
			} else {
				/* Device type specified; use literally */
				devpath = argv[0];
				qualifier = '\0';
			}
		} else {
			/* assume a nickname */
			devpath = device_by_nickname(argv[0],
						     &devtype, &qualifier);
		}
	}

	if (devpath == NULL) {
		errx(1, "%s: unknown device", argv[0]);
		/*NOTREACHED*/
	}

	snprintf(device, sizeof(device), "%s%c", devpath, qualifier);

	if (load_flag) {
		if (devtype != CDROM)
			errx(1, "Can only load CDROM device type\n");
		if (verbose)
			printf("Loading device `%s'\n", device);
		load_cdrom(device);
		exit(0);
	}

	if (umount_flag && MOUNTABLE(devtype)) {
		umount_mounted(device);
	}

	if (verbose)
		printf("Ejecting device `%s'\n", device);
	switch (devtype) {
	case DISK:
	case CDROM:
		eject_disk(device, umount_flag);
		break;
	case TAPE:
		eject_tape(device);
		break;
	default:
		errx(1, "impossible... devtype = %d", devtype);
	}

	exit(0);
}
