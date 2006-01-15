/*	$NetBSD: newfs.c,v 1.88 2006/01/15 19:49:25 dsl Exp $	*/

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

/*
 * Copyright (c) 2002 Networks Associates Technology, Inc.
 * All rights reserved.
 *
 * This software was developed for the FreeBSD Project by Marshall
 * Kirk McKusick and Network Associates Laboratories, the Security
 * Research Division of Network Associates, Inc. under DARPA/SPAWAR
 * contract N66001-01-C-8035 ("CBOSS"), as part of the DARPA CHATS
 * research program
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
__RCSID("$NetBSD: newfs.c,v 1.88 2006/01/15 19:49:25 dsl Exp $");
#endif
#endif /* not lint */

/*
 * newfs: friendly front end to mkfs
 */
#include <sys/param.h>
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
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <limits.h>
#include <paths.h>
#include <pwd.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <util.h>

#include "mntopts.h"
#include "dkcksum.h"
#include "extern.h"

struct mntopt mopts[] = {
	MOPT_STDOPTS,
	MOPT_ASYNC,
	MOPT_UPDATE,
	MOPT_GETARGS,
	MOPT_NOATIME,
	{ NULL },
};

static struct disklabel *getdisklabel(char *, int);
static void rewritelabel(char *, int, struct disklabel *);
static gid_t mfs_group(const char *);
static uid_t mfs_user(const char *);
static int64_t strsuftoi64(const char *, const char *, int64_t, int64_t, int *);
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
/*
 * For file systems smaller than SMALL_FSSIZE we use the S_DFL_* defaults,
 * otherwise if less than MEDIUM_FSSIZE use M_DFL_*, otherwise use
 * L_DFL_*.
 */
#define	SMALL_FSSIZE	(20*1024*2)
#define	S_DFL_FRAGSIZE	512
#define	MEDIUM_FSSIZE	(1000*1024*2)
#define	M_DFL_FRAGSIZE	1024
#define	L_DFL_FRAGSIZE	2048
#define	DFL_FRAG_BLK	8

/* Apple requires the fragment size to be at least APPLEUFS_DIRBLKSIZ
 * but the block size cannot be larger than Darwin's PAGE_SIZE.  See
 * the mount check in Darwin's ffs_mountfs for an explanation.
 */
#define APPLEUFS_DFL_FRAGSIZE APPLEUFS_DIRBLKSIZ /* 1024 */
#define APPLEUFS_DFL_BLKSIZE 4096 /* default Darwin PAGE_SIZE */

/*
 * Default sector size.
 */
#define	DFL_SECSIZE	512

/*
 * MAXBLKPG determines the maximum number of data blocks which are
 * placed in a single cylinder group. The default is one indirect
 * block worth of data blocks.
 */
#define	MAXBLKPG_UFS1(bsize)	((bsize) / sizeof(int32_t))
#define	MAXBLKPG_UFS2(bsize)	((bsize) / sizeof(int64_t))

/*
 * Each file system has a number of inodes statically allocated.
 * We allocate one inode slot per NFPI fragments, expecting this
 * to be far more than we will ever need.
 */
#define	NFPI		4


int	mfs;			/* run as the memory based filesystem */
int	Nflag;			/* run without writing file system */
int	Oflag = 1;		/* format as an 4.3BSD file system */
int	verbosity;		/* amount of printf() output */
int64_t	fssize;			/* file system size */
int	sectorsize;		/* bytes/sector */
int	fsize = 0;		/* fragment size */
int	bsize = 0;		/* block size */
int	maxbsize = 0;		/* maximum clustering */
int	minfree = MINFREE;	/* free space threshold */
int	opt = DEFAULTOPT;	/* optimization preference (space or time) */
int	density;		/* number of bytes per inode */
int	num_inodes;		/* number of inodes (overrides density) */
int	maxcontig = 0;		/* max contiguous blocks to allocate */
int	maxbpg;			/* maximum blocks per file in a cyl group */
int	avgfilesize = AVFILESIZ;/* expected average file size */
int	avgfpdir = AFPDIR;	/* expected number of files per directory */
int	mntflags = MNT_ASYNC;	/* flags to be passed to mount */
u_long	memleft;		/* virtual memory available */
caddr_t	membase;		/* start address of memory based filesystem */
int	needswap;		/* Filesystem not in native byte order */
#ifdef COMPAT
char	*disktype;
int	unlabeled;
#endif
char *appleufs_volname = 0; /* Apple UFS volume name */
int isappleufs = 0;

