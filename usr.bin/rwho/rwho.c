/*	$NetBSD: rwho.c,v 1.16.12.1 2008/01/09 02:00:53 matt Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)rwho.c	8.1 (Berkeley) 6/6/93";*/
__RCSID("$NetBSD: rwho.c,v 1.16.12.1 2008/01/09 02:00:53 matt Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>

#include <protocols/rwhod.h>

#include <dirent.h>
#include <err.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utmp.h>
#include <sysexits.h>

static void usage(void) __dead;
static int utmpcmp(const void *, const void *);

/* 
 * this macro should be shared with ruptime.
 */
#define	DOWN(w,now)	((now) - (w).wd_recvtime > 11 * 60)
#define WHDRSIZE	(sizeof(struct whod) - \
    sizeof (((struct whod *)NULL)->wd_we))

struct	my_utmp {
	char	myhost[MAXHOSTNAMELEN];
	int	myidle;
	struct	outmp myutmp;
};

int
main(int argc, char **argv)
{
	DIR *dirp;
	struct dirent *dp;
	struct whoent *we;
	struct whod wd;
	struct my_utmp *mp, *start;
	time_t now;
	int ch;
	size_t width, n, i, nhosts, nusers, namount;
	int aflg, qflg, hflg;

	setprogname(argv[0]);
	(void)setlocale(LC_TIME, "");
	aflg = nusers = nhosts = qflg = hflg = 0;
	namount = 40;
	start = NULL;
	now = 0;

	while ((ch = getopt(argc, argv, "aHq")) != -1)
		switch(ch) {
		case 'a':
			aflg = 1;
			break;
		case 'q':
			qflg = 1;
			break;
		case 'H':
			hflg = 1;
			break;
		default:
			usage();
		}

	if(optind != argc)
		usage();

	if (qflg) {
		aflg = 0;
		hflg = 0;
	}

	if (chdir(_PATH_RWHODIR) || (dirp = opendir(".")) == NULL)
		err(EXIT_FAILURE, "Cannot access `%s'", _PATH_RWHODIR);

	(void)time(&now);

	if ((start = malloc(sizeof(*start) * namount)) == NULL)
		err(EXIT_FAILURE, "malloc");

	while ((dp = readdir(dirp)) != NULL) {
		int f;
		ssize_t cc;

		if (dp->d_ino == 0 || strncmp(dp->d_name, "whod.", 5))
			continue;

		f = open(dp->d_name, O_RDONLY);
		if (f < 0)
			continue;
		cc = read(f, &wd, sizeof (wd));
		if (cc < WHDRSIZE) {
			(void)close(f);
			continue;
		}
		nhosts++;
		if (DOWN(wd, now)) {
			(void)close(f);
			continue;
		}
		cc -= WHDRSIZE;
		we = wd.wd_we;
		for (n = cc / sizeof (struct whoent); n > 0; n--) {
			if (aflg == 0 && we->we_idle >= 60*60) {
				we++;
				continue;
			}

			if (nusers == namount) {
				namount = namount + 40;
				if ((start = realloc(start,
				    sizeof(struct my_utmp) * namount)) == NULL)
					err(1, "realloc");
			}

			mp = start + nusers;
	
			mp->myutmp = we->we_utmp;
			mp->myidle = we->we_idle;
			(void)strcpy(mp->myhost, wd.wd_hostname);
			nusers++; we++; mp++;
		}
		(void)close(f);
	}

	if (nhosts == 0)
		errx(EX_OK, "No hosts in `%s'.", _PATH_RWHODIR);

	mp = start;
	qsort(start, nusers, sizeof(*start), utmpcmp);
	width = 0;
	for (i = 0; i < nusers; i++) {
		size_t j = strlen(mp->myhost) + 1 + strlen(mp->myutmp.out_line);
		if (j > width)
			width = j;
		mp++;
	}
	mp = start;
	if (hflg) {
		(void)printf("%-*.*s %-*s %-12s %-*s\n", UT_NAMESIZE,
		    UT_NAMESIZE, "USER", (int)width, "LINE", "WHEN",
		    (int)width, "IDLE");
	}
	for (i = 0; i < nusers; i++) {
		char buf[BUFSIZ], cbuf[80];
		time_t t;

		if (qflg) {
			(void)printf("%-*.*s\n", UT_NAMESIZE, UT_NAMESIZE,
			    mp->myutmp.out_name);
			mp++;
			continue;
		}
		t = mp->myutmp.out_time;
		(void)strftime(cbuf, sizeof(cbuf), "%c", localtime(&t));
		(void)snprintf(buf, sizeof(buf), "%s:%s", mp->myhost,
		    mp->myutmp.out_line);
		(void)printf("%-*.*s %-*s %.12s", UT_NAMESIZE, UT_NAMESIZE,
		    mp->myutmp.out_name, (int)width, buf, cbuf + 4);
		mp->myidle /= 60;
		if (mp->myidle) {
			if (aflg) {
				if (mp->myidle >= 100 * 60)
					mp->myidle = 100 * 60 - 1;
				if (mp->myidle >= 60)
					(void)printf(" %2d", mp->myidle / 60);
				else
					(void)printf("   ");
			} else
				(void)printf(" ");
			(void)printf(":%02d", mp->myidle % 60);
		}
		(void)printf("\n");
		mp++;
	}

	if (qflg)
		(void)printf("# users = %zd\n", nusers);

	return EX_OK;
}

static int
utmpcmp(const void *v1, const void *v2)
{
	const struct my_utmp *u1, *u2;
	int rc;

	u1 = v1;
	u2 = v2;
	rc = strncmp(u1->myutmp.out_name, u2->myutmp.out_name, 8);
	if (rc)
		return rc;
	rc = strcmp(u1->myhost, u2->myhost);
	if (rc)
		return rc;
	return strncmp(u1->myutmp.out_line, u2->myutmp.out_line, 8);
}

static void
usage(void)
{
	(void)fprintf(stderr, "Usage: %s [-aHq]\n", getprogname());
	exit(1);
}
