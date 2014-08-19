/*	$NetBSD: quot.c,v 1.29.10.3 2014/08/20 00:05:12 tls Exp $	*/

/*
 * Copyright (C) 1991, 1994 Wolfgang Solfrank.
 * Copyright (C) 1991, 1994 TooLs GmbH.
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
 *	This product includes software developed by TooLs GmbH.
 * 4. The name of TooLs GmbH may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY TOOLS GMBH ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL TOOLS GMBH BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__RCSID("$NetBSD: quot.c,v 1.29.10.3 2014/08/20 00:05:12 tls Exp $");
#endif /* not lint */

#include <sys/param.h>
#include <sys/mount.h>
#include <sys/time.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* some flags of what to do: */
static char estimate;
static char count;
static char unused;
static void (*func)(int, struct fs *, const char *);
static long blocksize;
static char *header;

/*
 * Original BSD quot doesn't round to number of frags/blocks,
 * doesn't account for indirection blocks and gets it totally
 * wrong if the	size is a multiple of the blocksize.
 * The new code always counts the number of DEV_BSIZE byte blocks
 * instead of the number of kilobytes and converts them	to
 * kByte when done (on request).
 */
#ifdef	COMPAT
#define	SIZE(n)	((long long)(n))
#else
#define	SIZE(n)	howmany((long long)(n) * DEV_BSIZE, (long long)blocksize)
#endif

#define	INOCNT(fs)	((fs)->fs_ipg)
#define INOSZ(fs) \
	(((fs)->fs_magic == FS_UFS1_MAGIC ? sizeof(struct ufs1_dinode) : \
	sizeof(struct ufs2_dinode)) * INOCNT(fs))

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};
#define       DIP(fs, dp, field) \
	(((fs)->fs_magic == FS_UFS1_MAGIC) ? \
	(dp)->dp1.di_##field : (dp)->dp2.di_##field)


static	int		cmpusers(const void *, const void *);
static	void		dofsizes(int, struct fs *, const char *);
static	void		donames(int, struct fs *, const char *);
static	void		douser(int, struct fs *, const char *);
static	union dinode  *get_inode(int, struct fs *, ino_t);
static	void		ffs_oldfscompat(struct fs *);
static	void		initfsizes(void);
static	void		inituser(void);
static	int		isfree(struct fs *, union dinode *);
static	void		quot(const char *, const char *);
static	void		usage(void) __attribute__((__noreturn__));
static	struct user    *user(uid_t);
static	void		uses(uid_t, daddr_t, time_t);
static	void		usrrehash(void);
static	int		virtualblocks(struct fs *, union dinode *);


static union dinode *
get_inode(int fd, struct fs *super, ino_t ino)
{
	static char *ipbuf;
	static ino_t last;
	
	if (fd < 0) {		/* flush cache */
		if (ipbuf) {
			free(ipbuf);
			ipbuf = NULL;
		}
		return 0;
	}
	
	if (!ipbuf || ino < last || ino >= last + INOCNT(super)) {
		if (!ipbuf
		    && !(ipbuf = malloc(INOSZ(super))))
			errx(1, "allocate inodes");
		last = (ino / INOCNT(super)) * INOCNT(super);
		if (lseek(fd,
		    (off_t)ino_to_fsba(super, last) << super->fs_fshift,
		    0) < 0 ||
		    read(fd, ipbuf, INOSZ(super)) != (ssize_t)INOSZ(super))
			errx(1, "read inodes");
	}

	if (super->fs_magic == FS_UFS1_MAGIC)
		return ((union dinode *)
		    &((struct ufs1_dinode *)ipbuf)[ino % INOCNT(super)]);
	return ((union dinode *)
	    &((struct ufs2_dinode *)ipbuf)[ino % INOCNT(super)]);
}

#ifdef	COMPAT
#define	actualblocks(fs, dp)	(int)(DIP(fs, dp, blocks) / 2)
#else
#define	actualblocks(fs, dp)	(int)(DIP(fs, dp, blocks))
#endif

static int
virtualblocks(struct fs *super, union dinode *dp)
{
	off_t nblk, sz;
	
	sz = DIP(super, dp, size);
#ifdef	COMPAT
	if (ffs_lblkno(super, sz) >= UFS_NDADDR) {
		nblk = ffs_blkroundup(super, sz);
		if (sz == nblk)
			nblk += super->fs_bsize;
	}
	
	return sz / 1024;
#else	/* COMPAT */
	
	if (ffs_lblkno(super, sz) >= UFS_NDADDR) {
		nblk = ffs_blkroundup(super, sz);
		sz = ffs_lblkno(super, nblk);
		sz = howmany(sz - UFS_NDADDR, FFS_NINDIR(super));
		while (sz > 0) {
			nblk += sz * super->fs_bsize;
			/* One block on this level is in the inode itself */
			sz = howmany(sz - 1, FFS_NINDIR(super));
		}
	} else
		nblk = ffs_fragroundup(super, sz);
	
	return nblk / DEV_BSIZE;
#endif	/* COMPAT */
}

static int
isfree(fs, dp)
	struct fs *fs;
	union dinode *dp;
{
#ifdef	COMPAT
	return (DIP(fs, dp, mode) & IFMT) == 0;
#else	/* COMPAT */
	switch (DIP(fs, dp, mode) & IFMT) {
	case IFIFO:
	case IFLNK:		/* should check FASTSYMLINK? */
	case IFDIR:
	case IFREG:
		return 0;
	default:
		return 1;
	}
#endif
}

static struct user {
	uid_t uid;
	char *name;
	daddr_t space;
	long count;
	daddr_t spc30;
	daddr_t spc60;
	daddr_t spc90;
} *users;
static int nusers;

static void
inituser(void)
{
	int i;
	struct user *usr;
	
	if (!nusers) {
		nusers = 8;
		if (!(users = calloc(nusers, sizeof(*users))))
			err(1, "allocate users");
	} else {
		for (usr = users, i = nusers; --i >= 0; usr++) {
			usr->space = usr->spc30 = usr->spc60 = usr->spc90 = 0;
			usr->count = 0;
		}
	}
}

static void
usrrehash(void)
{
	int i;
	struct user *usr, *usrn;
	struct user *svusr;
	
	svusr = users;
	nusers <<= 1;
	if (!(users = calloc(nusers, sizeof(*users))))
		err(1, "allocate users");
	for (usr = svusr, i = nusers >> 1; --i >= 0; usr++) {
		for (usrn = users + (usr->uid&(nusers - 1));
		     usrn->name;
		     usrn--) {
			if (usrn <= users)
				usrn = users + nusers;
		}
		*usrn = *usr;
	}
}

static struct user *
user(uid_t uid)
{
	struct user *usr;
	int i;
	struct passwd *pwd;
	
	for (;;) {
		for (usr = users + (uid & (nusers - 1)), i = nusers;
		     --i >= 0;
		     usr--) {
			if (!usr->name) {
				usr->uid = uid;
				
				if (!(pwd = getpwuid(uid)))
					asprintf(&usr->name, "#%u", uid);
				else
					asprintf(&usr->name, "%s",
					    pwd->pw_name);
				if (!usr->name)
					errx(1, "allocate users");
				return usr;
			} else if (usr->uid == uid)
				return usr;

			if (usr <= users)
				usr = users + nusers;
		}
		usrrehash();
	}
}

static int
cmpusers(u1, u2)
	const void *u1, *u2;
{
	return ((const struct user *)u2)->space - ((const struct user *)u1)->space;
}

#define	sortusers(users)	(qsort((users), nusers, sizeof(struct user), \
				       cmpusers))