char	device[MAXPATHLEN];

int
main(int argc, char *argv[])
{
	struct partition *pp = NULL;
	struct disklabel *lp = NULL;
	struct partition oldpartition;
	struct statvfs *mp;
	struct stat sb;
	int ch, fsi, fso, len, n, Fflag, Iflag, Zflag;
	uint ptn = 0;	/* gcc -Wuninitialised */
	char *cp, *s1, *s2, *special;
	const char *opstring;
	int byte_sized = 0;
#ifdef MFS
	struct mfs_args args;
	char mountfromname[100];
	pid_t pid, res;
	struct statvfs sf;
	int status;
#endif
	mode_t mfsmode = 01777;	/* default mode for a /tmp-type directory */
	uid_t mfsuid = 0;	/* user root */
	gid_t mfsgid = 0;	/* group wheel */

	cp = NULL;
	fsi = fso = -1;
	Fflag = Iflag = Zflag = 0;
	verbosity = -1;
	if (strstr(getprogname(), "mfs")) {
		mfs = 1;
	} else {
		/* Undocumented, for ease of testing */
		if (argv[1] != NULL && !strcmp(argv[1], "-mfs")) {
			argv++;
			argc--;
			mfs = 1;
		}
	}

	opstring = mfs ?
	    "NT:V:a:b:d:e:f:g:h:i:m:n:o:p:s:u:" :
	    "B:FINO:S:T:V:Za:b:d:e:f:g:h:i:l:m:n:o:r:s:v:";
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
		case 'I':
			Iflag = 1;
			break;
		case 'N':
			Nflag = 1;
			if (verbosity == -1)
				verbosity = 3;
			break;
		case 'O':
			Oflag = strsuftoi64("format", optarg, 0, 2, NULL);
			break;
		case 'S':
			/* non-512 byte sectors almost certainly don't work. */
			sectorsize = strsuftoi64("sector size",
			    optarg, 512, 65536, NULL);
			if (sectorsize & (sectorsize - 1))
				errx(1, "sector size `%s' is not a power of 2.",
				    optarg);
			break;
#ifdef COMPAT
		case 'T':
			disktype = optarg;
			break;
#endif
		case 'V':
			verbosity = strsuftoi64("verbose", optarg, 0, 4, NULL);
			break;
		case 'Z':
			Zflag = 1;
			break;
		case 'a':
			maxcontig = strsuftoi64("maximum contiguous blocks",
			    optarg, 1, INT_MAX, NULL);
			break;
		case 'b':
			bsize = strsuftoi64("block size",
			    optarg, MINBSIZE, MAXBSIZE, NULL);
			break;
		case 'd':
			maxbsize = strsuftoi64("maximum extent size",
			    optarg, 0, INT_MAX, NULL);
			break;
		case 'e':
			maxbpg = strsuftoi64(
			    "blocks per file in a cylinder group",
			    optarg, 1, INT_MAX, NULL);
			break;
		case 'f':
			fsize = strsuftoi64("fragment size",
			    optarg, 1, MAXBSIZE, NULL);
			break;
		case 'g':
			if (mfs)
				mfsgid = mfs_group(optarg);
			else {
				avgfilesize = strsuftoi64("average file size",
				    optarg, 1, INT_MAX, NULL);
			}
			break;
		case 'h':
			avgfpdir = strsuftoi64("expected files per directory",
			    optarg, 1, INT_MAX, NULL);
			break;
		case 'i':
			density = strsuftoi64("bytes per inode",
			    optarg, 1, INT_MAX, NULL);
			break;
		case 'm':
			minfree = strsuftoi64("free space %",
			    optarg, 0, 99, NULL);
			break;
		case 'n':
			num_inodes = strsuftoi64("number of inodes",
			    optarg, 1, INT_MAX, NULL);
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
			/* mfs only */
			if ((mfsmode = strtol(optarg, NULL, 8)) <= 0)
				errx(1, "bad mode `%s'", optarg);
			break;
		case 's':
			fssize = strsuftoi64("file system size",
			    optarg, INT64_MIN, INT64_MAX, &byte_sized);
			break;
		case 'u':
			/* mfs only */
			mfsuid = mfs_user(optarg);
			break;
		case 'v':
			appleufs_volname = optarg;
			if (strchr(appleufs_volname, ':') || strchr(appleufs_volname, '/'))
				errx(1,"Apple UFS volume name cannot contain ':' or '/'");
			if (appleufs_volname[0] == '\0')
				errx(1,"Apple UFS volume name cannot be zero length");
			isappleufs = 1;
			break;
		case '?':
		default:
			usage();
		}
	argc -= optind;
	argv += optind;

	if (verbosity == -1)
		/* Default to not showing CG info if mfs */
		verbosity = mfs ? 0 : 3;

