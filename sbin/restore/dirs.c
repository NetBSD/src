/*	$NetBSD: dirs.c,v 1.51 2015/03/02 03:17:24 enami Exp $	*/

/*
 * Copyright (c) 1983, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
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
#if 0
static char sccsid[] = "@(#)dirs.c	8.7 (Berkeley) 5/1/95";
#else
__RCSID("$NetBSD: dirs.c,v 1.51 2015/03/02 03:17:24 enami Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <protocols/dumprestore.h>

#include <err.h>
#include <errno.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <machine/endian.h>

#include "restore.h"
#include "extern.h"

/*
 * Symbol table of directories read from tape.
 */
#define HASHSIZE	1000
#define INOHASH(val) (val % HASHSIZE)
struct inotab {
	struct	inotab *t_next;
	ino_t	t_ino;
	int32_t	t_seekpt;
	int32_t	t_size;
};
static struct inotab *inotab[HASHSIZE];

/*
 * Information retained about directories.
 */
struct modeinfo {
	ino_t ino;
	struct timespec ctimep[2];
	struct timespec mtimep[2];
	mode_t mode;
	uid_t uid;
	gid_t gid;
	int flags;
};

/*
 * Definitions for library routines operating on directories.
 */
#undef DIRBLKSIZ
#define DIRBLKSIZ 1024
struct rstdirdesc {
	int	dd_fd;
	int32_t	dd_loc;
	int32_t	dd_size;
	char	dd_buf[DIRBLKSIZ];
};

/*
 * Global variables for this file.
 */
static long	seekpt;
static FILE	*df;
static RST_DIR	*dirp;
static char	dirfile[MAXPATHLEN] = "#";	/* No file */
static char	modefile[MAXPATHLEN] = "#";	/* No file */
static char	dot[2] = ".";			/* So it can be modified */

/*
 * Format of old style directories.
 */
#define ODIRSIZ 14
struct odirect {
	u_short	d_ino;
	char	d_name[ODIRSIZ];
};

static struct inotab	*allocinotab(FILE *, struct context *, long);
static void		 dcvt(struct odirect *, struct direct *);
static void		 flushent(void);
static struct inotab	*inotablookup(ino_t);
static RST_DIR		*opendirfile(const char *);
static void		 putdir(char *, long);
static void		 putent(struct direct *);
static void		 rst_seekdir(RST_DIR *, long, long);
static long		 rst_telldir(RST_DIR *);
static struct direct	*searchdir(ino_t, char *);

/*
 *	Extract directory contents, building up a directory structure
 *	on disk for extraction by name.
 *	If genmode is requested, save mode, owner, and times for all
 *	directories on the tape.
 */
void
extractdirs(int genmode)
{
	FILE *mf;
	int i, dfd, mfd;
	struct inotab *itp;
	struct direct nulldir;

	mf = NULL;
	vprintf(stdout, "Extract directories from tape\n");
	(void) snprintf(dirfile, sizeof(dirfile), "%s/rstdir%d",
	    tmpdir, (int)dumpdate);
	if (command != 'r' && command != 'R') {
		(void) snprintf(dirfile, sizeof(dirfile), "%s/rstdir%d-XXXXXX",
		    tmpdir, (int)dumpdate);
		if ((dfd = mkstemp(dirfile)) == -1)
			err(1, "cannot mkstemp temporary file %s", dirfile);
		df = fdopen(dfd, "w");
	}
	else
		df = fopen(dirfile, "w");
	if (df == NULL)
		err(1, "cannot open temporary file %s", dirfile);

	if (genmode != 0) {
		(void) snprintf(modefile, sizeof(modefile), "%s/rstmode%d",
		    tmpdir, (int)dumpdate);
		if (command != 'r' && command != 'R') {
			(void) snprintf(modefile, sizeof(modefile),
			    "%s/rstmode%d-XXXXXX", tmpdir, (int)dumpdate);
			if ((mfd = mkstemp(modefile)) == -1)
				err(1, "cannot mkstemp temporary file %s",
				    modefile);
			mf = fdopen(mfd, "w");
		}
		else
			mf = fopen(modefile, "w");
		if (mf == NULL)
			err(1, "cannot open temporary file %s", modefile);
	}
	nulldir.d_ino = 0;
	nulldir.d_type = DT_DIR;
	nulldir.d_namlen = 1;
	(void) strcpy(nulldir.d_name, "/");
	nulldir.d_reclen = UFS_DIRSIZ(0, &nulldir, 0);
	for (;;) {
		curfile.name = "<directory file - name unknown>";
		curfile.action = USING;
		if (curfile.mode == 0 || (curfile.mode & IFMT) != IFDIR) {
			(void) fclose(df);
			dirp = opendirfile(dirfile);
			if (dirp == NULL)
				fprintf(stderr, "opendirfile: %s\n",
				    strerror(errno));
			if (mf != NULL)
				(void) fclose(mf);
			i = dirlookup(dot);
			if (i == 0)
				panic("Root directory is not on tape\n");
			return;
		}
		itp = allocinotab(mf, &curfile, seekpt);
		getfile(putdir, xtrnull);
		putent(&nulldir);
		flushent();
		itp->t_size = seekpt - itp->t_seekpt;
	}
}

