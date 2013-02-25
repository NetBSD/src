/*	$NetBSD: swapctl.c,v 1.37.2.1 2013/02/25 00:28:11 tls Exp $	*/

/*
 * Copyright (c) 1996, 1997, 1999 Matthew R. Green
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

/*
 * swapctl command:
 *	-A		add all devices listed as `sw' in /etc/fstab (also
 *			(sets the dump device, if listed in fstab)
 *	-D [<dev>|none]	set dumpdev to <dev> or disable dumps
 *	-z		show dumpdev
 *	-U		remove all devices listed as `sw' in /etc/fstab.
 *	-t [blk|noblk|auto]
 *			if -A or -U , add (remove) either all block device
 *			or all non-block devices, or all swap partitions
 *	-q		check if any swap or dump devices are defined in
 *			/etc/fstab
 *	-a <dev>	add this device
 *	-d <dev>	remove this swap device
 *	-f		with -A -t auto, use the first swap as dump device
 *	-g		use gigabytes
 *	-h		use humanize_number(3) for listing
 *	-l		list swap devices
 *	-m		use megabytes
 *	-n		print actions, but do not add/remove swap or
 *			with -A/-U
 *	-o		with -A -t auto only configure the first swap as dump,
 *			(similar to -f), but do not add any further swap devs
 *	-s		short listing of swap devices
 *	-k		use kilobytes
 *	-p <pri>	use this priority
 *	-c		change priority
 *
 * or, if invoked as "swapon" (compatibility mode):
 *
 *	-a		all devices listed as `sw' in /etc/fstab
 *	-t		same as -t above (feature not present in old
 *			swapon(8) command)
 *	<dev>		add this device
 */
#include <sys/cdefs.h>

#ifndef lint
__RCSID("$NetBSD: swapctl.c,v 1.37.2.1 2013/02/25 00:28:11 tls Exp $");
#endif


#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/sysctl.h>
#include <sys/disk.h>
#include <sys/disklabel.h>

#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstab.h>
#include <fcntl.h>
#include <util.h>
#include <paths.h>

#include "swapctl.h"

static int	command;

/*
 * Commands for swapctl(8).  These are mutually exclusive.
 */
#define	CMD_A		0x01	/* process /etc/fstab for adding */
#define	CMD_D		0x02	/* set dumpdev */
#define	CMD_U		0x04	/* process /etc/fstab for removing */
#define	CMD_a		0x08	/* add a swap file/device */
#define	CMD_c		0x10	/* change priority of a swap file/device */
#define	CMD_d		0x20	/* delete a swap file/device */
#define	CMD_l		0x40	/* list swap files/devices */
#define	CMD_s		0x80	/* summary of swap files/devices */
#define	CMD_z		0x100	/* show dump device */
#define	CMD_q		0x200	/* check for dump/swap in /etc/fstab */

#define	SET_COMMAND(cmd) \
do { \
	if (command) \
		usage(); \
	command = (cmd); \
} while (0)

/*
 * Commands that require a "path" argument at the end of the command
 * line, and the ones which require that none exist.
 */
#define	REQUIRE_PATH	(CMD_D | CMD_a | CMD_c | CMD_d)
#define	REQUIRE_NOPATH	(CMD_A | CMD_U | CMD_l | CMD_s | CMD_z | CMD_q)

/*
 * Option flags, and the commands with which they are valid.
 */
static int	kflag;		/* display in 1K^x blocks */
#define	KFLAG_CMDS	(CMD_l | CMD_s)
#define MFLAG_CMDS	(CMD_l | CMD_s)
#define GFLAG_CMDS	(CMD_l | CMD_s)

static int	hflag;		/* display with humanize_number */
#define HFLAG_CMDS	(CMD_l | CMD_s)

static int	pflag;		/* priority was specified */
#define	PFLAG_CMDS	(CMD_A | CMD_a | CMD_c)

static char	*tflag;		/* swap device type (blk, noblk, auto) */
static int	autoflag;	/* 1, if tflag is "auto" */
#define	TFLAG_CMDS	(CMD_A | CMD_U)

static int	fflag;		/* first swap becomes dump */
#define	FFLAG_CMDS	(CMD_A)

static int	oflag;		/* only autoset dump device */
#define	OFLAG_CMDS	(CMD_A)

static int	nflag;		/* no execute, just print actions */
#define	NFLAG_CMDS	(CMD_A | CMD_U)

static int	pri;		/* uses 0 as default pri */