#ifdef MFS
	/* This is enough to get through the correct kernel code paths */
	memset(&args, 0, sizeof args);
	args.fspec = mountfromname;
	if (mntflags & (MNT_GETARGS | MNT_UPDATE)) {
		if (mount(MOUNT_MFS, argv[1], mntflags, &args) < 0)
			err(1, "mount `%s' failed", argv[1]);
		if (mntflags & MNT_GETARGS)
			printf("base=%p, size=%ld\n", args.base, args.size);
		exit(0);
	}
#endif

	if (argc != 2 && (mfs || argc != 1))
		usage();

	memset(&oldpartition, 0, sizeof oldpartition);
	memset(&sb, 0, sizeof sb);
	special = argv[0];
	if (Fflag || mfs) {
		/*
		 * It's a file system image or an MFS,
		 * no label, use fixed default for sectorsize.
		 */
		if (sectorsize == 0)
			sectorsize = DFL_SECSIZE;

		if (mfs) {
			/* Default filesystem size to that of supplied device */
			if (fssize == 0)
				stat(special, &sb);
		} else {
			/* creating image in a regular file */
			int fl;
			if (Nflag)
				fl = O_RDONLY;
			else {
				if (fssize > 0)
					fl = O_RDWR | O_CREAT;
				else
					fl = O_RDWR;
			}
			fsi = open(special, fl, 0777);
			if (fsi == -1)
				err(1, "can't open file %s", special);
			if (fstat(fsi, &sb) == -1)
				err(1, "can't fstat opened %s", special);
			if (!Nflag)
				fso = fsi;
		}
	} else {	/* !Fflag && !mfs */
		fsi = opendisk(special, O_RDONLY, device, sizeof(device), 0);
		special = device;
		if (fsi < 0 || fstat(fsi, &sb) == -1)
			err(1, "%s: open for read", special);

		if (!Nflag) {
			fso = open(special, O_WRONLY, 0);
			if (fso < 0)
				err(1, "%s: open for write", special);

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
				if (strcmp(s1, s2) == 0 ||
				    strcmp(s1, &s2[1]) == 0)
					errx(1, "%s is mounted on %s",
					    special, mp->f_mntonname);
				++mp;
			}
		}

#ifdef COMPAT
		if (disktype == NULL)
			disktype = argv[1];
