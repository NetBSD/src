/*
 * Copyright (c) 1993 Winning Strategies, Inc.
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
 *      This product includes software developed by Winning Strategies, Inc.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
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
 */

#ifndef lint
static char rcsid[] = "$Id: catman.c,v 1.3 1994/01/29 01:43:13 jtc Exp $";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>
#include <err.h>

int             no_whatis = 0;
int             just_print = 0;
int             just_whatis = 0;

char            manpath[] = "/usr/share/man";
char            sections[] = "12345678lnop";
char           *mp = manpath;
char	       *sp = sections;

void            catman();
void            makewhatis();
void            usage();

int
main(argc, argv)
	int             argc;
	char          **argv;
{
	int             c;

	while ((c = getopt(argc, argv, "npwM:")) != EOF) {
		switch (c) {
		case 'n':
			no_whatis = 1;
			break;
		case 'p':
			just_print = 1;
			break;
		case 'w':
			just_whatis = 1;
			break;

		case 'M':
			mp = optarg;
			break;

		case '?':
		default:
			usage();
		}
	}

	argc -= optind;
	argv += optind;

	if (argc > 1) 
		usage ();
	if (argc == 0)
		sp = *argv;

	catman(mp, sp);

	exit(0);
}


void
catman(path, section)
	char           *path;
	char           *section;
{
	DIR            *dirp;
	struct dirent  *dp;
	struct stat     manstat;
	struct stat	catstat;
	char            mandir[PATH_MAX];
	char            catdir[PATH_MAX];
	char            manpage[PATH_MAX];
	char            catpage[PATH_MAX];
	char            sysbuf[1024];
	char           *s;
	int             formatted = 0;

	if (!just_whatis) {
		for (s = section; *s; s++) {
			sprintf(mandir, "%s/man%c", path, *s);
			sprintf(catdir, "%s/cat%c", path, *s);

			if ((dirp = opendir(mandir)) == 0) {
				warn("can't open %s", mandir);
				continue;
			}

			if (stat(catdir, &catstat) < 0) {
				if (errno != ENOENT) {
					warn("can't stat %s", catdir);
					closedir(dirp);
					continue;
				}
				if (just_print) {
					printf("mkdir %s\n", catdir);
				} else if (mkdir(catdir, 0755) < 0) {
					warn("can't create %s", catdir);
					closedir(dirp);
					return;
				}
			}

			while ((dp = readdir(dirp)) != NULL) {
				if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
					continue;

				sprintf(manpage, "%s/%s", mandir, dp->d_name);
				sprintf(catpage, "%s/%s", catdir, dp->d_name);
				if ((s = strrchr(catpage, '.')) != NULL)
					strcpy(s, ".0");

				if (stat(manpage, &manstat) < 0) {
					warn("can't stat %s", manpage);
					continue;
				}

				if (!S_ISREG(manstat.st_mode)) {
					warnx("not a regular file %s", manpage);
					continue;
				}

				if (stat(catpage, &catstat) < 0 && errno != ENOENT) {
					warn("can't stat %s", catpage);
					continue;
				}

				if (errno == ENOENT || manstat.st_mtime >= catstat.st_mtime) {
					/*
					 * manpage is out of date,
					 * reformat
					 */
					sprintf(sysbuf, "nroff -mandoc %s > %s", manpage, catpage);

					if (just_print) {
						printf("%s\n", sysbuf);
					} else {
						if (system(sysbuf) != 0) {
						}
					}
					formatted = 1;
				}
			}

			closedir(dirp);
		}
	}

	if (!no_whatis && formatted) {
		makewhatis(path);
	}
}

void
makewhatis(path)
{
	char            sysbuf[1024];

	sprintf(sysbuf, "/usr/libexec/makewhatis %s", path);
	if (just_print) {
		printf("%s\n", sysbuf);
	} else {
		if (system(sysbuf) != 0) {
		}
	}
}


void
usage()
{
	(void) fprintf(stderr, "usage: catman [-npw] [-M manpath] [sections]\n");
	exit(1);
}