/*
 * skip over all the directories on the tape
 */
void
skipdirs(void)
{

	while (curfile.ino && (curfile.mode & IFMT) == IFDIR) {
		skipfile();
	}
}

/*
 *	Recursively find names and inumbers of all files in subtree
 *	pname and pass them off to be processed.
 */
void
treescan(const char *pname, ino_t ino, long (*todo)(const char *, ino_t, int))
{
	struct inotab *itp;
	struct direct *dp;
	size_t namelen;
	long bpt;
	char locname[MAXPATHLEN + 1];

	itp = inotablookup(ino);
	if (itp == NULL) {
		/*
		 * Pname is name of a simple file or an unchanged directory.
		 */
		(void) (*todo)(pname, ino, LEAF);
		return;
	}
	/*
	 * Pname is a dumped directory name.
	 */
	if ((*todo)(pname, ino, NODE) == FAIL)
		return;
	/*
	 * begin search through the directory
	 * skipping over "." and ".."
	 */
	(void) snprintf(locname, sizeof(locname), "%s/", pname);
	namelen = strlen(locname);
	rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	dp = rst_readdir(dirp); /* "." */
	if (dp != NULL && strcmp(dp->d_name, ".") == 0)
		dp = rst_readdir(dirp); /* ".." */
	else
		fprintf(stderr, "Warning: `.' missing from directory %s\n",
			pname);
	if (dp != NULL && strcmp(dp->d_name, "..") == 0)
		dp = rst_readdir(dirp); /* first real entry */
	else
		fprintf(stderr, "Warning: `..' missing from directory %s\n",
			pname);
	bpt = rst_telldir(dirp);
	/*
	 * a zero inode signals end of directory
	 */
	while (dp != NULL) {
		locname[namelen] = '\0';
		if (namelen + dp->d_namlen >= sizeof(locname)) {
			fprintf(stderr, "%s%s: name exceeds %lu char\n",
			    locname, dp->d_name, (u_long)(sizeof(locname) - 1));
		} else {
			(void) strncat(locname, dp->d_name, (int)dp->d_namlen);
			locname[namelen + dp->d_namlen] = '\0';
			treescan(locname, dp->d_ino, todo);
			rst_seekdir(dirp, bpt, itp->t_seekpt);
		}
		dp = rst_readdir(dirp);
		bpt = rst_telldir(dirp);
	}
}

/*
 * Lookup a pathname which is always assumed to start from the root inode.
 */
struct direct *
pathsearch(const char *pathname)
{
	ino_t ino;
	struct direct *dp;
	char *path, *name, buffer[MAXPATHLEN];

	strcpy(buffer, pathname);
	path = buffer;
	ino = UFS_ROOTINO;
	while (*path == '/')
		path++;
	dp = NULL;
	while ((name = strsep(&path, "/")) != NULL && *name != '\0') {
		if ((dp = searchdir(ino, name)) == NULL)
			return (NULL);
		ino = dp->d_ino;
	}
	return (dp);
}

/*
 * Lookup the requested name in directory inum.
 * Return its inode number if found, zero if it does not exist.
 */
static struct direct *
searchdir(ino_t inum, char *name)
{
	struct direct *dp;
	struct inotab *itp;
	int len;

	itp = inotablookup(inum);
	if (itp == NULL)
		return (NULL);
	rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	len = strlen(name);
	do {
		dp = rst_readdir(dirp);
		if (dp == NULL)
			return (NULL);
	} while (dp->d_namlen != len || strncmp(dp->d_name, name, len) != 0);
	return (dp);
}

/*
 * Put the directory entries in the directory file
 */