static	void change_priority(char *);
static	int  add_swap(char *, int);
static	int  delete_swap(char *);
static	void set_dumpdev1(char *);
static	void set_dumpdev(char *);
static	int get_dumpdev(void);
__dead static	void do_fstab(int);
static	int check_fstab(void);
static	void do_localdevs(int);
static	void do_localdisk(const char *, int);
static	int do_wedgesofdisk(int fd, int);
static	int do_partitionsofdisk(const char *, int fd, int);
__dead static	void usage(void);
__dead static	void swapon_command(int, char **);
#if 0
static	void swapoff_command(int, char **);
#endif

int
main(int argc, char *argv[])
{
	int	c;

	if (strcmp(getprogname(), "swapon") == 0) {
		swapon_command(argc, argv);
		/* NOTREACHED */
	}

#if 0
	if (strcmp(getprogname(), "swapoff") == 0) {
		swapoff_command(argc, argv);
		/* NOTREACHED */
	}
#endif

	while ((c = getopt(argc, argv, "ADUacdfghklmnop:qst:z")) != -1) {
		switch (c) {
		case 'A':
			SET_COMMAND(CMD_A);
			break;

		case 'D':
			SET_COMMAND(CMD_D);
			break;

		case 'U':
			SET_COMMAND(CMD_U);
			break;

		case 'a':
			SET_COMMAND(CMD_a);
			break;

		case 'c':
			SET_COMMAND(CMD_c);
			break;

		case 'd':
			SET_COMMAND(CMD_d);
			break;

		case 'f':
			fflag = 1;
			break;

		case 'g':
			kflag = 3; /* 1k ^ 3 */
			break;

		case 'h':
			hflag = 1;
			break;

		case 'k':
			kflag = 1;
			break;

		case 'l':
			SET_COMMAND(CMD_l);
			break;

		case 'm':
			kflag = 2; /* 1k ^ 2 */
			break;

		case 'n':
			nflag = 1;
			break;

		case 'o':
			oflag = 1;
			break;

		case 'p':
			pflag = 1;
			/* XXX strtol() */
			pri = atoi(optarg);
			break;

		case 'q':
			SET_COMMAND(CMD_q);
			break;

		case 's':
			SET_COMMAND(CMD_s);
			break;

		case 't':
			if (tflag != NULL)
				usage();
			tflag = optarg;
			if (strcmp(tflag, "auto") == 0)
				autoflag = 1;
			break;

		case 'z':
			SET_COMMAND(CMD_z);
			break;

		default:
			usage();
			/* NOTREACHED */
		}
	}

	/* Did the user specify a command? */
	if (command == 0)
		usage();

	argv += optind;
	argc -= optind;

	switch (argc) {
	case 0:
		if (command & REQUIRE_PATH)
			usage();
		break;

	case 1:
		if (command & REQUIRE_NOPATH)
			usage();
		break;

	default:
		usage();
	}

	/* To change priority, you have to specify one. */
	if ((command == CMD_c) && pflag == 0)
		usage();

	/* -f and -o are mutualy exclusive */
	if (fflag && oflag)
		usage();
		
	/* Sanity-check -t */
	if (tflag != NULL) {
		if (command != CMD_A && command != CMD_U)
			usage();
		if (strcmp(tflag, "blk") != 0 &&
		    strcmp(tflag, "noblk") != 0 &&
		    strcmp(tflag, "auto") != 0)
			usage();
	}

	/* Dispatch the command. */
	switch (command) {
	case CMD_l:
		if (!list_swap(pri, kflag, pflag, 0, 1, hflag))
			exit(1);
		break;

	case CMD_s:
		list_swap(pri, kflag, pflag, 0, 0, hflag);
		break;

	case CMD_c:
		change_priority(argv[0]);
		break;

	case CMD_a:
		if (! add_swap(argv[0], pri))
			exit(1);
		break;

	case CMD_d:
		if (! delete_swap(argv[0]))
			exit(1);
		break;

	case CMD_A:
		if (autoflag)
			do_localdevs(1);
		else
			do_fstab(1);
		break;

	case CMD_D:
		set_dumpdev(argv[0]);
		break;

	case CMD_z:
		if (!get_dumpdev())
			exit(1);
		break;

	case CMD_U:
		if (autoflag)
			do_localdevs(0);
		else
			do_fstab(0);
		break;
	case CMD_q:
		if (check_fstab()) {
			printf("%s: there are swap or dump devices defined in "
			    _PATH_FSTAB "\n", getprogname());
			exit(0);
		} else {
			printf("%s: no swap or dump devices in "
			    _PATH_FSTAB "\n", getprogname());
			exit(1);
		}
	}

	exit(0);
}

