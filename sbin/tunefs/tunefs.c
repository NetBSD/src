/*	$NetBSD: tunefs.c,v 1.57 2022/12/19 21:13:16 chs Exp $	*/

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
__COPYRIGHT("@(#) Copyright (c) 1983, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)tunefs.c	8.3 (Berkeley) 5/3/95";
#else
__RCSID("$NetBSD: tunefs.c,v 1.57 2022/12/19 21:13:16 chs Exp $");
#endif
#endif /* not lint */

/*
 * tunefs: change layout parameters to an existing file system.
 */
#include <sys/param.h>
#include <sys/statvfs.h>

#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/ufs_wapbl.h>
#include <ufs/ufs/ufsmount.h>
#include <ufs/ufs/quota2.h>

#include <machine/bswap.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <fstab.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <util.h>

/* the optimization warning string template */
#define	OPTWARN	"should optimize for %s with minfree %s %d%%"

union {
	struct	fs sb;
	char data[MAXBSIZE];
} sbun, buf;
#define	sblock sbun.sb

int	fi;
long	dev_bsize = 512;
int	needswap = 0;
int	is_ufs2 = 0;
int	extattr = 0;
off_t	sblockloc;
int	userquota = 0;
int	groupquota = 0;
#define Q2_EN  (1)
#define Q2_IGN (0)
#define Q2_DIS (-1)

static off_t sblock_try[] = SBLOCKSEARCH;

static	void	bwrite(daddr_t, char *, int, const char *);
static	void	bread(daddr_t, char *, int, const char *);
static	void	change_log_info(long long);
static	void	getsb(struct fs *, const char *);
static	int	openpartition(const char *, int, char *, size_t);
static	int	isactive(int, struct statvfs *);
static	void	show_log_info(void);
__dead static	void	usage(void);

int
main(int argc, char *argv[])
{
	int		i, ch, aflag, pflag, Aflag, Fflag, Nflag, openflags;
	const char	*special, *chg[2];
	char		device[MAXPATHLEN];
	int		maxbpg, minfree, optim, secsize;
	int		avgfilesize, avgfpdir, active;
	long long	logfilesize;
	int		secshift, fsbtodb;
	const char  	*avalue, *pvalue, *name;
	struct statvfs	sfs;

	aflag = pflag = Aflag = Fflag = Nflag = 0;
	avalue = pvalue = NULL;
	maxbpg = minfree = optim = secsize = -1;
	avgfilesize = avgfpdir = -1;
	logfilesize = -1;
	secshift = 0;
	chg[FS_OPTSPACE] = "space";
	chg[FS_OPTTIME] = "time";

	while ((ch = getopt(argc, argv, "AFNa:e:g:h:l:m:o:p:q:S:")) != -1) {
		switch (ch) {

		case 'A':
			Aflag++;
			break;

		case 'F':
			Fflag++;
			break;

		case 'N':
			Nflag++;
			break;

		case 'a':
			name = "ACLs";
			avalue = optarg;
			if (strcmp(avalue, "enable") &&
			    strcmp(avalue, "disable")) {
				errx(10, "bad %s (options are %s)",
				    name, "`enable' or `disable'");
			}
			aflag = 1;
 			break;

		case 'e':
			maxbpg = strsuftoll(
			    "maximum blocks per file in a cylinder group",
			    optarg, 1, INT_MAX);
			break;

		case 'g':
			avgfilesize = strsuftoll("average file size", optarg,
			    1, INT_MAX);
			break;

		case 'h':
			avgfpdir = strsuftoll(
			    "expected number of files per directory",
			    optarg, 1, INT_MAX);
			break;

		case 'l':
			logfilesize = strsuftoll("journal log file size",
			    optarg, 0, INT_MAX);
			break;

		case 'm':
			minfree = strsuftoll("minimum percentage of free space",
			    optarg, 0, 99);
			break;

		case 'o':
			if (strcmp(optarg, chg[FS_OPTSPACE]) == 0)
				optim = FS_OPTSPACE;
			else if (strcmp(optarg, chg[FS_OPTTIME]) == 0)
				optim = FS_OPTTIME;
			else
				errx(10,
				    "bad %s (options are `space' or `time')",
				    "optimization preference");
			break;

		case 'p':
			name = "POSIX1e ACLs";
			pvalue = optarg;
			if (strcmp(pvalue, "enable") &&
			    strcmp(pvalue, "disable")) {
				errx(10, "bad %s (options are %s)",
				    name, "`enable' or `disable'");
			}
			pflag = 1;
			break;

		case 'q':
			if      (strcmp(optarg, "user") == 0)
				userquota = Q2_EN;
			else if (strcmp(optarg, "group") == 0)
				groupquota = Q2_EN;
			else if (strcmp(optarg, "nouser") == 0)
				userquota = Q2_DIS;
			else if (strcmp(optarg, "nogroup") == 0)
				groupquota = Q2_DIS;
			else
			    errx(11, "invalid quota type %s", optarg);
			break;

		case 'S':
			secsize = strsuftoll("physical sector size",
			    optarg, 0, INT_MAX);
			secshift = ffs(secsize) - 1;
			if (secsize != 0 && 1 << secshift != secsize)
				errx(12, "sector size %d is not a power of two", secsize);
			break;

		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind; 
	if (argc != 1)
		usage();

	special = argv[0];
	openflags = Nflag ? O_RDONLY : O_RDWR;
	if (Fflag)
		fi = open(special, openflags);
	else {
		fi = openpartition(special, openflags, device, sizeof(device));
		special = device;
	}
	if (fi == -1)
		err(1, "%s", special);
	active = !Fflag && isactive(fi, &sfs);
	getsb(&sblock, special);

#define CHANGEVAL(old, new, type, suffix) do				\
	if ((new) != -1) {						\
		if ((new) == (old))					\
			warnx("%s remains unchanged at %d%s",		\
			    (type), (old), (suffix));			\
		else {							\
			warnx("%s changes from %d%s to %d%s",		\
			    (type), (old), (suffix), (new), (suffix));	\
			(old) = (new);					\
		}							\
	} while (/* CONSTCOND */0)

	warnx("tuning %s", special);
	CHANGEVAL(sblock.fs_maxbpg, maxbpg,
	    "maximum blocks per file in a cylinder group", "");
	CHANGEVAL(sblock.fs_minfree, minfree,
	    "minimum percentage of free space", "%");
	if (minfree != -1) {
		if (minfree >= MINFREE &&
		    sblock.fs_optim == FS_OPTSPACE)
			warnx(OPTWARN, "time", ">=", MINFREE);
		if (minfree < MINFREE &&
		    sblock.fs_optim == FS_OPTTIME)
			warnx(OPTWARN, "space", "<", MINFREE);
	}
	if (optim != -1) {
		if (sblock.fs_optim == optim) {
			warnx("%s remains unchanged as %s",
			    "optimization preference",
			    chg[optim]);
		} else {
			warnx("%s changes from %s to %s",
			    "optimization preference",
			    chg[sblock.fs_optim], chg[optim]);
			sblock.fs_optim = optim;
			if (sblock.fs_minfree >= MINFREE &&
			    optim == FS_OPTSPACE)
				warnx(OPTWARN, "time", ">=", MINFREE);
			if (sblock.fs_minfree < MINFREE &&
			    optim == FS_OPTTIME)
				warnx(OPTWARN, "space", "<", MINFREE);
		}
	}
	if (secsize != -1) {
		if (secsize == 0) {
			secsize = sblock.fs_fsize / FFS_FSBTODB(&sblock, 1);
			secshift = ffs(secsize) - 1;
		}

		if (secshift < DEV_BSHIFT)
			warnx("sector size must be at least %d", DEV_BSIZE);
		else if (secshift > sblock.fs_fshift)
			warnx("sector size %d cannot be larger than fragment size %d",
				secsize, sblock.fs_fsize);
		else {
			fsbtodb = sblock.fs_fshift - secshift;
			if (fsbtodb == sblock.fs_fsbtodb) {
				warnx("sector size remains unchanged as %d",
					sblock.fs_fsize / FFS_FSBTODB(&sblock, 1));
			} else {
				warnx("sector size changed from %d to %d",
					sblock.fs_fsize / FFS_FSBTODB(&sblock, 1),
					secsize);
				sblock.fs_fsbtodb = fsbtodb;
			}
		}
	}
	CHANGEVAL(sblock.fs_avgfilesize, avgfilesize,
	    "average file size", "");
	CHANGEVAL(sblock.fs_avgfpdir, avgfpdir,
	    "expected number of files per directory", "");

	if (logfilesize >= 0)
		change_log_info(logfilesize);
	if (userquota == Q2_EN || groupquota == Q2_EN)
		sblock.fs_flags |= FS_DOQUOTA2;
	if (sblock.fs_flags & FS_DOQUOTA2) {
		sblock.fs_quota_magic = Q2_HEAD_MAGIC;
		switch(userquota) {
		case Q2_EN:
			if ((sblock.fs_quota_flags & FS_Q2_DO_TYPE(USRQUOTA))
			    == 0) {
				printf("enabling user quotas\n");
				sblock.fs_quota_flags |=
				    FS_Q2_DO_TYPE(USRQUOTA);
				sblock.fs_quotafile[USRQUOTA] = 0;
			}
			break;
		case Q2_DIS:
			if ((sblock.fs_quota_flags & FS_Q2_DO_TYPE(USRQUOTA))
			    != 0) {
				printf("disabling user quotas\n");
				sblock.fs_quota_flags &=
				    ~FS_Q2_DO_TYPE(USRQUOTA);
			}
		}
		switch(groupquota) {
		case Q2_EN:
			if ((sblock.fs_quota_flags & FS_Q2_DO_TYPE(GRPQUOTA))
			    == 0) {
				printf("enabling group quotas\n");
				sblock.fs_quota_flags |=
				    FS_Q2_DO_TYPE(GRPQUOTA);
				sblock.fs_quotafile[GRPQUOTA] = 0;
			}
			break;
		case Q2_DIS:
			if ((sblock.fs_quota_flags & FS_Q2_DO_TYPE(GRPQUOTA))
			    != 0) {
				printf("disabling group quotas\n");
				sblock.fs_quota_flags &=
				    ~FS_Q2_DO_TYPE(GRPQUOTA);
			}
		}
	}
	/*
	 * if we disabled all quotas, FS_DOQUOTA2 and associated inode(s) will
	 * be cleared by kernel or fsck.
	 */
	if (aflag) {
		name = "NFSv4 ACLs";
		if (strcmp(avalue, "enable") == 0) {
			if (is_ufs2 && !extattr) {
				warnx("%s not supported by this fs", name);
			} else if (sblock.fs_flags & FS_NFS4ACLS) {
				warnx("%s remains unchanged as enabled", name);
			} else if (sblock.fs_flags & FS_POSIX1EACLS) {
				warnx("%s and POSIX.1e ACLs are mutually "
				    "exclusive", name);
			} else {
				sblock.fs_flags |= FS_NFS4ACLS;
				printf("%s set\n", name);
			}
		} else if (strcmp(avalue, "disable") == 0) {
			if ((~sblock.fs_flags & FS_NFS4ACLS) == FS_NFS4ACLS) {
				warnx("%s remains unchanged as disabled",
				    name);
			} else {
				sblock.fs_flags &= ~FS_NFS4ACLS;
				printf("%s cleared\n", name);
			}
 		}
 	}

	if (pflag) {
		name = "POSIX1e ACLs";
		if (strcmp(pvalue, "enable") == 0) {
			if (is_ufs2 && !extattr) {
				warnx("%s not supported by this fs", name);
			} else if (sblock.fs_flags & FS_POSIX1EACLS) {
				warnx("%s remains unchanged as enabled", name);
			} else if (sblock.fs_flags & FS_NFS4ACLS) {
				warnx("%s and ACLs are mutually "
				    "exclusive", name);
			} else {
				sblock.fs_flags |= FS_POSIX1EACLS;
				printf("%s set\n", name);
			}
		} else if (strcmp(pvalue, "disable") == 0) {
			if ((~sblock.fs_flags & FS_POSIX1EACLS) ==
			    FS_POSIX1EACLS) {
				warnx("%s remains unchanged as disabled",
				    name);
			} else {
				sblock.fs_flags &= ~FS_POSIX1EACLS;
				printf("%s cleared\n", name);
			}
		}
	}

	if (Nflag) {
		printf("%s: current settings of %s\n", getprogname(), special);
		printf("\tmaximum contiguous block count %d\n",
		    sblock.fs_maxcontig);
		printf("\tmaximum blocks per file in a cylinder group %d\n",
		    sblock.fs_maxbpg);
		printf("\tminimum percentage of free space %d%%\n",
		    sblock.fs_minfree);
		printf("\toptimization preference: %s\n", chg[sblock.fs_optim]);
		printf("\taverage file size: %d\n", sblock.fs_avgfilesize);
		printf("\texpected number of files per directory: %d\n",
		    sblock.fs_avgfpdir);
		show_log_info();
		printf("\tquotas");
		if (sblock.fs_flags & FS_DOQUOTA2) {
			if (sblock.fs_quota_flags & FS_Q2_DO_TYPE(USRQUOTA)) {
				printf(" user");
				if (sblock.fs_quota_flags &
				    FS_Q2_DO_TYPE(GRPQUOTA))
					printf(",");
			}
			if (sblock.fs_quota_flags & FS_Q2_DO_TYPE(GRPQUOTA))
				printf(" group");
			printf(" enabled\n");
		} else {
			printf(" disabled\n");
		}
		printf("\tPOSIX.1e ACLs %s\n",
		    (sblock.fs_flags & FS_POSIX1EACLS) ? "enabled" : "disabled");
		printf("\tNFS4 ACLs %s\n",
		    (sblock.fs_flags & FS_NFS4ACLS) ? "enabled" : "disabled");
		printf("%s: no changes made\n", getprogname());
		return 0;
	}

	memcpy(&buf, (char *)&sblock, SBLOCKSIZE);
	if (needswap)
		ffs_sb_swap((struct fs*)&buf, (struct fs*)&buf);

	/* write superblock to original coordinates (use old dev_bsize!) */
	bwrite(sblockloc, buf.data, SBLOCKSIZE, special);

	if (active) {
		struct ufs_args args;
		args.fspec = sfs.f_mntfromname;
		if (mount(MOUNT_FFS, sfs.f_mntonname, sfs.f_flag | MNT_UPDATE,
		     &args, sizeof args) == -1)
			 warn("mount");
		else
			printf("%s: mount of %s on %s updated\n",
			    getprogname(), sfs.f_mntfromname, sfs.f_mntonname);
	}


	/* correct dev_bsize from possibly changed superblock data */
	dev_bsize = sblock.fs_fsize / FFS_FSBTODB(&sblock, 1);

	if (Aflag)
		for (i = 0; i < sblock.fs_ncg; i++)
			bwrite(FFS_FSBTODB(&sblock, cgsblock(&sblock, i)),
			    buf.data, SBLOCKSIZE, special);
	close(fi);
	exit(0);
}

static int
isactive(int fd, struct statvfs *rsfs)
{
	struct stat st0, st;
	struct statvfs *sfs;
	int n;

	if (fstat(fd, &st0) == -1) {
		warn("stat");
		return 0;
	}

	if ((n = getmntinfo(&sfs, 0)) == -1) {
		warn("getmntinfo");
		return 0;
	}

	for (int i = 0; i < n; i++) {
		if (stat(sfs[i].f_mntfromname, &st) == -1)
			continue;
		if (st.st_rdev != st0.st_rdev)
			continue;
		*rsfs = sfs[i];
		return 1;

	}
	return 0;
}

static void
show_log_info(void)
{
	const char *loc;
	uint64_t size, blksize, logsize;
	int print;

	switch (sblock.fs_journal_location) {
	case UFS_WAPBL_JOURNALLOC_NONE:
		print = blksize = 0;
		/* nothing */
		break;
	case UFS_WAPBL_JOURNALLOC_END_PARTITION:
		loc = "end of partition";
		size = sblock.fs_journallocs[UFS_WAPBL_EPART_COUNT];
		blksize = sblock.fs_journallocs[UFS_WAPBL_EPART_BLKSZ];
		print = 1;
		break;
	case UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM:
		loc = "in filesystem";
		size = sblock.fs_journallocs[UFS_WAPBL_INFS_COUNT];
		blksize = sblock.fs_journallocs[UFS_WAPBL_INFS_BLKSZ];
		print = 1;
		break;
	default:
		loc = "unknown";
		size = blksize = 0;
		print = 1;
		break;
	}

	if (print) {
		logsize = size * blksize;

		printf("\tjournal log file location: %s\n", loc);
		printf("\tjournal log file size: ");
		if (logsize == 0)
			printf("0\n");
		else {
			char sizebuf[8];
			humanize_number(sizebuf, 6, size * blksize, "B",
			    HN_AUTOSCALE, HN_B | HN_NOSPACE | HN_DECIMAL);
			printf("%s (%" PRId64 " bytes)", sizebuf, logsize);
		}
		printf("\n");
		printf("\tjournal log flags:");
		if (sblock.fs_journal_flags & UFS_WAPBL_FLAGS_CREATE_LOG)
			printf(" create-log");
		if (sblock.fs_journal_flags & UFS_WAPBL_FLAGS_CLEAR_LOG)
			printf(" clear-log");
		printf("\n");
	}
}

static void
change_log_info(long long logfilesize)
{
	/*
	 * NOTES:
	 *  - only operate on in-filesystem log sizes
	 *  - can't change size of existing log
	 *  - if current is same, no action
	 *  - if current is zero and new is non-zero, set flag to create log
	 *    on next mount
	 *  - if current is non-zero and new is zero, set flag to clear log
	 *    on next mount
	 */
	int in_fs_log;
	uint64_t old_size;

	old_size = 0;
	switch (sblock.fs_journal_location) {
	case UFS_WAPBL_JOURNALLOC_END_PARTITION:
		in_fs_log = 0;
		old_size = sblock.fs_journallocs[UFS_WAPBL_EPART_COUNT] *
		    sblock.fs_journallocs[UFS_WAPBL_EPART_BLKSZ];
		break;

	case UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM:
		in_fs_log = 1;
		old_size = sblock.fs_journallocs[UFS_WAPBL_INFS_COUNT] *
		    sblock.fs_journallocs[UFS_WAPBL_INFS_BLKSZ];
		break;

	case UFS_WAPBL_JOURNALLOC_NONE:
	default:
		in_fs_log = 0;
		old_size = 0;
		break;
	}

	if (logfilesize == 0) {
		/*
		 * Don't clear out the locators - the kernel might need
		 * these to find the log!  Just set the "clear the log"
		 * flag and let the kernel do the rest.
		 */
		sblock.fs_journal_flags |= UFS_WAPBL_FLAGS_CLEAR_LOG;
		sblock.fs_journal_flags &= ~UFS_WAPBL_FLAGS_CREATE_LOG;
		warnx("log file size cleared from %" PRIu64 "", old_size);
		return;
	}

	if (!in_fs_log && logfilesize > 0 && old_size > 0)
		errx(1, "Can't change size of non-in-filesystem log");

	if (old_size == (uint64_t)logfilesize && logfilesize > 0) {
		/* no action */
		warnx("log file size remains unchanged at %lld", logfilesize);
		return;
	}

	if (old_size == 0) {
		/* create new log of desired size next mount */
		sblock.fs_journal_location = UFS_WAPBL_JOURNALLOC_IN_FILESYSTEM;
		sblock.fs_journallocs[UFS_WAPBL_INFS_ADDR] = 0;
		sblock.fs_journallocs[UFS_WAPBL_INFS_COUNT] = logfilesize;
		sblock.fs_journallocs[UFS_WAPBL_INFS_BLKSZ] = 0;
		sblock.fs_journallocs[UFS_WAPBL_INFS_INO] = 0;
		sblock.fs_journal_flags |= UFS_WAPBL_FLAGS_CREATE_LOG;
		sblock.fs_journal_flags &= ~UFS_WAPBL_FLAGS_CLEAR_LOG;
		warnx("log file size set to %lld", logfilesize);
	} else {
		errx(1,
		    "Can't change existing log size from %" PRIu64 " to %lld",
		     old_size, logfilesize);
	} 
}

static void
usage(void)
{

	fprintf(stderr, "usage: tunefs [-AFN] tuneup-options special-device\n");
	fprintf(stderr, "where tuneup-options are:\n");
	fprintf(stderr, "\t-a ACLS: `enable' or `disable'\n");
	fprintf(stderr, "\t-e maximum blocks per file in a cylinder group\n");
	fprintf(stderr, "\t-g average file size\n");
	fprintf(stderr, "\t-h expected number of files per directory\n");
	fprintf(stderr, "\t-l journal log file size (`0' to clear journal)\n");
	fprintf(stderr, "\t-m minimum percentage of free space\n");
	fprintf(stderr, "\t-p POSIX.1e ACLS: `enable' or `disable'\n");
	fprintf(stderr, "\t-o optimization preference (`space' or `time')\n");
	fprintf(stderr, "\t-q quota type (`[no]user' or `[no]group')\n");
	fprintf(stderr, "\t-S sector size\n");
	exit(2);
}

static void
getsb(struct fs *fs, const char *file)
{
	int i;

	for (i = 0; ; i++) {
		if (sblock_try[i] == -1)
			errx(5, "cannot find filesystem superblock");
		bread(sblock_try[i] / dev_bsize, (char *)fs, SBLOCKSIZE, file);
		switch(fs->fs_magic) {
		case FS_UFS2EA_MAGIC:
			extattr = 1;
			/*FALLTHROUGH*/
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
		case FS_UFS1_MAGIC:
			break;
		case FS_UFS2EA_MAGIC_SWAPPED:
			extattr = 1;
			/*FALLTHROUGH*/
		case FS_UFS2_MAGIC_SWAPPED:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
		case FS_UFS1_MAGIC_SWAPPED:
			warnx("%s: swapping byte order", file);
			needswap = 1;
			ffs_sb_swap(fs, fs);
			break;
		default:
			continue;
		}
		if (!is_ufs2 && sblock_try[i] == SBLOCK_UFS2)
			continue;
		if ((is_ufs2 || fs->fs_old_flags & FS_FLAGS_UPDATED)
		    && fs->fs_sblockloc != sblock_try[i])
			continue;
		break;
	}

	dev_bsize = fs->fs_fsize / FFS_FSBTODB(fs, 1);
	sblockloc = sblock_try[i] / dev_bsize;
}

static void
bwrite(daddr_t blk, char *buffer, int size, const char *file)
{
	off_t	offset;

	offset = (off_t)blk * dev_bsize;
	if (lseek(fi, offset, SEEK_SET) == -1)
		err(6, "%s: seeking to %lld", file, (long long)offset);
	if (write(fi, buffer, size) != size)
		err(7, "%s: writing %d bytes", file, size);
}

static void
bread(daddr_t blk, char *buffer, int cnt, const char *file)
{
	off_t	offset;
	int	i;

	offset = (off_t)blk * dev_bsize;
	if (lseek(fi, offset, SEEK_SET) == -1)
		err(4, "%s: seeking to %lld", file, (long long)offset);
	if ((i = read(fi, buffer, cnt)) != cnt)
		errx(5, "%s: short read", file);
}

static int
openpartition(const char *name, int flags, char *device, size_t devicelen)
{
	char		specname[MAXPATHLEN];
	char		rawname[MAXPATHLEN];
	const char	*special, *raw;
	struct fstab	*fs;
	int		fd, oerrno;

	fs = getfsfile(name);
	special = fs ? fs->fs_spec : name;

	raw = getfsspecname(specname, sizeof(specname), special);
	if (raw == NULL)
		err(1, "%s: %s", name, specname);
	special = getdiskrawname(rawname, sizeof(rawname), raw); 
	if (special == NULL)
		special = raw;

	fd = opendisk(special, flags, device, devicelen, 0);
	if (fd == -1 && errno == ENOENT) {
		oerrno = errno;
		strlcpy(device, special, devicelen);
		errno = oerrno;
	}
	return (fd);
}