static void
putdir(char *buf, long size)
{
	struct direct cvtbuf;
	struct odirect *odp;
	struct odirect *eodp;
	struct direct *dp;
	long loc, i;

	if (cvtflag) {
		eodp = (struct odirect *)&buf[size];
		for (odp = (struct odirect *)buf; odp < eodp; odp++)
			if (odp->d_ino != 0) {
				dcvt(odp, &cvtbuf);
				putent(&cvtbuf);
			}
	} else {
		for (loc = 0; loc < size; ) {
			dp = (struct direct *)(buf + loc);
			if (Bcvt) {
				dp->d_ino = bswap32(dp->d_ino);
				dp->d_reclen = bswap16(dp->d_reclen);
			}
			if (oldinofmt && dp->d_ino != 0) {
#				if BYTE_ORDER == BIG_ENDIAN
					if (Bcvt)
						dp->d_namlen = dp->d_type;
#				else
					if (!Bcvt)
						dp->d_namlen = dp->d_type;
#				endif
				dp->d_type = DT_UNKNOWN;
			}
			i = DIRBLKSIZ - (loc & (DIRBLKSIZ - 1));
			if ((dp->d_reclen & 0x3) != 0 ||
			    dp->d_reclen > i ||
			    dp->d_reclen < UFS_DIRSIZ(0, dp, 0) /* ||
			    dp->d_namlen > NAME_MAX */) {
				vprintf(stdout, "Mangled directory: ");
				if ((dp->d_reclen & 0x3) != 0)
					vprintf(stdout,
					   "reclen not multiple of 4 ");
				if (dp->d_reclen < UFS_DIRSIZ(0, dp, 0))
					vprintf(stdout,
					   "reclen less than UFS_DIRSIZ (%d < %lu) ",
					   dp->d_reclen, (u_long)UFS_DIRSIZ(0, dp, 0));
#if 0	/* dp->d_namlen is a uint8_t, always < NAME_MAX */
				if (dp->d_namlen > NAME_MAX)
					vprintf(stdout,
					   "reclen name too big (%d > %d) ",
					   dp->d_namlen, NAME_MAX);
#endif
				vprintf(stdout, "\n");
				loc += i;
				continue;
			}
			loc += dp->d_reclen;
			if (dp->d_ino != 0) {
				putent(dp);
			}
		}
	}
}

/*
 * These variables are "local" to the following two functions.
 */
char dirbuf[DIRBLKSIZ];
long dirloc = 0;
long prev = 0;

/*
 * add a new directory entry to a file.
 */
static void
putent(struct direct *dp)
{
	dp->d_reclen = UFS_DIRSIZ(0, dp, 0);
	if (dirloc + dp->d_reclen > DIRBLKSIZ) {
		((struct direct *)(dirbuf + prev))->d_reclen =
		    DIRBLKSIZ - prev;
		(void) fwrite(dirbuf, 1, DIRBLKSIZ, df);
		dirloc = 0;
	}
	memmove(dirbuf + dirloc, dp, (long)dp->d_reclen);
	prev = dirloc;
	dirloc += dp->d_reclen;
}

/*
 * flush out a directory that is finished.
 */
static void
flushent(void)
{
	((struct direct *)(dirbuf + prev))->d_reclen = DIRBLKSIZ - prev;
	(void) fwrite(dirbuf, (int)dirloc, 1, df);
	seekpt = ftell(df);
	dirloc = 0;
}

static void
dcvt(struct odirect *odp, struct direct *ndp)
{

	memset(ndp, 0, sizeof(*ndp));
	if (Bcvt)
		ndp->d_ino = bswap16(odp->d_ino);
	else
		ndp->d_ino = odp->d_ino;
	ndp->d_type = DT_UNKNOWN;
	(void) strncpy(ndp->d_name, odp->d_name, ODIRSIZ);
	ndp->d_namlen = strlen(ndp->d_name);
	ndp->d_reclen = UFS_DIRSIZ(0, ndp, 0);
}

/*
 * Seek to an entry in a directory.
 * Only values returned by rst_telldir should be passed to rst_seekdir.
 * This routine handles many directories in a single file.
 * It takes the base of the directory in the file, plus
 * the desired seek offset into it.
 */
static void
rst_seekdir(RST_DIR *rdirp, long loc, long base)
{

	if (loc == rst_telldir(rdirp))
		return;
	loc -= base;
	if (loc < 0)
		fprintf(stderr, "bad seek pointer to rst_seekdir %d\n",
		    (int)loc);
	(void) lseek(rdirp->dd_fd, base + (loc & ~(DIRBLKSIZ - 1)), SEEK_SET);
	rdirp->dd_loc = loc & (DIRBLKSIZ - 1);
	if (rdirp->dd_loc != 0)
		rdirp->dd_size = read(rdirp->dd_fd, rdirp->dd_buf, DIRBLKSIZ);
}

