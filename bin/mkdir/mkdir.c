/*
 * Copyright (c) 1983 Regents of the University of California.
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
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)mkdir.c	5.7 (Berkeley) 5/31/90";*/
static char rcsid[] = "$Id: mkdir.c,v 1.9 1994/04/29 00:13:54 jtc Exp $";
#endif /* not lint */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <err.h>

main(argc, argv)
	int argc;
	char **argv;
{
	int ch, exitval, pflag;
	void *set;
	mode_t mode, dir_mode;

	setlocale(LC_ALL, "");

	/* default file mode is a=rwx (777) with selected permissions
	   removed in accordance with the file mode creation mask.
	   For intermediate path name components, the mode is the default
	   modified by u+wx so that the subdirectories can always be 
	   created. */
	mode = 0777 & ~umask(0);
	dir_mode = mode | S_IWUSR | S_IXUSR;

	pflag = 0;
	while ((ch = getopt(argc, argv, "pm:")) != -1)
		switch(ch) {
		case 'p':
			pflag = 1;
			break;
		case 'm':
			if ((set = setmode(optarg)) == NULL) {
				errx(1, "invalid file mode.");
				/* NOTREACHED */
			}
			mode = getmode (set, S_IRWXU | S_IRWXG | S_IRWXO);
			break;
		case '?':
		default:
			usage();
		}

	if (!*(argv += optind))
		usage();
	
	for (exitval = 0; *argv; ++argv) {
		register char *slash;

		/* delete trailing slashes */
		slash = strrchr(*argv, '\0');
		while (--slash > *argv && *slash == '/')
			*slash = '\0';

		if (pflag) {
			exitval |= mkpath(*argv, mode, dir_mode);
		} else if (mkdir(*argv, mode) < 0) {
			warn("%s", *argv);
			exitval = 1;
		}
	}
	exit(exitval);
}

/*
 * mkpath -- create directories.  
 *	path     - path
 *	mode     - file mode of terminal directory
 *	dir_mode - file mode of intermediate directories
 */
mkpath(path, mode, dir_mode)
	char *path;
	mode_t mode;
	mode_t dir_mode;
{
	struct stat sb;
	register char *slash;

	/* skip leading slashes */
	slash = path;
	while (*slash == '/')
		slash++;

	while ((slash = strchr(slash, '/')) != NULL) {
		*slash = '\0';

		if (stat(path, &sb)) {
			if (errno != ENOENT || mkdir(path, dir_mode)) {
				warn("%s", path);
				return 1;
			}
		} else if (!S_ISDIR(sb.st_mode)) {
		        warnx("%s: %s", path, strerror(ENOTDIR));
			return 1;
		}
		    
		/* skip multiple slashes */
		*slash++ = '/';
		while (*slash == '/')
			slash++;
	}

	if (mkdir (path, mode)) {
		warn("%s", path);
		return 1;
	}

	return(0);
}

usage()
{
	(void)fprintf(stderr, "usage: mkdir [-p] [-m mode] dirname ...\n");
	exit(1);
}
