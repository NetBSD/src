/*
 * Copyright (c) 1980 Regents of the University of California.
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
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)lastcomm.c	5.11 (Berkeley) 6/1/90";*/
static char rcsid[] = "$Id: lastcomm.c,v 1.5 1994/03/23 04:37:40 cgd Exp $";
#endif /* not lint */

/*
 * last command
 */
#include <sys/param.h>
#include <sys/acct.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <utmp.h>
#include <struct.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include "pathnames.h"

time_t	expand();
char	*flagbits();
char	*getdev();

extern char	*user_from_uid __P((unsigned long, int));

main(argc, argv)
	int argc;
	char *argv[];
{
	register char *p;
	struct acct ab;
	struct stat sb;
	FILE *fp;
	off_t size;
	time_t x;
	int ch;
	char *acctfile;

	acctfile = _PATH_ACCT;
	while ((ch = getopt(argc, argv, "f:")) != EOF)
		switch((char)ch) {
		case 'f':
			acctfile = optarg;
			break;
		case '?':
		default:
			fputs("lastcomm [ -f file ]\n", stderr);
			exit(1);
		}
	argv += optind;

	/* Open the file. */
	if ((fp = fopen(acctfile, "r")) == NULL || fstat(fileno(fp), &sb))
		err(1, "%s", acctfile);

	/*
	 * Round off to integral number of accounting records, probably
	 * not necessary, but it doesn't hurt.
	 */
	size = sb.st_size - sb.st_size % sizeof(struct acct);

	/* Check if any records to display. */ 
	if (size < sizeof(struct acct))
		exit(0); 

	/*
	 * Seek to before the last entry in the file; use lseek(2) in case
	 * the file is bigger than a "long".
	 */
	size -= sizeof(struct acct);
	if (lseek(fileno(fp), size, SEEK_SET) == -1)
		err(1, "%s", acctfile);

	for (;;) {
		if (fread(&ab, sizeof(struct acct), 1, fp) != 1)
			err(1, "%s", acctfile);

		if (fseek(fp, 2 * -(long)sizeof(struct acct), SEEK_CUR) == -1)
			err(1, "%s", acctfile);

		if (size == 0)
			break;
		size -= sizeof(struct acct);

		if (ab.ac_comm[0] == '\0') {
			ab.ac_comm[0] = '?';
			ab.ac_comm[1] = '\0';
		} else
			for (p = &ab.ac_comm[0];
			    p < &ab.ac_comm[fldsiz(acct, ac_comm)] && *p; ++p)
				if (!isprint(*p))
					*p = '?';
		if (*argv && !ok(argv, &ab))
			continue;

		x = expand(ab.ac_utime) + expand(ab.ac_stime);
		printf("%-*.*s %s %-*s %-*s %6.2f secs %.16s\n",
			fldsiz(acct, ac_comm), fldsiz(acct, ac_comm),
			ab.ac_comm, flagbits(ab.ac_flag),
			UT_NAMESIZE, user_from_uid(ab.ac_uid, 0),
			UT_LINESIZE, getdev(ab.ac_tty),
			x / (double)AHZ, ctime(&ab.ac_btime));
	}
	exit(0);
}

time_t
expand (t)
	unsigned t;
{
	register time_t nt;

	nt = t & 017777;
	t >>= 13;
	while (t) {
		t--;
		nt <<= 3;
	}
	return (nt);
}

char *
flagbits(f)
	register int f;
{
	static char flags[20];
	char *p, *strcpy();

#define	BIT(flag, ch)	if (f & flag) *p++ = ch;
	p = strcpy(flags, "-    ");
	BIT(ASU, 'S');
	BIT(AFORK, 'F');
	BIT(ACOMPAT, 'C');
	BIT(ACORE, 'D');
	BIT(AXSIG, 'X');
	return (flags);
}

ok(argv, acp)
	register char *argv[];
	register struct acct *acp;
{
	register char *cp;

	do {
		cp = user_from_uid(acp->ac_uid, 0);
		if (!strcmp(cp, *argv)) 
			return(1);
		if ((cp = getdev(acp->ac_tty)) && !strcmp(cp, *argv))
			return(1);
		if (!strncmp(acp->ac_comm, *argv, fldsiz(acct, ac_comm)))
			return(1);
	} while (*++argv);
	return(0);
}

#include <dirent.h>

#define N_DEVS		43		/* hash value for device names */
#define NDEVS		500		/* max number of file names in /dev */

struct	devhash {
	dev_t	dev_dev;
	char	dev_name [UT_LINESIZE + 1];
	struct	devhash * dev_nxt;
};
struct	devhash *dev_hash[N_DEVS];
struct	devhash *dev_chain;
#define HASH(d)	(((int) d) % N_DEVS)

setupdevs()
{
	register DIR * fd;
	register struct devhash * hashtab;
	register ndevs = NDEVS;
	struct dirent * dp;

	/*NOSTRICT*/
	hashtab = (struct devhash *)malloc(NDEVS * sizeof(struct devhash));
	if (hashtab == (struct devhash *)0) {
		fputs("No mem for dev table\n", stderr);
		return;
	}
	if ((fd = opendir(_PATH_DEV)) == NULL) {
		perror(_PATH_DEV);
		return;
	}
	while (dp = readdir(fd)) {
		if (dp->d_ino == 0)
			continue;
		if (dp->d_name[0] != 't' &&
#warning WARNING! ugly pc 'vga' console hack!
		    (strcmp(dp->d_name, "console") && strcmp(dp->d_name, "vga")))
			continue;
		(void)strncpy(hashtab->dev_name, dp->d_name, UT_LINESIZE);
		hashtab->dev_name[UT_LINESIZE] = 0;
		hashtab->dev_nxt = dev_chain;
		dev_chain = hashtab;
		hashtab++;
		if (--ndevs <= 0)
			break;
	}
	closedir(fd);
}

char *
getdev(dev)
	dev_t dev;
{
	register struct devhash *hp, *nhp;
	struct stat statb;
	char name[fldsiz(devhash, dev_name) + 6];
	static dev_t lastdev = (dev_t) -1;
	static char *lastname;
	static int init = 0;
	char *strcpy(), *strcat();

	if (dev == NODEV)
		return ("__");
	if (dev == lastdev)
		return (lastname);
	if (!init) {
		setupdevs();
		init++;
	}
	for (hp = dev_hash[HASH(dev)]; hp; hp = hp->dev_nxt)
		if (hp->dev_dev == dev) {
			lastdev = dev;
			return (lastname = hp->dev_name);
		}
	for (hp = dev_chain; hp; hp = nhp) {
		nhp = hp->dev_nxt;
		(void)strcpy(name, _PATH_DEV);
		strcat(name, hp->dev_name);
		if (stat(name, &statb) < 0)	/* name truncated usually */
			continue;
		if ((statb.st_mode & S_IFMT) != S_IFCHR)
			continue;
		hp->dev_dev = statb.st_rdev;
		hp->dev_nxt = dev_hash[HASH(hp->dev_dev)];
		dev_hash[HASH(hp->dev_dev)] = hp;
		if (hp->dev_dev == dev) {
			dev_chain = nhp;
			lastdev = dev;
			return (lastname = hp->dev_name);
		}
	}
	dev_chain = (struct devhash *) 0;
	return ("??");
}