static void
uses(uid, blks, act)
	uid_t uid;
	daddr_t blks;
	time_t act;
{
	static time_t today;
	struct user *usr;
	
	if (!today)
		time(&today);
	
	usr = user(uid);
	usr->count++;
	usr->space += blks;
	
	if (today - act > 90L * 24L * 60L * 60L)
		usr->spc90 += blks;
	if (today - act > 60L * 24L * 60L * 60L)
		usr->spc60 += blks;
	if (today - act > 30L * 24L * 60L * 60L)
		usr->spc30 += blks;
}

#ifdef	COMPAT
#define	FSZCNT	500
#else
#define	FSZCNT	512
#endif
struct fsizes {
	struct fsizes *fsz_next;
	daddr_t fsz_first, fsz_last;
	ino_t fsz_count[FSZCNT];
	daddr_t fsz_sz[FSZCNT];
} *fsizes;

static void
initfsizes()
{
	struct fsizes *fp;
	int i;
	
	for (fp = fsizes; fp; fp = fp->fsz_next) {
		for (i = FSZCNT; --i >= 0;) {
			fp->fsz_count[i] = 0;
			fp->fsz_sz[i] = 0;
		}
	}
}

static void
dofsizes(int fd, struct fs *super, const char *name)
{
	ino_t inode, maxino;
	union dinode *dp;
	daddr_t sz, ksz;
	struct fsizes *fp, **fsp;
	int i;
	
	maxino = super->fs_ncg * super->fs_ipg - 1;
#ifdef	COMPAT
	if (!(fsizes = malloc(sizeof(*fsizes))))
		err(1, "alloc fsize structure");
#endif	/* COMPAT */
	for (inode = 0; inode < maxino; inode++) {
		errno = 0;
		if ((dp = get_inode(fd, super, inode))
#ifdef	COMPAT
		    && ((DIP(super, dp, mode) & IFMT) == IFREG
			|| (DIP(dp, mode) & IFMT) == IFDIR)
#else	/* COMPAT */
		    && !isfree(super, dp)
#endif	/* COMPAT */
		    ) {
			sz = estimate ? virtualblocks(super, dp) :
			    actualblocks(super, dp);
#ifdef	COMPAT
			if (sz >= FSZCNT) {
				fsizes->fsz_count[FSZCNT-1]++;
				fsizes->fsz_sz[FSZCNT-1] += sz;
			} else {
				fsizes->fsz_count[sz]++;
				fsizes->fsz_sz[sz] += sz;
			}
#else	/* COMPAT */
			ksz = SIZE(sz);
			for (fsp = &fsizes; (fp = *fsp) != NULL;
			    fsp = &fp->fsz_next) {
				if (ksz < fp->fsz_last)
					break;
			}
			if (!fp || ksz < fp->fsz_first) {
				if (!(fp = malloc(sizeof(*fp))))
					err(1, "alloc fsize structure");
				fp->fsz_next = *fsp;
				*fsp = fp;
				fp->fsz_first = (ksz / FSZCNT) * FSZCNT;
				fp->fsz_last = fp->fsz_first + FSZCNT;
				for (i = FSZCNT; --i >= 0;) {
					fp->fsz_count[i] = 0;
					fp->fsz_sz[i] = 0;
				}
			}
			fp->fsz_count[ksz % FSZCNT]++;
			fp->fsz_sz[ksz % FSZCNT] += sz;
#endif	/* COMPAT */
		} else if (errno)
			errx(1, "%s", name);
	}
	sz = 0;
	for (fp = fsizes; fp; fp = fp->fsz_next) {
		for (i = 0; i < FSZCNT; i++) {
			if (fp->fsz_count[i])
				printf("%ld\t%ld\t%lld\n",
				    (long)(fp->fsz_first + i),
				    (long)fp->fsz_count[i],
				    SIZE(sz += fp->fsz_sz[i]));
		}
	}
}

