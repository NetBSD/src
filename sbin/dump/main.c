/*	$NetBSD: main.c,v 1.28 2001/05/07 21:17:48 tron Exp $	*/

/*-
 * Copyright (c) 1980, 1991, 1993, 1994
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

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.6 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: main.c,v 1.28 2001/05/07 21:17:48 tron Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>

#ifdef sunos
#include <sys/vnode.h>

#include <ufs/inode.h>
#include <ufs/fs.h>
#else
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#endif

#include <protocols/dumprestore.h>

#include <ctype.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dump.h"
#include "pathnames.h"

int	notify;			/* notify operator flag */
int	blockswritten;		/* number of blocks written on current tape */
int	tapeno;			/* current tape number */
int	density;		/* density in bytes/0.1" */
int	ntrec = NTREC;		/* # tape blocks in each tape record */
int	cartridge;		/* Assume non-cartridge tape */
long	dev_bsize = 1;		/* recalculated below */
long	blocksperfile;		/* output blocks per file */
char	*host;			/* remote host (if any) */
int	readcache = -1;		/* read cache size (in readblksize blks) */
int	readblksize = 32 * 1024; /* read block size */

int	main __P((int, char *[]));
static long numarg __P((char *, long, long));
static void obsolete __P((int *, char **[]));
static void usage __P((void));