/*
 * swapon_command: emulate the old swapon(8) program.
 */
static void
swapon_command(int argc, char **argv)
{
	int ch, fiztab = 0;

	while ((ch = getopt(argc, argv, "at:")) != -1) {
		switch (ch) {
		case 'a':
			fiztab = 1;
			break;
		case 't':
			if (tflag != NULL)
				usage();
			tflag = optarg;
			break;
		default:
			goto swapon_usage;
		}
	}
	argc -= optind;
	argv += optind;

	if (fiztab) {
		if (argc)
			goto swapon_usage;
		/* Sanity-check -t */
		if (tflag != NULL) {
			if (strcmp(tflag, "blk") != 0 &&
			    strcmp(tflag, "noblk") != 0)
				usage();
		}
		do_fstab(1);
		exit(0);
	} else if (argc == 0 || tflag != NULL)
		goto swapon_usage;

	while (argc) {
		if (! add_swap(argv[0], pri))
			exit(1);
		argc--;
		argv++;
	}
	exit(0);
	/* NOTREACHED */

 swapon_usage:
	fprintf(stderr, "usage: %s -a [-t blk|noblk]\n", getprogname());
	fprintf(stderr, "       %s <path> ...\n", getprogname());
	exit(1);
}

/*
 * change_priority:  change the priority of a swap device.
 */
static void
change_priority(char *path)
{

	if (swapctl(SWAP_CTL, path, pri) < 0)
		err(1, "%s", path);
}

/*
 * add_swap:  add the pathname to the list of swap devices.
 */
static int
add_swap(char *path, int priority)
{
	struct stat sb;
	char buf[MAXPATHLEN];
	char *spec;

	if (getfsspecname(buf, sizeof(buf), path) == NULL)
		goto oops;
	spec = buf;

	if (stat(spec, &sb) < 0)
		goto oops;

	if (sb.st_mode & S_IROTH) 
		warnx("WARNING: %s is readable by the world", path);
	if (sb.st_mode & S_IWOTH)
		warnx("WARNING: %s is writable by the world", path);

	if (fflag || oflag) {
		set_dumpdev1(spec);
		if (oflag)
			exit(0);
		else
			fflag = 0;
	}

	if (nflag)
		return 1;

	if (swapctl(SWAP_ON, spec, priority) < 0) {
oops:
		err(1, "%s", path);
	}
	return (1);
}

/*
 * delete_swap:  remove the pathname to the list of swap devices.
 */
static int
delete_swap(char *path)
{
	char buf[MAXPATHLEN];
	char *spec;

	if (getfsspecname(buf, sizeof(buf), path) == NULL)
		err(1, "%s", path);
	spec = buf;

	if (nflag)
		return 1;

	if (swapctl(SWAP_OFF, spec, pri) < 0) 
		err(1, "%s", path);
	return (1);
}

static void
set_dumpdev1(char *spec)
{
	int rv = 0;

	if (!nflag) {
		if (strcmp(spec, "none") == 0) 
			rv = swapctl(SWAP_DUMPOFF, NULL, 0);
		else
			rv = swapctl(SWAP_DUMPDEV, spec, 0);
	}

	if (rv == -1)
		err(1, "could not set dump device to %s", spec);
	else
		printf("%s: setting dump device to %s\n", getprogname(), spec);
}

static void
set_dumpdev(char *path)
{
	char buf[MAXPATHLEN];
	char *spec;

	if (getfsspecname(buf, sizeof(buf), path) == NULL)
		err(1, "%s", path);
	spec = buf;

	return set_dumpdev1(spec);
}

static int
get_dumpdev(void)
{
	dev_t	dev;
	char 	*name;

	if (swapctl(SWAP_GETDUMPDEV, &dev, 0) == -1) {
		warn("could not get dump device");
		return 0;
	} else if (dev == NODEV) {
		printf("no dump device set\n");
		return 0;
	} else {
		name = devname(dev, S_IFBLK);
		printf("dump device is ");
		if (name)
			printf("%s\n", name);
		else
			printf("major %llu minor %llu\n",
			    (unsigned long long)major(dev),
			    (unsigned long long)minor(dev));
	}
	return 1;
}

