/*
 * Copyright (c) 1987 Regents of the University of California.
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
"@(#) Copyright (c) 1987 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
/*static char sccsid[] = "from: @(#)ln.c	4.15 (Berkeley) 2/24/91";*/
static char rcsid[] = "$Id: ln.c,v 1.8 1994/02/08 05:09:26 mycroft Exp $";
#endif /* not lint */

#include <sys/param.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <err.h>

static int	forceflag,		/* force link by removing target */
		dirflag,		/* allow hard links to directories */
		sflag,			/* symbolic, not hard, link */
		(*linkf)();		/* system link call */
static int	linkit();
static void	usage();

int
main(argc, argv)
	int argc;
	char **argv;
{
	extern int optind;
	struct stat buf;
	int ch, exitval;
	char *sourcedir;

	while ((ch = getopt(argc, argv, "fFs")) != EOF)
		switch((char)ch) {
		case 'f':
			forceflag = 1;
			break;
		case 'F':
			dirflag = 1;
			break;
		case 's':
			sflag = 1;
			break;
		case '?':
		default:
			usage();
		}

	argv += optind;
	argc -= optind;

	linkf = sflag ? symlink : link;

	switch(argc) {
	case 0:
		usage();
		/* NOTREACHED */
	case 1:				/* ln target */
		exit(linkit(argv[0], ".", 1));
		/* NOTREACHED */
	case 2:				/* ln target source */
		exit(linkit(argv[0], argv[1], 0));
		/* NOTREACHED */
	default:			/* ln target1 target2 directory */
		sourcedir = argv[argc - 1];
		if (stat(sourcedir, &buf)) {
			err(1, "%s", sourcedir);
			/* NOTREACHED */
		}
		if (!S_ISDIR(buf.st_mode))
			usage();
		for (exitval = 0; *argv != sourcedir; ++argv)
			exitval |= linkit(*argv, sourcedir, 1);
		exit(exitval);
	}
	/* NOTREACHED */
}

static int
linkit(source, target, isdir)
	char *source, *target;
	int isdir;
{
	struct stat buf;
	char path[MAXPATHLEN], *cp;

	if (!sflag) {
		/* if source doesn't exist, quit now */
		if (stat(source, &buf)) {
			warn("%s", source);
			return(1);
		}
		/* only symbolic links to directories, unless -F option used */
		if (!dirflag && S_ISDIR(buf.st_mode)) {
			warnx("%s: %s", source, strerror(EISDIR));
			return(1);
		}
	}

	/* if the target is a directory, append the source's name */
	if (isdir || !stat(target, &buf) && S_ISDIR(buf.st_mode)) {
		if (!(cp = rindex(source, '/')))
			cp = source;
		else
			++cp;
		(void)sprintf(path, "%s/%s", target, cp);
		target = path;
	}

	/* Remove existing file if -f is was specified */
	if (forceflag && unlink (target) && errno != ENOENT) {
		warn("%s", target);
		return (1);
	}

	if ((*linkf)(source, target)) {
		warn("%s", target);
		return(1);
	}

	return(0);
}

static void
usage()
{
	(void)fprintf(stderr,
	    "usage:\tln [-fs] file1 file2\n\tln [-fs] file ... directory\n");
	exit(1);
}
