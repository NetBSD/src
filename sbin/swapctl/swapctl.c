/*	$NetBSD: swapctl.c,v 1.14.4.1 2000/07/27 16:12:35 itojun Exp $	*/

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
 * 3. The name of the author may not be used to endorse or promote products
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

/*
 * swapctl command:
 *	-A		add all devices listed as `sw' in /etc/fstab (also
 *			(sets the dump device, if listed in fstab)
 *	-D <dev>	set dumpdev to <dev>
 *	-U		remove all devices listed as `sw' in /etc/fstab.
 *	-t [blk|noblk]	if -A or -U , add (remove) either all block device
 *			or all non-block devices
 *	-a <dev>	add this device
 *	-d <dev>	remove this swap device (not supported yet)
 *	-l		list swap devices
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

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/swap.h>

#include <unistd.h>
#include <err.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstab.h>

#include "swapctl.h"

int	command;

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
#define	REQUIRE_NOPATH	(CMD_A | CMD_U | CMD_l | CMD_s)

/*
 * Option flags, and the commands with which they are valid.
 */
int	kflag;		/* display in 1K blocks */
#define	KFLAG_CMDS	(CMD_l | CMD_s)

int	pflag;		/* priority was specified */
#define	PFLAG_CMDS	(CMD_A | CMD_a | CMD_c)

char	*tflag;		/* swap device type (blk or noblk) */
#define	TFLAG_CMDS	(CMD_A | CMD_U )

int	pri;		/* uses 0 as default pri */

static	void change_priority __P((const char *));
static	int  add_swap __P((const char *, int));
static	int  delete_swap __P((const char *));
static	void set_dumpdev __P((const char *));
	int  main __P((int, char *[]));
static	void do_fstab __P((int));
static	void usage __P((void));
static	void swapon_command __P((int, char **));
#if 0
static	void swapoff_command __P((int, char **));
#endif

extern	char *__progname;	/* from crt0.o */

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	c;

	if (strcmp(__progname, "swapon") == 0) {
		swapon_command(argc, argv);
		/* NOTREACHED */
	}

#if 0
	if (strcmp(__progname, "swapoff") == 0) {
		swapoff_command(argc, argv);
		/* NOTREACHED */
	}
#endif

	while ((c = getopt(argc, argv, "ADUacdlkp:st:")) != -1) {
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

		case 'l':
			SET_COMMAND(CMD_l);
			break;

		case 'k':
			kflag = 1;
			break;

		case 'p':
			pflag = 1;
			/* XXX strtol() */
			pri = atoi(optarg);
			break;

		case 's':
			SET_COMMAND(CMD_s);
			break;

		case 't':
			if (tflag != NULL)
				usage();
			tflag = optarg;
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

	/* Sanity-check -t */
	if (tflag != NULL) {
		if (command != CMD_A && command != CMD_U)
			usage();
		if (strcmp(tflag, "blk") != 0 &&
		    strcmp(tflag, "noblk") != 0)
			usage();
	}

	/* Dispatch the command. */
	switch (command) {
	case CMD_l:
		list_swap(pri, kflag, pflag, 0, 1);
		break;

	case CMD_s:
		list_swap(pri, kflag, pflag, 0, 0);
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
		do_fstab(1);
		break;

	case CMD_D:
		set_dumpdev(argv[0]);
		break;

	case CMD_U:
		do_fstab(0);
		break;
	}

	exit(0);
}

/*
 * swapon_command: emulate the old swapon(8) program.
 */
static void
swapon_command(argc, argv)
	int argc;
	char **argv;
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
	fprintf(stderr, "usage: %s -a [-t blk|noblk]\n", __progname);
	fprintf(stderr, "       %s <path> ...\n", __progname);
	exit(1);
}

/*
 * change_priority:  change the priority of a swap device.
 */
static void
change_priority(path)
	const char	*path;
{

	if (swapctl(SWAP_CTL, path, pri) < 0)
		warn("%s", path);
}

/*
 * add_swap:  add the pathname to the list of swap devices.
 */
static int
add_swap(path, priority)
	const char *path;
	int priority;
{
	struct stat sb;

	if (stat(path, &sb) < 0)
		goto oops;

	if (sb.st_mode & S_IROTH) 
		warnx("%s is readable by the world", path);
	if (sb.st_mode & S_IWOTH)
		warnx("%s is writable by the world", path);

	if (swapctl(SWAP_ON, path, priority) < 0) {
oops:
		warn("%s", path);
		return (0);
	}
	return (1);
}

/*
 * delete_swap:  remove the pathname to the list of swap devices.
 */
static int
delete_swap(path)
	const char *path;
{

	if (swapctl(SWAP_OFF, path, pri) < 0) {
		warn("%s", path);
		return (0);
	}
	return (1);
}

static void
set_dumpdev(path)
	const char *path;
{

	if (swapctl(SWAP_DUMPDEV, path, NULL) == -1)
		warn("could not set dump device to %s", path);
	else
		printf("%s: setting dump device to %s\n", __progname, path);
}

static void
do_fstab(add)
	int add;
{
	struct	fstab *fp;
	char	*s;
	long	priority;
	struct	stat st;
	int	isblk;
	int	gotone = 0;
#define PATH_MOUNT	"/sbin/mount_nfs"
#define PATH_UMOUNT	"/sbin/umount"
	char	cmd[2*PATH_MAX+sizeof(PATH_MOUNT)+2];

#define PRIORITYEQ	"priority="
#define NFSMNTPT	"nfsmntpt="
	while ((fp = getfsent()) != NULL) {
		const char *spec;

		spec = fp->fs_spec;
		cmd[0] = '\0';

		if (strcmp(fp->fs_type, "dp") == 0 && add) {
			set_dumpdev(spec);
			continue;
		}

		if (strcmp(fp->fs_type, "sw") != 0)
			continue;
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
				free((char *)spec);
				continue;
			}
			if (add) {
				snprintf(cmd, sizeof(cmd), "%s %s %s",
					PATH_MOUNT, fp->fs_spec, spec);
				if (system(cmd) != 0) {
					warnx("%s: mount failed", fp->fs_spec);
					continue;
				}
			} else {
				snprintf(cmd, sizeof(cmd), "%s %s",
					PATH_UMOUNT, fp->fs_spec);
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
				gotone = 1;
				printf(
			    	"%s: adding %s as swap device at priority %d\n",
				    __progname, fp->fs_spec, (int)priority);
			}
		} else {
			if (delete_swap(spec)) {
				gotone = 1;
				printf(
				    "%s: removing %s as swap device\n",
				    __progname, fp->fs_spec);
			}
			if (cmd[0]) {
				if (system(cmd) != 0) {
					warnx("%s: umount failed", fp->fs_spec);
					continue;
				}
			}
		}

		if (spec != fp->fs_spec)
			free((char *)spec);
	}
	if (gotone == 0)
		exit(1);
}

static void
usage()
{

	fprintf(stderr, "usage: %s -A [-p priority] [-t blk|noblk]\n",
	    __progname);
	fprintf(stderr, "       %s -D dumppath\n", __progname);
	fprintf(stderr, "       %s -U [-t blk|noblk]\n", __progname);
	fprintf(stderr, "       %s -a [-p priority] path\n", __progname);
	fprintf(stderr, "       %s -c -p priority path\n", __progname);
	fprintf(stderr, "       %s -d path\n", __progname);
	fprintf(stderr, "       %s -l | -s [-k]\n", __progname);
	exit(1);
}