static void
do_localdevs(int add)
{
	size_t ressize;
	char *disknames, *disk;
	static const char mibname[] = "hw.disknames";

	ressize = 0;
	if (sysctlbyname(mibname, NULL, &ressize, NULL, 0))
		return;
	ressize += 200;	/* add some arbitrary slope */
	disknames = malloc(ressize);
	if (sysctlbyname(mibname, disknames, &ressize, NULL, 0) == 0) {
		for (disk = strtok(disknames, " "); disk;
		    disk = strtok(NULL, " "))
			do_localdisk(disk, add);
	}
	free(disknames);
}

static void
do_localdisk(const char *disk, int add)
{
	int fd;
	char dvname[MAXPATHLEN];

	if ((fd = opendisk(disk, O_RDONLY, dvname, sizeof(dvname), 0)) == -1)
		return;

	if (!do_wedgesofdisk(fd, add))
		do_partitionsofdisk(disk, fd, add);

	close(fd);
}

static int
do_wedgesofdisk(int fd, int add)
{
	char devicename[MAXPATHLEN];
	struct dkwedge_info *dkw;
	struct dkwedge_list dkwl;
	size_t bufsize;
	u_int i;

	dkw = NULL;
	dkwl.dkwl_buf = dkw;
	dkwl.dkwl_bufsize = 0;

	for (;;) {
		if (ioctl(fd, DIOCLWEDGES, &dkwl) == -1)
			return 0;
		if (dkwl.dkwl_nwedges == dkwl.dkwl_ncopied)
			break;
		bufsize = dkwl.dkwl_nwedges * sizeof(*dkw);
		if (dkwl.dkwl_bufsize < bufsize) {
			dkw = realloc(dkwl.dkwl_buf, bufsize);
			if (dkw == NULL)
				return 0;
			dkwl.dkwl_buf = dkw;
			dkwl.dkwl_bufsize = bufsize;
		}
	}

	for (i = 0; i < dkwl.dkwl_ncopied; i++) {
		if (strcmp(dkw[i].dkw_ptype, DKW_PTYPE_SWAP) != 0)
			continue;
		snprintf(devicename, sizeof(devicename), "%s%s", _PATH_DEV,
		    dkw[i].dkw_devname);
		devicename[sizeof(devicename)-1] = '\0';

		if (add) {
			if (add_swap(devicename, pri)) {
				printf(
			    	"%s: adding %s as swap device at priority 0\n",
				    getprogname(), devicename);
			}
		} else {
			if (delete_swap(devicename)) {
				printf(
				    "%s: removing %s as swap device\n",
				    getprogname(), devicename);
			}
		}

	}

	free(dkw);
	return dkwl.dkwl_nwedges != 0;
}

static int
do_partitionsofdisk(const char *prefix, int fd, int add)
{
	char devicename[MAXPATHLEN];
	struct disklabel lab;
	uint i;

	if (ioctl(fd, DIOCGDINFO, &lab) != 0)
		return 0;

	for (i = 0; i < lab.d_npartitions; i++) {
		if (lab.d_partitions[i].p_fstype != FS_SWAP)
			continue;
		snprintf(devicename, sizeof(devicename), "%s%s%c", _PATH_DEV,
		    prefix, 'a'+i);
		devicename[sizeof(devicename)-1] = '\0';

		if (add) {
			if (add_swap(devicename, pri)) {
				printf(
			    	"%s: adding %s as swap device at priority 0\n",
				    getprogname(), devicename);
			}
		} else {
			if (delete_swap(devicename)) {
				printf(
				    "%s: removing %s as swap device\n",
				    getprogname(), devicename);
			}
		}
	}

	return 1;
}

static int
check_fstab(void)
{
	struct	fstab *fp;

	while ((fp = getfsent()) != NULL) {
		if (strcmp(fp->fs_type, "dp") == 0)
			return 1;

		if (strcmp(fp->fs_type, "sw") == 0)
			return 1;
	}

	return 0;
}

