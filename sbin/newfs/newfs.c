/*	$NetBSD: newfs.c,v 1.47 2001/09/06 02:16:01 lukem Exp $	*/

/*
 * Copyright (c) 1983, 1989, 1993, 1994
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
__COPYRIGHT("@(#) Copyright (c) 1983, 1989, 1993, 1994\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)newfs.c	8.13 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: newfs.c,v 1.47 2001/09/06 02:16:01 lukem Exp $");
#endif
#endif /* not lint */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/disklabel.h>
#include <sys/file.h>
#include <sys/mount.h>
#include <sys/sysctl.h>
#include <sys/wait.h>

#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ffs/fs.h>

#include <ctype.h>
#include <disktab.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <signal.h>
#include <err.h>
#include <util.h>

#include "mntopts.h"
#include "dkcksum.h"
#include "extern.h"

struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_UPDATE,
	MOPT_NOATIME,
	{ NULL },
};

static struct disklabel *getdisklabel(char *, int);
static void rewritelabel(char *, int, struct disklabel *);
static int strsuftoi(const char *, const char *, int, int);
static void usage(void);
int main(int, char *[]);

#define	COMPAT			/* allow non-labeled disks */

/*
 * The following two constants set the default block and fragment sizes.
 * Both constants must be a power of 2 and meet the following constraints:
 *	MINBSIZE <= DESBLKSIZE <= MAXBSIZE
 *	sectorsize <= DESFRAGSIZE <= DESBLKSIZE
 *	DESBLKSIZE / DESFRAGSIZE <= 8
 */
#define	DFL_FRAGSIZE	1024
#define	DFL_BLKSIZE	8192

/*
 * Default sector size.
 */
#define DFL_SECSIZE	512

/*
 * Cylinder groups may have up to many cylinders. The actual
 * number used depends upon how much information can be stored
 * on a single cylinder. The default is to use 16 cylinders
 * per group.
 */
#define	DESCPG		16	/* desired fs_cpg */

/*
 * ROTDELAY gives the minimum number of milliseconds to initiate
 * another disk transfer on the same cylinder. It is used in
 * determining the rotationally optimal layout for disk blocks
 * within a file; the default of fs_rotdelay is 0ms.
 */
#define ROTDELAY	0

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. The default is one indirect
 * block worth of data blocks.
 */
#define MAXBLKPG(bsize)	((bsize) / sizeof(daddr_t))

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NFPI fragments, expecting this
 * to be far more than we will ever need.
 */
#define	NFPI		4

/*
 * For each cylinder we keep track of the availability of blocks at different
 * rotational positions, so that we can lay out the data to be picked
 * up with minimum rotational latency.  NRPOS is the default number of
 * rotational positions that we distinguish.  With NRPOS of 8 the resolution
 * of our summary information is 2ms for a typical 3600 rpm drive.  Caching
 * and zoning pretty much defeats rotational optimization, so we now use a
 * default of 1.
 */
#define	NRPOS		1	/* number distinct rotational positions */


int	mfs;			/* run as the memory based filesystem */
int	Nflag;			/* run without writing file system */
int	Oflag;			/* format as an 4.3BSD file system */
int	fssize;			/* file system size */
int	ntracks;		/* # tracks/cylinder */
int	nsectors;		/* # sectors/track */
int	nphyssectors;		/* # sectors/track including spares */
int	secpercyl;		/* sectors per cylinder */
int	trackspares = -1;	/* spare sectors per track */
int	cylspares = -1;		/* spare sectors per cylinder */
int	sectorsize;		/* bytes/sector */
int	rpm;			/* revolutions/minute of drive */
int	interleave;		/* hardware sector interleave */
int	trackskew = -1;		/* sector 0 skew, per track */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	cpg = DESCPG;		/* cylinders/cylinder group */
int	cpgflg;			/* cylinders/cylinder group flag was given */
int	minfree = MINFREE;	/* free space threshold */
int	opt = DEFAULTOPT;	/* optimization preference (space or time) */
int	density;		/* number of bytes per inode */
int	maxcontig = 0;		/* max contiguous blocks to allocate */
int	rotdelay = ROTDELAY;	/* rotational delay between blocks */
int	maxbpg;			/* maximum blocks per file in a cyl group */
int	nrpos = NRPOS;		/* # of distinguished rotational positions */
int	avgfilesize = AVFILESIZ;/* expected average file size */
int	avgfpdir = AFPDIR;	/* expected number of files per directory */
int	bbsize = BBSIZE;	/* boot block size */
int	sbsize = SBSIZE;	/* superblock size */
int	mntflags = MNT_ASYNC;	/* flags to be passed to mount */
u_long	memleft;		/* virtual memory available */
caddr_t	membase;		/* start address of memory based filesystem */
int	needswap;		/* Filesystem not in native byte order */
#ifdef COMPAT
char	*disktype;
int	unlabeled;
#endif