static void
douser(int fd, struct fs *super, const char *name)
{
	ino_t inode, maxino;
	struct user *usr, *usrs;
	union dinode *dp;
	int n;
	
	maxino = super->fs_ncg * super->fs_ipg - 1;
	for (inode = 0; inode < maxino; inode++) {
		errno = 0;
		if ((dp = get_inode(fd, super, inode))
		    && !isfree(super, dp))
			uses(DIP(super, dp, uid),
			    estimate ? virtualblocks(super, dp) :
			    actualblocks(super, dp), DIP(super, dp, atime));
		else if (errno)
			errx(1, "%s", name);
	}
	if (!(usrs = calloc(nusers, sizeof(*usrs))))
		errx(1, "allocate users");
	memmove(usrs, users, nusers * sizeof(*usrs));
	sortusers(usrs);
	for (usr = usrs, n = nusers; --n >= 0 && usr->count; usr++) {
		printf("%5lld", SIZE(usr->space));
		if (count)
			printf("\t%5ld", usr->count);
		printf("\t%-8s", usr->name);
		if (unused)
			printf("\t%5lld\t%5lld\t%5lld",
			    SIZE(usr->spc30), SIZE(usr->spc60),
			    SIZE(usr->spc90));
		printf("\n");
	}
	free(usrs);
}

static void
donames(int fd, struct fs *super, const char *name)
{
	int c;
	ino_t inode;
#ifdef COMPAT
	ino_t inode1 = -1;
#endif
	ino_t maxino;
	union dinode *dp;
	
	maxino = super->fs_ncg * super->fs_ipg - 1;
	/* first skip the name of the filesystem */
	while ((c = getchar()) != EOF && (c < '0' || c > '9'))
		while ((c = getchar()) != EOF && c != '\n');
	ungetc(c, stdin);
	while (scanf("%" SCNu64, &inode) == 1) {
		if (inode > maxino) {
#ifndef	COMPAT
			warnx("invalid inode %" PRIu64, inode);
#endif
			return;
		}
#ifdef	COMPAT
		if (inode < inode1)
			continue;
#endif
		errno = 0;
		if ((dp = get_inode(fd, super, inode))
		    && !isfree(super, dp)) {
			printf("%s\t", user(DIP(super, dp, uid))->name);
			/* now skip whitespace */
			while ((c = getchar()) == ' ' || c == '\t');
			/* and print out the remainder of the input line */
			while (c != EOF && c != '\n') {
				putchar(c);
				c = getchar();
			}
			putchar('\n');
#ifdef COMPAT
			inode1 = inode;
#endif
		} else {
			if (errno)
				errx(1, "%s", name);
			/* skip this line */
			while ((c = getchar()) != EOF && c != '\n')
				continue;
		}
		if (c == EOF)
			break;
	}
}