static void
do_fstab(int add)
{
	struct	fstab *fp;
	char	*s;
	long	priority;
	struct	stat st;
	int	isblk;
	int	success = 0;	/* set to 1 after a successful operation */
	int	error = 0;	/* set to 1 after an error */

#ifdef RESCUEDIR
#define PATH_MOUNT	RESCUEDIR "/mount_nfs"
#define PATH_UMOUNT	RESCUEDIR "/umount"
#else
#define PATH_MOUNT	"/sbin/mount_nfs"
#define PATH_UMOUNT	"/sbin/umount"
#endif

	char	cmd[2*PATH_MAX+sizeof(PATH_MOUNT)+2];

#define PRIORITYEQ	"priority="
#define NFSMNTPT	"nfsmntpt="
	while ((fp = getfsent()) != NULL) {
		char buf[MAXPATHLEN];
		char *spec, *fsspec;

		if (getfsspecname(buf, sizeof(buf), fp->fs_spec) == NULL) {
			warn("%s", buf);
			continue;
		}
		fsspec = spec = buf;
		cmd[0] = '\0';

		if (strcmp(fp->fs_type, "dp") == 0 && add) {
			set_dumpdev1(spec);
			continue;
		}

		if (strcmp(fp->fs_type, "sw") != 0)
			continue;

		/* handle dp as mnt option */
		if (strstr(fp->fs_mntops, "dp") && add)
			set_dumpdev1(spec);

		isblk = 0;

		if ((s = strstr(fp->fs_mntops, PRIORITYEQ)) != NULL) {
			s += sizeof(PRIORITYEQ) - 1;
			priority = atol(s);
		} else
			priority = pri;

		if ((s = strstr(fp->fs_mntops, NFSMNTPT)) != NULL) {
			char *t;

			/*
			 * Skip this song and dance if we're only
			 * doing block devices.
			 */
			if (tflag != NULL && strcmp(tflag, "blk") == 0)
				continue;

			t = strpbrk(s, ",");
			if (t != 0)
				*t = '\0';
			spec = strdup(s + strlen(NFSMNTPT));
			if (t != 0)
				*t = ',';

			if (spec == NULL)
				errx(1, "Out of memory");

			if (strlen(spec) == 0) {
				warnx("empty mountpoint");
				free(spec);
				continue;
			}
			if (add) {
				snprintf(cmd, sizeof(cmd), "%s %s %s",
					PATH_MOUNT, fsspec, spec);
				if (system(cmd) != 0) {
					warnx("%s: mount failed", fsspec);
					continue;
				}
			} else {
				snprintf(cmd, sizeof(cmd), "%s %s",
					PATH_UMOUNT, fsspec);
			}
		} else {
			/*
			 * Determine blk-ness.
			 */
			if (stat(spec, &st) < 0) {
				warn("%s", spec);
				continue;
			}
			if (S_ISBLK(st.st_mode))
				isblk = 1;
		}

		/*
		 * Skip this type if we're told to.
		 */
		if (tflag != NULL) {
			if (strcmp(tflag, "blk") == 0 && isblk == 0)
				continue;
			if (strcmp(tflag, "noblk") == 0 && isblk == 1)
				continue;
		}

		if (add) {
			if (add_swap(spec, (int)priority)) {
				success = 1;
				printf(
			    	"%s: adding %s as swap device at priority %d\n",
				    getprogname(), fsspec, (int)priority);
			} else {
				error = 1;
				fprintf(stderr,
				    "%s: failed to add %s as swap device\n",
				    getprogname(), fsspec);
			}
		} else {
			if (delete_swap(spec)) {
				success = 1;
				printf(
				    "%s: removing %s as swap device\n",
				    getprogname(), fsspec);
			} else {
				error = 1;
				fprintf(stderr,
				    "%s: failed to remove %s as swap device\n",
				    getprogname(), fsspec);
			}
			if (cmd[0]) {
				if (system(cmd) != 0) {
					warnx("%s: umount failed", fsspec);
					error = 1;
					continue;
				}
			}
		}

		if (spec != fsspec)
			free(spec);
	}
	if (error)
		exit(1);
	else if (success)
		exit(0);
	else
		exit(2); /* not really an error, but no swap devices found */
}

static void
usage(void)
{
	const char *progname = getprogname();

	fprintf(stderr, "usage: %s -A [-f|-o] [-n] [-p priority] "
	    "[-t blk|noblk|auto]\n", progname);
	fprintf(stderr, "       %s -a [-p priority] path\n", progname);
	fprintf(stderr, "       %s -q\n", progname);
	fprintf(stderr, "       %s -c -p priority path\n", progname);
	fprintf(stderr, "       %s -D dumpdev|none\n", progname);
	fprintf(stderr, "       %s -d path\n", progname);
	fprintf(stderr, "       %s -l | -s [-k|-m|-g|-h]\n", progname);
	fprintf(stderr, "       %s -U [-n] [-t blk|noblk|auto]\n", progname);
	fprintf(stderr, "       %s -z\n", progname);
	exit(1);
}