char	device[MAXPATHLEN];

int
main(int argc, char *argv[])
{
	struct partition *pp;
	struct disklabel *lp;
	struct disklabel mfsfakelabel;
	struct partition oldpartition;
	struct stat st;
	struct statfs *mp;
	int ch, fsi, fso, len, maxpartitions, n, Fflag, Zflag;
	char *cp, *endp, *s1, *s2, *special;
	const char *opstring;
	long long llsize;
#ifdef MFS
	char mountfromname[100];
	pid_t pid, res;
	struct statfs sf;
	int status;
#endif

	cp = NULL;
	fsi = fso = -1;
	Fflag = Zflag = 0;
	if (strstr(getprogname(), "mfs")) {
		mfs = 1;
		Nflag++;
	}

	maxpartitions = getmaxpartitions();
	if (maxpartitions > 26)
		errx(1, "insane maxpartitions value %d", maxpartitions);

	opstring = mfs ?
	    "NT:a:b:c:d:e:f:g:h:i:m:o:s:" :
	    "B:FNOS:T:Za:b:c:d:e:f:g:h:i:k:l:m:n:o:p:r:s:t:u:x:";
	while ((ch = getopt(argc, argv, opstring)) != -1)
		switch (ch) {
		case 'B':
			if (strcmp(optarg, "be") == 0) {
#if BYTE_ORDER == LITTLE_ENDIAN
				needswap = 1;
#endif
			} else if (strcmp(optarg, "le") == 0) {
#if BYTE_ORDER == BIG_ENDIAN
				needswap = 1;
#endif
			} else
				usage();
			break;
		case 'F':
			Fflag = 1;
			break;
		case 'N':
			Nflag = 1;
			break;
		case 'O':
			Oflag = 1;
			break;
		case 'S':
			sectorsize = strsuftoi("sector size",
			    optarg, 1, INT_MAX);
			break;
#ifdef COMPAT
		case 'T':
			disktype = optarg;
			break;
#endif
		case 'Z':
			Zflag = 1;
			break;
		case 'a':
			maxcontig = strsuftoi("maximum contiguous blocks",
			    optarg, 1, INT_MAX);
			break;
		case 'b':
			bsize = strsuftoi("block size",
			    optarg, MINBSIZE, INT_MAX);
			break;
		case 'c':
			cpg = strsuftoi("cylinders per group",
			    optarg, 1, INT_MAX);
			cpgflg++;
			break;
		case 'd':
			rotdelay = strsuftoi("rotational delay",
			    optarg, 0, INT_MAX);
			break;
		case 'e':
			maxbpg = strsuftoi(
			    "blocks per file in a cylinder group",
			    optarg, 1, INT_MAX);
			break;
		case 'f':
			fsize = strsuftoi("fragment size",
			    optarg, 1, INT_MAX);
			break;
		case 'g':
			avgfilesize = strsuftoi("average file size",
			    optarg, 1, INT_MAX);
			break;
		case 'h':
			avgfpdir = strsuftoi("expected files per directory",
			    optarg, 1, INT_MAX);
			break;
		case 'i':
			density = strsuftoi("bytes per inode",
			    optarg, 1, INT_MAX);
			break;
		case 'k':
			trackskew = strsuftoi("track skew",
			    optarg, 0, INT_MAX);
			break;
		case 'l':
			interleave = strsuftoi("interleave",
			    optarg, 1, INT_MAX);
			break;
		case 'm':
			minfree = strsuftoi("free space %",
			    optarg, 0, 99);
			break;
		case 'n':
			nrpos = strsuftoi("rotational layout count",
			    optarg, 1, INT_MAX);
			break;
		case 'o':
			if (mfs)
				getmntopts(optarg, mopts, &mntflags, 0);
			else {
				if (strcmp(optarg, "space") == 0)
					opt = FS_OPTSPACE;
				else if (strcmp(optarg, "time") == 0)
					opt = FS_OPTTIME;
				else
				    errx(1, "%s %s",
					"unknown optimization preference: ",
					"use `space' or `time'.");
			}
			break;
		case 'p':
			trackspares = strsuftoi("spare sectors per track",
			    optarg, 0, INT_MAX);
			break;
		case 'r':
			rpm = strsuftoi("revolutions per minute",
			    optarg, 1, INT_MAX);
			break;
		case 's':
			llsize = strtoll(optarg, &endp, 10);
			if (endp[0] != '\0' && endp[1] != '\0')
				llsize = -1;
			else {
				int	ssiz;

				ssiz = (sectorsize ? sectorsize : DFL_SECSIZE);
				switch (tolower((unsigned char)endp[0])) {
				case 'b':
					llsize /= ssiz;
					break;
				case 'k':
					llsize *= 1024 / ssiz;
					break;
				case 'm':
					llsize *= 1024 * 1024 / ssiz;
					break;
				case 'g':
					llsize *= 1024 * 1024 * 1024 / ssiz;
					break;
				case '\0':
				case 's':
					break;
				default:
					llsize = -1;
				}
			}
			if (llsize > INT_MAX)
				errx(1, "file system size `%s' is too large.",
				    optarg);
			if (llsize <= 0)
				errx(1,
			    "`%s' is not a valid number for file system size.",
				    optarg);
			fssize = (int)llsize;
			break;
		case 't':
			ntracks = strsuftoi("total tracks",
			    optarg, 1, INT_MAX);
			break;
		case 'u':
			nsectors = strsuftoi("sectors per track",
			    optarg, 1, INT_MAX);
			break;
		case 'x':
			cylspares = strsuftoi("spare sectors per cylinder",
			    optarg, 0, INT_MAX);
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (argc != 2 && (mfs || argc != 1))
		usage();

	special = argv[0];
	if ((mfs && !strcmp(special, "swap")) || Fflag) {
		/*
		 * it's an MFS mounted on "swap" or a file system image;
		 * fake up a label.		XXX
		 */
		if (!sectorsize)
			sectorsize = DFL_SECSIZE;

		if (Fflag && (stat(special, &st) != -1 && !S_ISREG(st.st_mode)))
			errx(1, "%s is not a regular file", special);
		if (Fflag && !Nflag) {	/* creating image in a regular file */
			if (fssize == 0)
				errx(1, "need to specify size when using -F");
			fso = open(special, O_RDWR | O_CREAT | O_TRUNC, 0777);
			if (fso == -1)
				err(1, "can't open file %s", special);
			if ((fsi = dup(fso)) == -1)
				err(1, "can't dup(2) image fd");
			if (ftruncate(fso, (off_t)fssize * sectorsize) == -1)
				err(1, "can't resize %s to %d",
				    special, fssize);

			if (Zflag) {	/* pre-zero the file */
				char	*buf;
				int	bufsize, i;
				off_t	bufrem;
				struct statfs sfs;

				if (fstatfs(fso, &sfs) == -1) {
					warn("can't fstatfs `%s'", special);
					bufsize = 8192;
				} else
					bufsize = sfs.f_iosize;

				if ((buf = calloc(1, bufsize)) == NULL)
					err(1, "can't malloc buffer of %d",
					bufsize);
				bufrem = fssize * sectorsize;
				printf(
    "Creating file system image in `%s', size %lld bytes, in %d byte chunks.\n",
				    special, (long long)bufrem, bufsize);
				while (bufrem > 0) {
					i = write(fso, buf,
					    MIN(bufsize, bufrem));
					if (i == -1)
						err(1, "writing image");
					bufrem -= i;
				}
			}

		}

		memset(&mfsfakelabel, 0, sizeof(mfsfakelabel));
		mfsfakelabel.d_secsize = sectorsize;
		mfsfakelabel.d_nsectors = 64;	/* these 3 add up to 16MB */
		mfsfakelabel.d_ntracks = 16;
		mfsfakelabel.d_ncylinders = 16;
		mfsfakelabel.d_secpercyl =
		    mfsfakelabel.d_nsectors * mfsfakelabel.d_ntracks;
		mfsfakelabel.d_secperunit = 
		    mfsfakelabel.d_ncylinders * mfsfakelabel.d_secpercyl;
		mfsfakelabel.d_rpm = 3600;
		mfsfakelabel.d_interleave = 1;
		mfsfakelabel.d_npartitions = 1;
		mfsfakelabel.d_partitions[0].p_size = mfsfakelabel.d_secperunit;
		mfsfakelabel.d_partitions[0].p_fsize = 1024;
		mfsfakelabel.d_partitions[0].p_frag = 8;
		mfsfakelabel.d_partitions[0].p_cpg = 16;

		lp = &mfsfakelabel;
		pp = &mfsfakelabel.d_partitions[0];

		goto havelabel;
	}
	cp = strrchr(special, '/');
	if (cp == NULL) {
		/*
		 * No path prefix; try /dev/r%s then /dev/%s.
		 */
		(void)snprintf(device, sizeof(device), "%sr%s",
		    _PATH_DEV, special);
		if (stat(device, &st) == -1)
			(void)snprintf(device, sizeof(device), "%s%s",
			    _PATH_DEV, special);
		special = device;
	}
	if (Nflag) {
		fso = -1;
	} else {
		fso = open(special, O_WRONLY);
		if (fso < 0)
			err(1, "%s: open", special);

		/* Bail if target special is mounted */
		n = getmntinfo(&mp, MNT_NOWAIT);
		if (n == 0)
			err(1, "%s: getmntinfo", special);

		len = sizeof(_PATH_DEV) - 1;
		s1 = special;
		if (strncmp(_PATH_DEV, s1, len) == 0)
			s1 += len;

		while (--n >= 0) {
			s2 = mp->f_mntfromname;
			if (strncmp(_PATH_DEV, s2, len) == 0) {
				s2 += len - 1;
				*s2 = 'r';
			}
			if (strcmp(s1, s2) == 0 || strcmp(s1, &s2[1]) == 0)
				errx(1, "%s is mounted on %s",
				    special, mp->f_mntonname);
			++mp;
		}
	}
	if (mfs && disktype != NULL) {
		lp = (struct disklabel *)getdiskbyname(disktype);
		if (lp == NULL)
			errx(1, "%s: unknown disk type", disktype);
		pp = &lp->d_partitions[1];
	} else {
		fsi = open(special, O_RDONLY);
		if (fsi < 0)
			err(1, "%s: open", special);
		if (fstat(fsi, &st) < 0)
			err(1, "%s: fstat", special);
		if (!S_ISCHR(st.st_mode) && !mfs)
			warnx("%s: not a character-special device", special);
		cp = strchr(argv[0], '\0') - 1;
		if (cp == 0 || ((*cp < 'a' || *cp > ('a' + maxpartitions - 1))
		    && !isdigit(*cp)))
			errx(1, "can't figure out file system partition");
#ifdef COMPAT
		if (!mfs && disktype == NULL)
			disktype = argv[1];
#endif
		lp = getdisklabel(special, fsi);
		if (isdigit(*cp))
			pp = &lp->d_partitions[0];
		else
			pp = &lp->d_partitions[*cp - 'a'];
		if (pp->p_size == 0)
			errx(1, "`%c' partition is unavailable", *cp);
		if (pp->p_fstype == FS_BOOT)
			errx(1, "`%c' partition overlaps boot program", *cp);
	}

 havelabel:
	if (fssize == 0)
		fssize = pp->p_size;
	if (fssize > pp->p_size && !mfs && !Fflag)
		errx(1, "maximum file system size on the `%c' partition is %d",
		    *cp, pp->p_size);
	if (rpm == 0) {
		rpm = lp->d_rpm;
		if (rpm <= 0)
			rpm = 3600;
	}
	if (ntracks == 0) {
		ntracks = lp->d_ntracks;
		if (ntracks <= 0)
			errx(1, "no default #tracks");
	}
	if (nsectors == 0) {
		nsectors = lp->d_nsectors;
		if (nsectors <= 0)
			errx(1, "no default #sectors/track");
	}
	if (sectorsize == 0) {
		sectorsize = lp->d_secsize;
		if (sectorsize <= 0)
			errx(1, "no default sector size");
	}
	if (trackskew == -1) {
		trackskew = lp->d_trackskew;
		if (trackskew < 0)
			trackskew = 0;
	}
	if (interleave == 0) {
		interleave = lp->d_interleave;
		if (interleave <= 0)
			interleave = 1;
	}
	if (fsize == 0) {
		fsize = pp->p_fsize;
		if (fsize <= 0)
			fsize = MAX(DFL_FRAGSIZE, lp->d_secsize);
	}
	if (bsize == 0) {
		bsize = pp->p_frag * pp->p_fsize;
		if (bsize <= 0)
			bsize = MIN(DFL_BLKSIZE, 8 * fsize);
	}
	if (cpgflg == 0) {
		if (pp->p_cpg != 0)
			cpg = pp->p_cpg;
	}
	/*
	 * Maxcontig sets the default for the maximum number of blocks
	 * that may be allocated sequentially. With filesystem clustering
	 * it is possible to allocate contiguous blocks up to the maximum
	 * transfer size permitted by the controller or buffering.
	 */
	if (maxcontig == 0)
		maxcontig = MAX(1, MIN(MAXPHYS, MAXBSIZE) / bsize);
	if (density == 0)
		density = NFPI * fsize;
	if (minfree < MINFREE && opt != FS_OPTSPACE) {
		warnx("%s %s %d%%", "Warning: changing optimization to space",
		    "because minfree is less than", MINFREE);
		opt = FS_OPTSPACE;
	}
	if (trackspares == -1) {
		trackspares = lp->d_sparespertrack;
		if (trackspares < 0)
			trackspares = 0;
	}
	nphyssectors = nsectors + trackspares;
	if (cylspares == -1) {
		cylspares = lp->d_sparespercyl;
		if (cylspares < 0)
			cylspares = 0;
	}
	secpercyl = nsectors * ntracks - cylspares;
	if (secpercyl != lp->d_secpercyl)
		warnx("%s (%d) %s (%u)\n",
			"Warning: calculated sectors per cylinder", secpercyl,
			"disagrees with disk label", lp->d_secpercyl);
	if (maxbpg == 0)
		maxbpg = MAXBLKPG(bsize);
#ifdef notdef /* label may be 0 if faked up by kernel */
	bbsize = lp->d_bbsize;
	sbsize = lp->d_sbsize;
#endif
	oldpartition = *pp;
	mkfs(pp, special, fsi, fso);
	if (!Nflag && memcmp(pp, &oldpartition, sizeof(oldpartition)) && !Fflag)
		rewritelabel(special, fso, lp);
	if (!Nflag)
		close(fso);
	close(fsi);
#ifdef MFS
	if (mfs) {
		struct mfs_args args;

		switch (pid = fork()) {
		case -1:
			perror("mfs");
			exit(10);
		case 0:
			(void)snprintf(mountfromname, sizeof(mountfromname),
			    "mfs:%d", getpid());
			break;
		default:
			(void)snprintf(mountfromname, sizeof(mountfromname),
			    "mfs:%d", pid);
			for (;;) {
				/*
				 * spin until the mount succeeds
				 * or the child exits
				 */
				usleep(1);

				/*
				 * XXX Here is a race condition: another process
				 * can mount a filesystem which hides our
				 * ramdisk before we see the success.
				 */
				if (statfs(argv[1], &sf) < 0)
					err(88, "statfs %s", argv[1]);
				if (!strcmp(sf.f_mntfromname, mountfromname) &&
				    !strncmp(sf.f_mntonname, argv[1],
					     MNAMELEN) &&
				    !strcmp(sf.f_fstypename, "mfs"))
					exit(0);

				res = waitpid(pid, &status, WNOHANG);
				if (res == -1)
					err(11, "waitpid");
				if (res != pid)
					continue;
				if (WIFEXITED(status)) {
					if (WEXITSTATUS(status) == 0)
						exit(0);
					errx(1, "%s: mount: %s", argv[1],
					     strerror(WEXITSTATUS(status)));
				} else
					errx(11, "abnormal termination");
			}
			/* NOTREACHED */
		}

		(void) setsid();
		(void) close(0);
		(void) close(1);
		(void) close(2);
		(void) chdir("/");

		args.fspec = mountfromname;
		args.export.ex_root = -2;
		if (mntflags & MNT_RDONLY)
			args.export.ex_flags = MNT_EXRDONLY;
		else
			args.export.ex_flags = 0;
		args.base = membase;
		args.size = fssize * sectorsize;
		if (mount(MOUNT_MFS, argv[1], mntflags, &args) < 0)
			exit(errno); /* parent prints message */
	}
#endif
	exit(0);
}

#ifdef COMPAT
const char lmsg[] = "%s: can't read disk label; disk type must be specified";
#else
const char lmsg[] = "%s: can't read disk label";
#endif

static struct disklabel *
getdisklabel(char *s, volatile int fd)
/* XXX why is fs volatile?! */
{
	static struct disklabel lab;

	if (ioctl(fd, DIOCGDINFO, &lab) < 0) {
#ifdef COMPAT
		if (disktype) {
			struct disklabel *lp;

			unlabeled++;
			lp = getdiskbyname(disktype);
			if (lp == NULL)
				errx(1, "%s: unknown disk type", disktype);
			return (lp);
		}
#endif
		warn("ioctl (GDINFO)");
		errx(1, lmsg, s);
	}
	return (&lab);
}

static void
rewritelabel(char *s, volatile int fd, struct disklabel *lp)
/* XXX why is fd volatile?! */
{
#ifdef COMPAT
	if (unlabeled)
		return;
#endif
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (ioctl(fd, DIOCWDINFO, (char *)lp) < 0) {
		warn("ioctl (WDINFO)");
		errx(1, "%s: can't rewrite disk label", s);
	}
#if __vax__
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		int i;
		int cfd;
		daddr_t alt;
		char specname[64];
		char blk[1024];
		char *cp;

		/*
		 * Make name for 'c' partition.
		 */
		strcpy(specname, s);
		cp = specname + strlen(specname) - 1;
		if (!isdigit(*cp))
			*cp = 'c';
		cfd = open(specname, O_WRONLY);
		if (cfd < 0)
			err(1, "%s: open", specname);
		memset(blk, 0, sizeof(blk));
		*(struct disklabel *)(blk + LABELOFFSET) = *lp;
		alt = lp->d_ncylinders * lp->d_secpercyl - lp->d_nsectors;
		for (i = 1; i < 11 && i < lp->d_nsectors; i += 2) {
			off_t offset;

			offset = alt + i;
			offset *= lp->d_secsize;
			if (lseek(cfd, offset, SEEK_SET) == -1)
				err(1, "lseek to badsector area: ");
			if (write(cfd, blk, lp->d_secsize) < lp->d_secsize)
				warn("alternate label %d write", i/2);
		}
		close(cfd);
	}
#endif
}