static void
usage(void)
{
	const char *p = getprogname();
#ifdef	COMPAT
	fprintf(stderr, "Usage: %s [-nfcvha] [<filesystem> ...]\n", p);
#else	/* COMPAT */
	fprintf(stderr, "Usage: %s [ -acfhknv ] [<filesystem> ... ]\n", p);
#endif	/* COMPAT */
	exit(1);
}

/*
 * Sanity checks for old file systems.
 * Stolen from <sys/lib/libsa/ufs.c>
 */
static void
ffs_oldfscompat(struct fs *fs)
{
	int i;

	if (fs->fs_old_inodefmt < FS_44INODEFMT) {
		quad_t sizepb = fs->fs_bsize;

		fs->fs_maxfilesize = fs->fs_bsize * UFS_NDADDR - 1;
		for (i = 0; i < UFS_NIADDR; i++) {
			sizepb *= FFS_NINDIR(fs);
			fs->fs_maxfilesize += sizepb;
		}
		fs->fs_qbmask = ~fs->fs_bmask;
		fs->fs_qfmask = ~fs->fs_fmask;
	}
}

/*
 * Possible superblock locations ordered from most to least likely.
 */
static int sblock_try[] = SBLOCKSEARCH;
static char superblock[SBLOCKSIZE];


static void
quot(const char *name, const char *mp)
{
	int fd, i;
	struct fs *fs;
	int sbloc;
	
	get_inode(-1, 0, 0);		/* flush cache */
	inituser();
	initfsizes();
	if ((fd = open(name, 0)) < 0) {
		warn("%s", name);
		return;
	}

	for (i = 0; ; i++) {
		sbloc = sblock_try[i];
		if (sbloc == -1) {
			warnx("%s: not a BSD filesystem", name);
			close(fd);
			return;
		}
		if (pread(fd, superblock, SBLOCKSIZE, sbloc) != SBLOCKSIZE)
			continue;
		fs = (struct fs *)superblock;

		if (fs->fs_magic != FS_UFS1_MAGIC &&
		    fs->fs_magic != FS_UFS2_MAGIC)
			continue;

		if (fs->fs_magic == FS_UFS2_MAGIC
		    || fs->fs_old_flags & FS_FLAGS_UPDATED) {
			/* Not the main superblock */
			if (fs->fs_sblockloc != sbloc)
				continue;
		} else {
			/* might be a first alt. id blocksize 64k */
			if (sbloc == SBLOCK_UFS2)
				continue;
		}

		if (fs->fs_bsize > MAXBSIZE ||
		    (size_t)fs->fs_bsize < sizeof(struct fs))
			continue;
		break;
	}

	ffs_oldfscompat((struct fs *)superblock);
	printf("%s:", name);
	if (mp)
		printf(" (%s)", mp);
	putchar('\n');
	(*func)(fd, fs, name);
	close(fd);
}

int
main(int argc, char **argv)
{
	char all = 0;
	struct statvfs *mp;
	char dev[MNAMELEN + 1];
	char *nm;
	int cnt;
	
	func = douser;
#ifndef	COMPAT
	header = getbsize(NULL, &blocksize);
#endif
	while (--argc > 0 && **++argv == '-') {
		while (*++*argv) {
			switch (**argv) {
			case 'n':
				func = donames;
				break;
			case 'c':
				func = dofsizes;
				break;
			case 'a':
				all = 1;
				break;
			case 'f':
				count = 1;
				break;
			case 'h':
				estimate = 1;
				break;
#ifndef	COMPAT
			case 'k':
				blocksize = 1024;
				break;
#endif	/* COMPAT */
			case 'v':
				unused = 1;
				break;
			default:
				usage();
			}
		}
	}
	if (all) {
		cnt = getmntinfo(&mp, MNT_NOWAIT);
		for (; --cnt >= 0; mp++) {
			if (!strncmp(mp->f_fstypename, MOUNT_FFS,
			    sizeof(mp->f_fstypename))) {
				if ((nm =
				    strrchr(mp->f_mntfromname, '/')) != NULL) {
					snprintf(dev, sizeof(dev), "/dev/r%s",
					    nm + 1);
					nm = dev;
				} else
					nm = mp->f_mntfromname;
				quot(nm, mp->f_mntonname);
			}
		}
	}
	while (--argc >= 0)
		quot(*argv++, 0);
	return 0;
}
