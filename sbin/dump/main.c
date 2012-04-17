/*	$NetBSD: main.c,v 1.66.6.1 2012/04/17 00:05:39 yamt Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1980, 1991, 1993, 1994\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)main.c	8.6 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: main.c,v 1.66.6.1 2012/04/17 00:05:39 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

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
#include <util.h>

#include "dump.h"
#include "pathnames.h"
#include "snapshot.h"

int	timestamp;		/* print message timestamps */
int	notify;			/* notify operator flag */
int	blockswritten;		/* number of blocks written on current tape */
int	tapeno;			/* current tape number */
int	density;		/* density in bytes/0.1" */
int	ntrec = NTREC;		/* # tape blocks in each tape record */
int	cartridge;		/* Assume non-cartridge tape */
long	dev_bsize = 1;		/* recalculated below */
long	blocksperfile;		/* output blocks per file */
const char *host;		/* remote host (if any) */
int	readcache = -1;		/* read cache size (in readblksize blks) */
int	readblksize = 32 * 1024; /* read block size */
char    default_time_string[] = "%T %Z"; /* default timestamp string */
char    *time_string = default_time_string; /* timestamp string */

static long numarg(const char *, long, long);
static void obsolete(int *, char **[]);
static void usage(void);

int
main(int argc, char *argv[])
{
	ino_t ino;
	int dirty;
	union dinode *dp;
	struct fstab *dt;
	struct statvfs *mntinfo, fsbuf;
	char *map, *cp;
	int ch;
	int i, anydirskipped, bflag = 0, Tflag = 0, Fflag = 0, honorlevel = 1;
	int snap_internal = 0;
	ino_t maxino;
	time_t tnow, date;
	int dirc;
	char *mountpoint;
	int just_estimate = 0;
	char labelstr[LBLSIZE];
	char buf[MAXPATHLEN], rbuf[MAXPATHLEN];
	char *new_time_format;
	char *snap_backup = NULL;

	spcl.c_date = 0;
	(void)time(&tnow);
	spcl.c_date = tnow;
	tzset(); /* set up timezone for strftime */
	if ((new_time_format = getenv("TIMEFORMAT")) != NULL)
		time_string = new_time_format;

	tsize = 0;	/* Default later, based on 'c' option for cart tapes */
	if ((tape = getenv("TAPE")) == NULL)
		tape = _PATH_DEFTAPE;
	dumpdates = _PATH_DUMPDATES;
	temp = _PATH_DTMP;
	strcpy(labelstr, "none");	/* XXX safe strcpy. */
	if (TP_BSIZE / DEV_BSIZE == 0 || TP_BSIZE % DEV_BSIZE != 0)
		quit("TP_BSIZE must be a multiple of DEV_BSIZE\n");
	level = '0';
	timestamp = 0;

	if (argc < 2)
		usage();

	obsolete(&argc, &argv);
	while ((ch = getopt(argc, argv,
	    "0123456789aB:b:cd:eFf:h:ik:l:L:nr:s:StT:uWwx:X")) != -1)
		switch (ch) {
		/* dump level */
		case '0': case '1': case '2': case '3': case '4':
		case '5': case '6': case '7': case '8': case '9':
			level = ch;
			break;

		case 'a':		/* `auto-size', Write to EOM. */
			unlimited = 1;
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

		case 'e':		/* eject full tapes */
			eflag = 1;
			break;

		case 'F':		/* files-to-dump is an fs image */
			Fflag = 1;
			break;

		case 'f':		/* output file */
			tape = optarg;
			break;

		case 'h':
			honorlevel = numarg("honor level", 0L, 10L);
			break;

		case 'i':	/* "true incremental" regardless level */
			level = 'i';
			trueinc = 1;
			break;

		case 'k':
			readblksize = numarg("read block size", 0, 64) * 1024;
			break;

		case 'l':		/* autoload after eject full tapes */
			eflag = 1;
			lflag = numarg("timeout (in seconds)", 1, 0);
			break;

		case 'L':
			/*
			 * Note that although there are LBLSIZE characters,
			 * the last must be '\0', so the limit on strlen()
			 * is really LBLSIZE-1.
			 */
			if (strlcpy(labelstr, optarg, sizeof(labelstr))
			    >= sizeof(labelstr)) {
				msg(
		"WARNING Label `%s' is larger than limit of %lu characters.\n",
				    optarg,
				    (unsigned long)sizeof(labelstr) - 1);
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

		case 't':
			timestamp = 1;
			break;

		case 'T':		/* time of last dump */
			spcl.c_ddate = unctime(optarg);
			if (spcl.c_ddate < 0) {
				(void)fprintf(stderr, "bad time \"%s\"\n",
				    optarg);
				exit(X_STARTUP);
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
			exit(X_FINOK);	/* do nothing else */

		case 'x':
			snap_backup = optarg;
			break;

		case 'X':
			snap_internal = 1;
			break;

		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc < 1) {
		(void)fprintf(stderr,
		    "Must specify disk or image, or file list\n");
		exit(X_STARTUP);
	}


	/*
	 *	determine if disk is a subdirectory, and setup appropriately
	 */
	getfstab();		/* /etc/fstab snarfed */
	disk = NULL;
	disk_dev = NULL;
	mountpoint = NULL;
	dirc = 0;
	for (i = 0; i < argc; i++) {
		struct stat sb;

		if (lstat(argv[i], &sb) == -1)
			quit("Cannot stat %s: %s\n", argv[i], strerror(errno));
		if (Fflag || S_ISCHR(sb.st_mode) || S_ISBLK(sb.st_mode)) {
			disk = argv[i];
 multicheck:
			if (dirc != 0)
				quit(
	"Can't dump a disk or image at the same time as a file list\n");
			break;
		}
		if ((dt = fstabsearch(argv[i])) != NULL) {
			if (getfsspecname(buf, sizeof(buf), dt->fs_spec)
			    == NULL)
				quit("%s (%s)", buf, strerror(errno));
			disk = buf;
			mountpoint = xstrdup(dt->fs_file);
			goto multicheck;
		}
		if (statvfs(argv[i], &fsbuf) == -1)
			quit("Cannot statvfs %s: %s\n", argv[i],
			    strerror(errno));
		disk = fsbuf.f_mntfromname;
		if (strcmp(argv[i], fsbuf.f_mntonname) == 0)
			goto multicheck;
		if (mountpoint == NULL) {
			mountpoint = xstrdup(fsbuf.f_mntonname);
			if (uflag) {
				msg("Ignoring u flag for subdir dump\n");
				uflag = 0;
			}
			if (level > '0') {
				msg("Subdir dump is done at level 0\n");
				level = '0';
			}
			msg("Dumping sub files/directories from %s\n",
			    mountpoint);
		} else {
			if (strcmp(mountpoint, fsbuf.f_mntonname) != 0)
				quit("%s is not on %s\n", argv[i], mountpoint);
		}
		msg("Dumping file/directory %s\n", argv[i]);
		dirc++;
	}
	if (mountpoint)
		free(mountpoint);

	if (dirc == 0) {
		argv++;
		if (argc != 1) {
			(void)fprintf(stderr, "Excess arguments to dump:");
			while (--argc)
				(void)fprintf(stderr, " %s", *argv++);
			(void)fprintf(stderr, "\n");
			exit(X_STARTUP);
		}
	}
	if (Tflag && uflag) {
		(void)fprintf(stderr,
		    "You cannot use the T and u flags together.\n");
		exit(X_STARTUP);
	}
	if (strcmp(tape, "-") == 0) {
		pipeout++;
		tape = "standard output";
	}

	if (blocksperfile)
		blocksperfile = blocksperfile / ntrec * ntrec; /* round down */
	else if (!unlimited) {
		/*
		 * Determine how to default tape size and density
		 *
		 *		density				tape size
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

	if ((cp = strchr(tape, ':')) != NULL) {
		host = tape;
		/* This is fine, because all the const strings don't have : */
		*cp++ = '\0';
		tape = cp;
#ifdef RDUMP
		if (rmthost(host) == 0)
			exit(X_STARTUP);
#else
		(void)fprintf(stderr, "remote dump not enabled\n");
		exit(X_STARTUP);
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
#if 0
	if (signal(SIGSEGV, SIG_IGN) != SIG_IGN)
		signal(SIGSEGV, sig);
#endif
	if (signal(SIGTERM, SIG_IGN) != SIG_IGN)
		signal(SIGTERM, sig);
	if (signal(SIGINT, interrupt) == SIG_IGN)
		signal(SIGINT, SIG_IGN);

	/*
	 *	disk can be either the full special file name, or
	 *	the file system name.
	 */
	mountpoint = NULL;
	mntinfo = mntinfosearch(disk);
	if ((dt = fstabsearch(disk)) != NULL) {
		if (getfsspecname(buf, sizeof(buf), dt->fs_spec) == NULL)
			quit("%s (%s)", buf, strerror(errno));
		if (getdiskrawname(rbuf, sizeof(rbuf), buf) == NULL)
			quit("Can't get disk raw name for `%s' (%s)",
			    buf, strerror(errno));
		disk = rbuf;
		mountpoint = dt->fs_file;
		msg("Found %s on %s in %s\n", disk, mountpoint, _PATH_FSTAB);
	} else if (mntinfo != NULL) {
		if (getdiskrawname(rbuf, sizeof(rbuf), mntinfo->f_mntfromname)
		    == NULL)
			quit("Can't get disk raw name for `%s' (%s)",
			    mntinfo->f_mntfromname, strerror(errno));
		mountpoint = mntinfo->f_mntonname;
		msg("Found %s on %s in mount table\n", disk, mountpoint);
	}
	if (mountpoint != NULL) {
		if (dirc != 0)
			(void)snprintf(spcl.c_filesys, sizeof(spcl.c_filesys),
			    "a subset of %s", mountpoint);
		else
			(void)strlcpy(spcl.c_filesys, mountpoint,
			    sizeof(spcl.c_filesys));
	} else if (Fflag) {
		(void)strlcpy(spcl.c_filesys, "a file system image",
		    sizeof(spcl.c_filesys));
	} else {
		(void)strlcpy(spcl.c_filesys, "an unlisted file system",
		    sizeof(spcl.c_filesys));
	}
	(void)strlcpy(spcl.c_dev, disk, sizeof(spcl.c_dev));
	(void)strlcpy(spcl.c_label, labelstr, sizeof(spcl.c_label));
	(void)gethostname(spcl.c_host, sizeof(spcl.c_host));
	spcl.c_host[sizeof(spcl.c_host) - 1] = '\0';

	if ((snap_backup != NULL || snap_internal) && mntinfo == NULL) {
		msg("WARNING: Cannot use -x or -X on unmounted file system.\n");
		snap_backup = NULL;
		snap_internal = 0;
	}

#ifdef DUMP_LFS
	sync();
	if (snap_backup != NULL || snap_internal) {
		if (lfs_wrap_stop(mountpoint) < 0) {
			msg("Cannot stop writing on %s\n", mountpoint);
			exit(X_STARTUP);
		}
	}
	if ((diskfd = open(disk, O_RDONLY)) < 0) {
		msg("Cannot open %s\n", disk);
		exit(X_STARTUP);
	}
	disk_dev = disk;
#else /* ! DUMP_LFS */
	if (snap_backup != NULL || snap_internal) {
		diskfd = snap_open(mntinfo->f_mntonname, snap_backup,
		    &tnow, &disk_dev);
		if (diskfd < 0) {
			msg("Cannot open snapshot of %s\n",
				mntinfo->f_mntonname);
			exit(X_STARTUP);
		}
		spcl.c_date = tnow;
	} else {
		if ((diskfd = open(disk, O_RDONLY)) < 0) {
			msg("Cannot open %s\n", disk);
			exit(X_STARTUP);
		}
		disk_dev = disk;
	}
	sync();
#endif /* ! DUMP_LFS */

	needswap = fs_read_sblock(sblock_buf);

	/* true incremental is always a level 10 dump */
	spcl.c_level = trueinc? iswap32(10): iswap32(level - '0');
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
	if (snap_backup != NULL || snap_internal)
		msgtail("a snapshot of ");
	if (dirc != 0)
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
	usedinomap = (char *)xcalloc((unsigned) mapsize, sizeof(char));
	dumpdirmap = (char *)xcalloc((unsigned) mapsize, sizeof(char));
	dumpinomap = (char *)xcalloc((unsigned) mapsize, sizeof(char));
	tapesize = 3 * (howmany(mapsize * sizeof(char), TP_BSIZE) + 1);

	nonodump = iswap32(spcl.c_level) < honorlevel;

	initcache(readcache, readblksize);

	(void)signal(SIGINFO, statussig);

	msg("mapping (Pass I) [regular files]\n");
	anydirskipped = mapfiles(maxino, &tapesize, mountpoint,
	    (dirc ? argv : NULL));

	msg("mapping (Pass II) [directories]\n");
	while (anydirskipped) {
		anydirskipped = mapdirs(maxino, &tapesize);
	}

	if (pipeout || unlimited) {
		tapesize += 10;	/* 10 trailer blocks */
		msg("estimated %llu tape blocks.\n",
		    (unsigned long long)tapesize);
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
			(	  (double) tapesize	/* blocks */
				* TP_BSIZE	/* bytes/block */
				* (1.0/density)	/* 0.1" / byte */
			  +
				  (double) tapesize	/* blocks */
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
		msg("estimated %llu tape blocks on %3.2f tape(s).\n",
		    (unsigned long long)tapesize, fetapes);
	}
	/*
	 * If the user only wants an estimate of the number of
	 * tapes, exit now.
	 */
	if (just_estimate)
		exit(X_FINOK);

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
		if ((DIP(dp, mode) & IFMT) != IFDIR)
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
		mode = DIP(dp, mode) & IFMT;
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
	trewind(0);
	broadcast("DUMP IS DONE!\a\a\n");
#ifdef DUMP_LFS
	lfs_wrap_go();
#endif /* DUMP_LFS */
	msg("DUMP IS DONE\n");
	Exit(X_FINOK);
	/* NOTREACHED */
	exit(X_FINOK);		/* XXX: to satisfy gcc */
}

static void
usage(void)
{
	const char *prog = getprogname();

	(void)fprintf(stderr,
"usage: %s [-0123456789aceFinStuX] [-B records] [-b blocksize]\n"
"            [-d density] [-f file] [-h level] [-k read-blocksize]\n"
"            [-L label] [-l timeout] [-r cachesize] [-s feet]\n"
"            [-T date] [-x snap-backup] files-to-dump\n"
"       %s [-W | -w]\n", prog, prog);
	exit(X_STARTUP);
}

/*
 * Pick up a numeric argument.  It must be nonnegative and in the given
 * range (except that a vmax of 0 means unlimited).
 */
static long
numarg(const char *meaning, long vmin, long vmax)
{
	char *p;
	long val;

	val = strtol(optarg, &p, 10);
	if (*p)
		errx(X_STARTUP, "illegal %s -- %s", meaning, optarg);
	if (val < vmin || (vmax && val > vmax))
		errx(X_STARTUP, "%s must be between %ld and %ld",
		    meaning, vmin, vmax);
	return (val);
}

void
sig(int signo)
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
		msg("Rewriting attempted as response to signal %s.\n", sys_siglist[signo]);
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

/*
 * obsolete --
 *	Change set of key letters and ordered arguments into something
 *	getopt(3) will like.
 */
static void
obsolete(int *argcp, char **argvp[])
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
	*argvp = nargv = xmalloc((argc + 1) * sizeof(char *));
	p = flagsp = xmalloc(strlen(ap) + 2);

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
		case 'x':
			if (*argv == NULL) {
				warnx("option requires an argument -- %c", *ap);
				usage();
			}
			nargv[0] = xmalloc(strlen(*argv) + 2 + 1);
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
	} else
		free(flagsp);

	/* Copy remaining arguments. */
	while ((*nargv++ = *argv++) != NULL)
		;

	/* Update argument count. */
	*argcp = nargv - *argvp - 1;
}


void *
xcalloc(size_t number, size_t size)
{
	void *p;

	p = calloc(number, size);
	if (p == NULL)
		quit("%s\n", strerror(errno));
	return (p);
}

void *
xmalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (p == NULL)
		quit("%s\n", strerror(errno));
	return (p);
}

char *
xstrdup(const char *str)
{
	char *p;

	p = strdup(str);
	if (p == NULL)
		quit("%s\n", strerror(errno));
	return (p);
}
