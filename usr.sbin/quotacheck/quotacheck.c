/*	$NetBSD: quotacheck.c,v 1.46 2013/01/22 09:39:20 dholland Exp $	*/

/*
 * Copyright (c) 1980, 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Robert Elz at The University of Melbourne.
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
__COPYRIGHT("@(#) Copyright (c) 1980, 1990, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)quotacheck.c	8.6 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: quotacheck.c,v 1.46 2013/01/22 09:39:20 dholland Exp $");
#endif
#endif /* not lint */

/*
 * Fix up / report on disk quotas & usage
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <sys/statvfs.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/quota1.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <err.h>
#include <fcntl.h>
#include <fstab.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util.h>

#include "fsutil.h"
#include "quotautil.h"

#ifndef FS_UFS1_MAGIC
# define FS_UFS1_MAGIC		FS_MAGIC /* 0x011954 */
# define FS_UFS1_MAGIC_SWAPPED	0x54190100 /* bswap32(0x011954) */
# define DINODE1_SIZE		sizeof(struct dinode)
# define DINODE2_SIZE		0
#else
# define HAVE_UFSv2	1
#endif

#ifndef SBLOCKSIZE
# define SBLOCKSIZE	SBSIZE
#endif
#ifndef SBLOCKSEARCH
# define SBLOCKSEARCH	{ SBSIZE, -1 }
#endif

static const char *quotagroup = QUOTAGROUP;

static union {
	struct	fs	sblk;
	char	dummy[MAXBSIZE];
} un;
#define	sblock	un.sblk
static long dev_bsize;
static long maxino;

struct quotaname {
	long	flags;
	char	grpqfname[MAXPATHLEN + 1];
	char	usrqfname[MAXPATHLEN + 1];
};
#define	HASUSR	1
#define	HASGRP	2

struct fileusage {
	struct	fileusage *fu_next;
	u_long	fu_curinodes;
	u_long	fu_curblocks;
	uint32_t fu_id;		/* uid_t, gid_t */
	char	fu_name[1];
	/* actually bigger */
};
#define FUHASH 1024	/* must be power of two */
static struct fileusage *fuhead[MAXQUOTAS][FUHASH];