static int
strsuftoi(const char *desc, const char *arg, int min, int max)
{
	long long result;
	char	*ep;

	errno = 0;
	result = strtoll(arg, &ep, 10);
	if (ep[0] != '\0' && ep[1] != '\0')
		errx(1, "%s `%s' is not a valid number.", desc, arg);
	switch (tolower((unsigned char)ep[0])) {
	case '\0':
	case 'b':
		break;
	case 'k':
		result <<= 10;
		break;
	case 'm':
		result <<= 20;
		break;
	case 'g':
		result <<= 30;
		break;
	default:
		errx(1, "`%s' is not a valid suffix for %s.", ep, desc);
	}
	if (result < min)
		errx(1, "%s `%s' (%lld) is less than minimum (%d).",
		    desc, arg, result, min);
	if (result > max)
		errx(1, "%s `%s' (%lld) is greater than maximum (%d).",
		    desc, arg, result, max);
	return ((int)result);
}

static void
usage(void)
{

	if (mfs) {
		fprintf(stderr,
		    "usage: %s [ fsoptions ] special-device mount-point\n",
			getprogname());
	} else
		fprintf(stderr,
		    "usage: %s [ fsoptions ] special-device%s\n",
		    getprogname(),
#ifdef COMPAT
		    " [device-type]");
#else
		    "");