/*
 * get next entry in a directory.
 */
struct direct *
rst_readdir(RST_DIR *rdirp)
{
	struct direct *dp;

	for (;;) {
		if (rdirp->dd_loc == 0) {
			rdirp->dd_size = read(rdirp->dd_fd, rdirp->dd_buf,
			    DIRBLKSIZ);
			if (rdirp->dd_size <= 0) {
				dprintf(stderr, "error reading directory\n");
				return (NULL);
			}
		}
		if (rdirp->dd_loc >= rdirp->dd_size) {
			rdirp->dd_loc = 0;
			continue;
		}
		dp = (struct direct *)(rdirp->dd_buf + rdirp->dd_loc);
		if (dp->d_reclen == 0 ||
		    dp->d_reclen > DIRBLKSIZ + 1 - rdirp->dd_loc) {
			dprintf(stderr, "corrupted directory: bad reclen %d\n",
				dp->d_reclen);
			return (NULL);
		}
		rdirp->dd_loc += dp->d_reclen;
		if (dp->d_ino == 0 && strcmp(dp->d_name, "/") == 0)
			return (NULL);
		if (dp->d_ino >= maxino) {
			dprintf(stderr, "corrupted directory: bad inum %d\n",
				dp->d_ino);
			continue;
		}
		return (dp);
	}
}

/*
 * Simulate the opening of a directory
 */
RST_DIR *
rst_opendir(const char *name)
{
	struct inotab *itp;
	RST_DIR *rdirp;
	ino_t ino;

	if ((ino = dirlookup(name)) > 0 &&
	    (itp = inotablookup(ino)) != NULL) {
		rdirp = opendirfile(dirfile);
		rst_seekdir(rdirp, itp->t_seekpt, itp->t_seekpt);
		return (rdirp);
	}
	return (NULL);
}

/*
 * In our case, there is nothing to do when closing a directory.
 */
void
rst_closedir(RST_DIR *rdirp)
{

	(void)close(rdirp->dd_fd);
	free(rdirp);
	return;
}

/*
 * Simulate finding the current offset in the directory.
 */
static long
rst_telldir(RST_DIR *rdirp)
{
	return ((long)lseek(rdirp->dd_fd,
	    (off_t)0, SEEK_CUR) - rdirp->dd_size + rdirp->dd_loc);
}

/*
 * Open a directory file.
 */
static RST_DIR *
opendirfile(const char *name)
{
	RST_DIR *rdirp;
	int fd;

	if ((fd = open(name, O_RDONLY)) == -1)
		return (NULL);
	if ((rdirp = malloc(sizeof(RST_DIR))) == NULL) {
		(void)close(fd);
		return (NULL);
	}
	rdirp->dd_fd = fd;
	rdirp->dd_loc = 0;
	return (rdirp);
}

/*
 * Set the mode, owner, and times for all new or changed directories
 */
void
setdirmodes(int flags)
{
	FILE *mf;
	struct modeinfo node;
	struct entry *ep;
	char *cp;

	vprintf(stdout, "Set directory mode, owner, and times.\n");
	if (command == 'r' || command == 'R')
		(void) snprintf(modefile, sizeof(modefile), "%s/rstmode%d",
		    tmpdir, (int)dumpdate);
	if (modefile[0] == '#') {
		panic("modefile not defined\n");
		fprintf(stderr, "directory mode, owner, and times not set\n");
		return;
	}
	mf = fopen(modefile, "r");
	if (mf == NULL) {
		fprintf(stderr, "fopen: %s\n", strerror(errno));
		fprintf(stderr, "cannot open mode file %s\n", modefile);
		fprintf(stderr, "directory mode, owner, and times not set\n");
		return;
	}
	clearerr(mf);
	for (;;) {
		(void) fread((char *)&node, 1, sizeof(struct modeinfo), mf);
		if (feof(mf))
			break;
		ep = lookupino(node.ino);
		if (command == 'i' || command == 'x') {
			if (ep == NULL)
				continue;
			if ((flags & FORCE) == 0 && ep->e_flags & EXISTED) {
				ep->e_flags &= ~NEW;
				continue;
			}
			if (node.ino == UFS_ROOTINO && dotflag == 0)
				continue;
		}
		if (ep == NULL) {
			panic("cannot find directory inode %llu\n",
			    (unsigned long long)node.ino);
		} else {
			if (!Nflag) {
				cp = myname(ep);
				(void) utimens(cp, node.ctimep);
				(void) utimens(cp, node.mtimep);
				(void) chown(cp, node.uid, node.gid);
				(void) chmod(cp, node.mode);
				if (Mtreefile) {
					writemtree(cp, "dir",
					    node.uid, node.gid, node.mode,
					    node.flags);
				} else 
					(void) chflags(cp, node.flags);
			}
			ep->e_flags &= ~NEW;
		}
	}
	if (ferror(mf))
		panic("error setting directory modes\n");
	(void) fclose(mf);
}