union comb_dinode {
#ifdef HAVE_UFSv2
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
#else
	struct dinode dp1;
#endif
};
#ifdef HAVE_UFSv2
#define DIP(dp, field) \
	(is_ufs2 ? (dp)->dp2.di_##field : (dp)->dp1.di_##field)
#else
#define DIP(dp, field) (dp)->dp1.di_##field
#endif


static int	aflag;		/* all file systems */
static int	gflag;		/* check group quotas */
static int	uflag;		/* check user quotas */
static int	vflag;		/* verbose */
static int	qflag;		/* quick but untidy mode */
static int	fi;		/* open disk file descriptor */
static uint32_t highid[MAXQUOTAS];/* highest addid()'ed identifier per type */
static int needswap;	/* FS is in swapped order */
static int got_siginfo = 0; /* got a siginfo signal */
static int is_ufs2;


static void usage(void) __attribute__((__noreturn__));
static void *needchk(struct fstab *);
static int chkquota(const char *, const char *, const char *, void *, pid_t *);
static int update(const char *, const char *, int);
static uint32_t skipforward(uint32_t, uint32_t, FILE *);
static int getquotagid(void);
static struct fileusage *lookup(uint32_t, int);
static struct fileusage *addid(uint32_t, int, const char *);
static uint32_t subsequent(uint32_t, int) ;
static union comb_dinode *getnextinode(ino_t);
static void setinodebuf(ino_t);
static void freeinodebuf(void);
static void bread(daddr_t, char *, long);
static void infohandler(int sig);
static void swap_dinode1(union comb_dinode *, int);
#ifdef HAVE_UFSv2
static void swap_dinode2(union comb_dinode *, int);
#endif

int
main(int argc, char *argv[])
{
	struct fstab *fs;
	struct passwd *pw;
	struct group *gr;
	struct quotaname *auxdata;
	int i, argnum, maxrun, errs;
	long done = 0;
	int flags = CHECK_PREEN;
	const char *name;
	int ch;

	errs = maxrun = 0;
	while ((ch = getopt(argc, argv, "aguvqdl:")) != -1) {
		switch(ch) {
		case 'a':
			aflag++;
			break;
		case 'd':
			flags |= CHECK_DEBUG;
			break;
		case 'g':
			gflag++;
			break;
		case 'u':
			uflag++;
			break;
		case 'q':
			qflag++;
			break;
		case 'v':
			vflag++;
			break;
		case 'l':
			maxrun = atoi(optarg);
			break;
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;
	if ((argc == 0 && !aflag) || (argc > 0 && aflag) || (!aflag && maxrun))
		usage();
	if (!gflag && !uflag) {
		gflag++;
		uflag++;
	}

	/* If -a, we do not want to pay the cost of processing every
	 * group and password entry if there are no filesystems with quotas
	 */
	if (aflag) {
		i = 0;
		while ((fs = getfsent()) != NULL) {
			if (needchk(fs))
				i = 1;
		}
		endfsent();
		if (!i)	/* No filesystems with quotas */
			return 0;
	}

	if (gflag) {
		setgrent();
		while ((gr = getgrent()) != 0)
			(void) addid((uint32_t)gr->gr_gid, GRPQUOTA, gr->gr_name);
		endgrent();
	}
	if (uflag) {
		setpwent();
		while ((pw = getpwent()) != 0)
			(void) addid((uint32_t)pw->pw_uid, USRQUOTA, pw->pw_name);
		endpwent();
	}
	if (aflag)
		exit(checkfstab(flags, maxrun, needchk, chkquota));
	if (setfsent() == 0)
		err(1, "%s: can't open", FSTAB);
	while ((fs = getfsent()) != NULL) {
		const char *fsspec;
		char buf[MAXPATHLEN];
		fsspec = getfsspecname(buf, sizeof(buf), fs->fs_spec);
		if (fsspec == NULL) {
			warn("%s", buf);
			continue;
		}
		if (((argnum = oneof(fs->fs_file, argv, argc)) >= 0 ||
		    (argnum = oneof(fsspec, argv, argc)) >= 0) &&
		    (auxdata = needchk(fs)) &&
		    (name = blockcheck(fsspec))) {
			done |= 1 << argnum;
			errs += chkquota(fs->fs_type, name, fs->fs_file,
			    auxdata, NULL);
		}
	}
	endfsent();
	for (i = 0; i < argc; i++)
		if ((done & (1 << i)) == 0)
			warnx("%s not found in %s\n", argv[i], FSTAB);
	return errs;
}

static void
usage(void)
{
	const char *p = getprogname();
	(void)fprintf(stderr, "Usage: %s -a [-gquv] [-l <maxparallel>]\n"
	    "\t%s [-gquv] <filesys> ...\n", p, p);
	exit(1);
}

static void *
needchk(struct fstab *fs)
{
	struct quotaname *qnp;
	char qfnp[MAXPATHLEN];

	if (strcmp(fs->fs_vfstype, "ffs") ||
	    strcmp(fs->fs_type, FSTAB_RW))
		return (NULL);
	if ((qnp = malloc(sizeof(*qnp))) == NULL)
		err(1, "%s", strerror(errno));
	qnp->flags = 0;
	if (gflag && hasquota(qfnp, sizeof(qfnp), fs, GRPQUOTA)) {
		strlcpy(qnp->grpqfname, qfnp, sizeof(qnp->grpqfname));
		qnp->flags |= HASGRP;
	}
	if (uflag && hasquota(qfnp, sizeof(qfnp), fs, USRQUOTA)) {
		strlcpy(qnp->usrqfname, qfnp, sizeof(qnp->usrqfname));
		qnp->flags |= HASUSR;
	}
	if (qnp->flags)
		return (qnp);
	free(qnp);
	return (NULL);
}

static off_t sblock_try[] = SBLOCKSEARCH;

/*
 * Scan the specified filesystem to check quota(s) present on it.
 */
static int
chkquota(const char *type, const char *fsname, const char *mntpt, void *v,
    pid_t *pid)
{
	struct quotaname *qnp = v;
	struct fileusage *fup;
	union comb_dinode *dp;
	int cg, i, mode, errs = 0, inosused;
	ino_t ino;
	struct cg *cgp;
	char msgbuf[4096];

	if (pid != NULL) {
		fflush(stdout);
		switch ((*pid = fork())) {
		default:
			return 0;
		case 0:
			break;
		case -1:
			err(1, "Cannot fork");
		}
		setvbuf(stdout, msgbuf, _IOFBF, sizeof msgbuf);
	}

	if ((fi = open(fsname, O_RDONLY, 0)) < 0) {
		warn("Cannot open %s", fsname);
		if (pid != NULL)
			exit(1);
		return 1;
	}
	if (vflag) {
		(void)printf("*** Checking ");
		if (qnp->flags & HASUSR)
			(void)printf("%s%s", qfextension[USRQUOTA],
			    (qnp->flags & HASGRP) ? " and " : "");
		if (qnp->flags & HASGRP)
			(void)printf("%s", qfextension[GRPQUOTA]);
		(void)printf(" quotas for %s (%s)\n", fsname, mntpt);
		fflush(stdout);
	}
	signal(SIGINFO, infohandler);
	sync();
	dev_bsize = 1;

	for (i = 0; ; i++) {
		if (sblock_try[i] == -1) {
			warnx("%s: superblock not found", fsname);
			if (pid != NULL)
				exit(1);
			return 1;
		}
		bread(sblock_try[i], (char *)&sblock, SBLOCKSIZE);
		switch (sblock.fs_magic) {
#ifdef HAVE_UFSv2
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
#endif
		case FS_UFS1_MAGIC:
			break;
#ifdef HAVE_UFSv2
		case FS_UFS2_MAGIC_SWAPPED:
			is_ufs2 = 1;
			/*FALLTHROUGH*/
#endif
		case FS_UFS1_MAGIC_SWAPPED:
			needswap = 1;
			ffs_sb_swap(&sblock, &sblock);
			break;
		default:
			continue;
		}

#ifdef HAVE_UFSv2
		if (is_ufs2 || sblock.fs_old_flags & FS_FLAGS_UPDATED) {
			if (sblock.fs_sblockloc != sblock_try[i])
				continue;
		} else {
			if (sblock_try[i] == SBLOCK_UFS2)
				continue;
		}
#endif
		break;
	}

	cgp = malloc(sblock.fs_cgsize);
	if (cgp == NULL) {
		warn("%s: can't allocate %d bytes of cg space", fsname,
		    sblock.fs_cgsize);
		if (pid != NULL)
			exit(1);
		return 1;
	}

	dev_bsize = sblock.fs_fsize / fsbtodb(&sblock, 1);
	maxino = sblock.fs_ncg * sblock.fs_ipg;
	for (cg = 0; cg < sblock.fs_ncg; cg++) {
		ino = cg * sblock.fs_ipg;
		setinodebuf(ino);
#ifdef HAVE_UFSv2
		if (sblock.fs_magic == FS_UFS2_MAGIC) {
			bread(fsbtodb(&sblock, cgtod(&sblock, cg)), (char *)cgp,
			    sblock.fs_cgsize);
			if (needswap)
				ffs_cg_swap(cgp, cgp, &sblock);
			inosused = cgp->cg_initediblk;
		} else
#endif
			inosused = sblock.fs_ipg;
		for (i = 0; i < inosused; i++, ino++) {
			if (got_siginfo) {
				fprintf(stderr,
				    "%s: cyl group %d of %d (%d%%)\n",
				    fsname, cg, sblock.fs_ncg,
				    cg * 100 / sblock.fs_ncg);
				got_siginfo = 0;
			}
			if (ino < UFS_ROOTINO)
				continue;
			if ((dp = getnextinode(ino)) == NULL)
				continue;
			if ((mode = DIP(dp, mode) & IFMT) == 0)
				continue;
			if (qnp->flags & HASGRP) {
				fup = addid(DIP(dp, gid), GRPQUOTA,
				    (char *)0);
				fup->fu_curinodes++;
				if (mode == IFREG || mode == IFDIR ||
				    mode == IFLNK)
					fup->fu_curblocks += DIP(dp, blocks);
			}
			if (qnp->flags & HASUSR) {
				fup = addid(DIP(dp, uid), USRQUOTA,
				    (char *)0);
				fup->fu_curinodes++;
				if (mode == IFREG || mode == IFDIR ||
				    mode == IFLNK)
					fup->fu_curblocks += DIP(dp, blocks);
			}
		}
	}
	freeinodebuf();
	free(cgp);
	if (qnp->flags & HASUSR)
		errs += update(mntpt, qnp->usrqfname, USRQUOTA);
	if (qnp->flags & HASGRP)
		errs += update(mntpt, qnp->grpqfname, GRPQUOTA);
	close(fi);
	if (pid != NULL)
		exit(errs);
	return errs;
}

/*
 * Update a specified quota file.
 */
static int
update(const char *fsname, const char *quotafile, int type)
{
	struct fileusage *fup;
	FILE *qfi, *qfo;
	uint32_t id, lastid, nextid;
	int need_seek;
	struct dqblk dqbuf;
	static struct dqblk zerodqbuf;
	static struct fileusage zerofileusage;
	struct statvfs *fst;
	int nfst, i;

	nfst = getmntinfo(&fst, MNT_WAIT);
	if (nfst == 0)
		errx(1, "no filesystems mounted!");

	for (i = 0; i < nfst; i++) {
		if (strncmp(fst[i].f_fstypename, "ffs",
		    sizeof(fst[i].f_fstypename)) == 0 &&
		    strncmp(fst[i].f_mntonname, fsname,
		    sizeof(fst[i].f_mntonname)) == 0 &&
		    (fst[i].f_flag & ST_QUOTA) != 0) {
			warnx("filesystem %s has quotas already turned on",
			    fsname);
		}
	}

	if ((qfo = fopen(quotafile, "r+")) == NULL) {
		if (errno == ENOENT)
			qfo = fopen(quotafile, "w+");
		if (qfo) {
			(void) fprintf(stderr,
			    "quotacheck: creating quota file %s\n", quotafile);
#define	MODE	(S_IRUSR|S_IWUSR|S_IRGRP)
			(void) fchown(fileno(qfo), getuid(), getquotagid());
			(void) fchmod(fileno(qfo), MODE);
		} else {
			(void) fprintf(stderr,
			    "quotacheck: %s: %s\n", quotafile, strerror(errno));
			return (1);
		}
	}
	if ((qfi = fopen(quotafile, "r")) == NULL) {
		(void) fprintf(stderr,
		    "quotacheck: %s: %s\n", quotafile, strerror(errno));
		(void) fclose(qfo);
		return (1);
	}
	need_seek = 1;
	for (lastid = highid[type], id = 0; id <= lastid; id = nextid) {
		if (fread((char *)&dqbuf, sizeof(struct dqblk), 1, qfi) == 0)
			dqbuf = zerodqbuf;
		if ((fup = lookup(id, type)) == 0)
			fup = &zerofileusage;

		nextid = subsequent(id, type);
		/* watch out for id == UINT32_MAX */
		if (nextid > 0 && nextid != id + 1)
			nextid = skipforward(id, nextid, qfi);

		if (got_siginfo) {
			/*
			 * XXX this could try to show percentage through
			 * the ID list
			 */
			fprintf(stderr, "%s: updating %s quotas for id=%"
			    PRIu32 " (%s)\n", fsname,
			    qfextension[type < MAXQUOTAS ? type : MAXQUOTAS],
			    id, fup->fu_name);
			got_siginfo = 0;
		}
		if (dqbuf.dqb_curinodes == fup->fu_curinodes &&
		    dqbuf.dqb_curblocks == fup->fu_curblocks) {
			fup->fu_curinodes = 0;	/* reset usage  */
			fup->fu_curblocks = 0;	/* for next filesystem */

			need_seek = 1;
			/* infinite loop avoidance (OR do as "nextid < id"?) */
			if (id == UINT32_MAX || nextid == 0) {
				break;
			}
			continue;
		}
		if (vflag) {
			if (aflag)
				printf("%s: ", fsname);
			printf("%-8s fixed:", fup->fu_name);
			if (dqbuf.dqb_curinodes != fup->fu_curinodes)
				(void)printf("\tinodes %d -> %ld",
					dqbuf.dqb_curinodes, fup->fu_curinodes);
			if (dqbuf.dqb_curblocks != fup->fu_curblocks)
				(void)printf("\tblocks %d -> %ld",
					dqbuf.dqb_curblocks, fup->fu_curblocks);
			(void)printf("\n");
		}
		/*
		 * Reset time limit if have a soft limit and were
		 * previously under it, but are now over it.
		 */
		if (dqbuf.dqb_bsoftlimit &&
		    dqbuf.dqb_curblocks < dqbuf.dqb_bsoftlimit &&
		    fup->fu_curblocks >= dqbuf.dqb_bsoftlimit)
			dqbuf.dqb_btime = 0;
		if (dqbuf.dqb_isoftlimit &&
		    dqbuf.dqb_curinodes < dqbuf.dqb_isoftlimit &&
		    fup->fu_curinodes >= dqbuf.dqb_isoftlimit)
			dqbuf.dqb_itime = 0;
		dqbuf.dqb_curinodes = fup->fu_curinodes;
		dqbuf.dqb_curblocks = fup->fu_curblocks;

		if (need_seek) {
			(void) fseeko(qfo, (off_t)id * sizeof(struct dqblk),
			    SEEK_SET);
			need_seek = nextid != id + 1;
		}
		(void) fwrite(&dqbuf, sizeof(struct dqblk), 1, qfo);

		fup->fu_curinodes = 0;
		fup->fu_curblocks = 0;
		/* infinite loop avoidance (OR do as "nextid < id"?) */
		if (id == UINT32_MAX || nextid == 0) {
			break;
		}
	}
	(void)fclose(qfi);
	(void)fflush(qfo);
	if (highid[type] != UINT32_MAX)
		(void)ftruncate(fileno(qfo),
		    (off_t)((highid[type] + 1) * sizeof(struct dqblk)));
	(void)fclose(qfo);
	return 0;
}

static uint32_t
skipforward(uint32_t cur, uint32_t to, FILE *qfi)
{
	struct dqblk dqbuf;

	if (qflag) {
		(void)fseeko(qfi, (off_t)to * sizeof(struct dqblk), SEEK_SET);
		return to;
	}

	while (++cur < to) {
		/*
		 * if EOF occurs, nothing left to read, we're done
		 */
		if (fread((char *)&dqbuf, sizeof(struct dqblk), 1, qfi) == 0)
			return (to);

		/*
		 * If we find an entry that shows usage, before the next
		 * id that has actual usage, we have to stop here, so the
		 * incorrect entry can be corrected in the file
		 */
		if (dqbuf.dqb_curinodes != 0 || dqbuf.dqb_curblocks != 0) {
			(void)fseek(qfi, -(long)sizeof(struct dqblk), SEEK_CUR);
			return cur;
		}
	}
	return to;
}

/*
 * Determine the group identifier for quota files.
 */
static int
getquotagid(void)
{
	struct group *gr;

	if ((gr = getgrnam(quotagroup)) != NULL)
		return gr->gr_gid;
	return -1;
}

/*
 * Routines to manage the file usage table.
 *
 * Lookup an id of a specific type.
 */
static struct fileusage *
lookup(uint32_t id, int type)
{
	struct fileusage *fup;

	for (fup = fuhead[type][id & (FUHASH-1)]; fup != 0; fup = fup->fu_next)
		if (fup->fu_id == id)
			return fup;
	return NULL;
}

/*
 * Add a new file usage id if it does not already exist.
 */
static struct fileusage *
addid(uint32_t id, int type, const char *name)
{
	struct fileusage *fup, **fhp;
	size_t len;

	if ((fup = lookup(id, type)) != NULL)
		return fup;
	if (name)
		len = strlen(name);
	else
		len = 10;
	if ((fup = calloc(1, sizeof(*fup) + len)) == NULL)
		err(1, "%s", strerror(errno));
	fhp = &fuhead[type][id & (FUHASH - 1)];
	fup->fu_next = *fhp;
	*fhp = fup;
	fup->fu_id = id;
	if (id > highid[type])
		highid[type] = id;
	if (name)
		memmove(fup->fu_name, name, len + 1);
	else
		(void)snprintf(fup->fu_name, len + 1, "%" PRIu32, id);
	return fup;
}

static uint32_t
subsequent(uint32_t id, int type)
{
	struct fileusage *fup, **iup, **cup;
	uint32_t next, offset;

	next = highid[type] + 1;
	offset = 0;
	cup = iup = &fuhead[type][id & (FUHASH-1)];
	do {
		++offset;
		if (++cup >= &fuhead[type][FUHASH])
			cup = &fuhead[type][0];
		for (fup = *cup; fup != 0; fup = fup->fu_next) {
			if (fup->fu_id > id && fup->fu_id <= id + offset)
				return (fup->fu_id);
			if (fup->fu_id > id && fup->fu_id < next)
				next = fup->fu_id;
		}
	} while (cup != iup);

	return next;
}

/*
 * Special purpose version of ginode used to optimize first pass
 * over all the inodes in numerical order.
 */
static ino_t nextino, lastinum, lastvalidinum;
static long readcnt, readpercg, fullcnt, inobufsize, partialcnt, partialsize;
static union comb_dinode *inodebuf;
#define INOBUFSIZE	56*1024	/* size of buffer to read inodes */

static union comb_dinode *
getnextinode(ino_t inumber)
{
	long size;
	daddr_t dblk;
	static union comb_dinode *dp;
	union comb_dinode *ret;

	if (inumber != nextino++ || inumber > lastvalidinum) {
		errx(1, "bad inode number %llu to nextinode",
		    (unsigned long long)inumber);
	}

	if (inumber >= lastinum) {
		readcnt++;
		dblk = fsbtodb(&sblock, ino_to_fsba(&sblock, lastinum));
		if (readcnt % readpercg == 0) {
			size = partialsize;
			lastinum += partialcnt;
		} else {
			size = inobufsize;
			lastinum += fullcnt;
		}
		(void)bread(dblk, (caddr_t)inodebuf, size);
		if (needswap) {
#ifdef HAVE_UFSv2
			if (is_ufs2)
				swap_dinode2(inodebuf, lastinum - inumber);
			else
#endif
				swap_dinode1(inodebuf, lastinum - inumber);
		}
		dp = (union comb_dinode *)inodebuf;
	}
	ret = dp;
	dp = (union comb_dinode *)
	    ((char *)dp + (is_ufs2 ? DINODE2_SIZE : DINODE1_SIZE));
	return ret;
}

static void
setinodebuf(ino_t inum)
{

	if (inum % sblock.fs_ipg != 0)
		errx(1, "bad inode number %llu to setinodebuf",
		    (unsigned long long)inum);

	lastvalidinum = inum + sblock.fs_ipg - 1;
	nextino = inum;
	lastinum = inum;
	readcnt = 0;
	if (inodebuf != NULL)
		return;
	inobufsize = blkroundup(&sblock, INOBUFSIZE);
	fullcnt = inobufsize / (is_ufs2 ? DINODE2_SIZE : DINODE1_SIZE);
	readpercg = sblock.fs_ipg / fullcnt;
	partialcnt = sblock.fs_ipg % fullcnt;
	partialsize = partialcnt * (is_ufs2 ? DINODE2_SIZE : DINODE1_SIZE);
	if (partialcnt != 0) {
		readpercg++;
	} else {
		partialcnt = fullcnt;
		partialsize = inobufsize;
	}
	if (inodebuf == NULL &&
	    (inodebuf = malloc((unsigned)inobufsize)) == NULL)
		errx(1, "Cannot allocate space for inode buffer");
	while (nextino < UFS_ROOTINO)
		getnextinode(nextino);
}

static void
freeinodebuf(void)
{

	free(inodebuf);
	inodebuf = NULL;
}


#ifdef HAVE_UFSv2
static void
swap_dinode1(union comb_dinode *dp, int n)
{
	int i;
	struct ufs1_dinode *dp1;

	dp1 = (struct ufs1_dinode *)&dp->dp1;
	for (i = 0; i < n; i++, dp1++)
		ffs_dinode1_swap(dp1, dp1);
}

static void
swap_dinode2(union comb_dinode *dp, int n)
{
	int i;
	struct ufs2_dinode *dp2;

	dp2 = (struct ufs2_dinode *)&dp->dp2;
	for (i = 0; i < n; i++, dp2++)
		ffs_dinode2_swap(dp2, dp2);
}

#else

static void
swap_dinode1(union comb_dinode *dp, int n)
{
	int i;
	struct dinode *dp1;

	dp1 = (struct dinode *) &dp->dp1;
	for (i = 0; i < n; i++, dp1++)
		ffs_dinode_swap(dp1, dp1);
}

#endif

/*
 * Read specified disk blocks.
 */
static void
bread(daddr_t bno, char *buf, long cnt)
{

	if (lseek(fi, (off_t)bno * dev_bsize, SEEK_SET) < 0 ||
	    read(fi, buf, cnt) != cnt)
		err(1, "block %lld", (long long)bno);
}

static void    
infohandler(int sig)
{
	got_siginfo = 1;
} 