#endif
	fprintf(stderr, "where fsoptions are:\n");
	if (!mfs) {
		fprintf(stderr,
			"\t-B byteorder\tbyte order (`be' or `le')\n");
		fprintf(stderr,
			"\t-F\t\tcreate file system image in regular file\n");
	}
	fprintf(stderr, "\t-N\t\tdo not create file system, "
			"just print out parameters\n");
	if (!mfs) {
		fprintf(stderr,
			"\t-O\t\tcreate a 4.3BSD format filesystem\n");
		fprintf(stderr,
			"\t-S secsize\tsector size\n");
	}
#ifdef COMPAT
	fprintf(stderr, "\t-T disktype\tdisk type\n");
#endif
	if (!mfs)
		fprintf(stderr,
			"\t-Z\t\tpre-zero the image file (with -F)\n");
	fprintf(stderr, "\t-a maxcontig\tmaximum contiguous blocks\n");
	fprintf(stderr, "\t-b bsize\tblock size\n");
	fprintf(stderr, "\t-c cpg\t\tcylinders/group\n");
	fprintf(stderr, "\t-d rotdelay\trotational delay between "
			"contiguous blocks\n");
	fprintf(stderr, "\t-e maxbpg\tmaximum blocks per file "
			"in a cylinder group\n");
	fprintf(stderr, "\t-f fsize\tfragment size\n");
	fprintf(stderr, "\t-g average file size\n");
	fprintf(stderr, "\t-h average files per directory\n");
	fprintf(stderr, "\t-i density\tnumber of bytes per inode\n");
	if (!mfs) {
		fprintf(stderr,
			"\t-k trackskew\tsector 0 skew, per track\n");
		fprintf(stderr,
			"\t-l interleave\thardware sector interleave\n");
	}
	fprintf(stderr, "\t-m minfree\tminimum free space %%\n");
	if (!mfs)
		fprintf(stderr,
			"\t-n nrpos\tnumber of distinguished "
			"rotational positions\n");
	fprintf(stderr, "\t-o optim\toptimization preference "
			"(`space' or `time')\n");
	if (!mfs)
		fprintf(stderr,
			"\t-p trackspares\tspare sectors per track\n");
	fprintf(stderr, "\t-s fssize\tfile system size (sectors)\n");
	if (!mfs) {
		fprintf(stderr,
			"\t-r rpm\t\trevolutions/minute\n");
		fprintf(stderr,
			"\t-t ntracks\ttracks/cylinder\n");
		fprintf(stderr,
			"\t-u nsectors\tsectors/track\n");
		fprintf(stderr,
			"\t-x cylspares\tspare sectors per cylinder\n");
	}
	exit(1);
}