/*
 * Generate a literal copy of a directory.
 */
int
genliteraldir(const char *name, ino_t ino)
{
	struct inotab *itp;
	int ofile, dp, i, size;
	char buf[BUFSIZ];

	itp = inotablookup(ino);
	if (itp == NULL)
		panic("Cannot find directory inode %llu named %s\n",
		    (unsigned long long)ino, name);
	if ((ofile = open(name, O_WRONLY | O_CREAT | O_TRUNC, 0666)) < 0) {
		fprintf(stderr, "%s: ", name);
		(void) fflush(stderr);
		fprintf(stderr, "cannot create file: %s\n", strerror(errno));
		return (FAIL);
	}
	rst_seekdir(dirp, itp->t_seekpt, itp->t_seekpt);
	dp = dup(dirp->dd_fd);
	for (i = itp->t_size; i > 0; i -= BUFSIZ) {
		size = i < BUFSIZ ? i : BUFSIZ;
		if (read(dp, buf, (int) size) == -1) {
			fprintf(stderr,
			    "write error extracting inode %llu, name %s\n",
			    (unsigned long long)curfile.ino, curfile.name);
			fprintf(stderr, "read: %s\n", strerror(errno));
			exit(1);
		}
		if (!Nflag && write(ofile, buf, (int) size) == -1) {
			fprintf(stderr,
			    "write error extracting inode %llu, name %s\n",
			    (unsigned long long)curfile.ino, curfile.name);
			fprintf(stderr, "write: %s\n", strerror(errno));
			exit(1);
		}
	}
	(void) close(dp);
	(void) close(ofile);
	return (GOOD);
}

/*
 * Determine the type of an inode
 */
int
inodetype(ino_t ino)
{
	struct inotab *itp;

	itp = inotablookup(ino);
	if (itp == NULL)
		return (LEAF);
	return (NODE);
}

/*
 * Allocate and initialize a directory inode entry.
 * If requested, save its pertinent mode, owner, and time info.
 */
static struct inotab *
allocinotab(FILE *mf, struct context *ctxp, long aseekpt)
{
	struct inotab	*itp;
	struct modeinfo node;

	itp = calloc(1, sizeof(struct inotab));
	if (itp == NULL)
		panic("no memory directory table\n");
	itp->t_next = inotab[INOHASH(ctxp->ino)];
	inotab[INOHASH(ctxp->ino)] = itp;
	itp->t_ino = ctxp->ino;
	itp->t_seekpt = aseekpt;
	if (mf == NULL)
		return (itp);
	node.ino = ctxp->ino;
	node.mtimep[0].tv_sec = ctxp->atime_sec;
	node.mtimep[0].tv_nsec = ctxp->atime_nsec;
	node.mtimep[1].tv_sec = ctxp->mtime_sec;
	node.mtimep[1].tv_nsec = ctxp->mtime_nsec;
	node.ctimep[0].tv_sec = ctxp->atime_sec;
	node.ctimep[0].tv_nsec = ctxp->atime_nsec;
	node.ctimep[1].tv_sec = ctxp->birthtime_sec;
	node.ctimep[1].tv_nsec = ctxp->birthtime_nsec;
	node.mode = ctxp->mode;
	node.flags = ctxp->file_flags;
	node.uid = ctxp->uid;
	node.gid = ctxp->gid;
	(void) fwrite((char *)&node, 1, sizeof(struct modeinfo), mf);
	return (itp);
}

/*
 * Look up an inode in the table of directories
 */
static struct inotab *
inotablookup(ino_t ino)
{
	struct inotab *itp;

	for (itp = inotab[INOHASH(ino)]; itp != NULL; itp = itp->t_next)
		if (itp->t_ino == ino)
			return (itp);
	return (NULL);
}

/*
 * Clean up and exit
 */
void
cleanup(void)
{

	closemt();
	if (modefile[0] != '#')
		(void) unlink(modefile);
	if (dirfile[0] != '#')
		(void) unlink(dirfile);
}
