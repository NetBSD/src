/*	$NetBSD: swap.c,v 1.1.2.2.2.1 1997/05/06 14:30:51 mrg Exp $	*/

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
 *	-A		add all devices listed as `sw' in /etc/fstab	XXX
 *	-a <dev>	add this device
 *	-d <dev>	remove this swap device (not supported yet)
 *	-l		list swap devices
 *	-k		use kilobytes	XXX
 *	-p <pri>	use this priority
 */

#include <sys/types.h>

#include <vm/vm_swap.h>

#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fstab.h>

int	Aflag;
int	aflag;
#ifdef SWAP_OFF_WORKS
int	dflag;
#endif
int	lflag;
int	kflag;
int	pri;		/* uses 0 as default pri */
char	*path;

static	void list_swap __P((void));
static	void add_swap __P((char *));
#ifdef SWAP_OFF_WORKS
static	void del_swap __P((char *));
#endif /* SWAP_OFF_WORKS */
static	void do_fstab __P((void));
static	void usage __P((void));

#define DEBUG_SWAPON 1

int
main(argc, argv)
	int	argc;
	char	*argv[];
{
	int	c;
	int	am_swapon = 0;
#ifdef SWAP_OFF_WORKS
	int	swapoff = 0;
	static	char getoptstr[] = "Aadlkp:";
#else
	static	char getoptstr[] = "Aalkp:";
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
#ifdef SWAP_OFF_WORKS
			if (dflag) {
				warn("-a and -d are mutually exclusive\n");
				usage();
			}
#endif /* SWAP_OFF_WORKS */
			if (am_swapon)
				Aflag = 1;
			else
				aflag = 1;
			break;
#ifdef SWAP_OFF_WORKS
		case 'd':
			if (aflag) {
				warn("-d and -a are mutually exclusive\n");
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
			pri = atoi(optarg);
			break;
		}
	}
	/* SWAP_OFF_WORKS */
	if (!aflag && /* !dflag && */ !lflag)
		usage();

	argv += optind;
	if (!*argv && !lflag)
		usage();
	/* SWAP_OFF_WORKS */
	if (pri && !aflag /* && !dflag */ )
		usage();

	if (lflag)
		list_swap();
	else if (aflag) {
		path = argv[0];
		add_swap(path);
#ifdef SWAP_OFF_WORKS
	} else if (dflag) {
		path = argv[0];
		del_swap(path);
#endif /* SWAP_OFF_WORKS */
	} else if (Aflag) {
		do_fstab();
	}
	exit(0);
}

void
list_swap()
{
	int rnswap, nswap = swapon(SWAP_NSWAP, 0, 0);
	struct swapent *sep;
	struct swapinfo *sip;

	if (nswap < 1)
		errx(1, "no swap devices configured");

#ifdef DEBUG_SWAPON
	fprintf(stderr, "SWAP_NSWAP returned %d\n", nswap);
#endif
	sip = (struct swapinfo *)malloc(sizeof(int) + nswap * sizeof *sep);
	sip->si_num = nswap;
	rnswap = swapon(SWAP_STATS, (void *)sip, nswap);
#ifdef DEBUG_SWAPON
	fprintf(stderr, "SWAPSTATS returned %d\n", rnswap);
#endif
	if (nswap < 0)
		errx(1, "SWAP_STATS");
	if (nswap != rnswap)
		warnx("SWAP_STATS gave different value than SWAP_NSWAP");

#if 0
	/*
	 * XXX write me.  use kflag and BLOCKSIZE to determine size??  how
	 * does df do it?  it uses getbsize(3) ...
	 */
	for (sep = sip->si_ent; *sep; sep++)
		printf("%s\n", sep->se_name);
#endif
}

/*
 * add_swap:  add the pathname to the list of swap devices.
 */
void
add_swap(path)
	char *path;
{

	if (swapon(SWAP_ON, path, pri) < 0)
		warn("swapon on: %s", path);
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

	if (swapon(SWAP_OFF, path, pri) < 0)
		warn("swapon off: %s", path);
}
#endif /* SWAP_OFF_WORKS */

void
do_fstab()
{
	struct	fstab *fp;
	char	*s;
	long	priority;

#define PRIORITYEQ	"priority="
	while (fp = getfsent()) {
		if (strcmp(fp->fs_type, "sw") == 0) {
			if (s = strstr(fp->fs_mntops, PRIORITYEQ)) {
				s += sizeof(PRIORITYEQ);
				pri = atol(s);
			} else
				priority = pri;
			if (swapon(SWAP_ON, fp->fs_spec, (int)priority) < 0) {
				warn("swap_on all: %s", path);
			}
		}
	}
}

void
usage()
{
	extern char *__progname;
#ifdef SWAP_OFF_WORKS
	static	char usagemsg[] = "usage: %s [-k] [-A|-a|-d|-l] [-p <pri>] [device]\n";
#else
	static	char usagemsg[] = "usage: %s [-k] [-A|-a|-l] [-p <pri>] [device]\n";
#endif /* SWAP_OFF_WORKS */

	fprintf(stderr, usagemsg, __progname);
}
