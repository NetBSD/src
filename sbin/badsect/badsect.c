/*	$NetBSD: badsect.c,v 1.26 2004/03/21 19:13:59 dsl Exp $	*/

/*
 * Copyright (c) 1981, 1983, 1993
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
__COPYRIGHT("@(#) Copyright (c) 1981, 1983, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)badsect.c	8.2 (Berkeley) 5/4/95";
#else
__RCSID("$NetBSD: badsect.c,v 1.26 2004/03/21 19:13:59 dsl Exp $");
#endif
#endif /* not lint */

/*
 * badsect
 *
 * Badsect takes a list of file-system relative sector numbers
 * and makes files containing the blocks of which these sectors are a part.
 * It can be used to contain sectors which have problems if these sectors
 * are not part of the bad file for the pack (see bad144).  For instance,
 * this program can be used if the driver for the file system in question
 * does not support bad block forwarding.
 */
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/stat.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>

#include <fcntl.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>

union {
	struct	fs fs;
	char	fsx[SBLOCKSIZE];
} ufs;
#define sblock	ufs.fs
union {
	struct	cg cg;
	char	cgx[MAXBSIZE];
} ucg;
#define	acg	ucg.cg
struct	fs *fs;
int	fso, fsi;
int	errs;
long	dev_bsize = 1;
int needswap = 0;
int is_ufs2;

char buf[MAXBSIZE];

void	rdfs __P((daddr_t, int, char *));
int	chkuse __P((daddr_t, int));
int	main __P((int, char *[]));

const off_t sblock_try[] = SBLOCKSEARCH;

int
main(argc, argv)
	int argc;
	char *argv[];
{
	daddr_t number;
	struct stat stbuf, devstat;
	struct direct *dp;
	int i;
	DIR *dirp;
	char name[MAXPATHLEN];

	if (argc < 3) {
		(void) fprintf(stderr, "usage: %s bbdir blkno [ blkno ]\n",
		    getprogname());
		exit(1);
	}
	if (chdir(argv[1]) == -1)
		err(1, "Cannot change directory to `%s'", argv[1]);

	if (stat(".", &stbuf) == -1)
		err(1, "Cannot stat `%s'", argv[1]);

	(void) strlcpy(name, _PATH_DEV, sizeof(name));
	if ((dirp = opendir(name)) == NULL)
		err(1, "Cannot opendir `%s'", argv[1]);

	while ((dp = readdir(dirp)) != NULL) {
		(void) snprintf(name, sizeof(name), "%s%s", _PATH_DEV,
		    dp->d_name);
		if (stat(name, &devstat) == -1)
			err(1, "Cannot stat `%s'", name);
		if (stbuf.st_dev == devstat.st_rdev &&
		    S_ISBLK(devstat.st_mode))
			break;
	}
	if (dp == NULL) {
		closedir(dirp);
		errx(1, "Cannot find dev 0%o corresponding to %s", 
		    stbuf.st_rdev, argv[1]);
	}

	/*
	 * The filesystem is mounted; use the character device instead.
	 * XXX - Assume that prepending an `r' will give us the name of
	 * the character device.
	 */
	(void) snprintf(name, sizeof(name), "%sr%s", _PATH_DEV, dp->d_name);

	closedir(dirp); /* now *dp is invalid */

	if ((fsi = open(name, O_RDONLY)) == -1)
		err(1, "Cannot open `%s'", argv[1]);

	fs = &sblock;

	for (i = 0; ; i++) {
		if (sblock_try[i] == -1)
			errx(1, "%s: bad superblock", name);
		rdfs(sblock_try[i] / DEV_BSIZE, SBLOCKSIZE, (char *)fs);
		switch (fs->fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/* FALLTHROUGH */
		case FS_UFS1_MAGIC:
			break;
		case FS_UFS2_MAGIC_SWAPPED:
			is_ufs2 = 1;
			/* FALLTHROUGH */
		case FS_UFS1_MAGIC_SWAPPED:
			needswap = 1;
			ffs_sb_swap(fs, fs);
			break;
		default:
			continue;
		}

		/* Ensure we don't use 1st alternate if ffsv1 and bs=64k */
		if (fs->fs_old_flags & FS_FLAGS_UPDATED) {
			if (fs->fs_sblockloc != sblock_try[i])
				continue;
		} else {
			if (sblock_try[i] == SBLOCK_UFS2)
				continue;
		}
		break;
	}

	dev_bsize = fs->fs_fsize / fsbtodb(fs, 1);
	for (argc -= 2, argv += 2; argc > 0; argc--, argv++) {
		number = atoi(*argv);
		if (chkuse(number, 1))
			continue;
		if (mknod(*argv, S_IFMT|S_IRUSR|S_IWUSR,
		    dbtofsb(fs, number)) == -1) {
			warn("Cannot mknod `%s'", *argv);
			errs++;
		}
	}

	warnx("Don't forget to run ``fsck %s''", name);
	return errs;
}

int
chkuse(blkno, cnt)
	daddr_t blkno;
	int cnt;
{
	int cg;
	daddr_t fsbn, bn;

	fsbn = dbtofsb(fs, blkno);
	if (fsbn+cnt > fs->fs_size) {
		warnx("block %lld out of range of file system",
		    (long long)blkno);
		return (1);
	}

	cg = dtog(fs, fsbn);
	if (fsbn < cgdmin(fs, cg)) {
		if (cg == 0 || (fsbn+cnt) > cgsblock(fs, cg)) {
			warnx("block %lld in non-data area: cannot attach",
			    (long long)blkno);
			return (1);
		}
	} else {
		if ((fsbn+cnt) > cgbase(fs, cg+1)) {
			warnx("block %lld in non-data area: cannot attach",
			    (long long)blkno);
			return (1);
		}
	}

	rdfs(fsbtodb(fs, cgtod(fs, cg)), (int)sblock.fs_cgsize,
	    (char *)&acg);

	if (!cg_chkmagic(&acg, needswap)) {
		warnx("cg %d: bad magic number", cg);
		errs++;
		return (1);
	}

	bn = dtogd(fs, fsbn);
	if (isclr(cg_blksfree(&acg, needswap), bn))
		warnx("Warning: sector %lld is in use", (long long)blkno);

	return (0);
}

/*
 * read a block from the file system
 */
void
rdfs(bno, size, bf)
	daddr_t bno;
	int size;
	char *bf;
{
	int n;

	if (lseek(fsi, (off_t)bno * dev_bsize, SEEK_SET) == -1)
		err(1, "seek error at block %lld", (long long)bno);

	switch (n = read(fsi, bf, size)) {
	case -1:
		err(1, "read error at block %lld", (long long)bno);
		break;

	default:
		if (n == size)
			return;
		errx(1, "incomplete read at block %lld", (long long)bno);
	}
}