#endif
		lp = getdisklabel(special, fsi);
		if (sectorsize == 0) {
			sectorsize = lp->d_secsize;
			if (sectorsize <= 0)
				errx(1, "no default sector size");
		}

		ptn = strchr(special, '\0')[-1] - 'a';
		if (ptn < lp->d_npartitions && ptn == DISKPART(sb.st_rdev)) {
			/* Assume partition really does match the label */
			pp = &lp->d_partitions[ptn];
			oldpartition = *pp;
			if (pp->p_size == 0)
				errx(1, "`%c' partition is unavailable",
					'a' + ptn);
			if (pp->p_fstype == FS_APPLEUFS)
				isappleufs = 1;
			if (!Iflag) {
				if (isappleufs) {
					if (pp->p_fstype != FS_APPLEUFS)
						errx(1, "`%c' partition type is not `Apple UFS'", 'a' + ptn);
				} else {
					if (pp->p_fstype != FS_BSDFFS)
						errx(1, "`%c' partition type is not `4.2BSD'", 'a' + ptn);
				}
			}
		}
	}	/* !Fflag && !mfs */

	if (byte_sized)
		fssize /= sectorsize;
	if (fssize <= 0) {
		if (sb.st_size != 0)
			fssize += sb.st_size / sectorsize;
		else
			fssize += oldpartition.p_size;
		if (fssize <= 0)
			errx(1, "Unable to determine file system size");
	}

	if (pp != NULL && fssize > pp->p_size)
		errx(1, "size %" PRIu64 " exceeds maximum file system size on "
		    "`%s' of %u sectors",
		    fssize, special, pp->p_size);

	/* XXXLUKEM: only ftruncate() regular files ? (dsl: or at all?) */
	if (Fflag && fso != -1
	    && ftruncate(fso, (off_t)fssize * sectorsize) == -1)
		err(1, "can't ftruncate %s to %" PRId64, special, fssize);

	if (Zflag && fso != -1) {	/* pre-zero (and de-sparce) the file */
		char	*buf;
		int	bufsize, i;
		off_t	bufrem;
		struct statvfs sfs;

		if (fstatvfs(fso, &sfs) == -1) {
			warn("can't fstatvfs `%s'", special);
			bufsize = 8192;
		} else
			bufsize = sfs.f_iosize;

		if ((buf = calloc(1, bufsize)) == NULL)
			err(1, "can't malloc buffer of %d",
			bufsize);
		bufrem = fssize * sectorsize;
		if (verbosity > 0)
			printf( "Creating file system image in `%s', "
			    "size %lld bytes, in %d byte chunks.\n",
			    special, (long long)bufrem, bufsize);
		while (bufrem > 0) {
			i = write(fso, buf, MIN(bufsize, bufrem));
			if (i == -1)
				err(1, "writing image");
			bufrem -= i;
		}
		free(buf);
	}

	/* Sort out fragment and block sizes */
	if (fsize == 0) {
		fsize = bsize / DFL_FRAG_BLK;
		if (fsize == 0)
			fsize = oldpartition.p_fsize;
		if (fsize <= 0) {
			if (isappleufs) {
				fsize = APPLEUFS_DFL_FRAGSIZE;
			} else {
				if (fssize < SMALL_FSSIZE)
					fsize = S_DFL_FRAGSIZE;
				else if (fssize < MEDIUM_FSSIZE)
					fsize = M_DFL_FRAGSIZE;
				else
					fsize = L_DFL_FRAGSIZE;
				if (fsize < sectorsize)
					fsize = sectorsize;
			}
		}
	}
	if (bsize == 0) {
		bsize = oldpartition.p_frag * oldpartition.p_fsize;
		if (bsize <= 0) {
			if (isappleufs)
				bsize = APPLEUFS_DFL_BLKSIZE;
			else
				bsize = DFL_FRAG_BLK * fsize;
		}
	}

	if (isappleufs && (fsize < APPLEUFS_DFL_FRAGSIZE)) {
		warnx("Warning: chosen fsize of %d is less than Apple UFS minimum of %d",
			fsize, APPLEUFS_DFL_FRAGSIZE);
	}
	if (isappleufs && (bsize > APPLEUFS_DFL_BLKSIZE)) {
		warnx("Warning: chosen bsize of %d is greater than Apple UFS maximum of %d",
			bsize, APPLEUFS_DFL_BLKSIZE);
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
	if (maxbpg == 0) {
		if (Oflag <= 1)
			maxbpg = MAXBLKPG_UFS1(bsize);
		else
			maxbpg = MAXBLKPG_UFS2(bsize);
	}
	mkfs(pp, special, fsi, fso, mfsmode, mfsuid, mfsgid);
	if (!Nflag && pp != NULL
	    && memcmp(pp, &oldpartition, sizeof(oldpartition)))
		rewritelabel(special, fso, lp);
	if (fsi != -1 && fsi != fso)
		close(fsi);
	if (fso != -1)
		close(fso);
#ifdef MFS
	if (mfs) {

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
				if (statvfs(argv[1], &sf) < 0)
					err(88, "statvfs %s", argv[1]);
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
getdisklabel(char *s, int fd)
{
	static struct disklabel lab;

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
	if (ioctl(fd, DIOCGDINFO, &lab) < 0) {
		warn("ioctl (GDINFO)");
		errx(1, lmsg, s);
	}
	return (&lab);
}

static void
rewritelabel(char *s, int fd, struct disklabel *lp)
{
#ifdef COMPAT
	if (unlabeled)
		return;
#endif
	lp->d_checksum = 0;
	lp->d_checksum = dkcksum(lp);
	if (ioctl(fd, DIOCWDINFO, (char *)lp) < 0) {
		if (errno == ESRCH)
			return;
		warn("ioctl (WDINFO)");
		errx(1, "%s: can't rewrite disk label", s);
	}
#if __vax__
	if (lp->d_type == DTYPE_SMD && lp->d_flags & D_BADSECT) {
		int i;
		int cfd;
		daddr_t alt;
		off_t loff;
		char specname[64];
		char blk[1024];
		char *cp;

		/*
		 * Make name for 'c' partition.
		 */
		strlcpy(specname, s, sizeof(specname));
		cp = specname + strlen(specname) - 1;
		if (!isdigit((unsigned char)*cp))
			*cp = 'c';
		cfd = open(specname, O_WRONLY);
		if (cfd < 0)
			err(1, "%s: open", specname);
		if ((loff = getlabeloffset()) < 0)
			err(1, "getlabeloffset()");
		memset(blk, 0, sizeof(blk));
		*(struct disklabel *)(blk + loff) = *lp;
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

static gid_t
mfs_group(const char *gname)
{
	struct group *gp;

	if (!(gp = getgrnam(gname)) && !isdigit((unsigned char)*gname))
		errx(1, "unknown gname %s", gname);
	return gp ? gp->gr_gid : atoi(gname);
}

static uid_t
mfs_user(const char *uname)
{
	struct passwd *pp;

	if (!(pp = getpwnam(uname)) && !isdigit((unsigned char)*uname))
		errx(1, "unknown user %s", uname);
	return pp ? pp->pw_uid : atoi(uname);
}

static int64_t
strsuftoi64(const char *desc, const char *arg, int64_t min, int64_t max, int *num_suffix)
{
	int64_t result, r1;
	int shift = 0;
	char	*ep;

	errno = 0;
	r1 = strtoll(arg, &ep, 10);
	if (ep[0] != '\0' && ep[1] != '\0')
		errx(1, "%s `%s' is not a valid number.", desc, arg);
	switch (ep[0]) {
	case '\0':
	case 's': case 'S':
		if (num_suffix != NULL)
			*num_suffix = 0;
		break;
	case 'g': case 'G':
		shift += 10;
		/* FALLTHROUGH */
	case 'm': case 'M':
		shift += 10;
		/* FALLTHROUGH */
	case 'k': case 'K':
		shift += 10;
		/* FALLTHROUGH */
	case 'b': case 'B':
		if (num_suffix != NULL)
			*num_suffix = 1;
		break;
	default:
		errx(1, "`%s' is not a valid suffix for %s.", ep, desc);
	}
	result = r1 << shift;
	if (errno == ERANGE || result >> shift != r1)
		errx(1, "%s `%s' is too large to convert.", desc, arg);
	if (result < min)
		errx(1, "%s `%s' (%" PRId64 ") is less than the minimum (%" PRId64 ").",
		    desc, arg, result, min);
	if (result > max)
		errx(1, "%s `%s' (%" PRId64 ") is greater than the maximum (%" PRId64 ").",
		    desc, arg, result, max);
	return result;
}

#define	NEWFS		1
#define	MFS_MOUNT	2
#define	BOTH		NEWFS | MFS_MOUNT

struct help_strings {
	int flags;
	const char *str;
} const help_strings[] = {
	{ NEWFS,	"-B byteorder\tbyte order (`be' or `le')" },
	{ NEWFS,	"-F \t\tcreate file system image in regular file" },
	{ NEWFS,	"-I \t\tdo not check that the file system type is '4.2BSD'" },
	{ BOTH,		"-N \t\tdo not create file system, just print out "
			    "parameters" },
	{ NEWFS,	"-O N\t\tfilesystem format: 0 ==> 4.3BSD, 1 ==> FFS, 2 ==> UFS2" },
	{ NEWFS,	"-S secsize\tsector size" },
#ifdef COMPAT
	{ NEWFS,	"-T disktype\tdisk type" },
#endif
	{ NEWFS,	"-Z \t\tpre-zero the image file" },
	{ BOTH,		"-a maxcontig\tmaximum contiguous blocks" },
	{ BOTH,		"-b bsize\tblock size" },
	{ BOTH,		"-d maxbsize\tmaximum extent size" },
	{ BOTH,		"-e maxbpg\tmaximum blocks per file in a cylinder group"
			    },
	{ BOTH,		"-f fsize\tfrag size" },
	{ NEWFS,	"-g avgfilesize\taverage file size" },
	{ MFS_MOUNT,	"-g groupname\tgroup name of mount point" },
	{ BOTH,		"-h avgfpdir\taverage files per directory" },
	{ BOTH,		"-i density\tnumber of bytes per inode" },
	{ BOTH,		"-m minfree\tminimum free space %%" },
	{ BOTH,		"-n inodes\tnumber of inodes (overrides -i density)" },
	{ BOTH,		"-o optim\toptimization preference (`space' or `time')"
			    },
	{ MFS_MOUNT,	"-p perm\t\tpermissions (in octal)" },
	{ BOTH,		"-s fssize\tfile system size (sectors)" },
	{ MFS_MOUNT,	"-u username\tuser name of mount point" },
	{ NEWFS,	"-v volname\tApple UFS volume name" },
	{ 0, NULL }
};

static void
usage(void)
{
	int match;
	const struct help_strings *hs;

	if (mfs) {
		fprintf(stderr,
		    "usage: %s [ fsoptions ] special-device mount-point\n",
			getprogname());
	} else
		fprintf(stderr,
		    "usage: %s [ fsoptions ] special-device%s\n",
		    getprogname(),
#ifdef COMPAT
		    " [disk-type]");
#else
		    "");
#endif
	fprintf(stderr, "where fsoptions are:\n");

	match = mfs ? MFS_MOUNT : NEWFS;
	for (hs = help_strings; hs->flags != 0; hs++)
		if (hs->flags & match)
			fprintf(stderr, "\t%s\n", hs->str);
	exit(1);
}