int
main(argc, argv)
	int argc;
	char *argv[];
{
	ino_t ino;
	int dirty; 
	struct dinode *dp;
	struct	fstab *dt;
	char *map;
	int ch;
	int i, anydirskipped, bflag = 0, Tflag = 0, honorlevel = 1;
	ino_t maxino;
	time_t tnow, date;
	int dirlist;
	char *toplevel;
	int just_estimate = 0;
	char labelstr[LBLSIZE];

	spcl.c_date = 0;
	(void)time((time_t *)&spcl.c_date);

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	if ((tape = getenv("TAPE")) == NULL)
		tape = _PATH_DEFTAPE;
	dumpdates = _PATH_DUMPDATES;
	temp = _PATH_DTMP;
	strcpy(labelstr, "none");	/* XXX safe strcpy. */
	if (TP_BSIZE / DEV_BSIZE == 0 || TP_BSIZE % DEV_BSIZE != 0)
		quit("TP_BSIZE must be a multiple of DEV_BSIZE\n");
	level = '0';

	if (argc < 2)
		usage();

	obsolete(&argc, &argv);
	while ((ch = getopt(argc, argv,
	    "0123456789B:b:cd:ef:h:k:L:nr:s:ST:uWw")) != -1)
		switch (ch) {
		/* dump level */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			level = ch;
			break;

		case 'B':		/* blocks per output file */
			blocksperfile = numarg("blocks per file", 1L, 0L);
			break;

		case 'b':		/* blocks per tape write */
			ntrec = numarg("blocks per write", 1L, 1000L);
			bflag = 1;
			break;

		case 'c':		/* Tape is cart. not 9-track */
			cartridge = 1;
			break;

		case 'd':		/* density, in bits per inch */
			density = numarg("density", 10L, 327670L) / 10;
			if (density >= 625 && !bflag)
				ntrec = HIGHDENSITYTREC;
			break;

		case 'e':	/* eject full tapes */
			eflag = 1;
			break;

		case 'f':		/* output file */
			tape = optarg;
			break;

		case 'h':
			honorlevel = numarg("honor level", 0L, 10L);
			break;

		case 'k':
			readblksize = numarg("read block size", 0, 64) * 1024;
			break;

		case 'L':
			/*
			 * Note that although there are LBLSIZE characters,
			 * the last must be '\0', so the limit on strlen()
			 * is really LBLSIZE-1.
			 */
			strncpy(labelstr, optarg, LBLSIZE);
			labelstr[LBLSIZE-1] = '\0';
			if (strlen(optarg) > LBLSIZE-1) {
				msg(
		"WARNING Label `%s' is larger than limit of %d characters.\n",
				    optarg, LBLSIZE-1);
				msg("WARNING: Using truncated label `%s'.\n",
				    labelstr);
			}
			break;
		case 'n':		/* notify operators */
			notify = 1;
			break;

		case 'r':		/* read cache size */
			readcache = numarg("read cache size", 0, 512);
			break;
		
		case 's':		/* tape size, feet */
			tsize = numarg("tape size", 1L, 0L) * 12 * 10;
			break;

		case 'S':		/* exit after estimating # of tapes */
			just_estimate = 1;
			break;

		case 'T':		/* time of last dump */
			spcl.c_ddate = unctime(optarg);
			if (spcl.c_ddate < 0) {
				(void)fprintf(stderr, "bad time \"%s\"\n",
				    optarg);
				exit(X_ABORT);
			}
			Tflag = 1;
			lastlevel = '?';
			break;

		case 'u':		/* update /etc/dumpdates */
			uflag = 1;
			break;

		case 'W':		/* what to do */
		case 'w':
			lastdump(ch);
			exit(0);	/* do nothing else */

		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		(void)fprintf(stderr, "Must specify disk or filesystem\n");
		exit(X_ABORT);
	}

	/*
	 *	determine if disk is a subdirectory, and setup appropriately
	 */
	dirlist = 0;
	toplevel = NULL;
	for (i = 0; i < argc; i++) {
		struct stat sb;
		struct statfs fsbuf;

		if (lstat(argv[i], &sb) == -1) {
			msg("Cannot lstat %s: %s\n", argv[i], strerror(errno));
			exit(X_ABORT);
		}
		if (!S_ISDIR(sb.st_mode) && !S_ISREG(sb.st_mode))
			break;
		if (statfs(argv[i], &fsbuf) == -1) {
			msg("Cannot statfs %s: %s\n", argv[i], strerror(errno));
			exit(X_ABORT);
		}
		if (strcmp(argv[i], fsbuf.f_mntonname) == 0) {
			if (dirlist != 0) {
				msg("Can't dump a mountpoint and a filelist\n");
				exit(X_ABORT);
			}
			break;		/* exit if sole mountpoint */
		}
		if (!disk) {
			if ((toplevel = strdup(fsbuf.f_mntonname)) == NULL) {
				msg("Cannot malloc diskname\n");
				exit(X_ABORT);
			}
			disk = toplevel;
			if (uflag) {
				msg("Ignoring u flag for subdir dump\n");
				uflag = 0;
			}
			if (level > '0') {
				msg("Subdir dump is done at level 0\n");
				level = '0';
			}
			msg("Dumping sub files/directories from %s\n", disk);
		} else {
			if (strcmp(disk, fsbuf.f_mntonname) != 0) {
				msg("%s is not on %s\n", argv[i], disk);
				exit(X_ABORT);
			}
		}
		msg("Dumping file/directory %s\n", argv[i]);
		dirlist++;
	}
	if (dirlist == 0) {
		disk = *argv++;
		if (argc != 1) {
			(void)fprintf(stderr, "Excess arguments to dump:");
			while (--argc)
				(void)fprintf(stderr, " %s", *argv++);
			(void)fprintf(stderr, "\n");
			exit(X_ABORT);
		}
	}
	if (Tflag && uflag) {
	        (void)fprintf(stderr,
		    "You cannot use the T and u flags together.\n");
		exit(X_ABORT);
	}
	if (strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}

	if (blocksperfile)
		blocksperfile = blocksperfile / ntrec * ntrec; /* round down */
	else {
		/*
		 * Determine how to default tape size and density
		 *
		 *         	density				tape size
		 * 9-track	1600 bpi (160 bytes/.1")	2300 ft.
		 * 9-track	6250 bpi (625 bytes/.1")	2300 ft.
		 * cartridge	8000 bpi (100 bytes/.1")	1700 ft.
		 *						(450*4 - slop)
		 */
		if (density == 0)
			density = cartridge ? 100 : 160;
		if (tsize == 0)
			tsize = cartridge ? 1700L*120L : 2300L*120L;
	}

	if (strchr(tape, ':')) {
		host = tape;
		tape = strchr(host, ':');
		*tape++ = '\0';
#ifdef RDUMP
		if (rmthost(host) == 0)
			exit(X_ABORT);
#else
		(void)fprintf(stderr, "remote dump not enabled\n");
		exit(X_ABORT);
#endif
	}

	if (signal(SIGHUP, SIG_IGN) != SIG_IGN)
		signal(SIGHUP, sig);
	if (signal(SIGTRAP, SIG_IGN) != SIG_IGN)
		signal(SIGTRAP, sig);
	if (signal(SIGFPE, SIG_IGN) != SIG_IGN)
		signal(SIGFPE, sig);
	if (signal(SIGBUS, SIG_IGN) != SIG_IGN)
		signal(SIGBUS, sig);
	if (signal(SIGSEGV, SIG_IGN) != SIG_IGN)
		signal(SIGSEGV, sig);
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, sig);
	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	set_operators();	/* /etc/group snarfed */
	getfstab();		/* /etc/fstab snarfed */

	/*
	 *	disk can be either the full special file name,
	 *	the suffix of the special file name,
	 *	the special name missing the leading '/',
	 *	the file system name with or without the leading '/'.
	 */
	dt = fstabsearch(disk);
	if (dt != NULL) {
		disk = rawname(dt->fs_spec);
		(void)strncpy(spcl.c_dev, dt->fs_spec, NAMELEN);
		if (dirlist != 0)
			(void)snprintf(spcl.c_filesys, NAMELEN,
			    "a subset of %s", dt->fs_file);
		else
			(void)strncpy(spcl.c_filesys, dt->fs_file, NAMELEN);
	} else {
		(void)strncpy(spcl.c_dev, disk, NAMELEN);
		(void)strncpy(spcl.c_filesys, "an unlisted file system",
		    NAMELEN);
	}
	(void)strncpy(spcl.c_label, labelstr, sizeof(spcl.c_label) - 1);
	(void)gethostname(spcl.c_host, NAMELEN);
	spcl.c_host[sizeof(spcl.c_host) - 1] = '\0';

	if ((diskfd = open(disk, O_RDONLY)) < 0) {
		msg("Cannot open %s\n", disk);
		exit(X_ABORT);
	}
	sync();

	needswap = fs_read_sblock(sblock_buf);

	spcl.c_level = iswap32(level - '0');
	spcl.c_type = iswap32(TS_TAPE);
	spcl.c_date = iswap32(spcl.c_date);
	spcl.c_ddate = iswap32(spcl.c_ddate);
	if (!Tflag)
	        getdumptime();		/* /etc/dumpdates snarfed */

	date = iswap32(spcl.c_date);
	msg("Date of this level %c dump: %s", level,
		spcl.c_date == 0 ? "the epoch\n" : ctime(&date));
	date = iswap32(spcl.c_ddate);
 	msg("Date of last level %c dump: %s", lastlevel,
		spcl.c_ddate == 0 ? "the epoch\n" : ctime(&date));
	msg("Dumping ");
	if (dt != NULL && dirlist != 0)
		msgtail("a subset of ");
	msgtail("%s (%s) ", disk, spcl.c_filesys);
	if (host)
		msgtail("to %s on host %s\n", tape, host);
	else
		msgtail("to %s\n", tape);
	msg("Label: %s\n", labelstr);

	ufsib = fs_parametrize();

	dev_bshift = ffs(dev_bsize) - 1;
	if (dev_bsize != (1 << dev_bshift))
		quit("dev_bsize (%ld) is not a power of 2", dev_bsize);
	tp_bshift = ffs(TP_BSIZE) - 1;
	if (TP_BSIZE != (1 << tp_bshift))
		quit("TP_BSIZE (%d) is not a power of 2", TP_BSIZE);
	maxino = fs_maxino();
	mapsize = roundup(howmany(maxino, NBBY), TP_BSIZE);
	usedinomap = (char *)calloc((unsigned) mapsize, sizeof(char));
	dumpdirmap = (char *)calloc((unsigned) mapsize, sizeof(char));
	dumpinomap = (char *)calloc((unsigned) mapsize, sizeof(char));
	tapesize = 3 * (howmany(mapsize * sizeof(char), TP_BSIZE) + 1);

	nonodump = iswap32(spcl.c_level) < honorlevel;

	initcache(readcache, readblksize);
	
	(void)signal(SIGINFO, statussig);

	msg("mapping (Pass I) [regular files]\n");
	anydirskipped = mapfiles(maxino, &tapesize, toplevel,
	    (dirlist ? argv : NULL));

	msg("mapping (Pass II) [directories]\n");
	while (anydirskipped) {
		anydirskipped = mapdirs(maxino, &tapesize);
	}

	if (pipeout) {
		tapesize += 10;	/* 10 trailer blocks */
		msg("estimated %ld tape blocks.\n", tapesize);
	} else {
		double fetapes;

		if (blocksperfile)
			fetapes = (double) tapesize / blocksperfile;
		else if (cartridge) {
			/* Estimate number of tapes, assuming streaming stops at
			   the end of each block written, and not in mid-block.
			   Assume no erroneous blocks; this can be compensated
			   for with an artificially low tape size. */
			fetapes = 
			(	  tapesize	/* blocks */
				* TP_BSIZE	/* bytes/block */
				* (1.0/density)	/* 0.1" / byte */
			  +
				  tapesize	/* blocks */
				* (1.0/ntrec)	/* streaming-stops per block */
				* 15.48		/* 0.1" / streaming-stop */
			) * (1.0 / tsize );	/* tape / 0.1" */
		} else {
			/* Estimate number of tapes, for old fashioned 9-track
			   tape */
			int tenthsperirg = (density == 625) ? 3 : 7;
			fetapes =
			(	  tapesize	/* blocks */
				* TP_BSIZE	/* bytes / block */
				* (1.0/density)	/* 0.1" / byte */
			  +
				  tapesize	/* blocks */
				* (1.0/ntrec)	/* IRG's / block */
				* tenthsperirg	/* 0.1" / IRG */
			) * (1.0 / tsize );	/* tape / 0.1" */
		}
		etapes = fetapes;		/* truncating assignment */
		etapes++;
		/* count the dumped inodes map on each additional tape */
		tapesize += (etapes - 1) *
			(howmany(mapsize * sizeof(char), TP_BSIZE) + 1);
		tapesize += etapes + 10;	/* headers + 10 trailer blks */
		msg("estimated %ld tape blocks on %3.2f tape(s).\n",
		    tapesize, fetapes);
	}
	/*
	 * If the user only wants an estimate of the number of
	 * tapes, exit now.
	 */
	if (just_estimate)
		exit(0);

	/*
	 * Allocate tape buffer.
	 */
	if (!alloctape())
		quit("can't allocate tape buffers - try a smaller blocking factor.\n");

	startnewtape(1);
	(void)time((time_t *)&(tstart_writing));
	xferrate = 0;
	dumpmap(usedinomap, TS_CLRI, maxino - 1);

	msg("dumping (Pass III) [directories]\n");
	dirty = 0;		/* XXX just to get gcc to shut up */
	for (map = dumpdirmap, ino = 1; ino < maxino; ino++) {
		if (((ino - 1) % NBBY) == 0)	/* map is offset by 1 */
			dirty = *map++;
		else
			dirty >>= 1;
		if ((dirty & 1) == 0)
			continue;
		/*
		 * Skip directory inodes deleted and maybe reallocated
		 */
		dp = getino(ino);
		if ((dp->di_mode & IFMT) != IFDIR)
			continue;
		(void)dumpino(dp, ino);
	}

	msg("dumping (Pass IV) [regular files]\n");
	for (map = dumpinomap, ino = 1; ino < maxino; ino++) {
		int mode;

		if (((ino - 1) % NBBY) == 0)	/* map is offset by 1 */
			dirty = *map++;
		else
			dirty >>= 1;
		if ((dirty & 1) == 0)
			continue;
		/*
		 * Skip inodes deleted and reallocated as directories.
		 */
		dp = getino(ino);
		mode = dp->di_mode & IFMT;
		if (mode == IFDIR)
			continue;
		(void)dumpino(dp, ino);
	}

	spcl.c_type = iswap32(TS_END);
	for (i = 0; i < ntrec; i++)
		writeheader(maxino - 1);
	if (pipeout)
		msg("%d tape blocks\n",iswap32(spcl.c_tapea));
	else
		msg("%d tape blocks on %d volume%s\n",
		    iswap32(spcl.c_tapea), iswap32(spcl.c_volume),
		    (iswap32(spcl.c_volume) == 1) ? "" : "s");
	tnow = do_stats();
	date = iswap32(spcl.c_date);
	msg("Date of this level %c dump: %s", level,
		spcl.c_date == 0 ? "the epoch\n" : ctime(&date));
	msg("Date this dump completed:  %s", ctime(&tnow));
	msg("Average transfer rate: %d KB/s\n", xferrate / tapeno);
	putdumptime();
	trewind();
	broadcast("DUMP IS DONE!\7\7\n");
	msg("DUMP IS DONE\n");
	Exit(X_FINOK);
	/* NOTREACHED */
	exit(X_FINOK);		/* XXX: to satisfy gcc */
}

