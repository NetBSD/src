/*	$NetBSD: swap.c,v 1.1.2.2.2.14 1997/06/05 23:08:14 pk Exp $	*/

/*
 * Copyright (c) 1996, 1997 Matthew R. Green
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
 *      This product includes software developed by Matthew R. Green for
 *      The NetBSD Foundation.
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

/*
 * swap command:
 *	-A		add all devices listed as `sw' in /etc/fstab
 *	-a <dev>	add this device
 *	-d <dev>	remove this swap device (not supported yet)
 *	-l		list swap devices
 *	-s		short listing of swap devices
 *	-k		use kilobytes
 *	-p <pri>	use this priority
 *	-c		change priority
 */

#include <sys/param.h>

#include <vm/vm_swap.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fstab.h>

#include "swap.h"

int	Aflag;
int	aflag;
int	cflag;
#ifdef SWAP_OFF_WORKS
int	dflag;
#endif
int	lflag;
int	kflag;
int	sflag;
int	pflag;
int	pri;		/* uses 0 as default pri */
char	*path;

static	void change_priority __P((char *));
static	void add_swap __P((char *));
#ifdef SWAP_OFF_WORKS
static	void del_swap __P((char *));
#endif /* SWAP_OFF_WORKS */
static	void do_fstab __P((void));
static	void usage __P((void));

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	c;
	int	am_swapon = 0;
#ifdef SWAP_OFF_WORKS
	int	swapoff = 0;
	static	char getoptstr[] = "Aacdlkp:s";
#else
	static	char getoptstr[] = "Aaclkp:s";
#endif /* SWAP_OFF_WORKS */
	extern	char *__progname;		/* XXX */

	if (strcmp(__progname, "swapon") == 0)
		aflag = am_swapon = 1;
#ifdef SWAP_OFF_WORKS
	else if (strcmp(__progname, "swapoff") == 0)
		dflag = swapoff = 1;
#endif /* SWAP_OFF_WORKS */
	while ((c = getopt(argc, argv, getoptstr)) != EOF) {
		switch(c) {
		case 'A':
			Aflag = 1;
			break;
		case 'a':
			if (cflag) {
				warn("-a and -c are mutually exclusive");
				usage();
			}
#ifdef SWAP_OFF_WORKS
			if (dflag) {
				warn("-a and -d are mutually exclusive");
				usage();
			}
#endif /* SWAP_OFF_WORKS */
			if (am_swapon)
				Aflag = 1;
			else
				aflag = 1;
			break;
		case 'c':
			if (aflag) {
				warn("-c and -a are mutually exclusive");
				usage();
			}
			cflag = 1;
			break;
#ifdef SWAP_OFF_WORKS
		case 'd':
			if (aflag || cflag) {
				warn("-d and -a or -c are mutually exclusive");
				usage();
			}
			dflag = 1;
			break;
#endif /* SWAP_OFF_WORKS */
		case 'l':
			lflag = 1;
			break;
		case 'k':
			kflag = 1;
			break;
		case 'p':
			pflag = 1;
			pri = atoi(optarg);
			break;
		case 's':
			sflag = 1;
			break;
		}
	}
	/* SWAP_OFF_WORKS */
	if (!aflag && !Aflag && !cflag && !lflag && !sflag /* && !dflag */)
		usage();

	argv += optind;
	if (!*argv && !Aflag && !lflag && !sflag)
		usage();
	if (cflag && !pflag)
		usage();

	if (lflag)
		list_swap(pri, kflag, pflag, 0, 1);
	else if (sflag)
		list_swap(pri, kflag, pflag, 0, 0);
	else if (cflag)
		change_priority(argv[0]);
	else if (aflag)
		add_swap(argv[0]);
#ifdef SWAP_OFF_WORKS
	else if (dflag)
		del_swap(argv[0]);
#endif /* SWAP_OFF_WORKS */
	else if (Aflag)
		do_fstab();
	exit(0);
}

/*
 * change_priority:  change the priority of a swap device.
 */
void
change_priority(path)
	char	*path;
{

	if (swapctl(SWAP_CTL, path, pri) < 0)
		warn("%s", path);
}

/*
 * add_swap:  add the pathname to the list of swap devices.
 */
void
add_swap(path)
	char *path;
{

	if (swapctl(SWAP_ON, path, pri) < 0)
		warn("%s", path);
}

#if SWAP_OFF_WORKS
/*
 * del_swap:  remove the pathname to the list of swap devices.
 *
 * XXX note that the kernel does not support this operation (yet).
 */
void
del_swap(path)
	char path;
{

	if (swapctl(SWAP_OFF, path, pri) < 0)
		warn("%s", path);
}
#endif /* SWAP_OFF_WORKS */

void
do_fstab()
{
	struct	fstab *fp;
	char	*s;
	long	priority;

#define PRIORITYEQ	"priority="
#define NFSMNTPT	"nfsmntpt="
#define PATH_MOUNT	"/sbin/mount_nfs"
	while (fp = getfsent()) {
		char *spec;

		if (strcmp(fp->fs_type, "sw") != 0)
			continue;

		spec = fp->fs_spec;

		if (s = strstr(fp->fs_mntops, PRIORITYEQ)) {
			s += sizeof(PRIORITYEQ) - 1;
			priority = atol(s);
		} else
			priority = pri;

		if (s = strstr(fp->fs_mntops, NFSMNTPT)) {
			char *t, cmd[2*PATH_MAX+sizeof(PATH_MOUNT)+2];

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
			snprintf(cmd, sizeof(cmd), "%s %s %s",
				PATH_MOUNT, fp->fs_spec, spec);
			if (system(cmd) != 0) {
				warnx("%s: mount failed", fp->fs_spec);
				continue;
			}
		}

		if (swapctl(SWAP_ON, spec, (int)priority) < 0)
			warn("%s", spec);
		else
printf("swap: adding %s as swap device at priority %d\n",
	    fp->fs_spec, priority);

		if (spec != fp->fs_spec)
			free(spec);
	}
}

void
usage()
{
	extern char *__progname;
#ifdef SWAP_OFF_WORKS
	static	char usagemsg[] =
	    "usage: %s [-k] [-A|-a|-c|-d|-l|-s] [-p <pri>] [device]\n";
#else
	static	char usagemsg[] =
	    "usage: %s [-k] [-A|-a|-c|-l|-s] [-p <pri>] [device]\n";
#endif /* SWAP_OFF_WORKS */

	fprintf(stderr, usagemsg, __progname);
	exit(1);
}