static void
usage()
{

	(void)fprintf(stderr, "%s\n%s\n%s\n%s\n",
"usage: dump [-0123456789cnu] [-B records] [-b blocksize] [-d density]",
"            [-f file] [-h level] [-k read block size] [-L label]",
"            [-r read cache size] [-s feet] [-T date] filesystem",
"       dump [-W | -w]");
	exit(1);
}

/*
 * Pick up a numeric argument.  It must be nonnegative and in the given
 * range (except that a vmax of 0 means unlimited).
 */
static long
numarg(meaning, vmin, vmax)
	char *meaning;
	long vmin, vmax;
{
	char *p;
	long val;

	val = strtol(optarg, &p, 10);
	if (*p)
		errx(1, "illegal %s -- %s", meaning, optarg);
	if (val < vmin || (vmax && val > vmax))
		errx(1, "%s must be between %ld and %ld", meaning, vmin, vmax);
	return (val);
}

void
sig(signo)
	int signo;
{
	switch(signo) {
	case SIGALRM:
	case SIGBUS:
	case SIGFPE:
	case SIGHUP:
	case SIGTERM:
	case SIGTRAP:
		if (pipeout)
			quit("Signal on pipe: cannot recover\n");
		msg("Rewriting attempted as response to unknown signal.\n");
		(void)fflush(stderr);
		(void)fflush(stdout);
		close_rewind();
		exit(X_REWRITE);
		/* NOTREACHED */
	case SIGSEGV:
		msg("SIGSEGV: ABORTING!\n");
		(void)signal(SIGSEGV, SIG_DFL);
		(void)kill(0, SIGSEGV);
		/* NOTREACHED */
	}
}

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[MAXPATHLEN];
	char *dp = strrchr(cp, '/');

	if (dp == NULL)
		return (NULL);
	*dp = '\0';
	(void)snprintf(rawbuf, sizeof rawbuf, "%s/r%s", cp, dp + 1);
	*dp = '/';
	return (rawbuf);
}

/*
 * obsolete --
 *	Change set of key letters and ordered arguments into something
 *	getopt(3) will like.
 */
static void
obsolete(argcp, argvp)
	int *argcp;
	char **argvp[];
{
	int argc, flags;
	char *ap, **argv, *flagsp, **nargv, *p;

	/* Setup. */
	argv = *argvp;
	argc = *argcp;

	/* Return if no arguments or first argument has leading dash. */
	ap = argv[1];
	if (argc == 1 || *ap == '-')
		return;

	/* Allocate space for new arguments. */
	if ((*argvp = nargv = malloc((argc + 1) * sizeof(char *))) == NULL ||
	    (p = flagsp = malloc(strlen(ap) + 2)) == NULL)
		err(1, "malloc");

	*nargv++ = *argv;
	argv += 2;

	for (flags = 0; *ap; ++ap) {
		switch (*ap) {
		case 'B':
		case 'b':
		case 'd':
		case 'f':
		case 'h':
		case 's':
		case 'T':
			if (*argv == NULL) {
				warnx("option requires an argument -- %c", *ap);
				usage();
			}
			if ((nargv[0] = malloc(strlen(*argv) + 2 + 1)) == NULL)
				err(1, "malloc");
			nargv[0][0] = '-';
			nargv[0][1] = *ap;
			(void)strcpy(&nargv[0][2], *argv); /* XXX safe strcpy */
			++argv;
			++nargv;
			break;
		default:
			if (!flags) {
				*p++ = '-';
				flags = 1;
			}
			*p++ = *ap;
			break;
		}
	}

	/* Terminate flags. */
	if (flags) {
		*p = '\0';
		*nargv++ = flagsp;
	}

	/* Copy remaining arguments. */
	while ((*nargv++ = *argv++) != NULL)
		;

	/* Update argument count. */
	*argcp = nargv - *argvp - 1;
}
