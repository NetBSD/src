/*	$NetBSD: ffs.c,v 1.14.2.1 2002/05/30 21:09:38 tv Exp $	*/

/*
 * Copyright (c) 2001 Wasabi Systems, Inc.
 * All rights reserved.
 *
 * Written by Luke Mewburn for Wasabi Systems, Inc.
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
 *      This product includes software developed for the NetBSD Project by
 *      Wasabi Systems, Inc.
 * 4. The name of Wasabi Systems, Inc. may not be used to endorse
 *    or promote products derived from this software without specific prior
 *    written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY WASABI SYSTEMS, INC. ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL WASABI SYSTEMS, INC
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1982, 1986, 1989, 1993
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
 *
 *	@(#)ffs_alloc.c	8.19 (Berkeley) 7/13/95
 */

#include <sys/cdefs.h>
#if defined(__RCSID) && !defined(__lint)
__RCSID("$NetBSD: ffs.c,v 1.14.2.1 2002/05/30 21:09:38 tv Exp $");
#endif	/* !__lint */

#include <sys/param.h>
#include <sys/mount.h>

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "makefs.h"

#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <ufs/ufs/ufs_bswap.h>

#include "ffs/ufs_inode.h"
#include "ffs/newfs_extern.h"
#include "ffs/ffs_extern.h"

/*
 * Various file system defaults (cribbed from newfs(8)).
 */
#define	DFL_FRAGSIZE		1024		/* fragment size */
#define	DFL_BLKSIZE		8192		/* block size */
#define	DFL_SECSIZE		512		/* sector size */
#define	DFL_CYLSPERGROUP	65536		/* cylinders per group */
#define	DFL_FRAGSPERINODE	4		/* fragments per inode */
#define	DFL_ROTDELAY		0		/* rotational delay */
#define	DFL_NRPOS		1		/* rotational positions */
#define	DFL_RPM			3600		/* rpm of disk */
#define	DFL_NSECTORS		64		/* # of sectors */
#define	DFL_NTRACKS		16		/* # of tracks */


typedef struct {
	u_char		*buf;		/* buf for directory */
	doff_t		size;		/* full size of buf */
	doff_t		cur;		/* offset of current entry */
} dirbuf_t;


static	int	ffs_create_image(const char *, fsinfo_t *);
static	void	ffs_dump_fsinfo(fsinfo_t *);
static	void	ffs_dump_dirbuf(dirbuf_t *, const char *, int);
static	void	ffs_make_dirbuf(dirbuf_t *, const char *, fsnode *, int);
static	int	ffs_populate_dir(const char *, fsnode *, fsinfo_t *);
static	void	ffs_size_dir(fsnode *, fsinfo_t *);
static	void	ffs_validate(const char *, fsnode *, fsinfo_t *);
static	void	ffs_write_file(struct dinode *, uint32_t, void *, fsinfo_t *);
static	void	ffs_write_inode(struct dinode *, uint32_t, const fsinfo_t *);


int	sectorsize;		/* XXX: for buf.c::getblk() */

	/* publically visible functions */

int
ffs_parse_opts(const char *option, fsinfo_t *fsopts)
{
	option_t ffs_options[] = {
		{ "bsize",	&fsopts->bsize,		1,	INT_MAX,
					"block size" },
		{ "fsize",	&fsopts->fsize,		1,	INT_MAX,
					"fragment size" },
		{ "cpg",	&fsopts->cpg,		1,	INT_MAX,
					"cylinders per group" },
		{ "density",	&fsopts->density,	1,	INT_MAX,
					"bytes per inode" },
		{ "ntracks",	&fsopts->ntracks,	1,	INT_MAX,
					"number of tracks" },
		{ "nsectors",	&fsopts->nsectors,	1,	INT_MAX,
					"number of sectors" },
		{ "rpm",	&fsopts->rpm,		1,	INT_MAX,
					"revolutions per minute" },
		{ "minfree",	&fsopts->minfree,	0,	99,
					"minfree" },
		{ "rotdelay",	&fsopts->rotdelay,	0,	INT_MAX,
					"rotational delay" },
		{ "maxbpg",	&fsopts->maxbpg,	1,	INT_MAX,
					"max blocks per cylgroup" },
		{ "nrpos",	&fsopts->nrpos,		1,	INT_MAX,
					"number of rotational positions" },
		{ "avgfilesize", &fsopts->avgfilesize,	1,	INT_MAX,
					"expected average file size" },
		{ "avgfpdir",	&fsopts->avgfpdir,	1,	INT_MAX,
					"expected # of files per directory" },
		{ NULL }
	};

	char	*var, *val;
	int	rv;

	(void)&ffs_options;
	assert(option != NULL);
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_PARSE_OPTS)
		printf("ffs_parse_opts: got `%s'\n", option);

	if ((var = strdup(option)) == NULL)
		err(1, "Allocating memory for copy of option string");
	rv = 0;

	if ((val = strchr(var, '=')) == NULL) {
		warnx("Option `%s' doesn't contain a value", var);
		goto leave_ffs_parse_opts;
	}
	*val++ = '\0';

	if (strcmp(var, "optimization") == 0) {
		if (strcmp(val, "time") == 0) {
			fsopts->optimization = FS_OPTTIME;
		} else if (strcmp(val, "space") == 0) {
			fsopts->optimization = FS_OPTSPACE;
		} else {
			warnx("Invalid optimization `%s'", val);
			goto leave_ffs_parse_opts;
		}
		rv = 1;
	} else
		rv = set_option(ffs_options, var, val);

 leave_ffs_parse_opts:
	if (var)
		free(var);
	return (rv);
}


void
ffs_makefs(const char *image, const char *dir, fsnode *root, fsinfo_t *fsopts)
{
	struct timeval start;

	assert(image != NULL);
	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_MAKEFS)
		printf("ffs_makefs: image %s directory %s root %p\n",
		    image, dir, root);

		/* validate tree and options */
	TIMER_START(start);
	ffs_validate(dir, root, fsopts);
	TIMER_RESULTS(start, "ffs_validate");

	printf("Calculated size of `%s': %lld bytes, %lld inodes\n",
	    image, (long long)fsopts->size, (long long)fsopts->inodes);

		/* create image */
	TIMER_START(start);
	if (ffs_create_image(image, fsopts) == -1)
		errx(1, "Image file `%s' not created.", image);
	TIMER_RESULTS(start, "ffs_create_image");

	fsopts->curinode = ROOTINO;

	if (debug & DEBUG_FS_MAKEFS)
		putchar('\n');

		/* populate image */
	printf("Populating `%s'\n", image);
	TIMER_START(start);
	if (! ffs_populate_dir(dir, root, fsopts))
		errx(1, "Image file `%s' not populated.", image);
	TIMER_RESULTS(start, "ffs_populate_dir");

		/* ensure no outstanding buffers remain */
	if (debug & DEBUG_FS_MAKEFS)
		bcleanup();

		/* write out superblock; image is now complete */

	((struct fs *)fsopts->superblock)->fs_fmod = 0;
	ffs_write_superblock(fsopts->superblock, fsopts);
	if (close(fsopts->fd) == -1)
		err(1, "Closing `%s'", image);
	fsopts->fd = -1;
	printf("Image `%s' complete\n", image);
}

	/* end of public functions */


static void
ffs_validate(const char *dir, fsnode *root, fsinfo_t *fsopts)
{
	int32_t	ncg = 1;
#if notyet
	int32_t	spc, nspf, ncyl, fssize;
#endif

	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_VALIDATE) {
		printf("ffs_validate: before defaults set:\n");
		ffs_dump_fsinfo(fsopts);
	}

		/* set FFS defaults */
	if (fsopts->sectorsize == -1)
		fsopts->sectorsize = DFL_SECSIZE;
	if (fsopts->fsize == -1)
		fsopts->fsize = MAX(DFL_FRAGSIZE, fsopts->sectorsize);
	if (fsopts->bsize == -1)
		fsopts->bsize = MIN(DFL_BLKSIZE, 8 * fsopts->fsize);
	if (fsopts->cpg == -1)
		fsopts->cpg = DFL_CYLSPERGROUP;
	else
		fsopts->cpgflg = 1;
				/* fsopts->density is set below */
	if (fsopts->ntracks == -1)
		fsopts->ntracks = DFL_NTRACKS;
	if (fsopts->nsectors == -1)
		fsopts->nsectors = DFL_NSECTORS;
	if (fsopts->rpm == -1)
		fsopts->rpm = DFL_RPM;
	if (fsopts->minfree == -1)
		fsopts->minfree = MINFREE;
	if (fsopts->optimization == -1)
		fsopts->optimization = DEFAULTOPT;
	if (fsopts->maxcontig == -1)
		fsopts->maxcontig =
		    MAX(1, MIN(MAXPHYS, MAXBSIZE) / fsopts->bsize);
	if (fsopts->rotdelay == -1)
		fsopts->rotdelay = DFL_ROTDELAY;
	if (fsopts->maxbpg == -1)
		fsopts->maxbpg = fsopts->bsize / sizeof(ufs_daddr_t);
	if (fsopts->nrpos == -1)
		fsopts->nrpos = DFL_NRPOS;
	if (fsopts->avgfilesize == -1)
		fsopts->avgfilesize = AVFILESIZ;
	if (fsopts->avgfpdir == -1)
		fsopts->avgfpdir = AFPDIR;

		/* calculate size of tree */
	ffs_size_dir(root, fsopts);
	fsopts->inodes += ROOTINO;		/* include first two inodes */

	if (debug & DEBUG_FS_VALIDATE)
		printf("ffs_validate: size of tree: %lld bytes, %lld inodes\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);

		/* add requested slop */
	fsopts->size += fsopts->freeblocks;
	fsopts->inodes += fsopts->freefiles;
	if (fsopts->freefilepc > 0)
		fsopts->inodes =
		    fsopts->inodes * (100 + fsopts->freefilepc) / 100;
	if (fsopts->freeblockpc > 0)
		fsopts->size =
		    fsopts->size * (100 + fsopts->freeblockpc) / 100;

#if notyet
		/*
		 * estimate number of cylinder groups
		 */
	spc = fsopts->nsectors * fsopts->ntracks;
	nspf = fsopts->fsize / fsopts->sectorsize;
	fssize = fsopts->size / fsopts->sectorsize;
	ncyl = fssize * nspf / spc;
	if (fssize * nspf > ncyl * spc)
		ncyl++;
	ncg = ncyl / fsopts->cpg;
	if (ncg == 0)
		ncg = 1;
	if (debug & DEBUG_FS_VALIDATE)
		printf(
		    "ffs_validate: spc %d nspf %d fssize %d ncyl %d ncg %d\n",
		    spc, nspf, fssize, ncyl, ncg);
#endif	/* notyet */

		/* add space needed for superblocks */
	fsopts->size += (SBOFF + SBSIZE) * ncg;
		/* add space needed to store inodes, x3 for blockmaps, etc */
	fsopts->size += ncg * DINODE_SIZE * 3 * 
	    roundup(fsopts->inodes / ncg, fsopts->bsize / DINODE_SIZE);

		/* add minfree */
	if (fsopts->minfree > 0)
		fsopts->size =
		    fsopts->size * (100 + fsopts->minfree) / 100;
	/*
	 * XXX	any other fs slop to add, such as csum's, etc ??
	 */

	if (fsopts->size < fsopts->minsize)	/* ensure meets minimum size */
		fsopts->size = fsopts->minsize;

		/* round up to the next block */
	fsopts->size = roundup(fsopts->size, fsopts->bsize);

		/* calculate density if necessary */
	if (fsopts->density == -1)
		fsopts->density = fsopts->size / fsopts->inodes + 1;

	if (debug & DEBUG_FS_VALIDATE) {
		printf("ffs_validate: after defaults set:\n");
		ffs_dump_fsinfo(fsopts);
		printf("ffs_validate: dir %s; %lld bytes, %lld inodes\n",
		    dir, (long long)fsopts->size, (long long)fsopts->inodes);
	}
	sectorsize = fsopts->sectorsize;	/* XXX - see earlier */

		/* now check calculated sizes vs requested sizes */
	if (fsopts->maxsize > 0 && fsopts->size > fsopts->maxsize) {
		errx(1, "`%s' size of %lld is larger than the maxsize of %lld.",
		    dir, (long long)fsopts->size, (long long)fsopts->maxsize);
	}
}


static void
ffs_dump_fsinfo(fsinfo_t *f)
{

	printf("fsopts at %p\n", f);

	printf("\tsize %lld, inodes %lld, curinode %u\n",
	    (long long)f->size, (long long)f->inodes, f->curinode);

	printf("\tminsize %lld, maxsize %lld\n",
	    (long long)f->minsize, (long long)f->maxsize);
	printf("\tfree files %lld, freefile %% %d\n",
	    (long long)f->freefiles, f->freefilepc);
	printf("\tfree blocks %lld, freeblock %% %d\n",
	    (long long)f->freeblocks, f->freeblockpc);
	printf("\tneedswap %d, sectorsize %d\n", f->needswap, f->sectorsize);

	printf("\tbsize %d, fsize %d, cpg %d, density %d\n",
	    f->bsize, f->fsize, f->cpg, f->density);
	printf("\tntracks %d, nsectors %d, rpm %d, minfree %d\n",
	    f->ntracks, f->nsectors, f->rpm, f->minfree);
	printf("\tmaxcontig %d, rotdelay %d, maxbpg %d, nrpos %d\n",
	    f->maxcontig, f->rotdelay, f->maxbpg, f->nrpos);
	printf("\toptimization %s\n",
	    f->optimization == FS_OPTSPACE ? "space" : "time");
}


static int
ffs_create_image(const char *image, fsinfo_t *fsopts)
{
#if HAVE_STRUCT_STATFS_F_IOSIZE
	struct statfs	sfs;
#endif
	struct fs	*fs;
	char	*buf;
	int	i, bufsize;
	off_t	bufrem;

	assert (image != NULL);
	assert (fsopts != NULL);

		/* create image */
	if ((fsopts->fd = open(image, O_RDWR | O_CREAT | O_TRUNC, 0777))
	    == -1) {
		warn("Can't open `%s' for writing", image);
		return (-1);
	}

		/* zero image */
#if HAVE_STRUCT_STATFS_F_IOSIZE
	if (fstatfs(fsopts->fd, &sfs) == -1) {
#endif
		bufsize = 8192;
#if HAVE_STRUCT_STATFS_F_IOSIZE
		warn("can't fstatfs `%s', using default %d byte chunk",
		    image, bufsize);
	} else
		bufsize = sfs.f_iosize;
#endif
	bufrem = fsopts->size;
	if (debug & DEBUG_FS_CREATE_IMAGE)
		printf(
		    "zero-ing image `%s', %lld sectors, using %d byte chunks\n",
		    image, (long long)bufrem, bufsize);
	if ((buf = calloc(1, bufsize)) == NULL) {
		warn("Can't create buffer for sector");
		return (-1);
	}
	while (bufrem > 0) {
		i = write(fsopts->fd, buf, MIN(bufsize, bufrem));
		if (i == -1) {
			warn("zeroing image, %lld bytes to go",
			    (long long)bufrem);
			return (-1);
		}
		bufrem -= i;
	}

		/* make the file system */
	if (debug & DEBUG_FS_CREATE_IMAGE)
		printf("calling mkfs(\"%s\", ...)\n", image);
	fs = ffs_mkfs(image, fsopts);
	fsopts->superblock = (void *)fs;
	if (debug & DEBUG_FS_CREATE_IMAGE) {
		time_t t;

		t = ((struct fs *)fsopts->superblock)->fs_time;
		printf("mkfs returned %p; fs_time %s",
		    fsopts->superblock, ctime(&t));
		printf("fs totals: nbfree %d, nffree %d, nifree %d, ndir %d\n",
		    fs->fs_cstotal.cs_nbfree, fs->fs_cstotal.cs_nffree,
		    fs->fs_cstotal.cs_nifree, fs->fs_cstotal.cs_ndir);
	}

	if (fs->fs_cstotal.cs_nifree < fsopts->inodes) {
		warnx(
		"Image file `%s' has %lld free inodes; %lld are required.",
		    image,
		    (long long)fs->fs_cstotal.cs_nifree,
		    (long long)fsopts->inodes);
		return (-1);
	}
	return (fsopts->fd);
}


static void
ffs_size_dir(fsnode *root, fsinfo_t *fsopts)
{
	struct direct	tmpdir;
	fsnode *	node;
	int		curdirsize, this;

	/* node may be NULL (empty directory) */
	assert(fsopts != NULL);

	if (debug & DEBUG_FS_SIZE_DIR)
		printf("ffs_size_dir: entry: bytes %lld inodes %lld\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);

#define	ADDDIRENT(e) do {						\
	tmpdir.d_namlen = strlen((e));					\
	this = DIRSIZ(0, &tmpdir, 0);					\
	if (debug & DEBUG_FS_SIZE_DIR_ADD_DIRENT)			\
		printf("ADDDIRENT: was: %s (%d) this %d cur %d\n",	\
		    e, tmpdir.d_namlen, this, curdirsize);		\
	if (this + curdirsize > roundup(curdirsize, DIRBLKSIZ))		\
		curdirsize = roundup(curdirsize, DIRBLKSIZ);		\
	curdirsize += this;						\
	if (debug & DEBUG_FS_SIZE_DIR_ADD_DIRENT)			\
		printf("ADDDIRENT: now: %s (%d) this %d cur %d\n",	\
		    e, tmpdir.d_namlen, this, curdirsize);		\
} while (0);

	/*
	 * XXX	this needs to take into account extra space consumed
	 *	by indirect blocks, etc.
	 */
#define	ADDSIZE(x) do {							\
	fsopts->size += roundup((x), fsopts->fsize);			\
} while (0);

	curdirsize = 0;
	for (node = root; node != NULL; node = node->next) {
		ADDDIRENT(node->name);
		if (node == root) {			/* we're at "." */
			assert(strcmp(node->name, ".") == 0);
			ADDDIRENT("..");
		} else if ((node->inode->flags & FI_SIZED) == 0) {
				/* don't count duplicate names */
			node->inode->flags |= FI_SIZED;
			if (debug & DEBUG_FS_SIZE_DIR_NODE)
				printf("ffs_size_dir: `%s' size %lld\n",
				    node->name,
				    (long long)node->inode->st.st_size);
			fsopts->inodes++;
			if (node->type == S_IFREG)
				ADDSIZE(node->inode->st.st_size);
			if (node->type == S_IFLNK) {
				int	slen;

				slen = strlen(node->symlink) + 1;
				if (slen >= MAXSYMLINKLEN)
					ADDSIZE(slen);
			}
		}
		if (node->type == S_IFDIR)
			ffs_size_dir(node->child, fsopts);
	}
	ADDSIZE(curdirsize);

	if (debug & DEBUG_FS_SIZE_DIR)
		printf("ffs_size_dir: exit: size %lld inodes %lld\n",
		    (long long)fsopts->size, (long long)fsopts->inodes);
}


static int
ffs_populate_dir(const char *dir, fsnode *root, fsinfo_t *fsopts)
{
	fsnode		*cur;
	dirbuf_t	dirbuf;
	struct dinode	din;
	void		*membuf;
	char		path[MAXPATHLEN + 1];

	assert(dir != NULL);
	assert(root != NULL);
	assert(fsopts != NULL);

	(void)memset(&dirbuf, 0, sizeof(dirbuf));

	if (debug & DEBUG_FS_POPULATE)
		printf("ffs_populate_dir: PASS 1  dir %s node %p\n", dir, root);

		/*
		 * pass 1: allocate inode numbers, build directory `file'
		 */
	for (cur = root; cur != NULL; cur = cur->next) {
		if ((cur->inode->flags & FI_ALLOCATED) == 0) {
			cur->inode->flags |= FI_ALLOCATED;
			if (cur == root && cur->parent != NULL)
				cur->inode->ino = cur->parent->inode->ino;
			else {
				cur->inode->ino = fsopts->curinode;
				fsopts->curinode++;
			}
		}
		ffs_make_dirbuf(&dirbuf, cur->name, cur, fsopts->needswap);
		if (cur == root) {		/* we're at "."; add ".." */
			ffs_make_dirbuf(&dirbuf, "..",
			    cur->parent == NULL ? cur : cur->parent->first,
			    fsopts->needswap);
			root->inode->nlink++;	/* count my parent's link */
		} else if (cur->child != NULL)
			root->inode->nlink++;	/* count my child's link */

		/*
		 * XXX	possibly write file and long symlinks here,
		 *	ensuring that blocks get written before inodes?
		 *	otoh, this isn't a real filesystem, so who
		 *	cares about ordering? :-)
		 */
	}
	if (debug & DEBUG_FS_POPULATE_DIRBUF)
		ffs_dump_dirbuf(&dirbuf, dir, fsopts->needswap);

		/*
		 * pass 2: write out dirbuf, then non-directories at this level
		 */
	if (debug & DEBUG_FS_POPULATE)
		printf("ffs_populate_dir: PASS 2  dir %s\n", dir);
	for (cur = root; cur != NULL; cur = cur->next) {
		if (cur->inode->flags & FI_WRITTEN)
			continue;		/* skip hard-linked entries */
		cur->inode->flags |= FI_WRITTEN;

		if (snprintf(path, sizeof(path), "%s/%s", dir, cur->name)
		    >= sizeof(path))
			errx(1, "Pathname too long.");

		if (cur->child != NULL)
			continue;		/* child creates own inode */

				/* build on-disk inode */
		memset(&din, 0, sizeof(din));
		din.di_mode = cur->inode->st.st_mode;
		din.di_nlink = cur->inode->nlink;
		din.di_size = cur->inode->st.st_size;
		din.di_atime = cur->inode->st.st_atime;
		din.di_mtime = cur->inode->st.st_mtime;
		din.di_ctime = cur->inode->st.st_ctime;
#if HAVE_STRUCT_STAT_ST_MTIMENSEC
		din.di_atimensec = cur->inode->st.st_atimensec;
		din.di_mtimensec = cur->inode->st.st_mtimensec;
		din.di_ctimensec = cur->inode->st.st_ctimensec;
#endif
#if HAVE_STRUCT_STAT_ST_FLAGS
		din.di_flags = cur->inode->st.st_flags;
#endif
#if HAVE_STRUCT_STAT_ST_GEN
		din.di_gen = cur->inode->st.st_gen;
#endif
		din.di_uid = cur->inode->st.st_uid;
		din.di_gid = cur->inode->st.st_gid;
			/* not set: di_db, di_ib, di_blocks, di_spare */

		membuf = NULL;
		if (cur == root) {			/* "."; write dirbuf */
			membuf = dirbuf.buf;
			din.di_size = dirbuf.size;
		} else if (S_ISBLK(cur->type) || S_ISCHR(cur->type)) {
			din.di_size = 0;	/* a device */
			din.di_rdev =
			    ufs_rw32(cur->inode->st.st_rdev, fsopts->needswap);
		} else if (S_ISLNK(cur->type)) {	/* symlink */
			int slen;

			slen = strlen(cur->symlink);
			if (slen < MAXSYMLINKLEN) {	/* short link */
				memcpy(din.di_shortlink, cur->symlink, slen);
			} else
				membuf = cur->symlink;
			din.di_size = slen;
		}

		if (debug & DEBUG_FS_POPULATE_NODE) {
			printf("ffs_populate_dir: writing ino %d, %s",
			    cur->inode->ino, inode_type(cur->type));
			if (cur->inode->nlink > 1)
				printf(", nlink %d", cur->inode->nlink);
			putchar('\n');
		}

		if (membuf != NULL) {
			ffs_write_file(&din, cur->inode->ino, membuf, fsopts);
		} else if (S_ISREG(cur->type)) {
			ffs_write_file(&din, cur->inode->ino, path, fsopts);
		} else {
			assert (! S_ISDIR(cur->type));
			ffs_write_inode(&din, cur->inode->ino, fsopts);
		}
	}

		/*
		 * pass 3: write out sub-directories
		 */
	if (debug & DEBUG_FS_POPULATE)
		printf("ffs_populate_dir: PASS 3  dir %s\n", dir);
	for (cur = root; cur != NULL; cur = cur->next) {
		if (cur->child == NULL)
			continue;
		if (snprintf(path, sizeof(path), "%s/%s", dir, cur->name)
		    >= sizeof(path))
			errx(1, "Pathname too long.");
		if (! ffs_populate_dir(path, cur->child, fsopts))
			return (0);
	}

	if (debug & DEBUG_FS_POPULATE)
		printf("ffs_populate_dir: DONE dir %s\n", dir);

		/* cleanup */
	if (dirbuf.buf != NULL)
		free(dirbuf.buf);
	return (1);
}


static void
ffs_write_file(struct dinode *din, uint32_t ino, void *buf, fsinfo_t *fsopts)
{
	int 	isfile, ffd;
	char	*fbuf, *p;
	off_t	bufleft, chunk, offset;
	struct inode	in;
	struct buf *	bp;

	assert (din != NULL);
	assert (buf != NULL);
	assert (fsopts != NULL);

	isfile = S_ISREG(din->di_mode);
	fbuf = NULL;
	ffd = -1;

	if (debug & DEBUG_FS_WRITE_FILE) {
		printf(
		    "ffs_write_file: ino %u, din %p, isfile %d, %s, size %lld",
		    ino, din, isfile, inode_type(din->di_mode & S_IFMT),
		    (long long)din->di_size);
		if (isfile)
			printf(", file '%s'\n", (char *)buf);
		else
			printf(", buffer %p\n", buf);
	}

	in.i_number = ino;
	in.i_fs = (struct fs *)fsopts->superblock;
	memcpy(&in.i_din.ffs_din, din, sizeof(in.i_din.ffs_din));
	in.i_fd = fsopts->fd;

	if (din->di_size == 0)
		goto write_inode_and_leave;		/* mmm, cheating */

	if (isfile) {
		if ((fbuf = malloc(fsopts->bsize)) == NULL)
			err(1, "Allocating memory for write buffer");
		if ((ffd = open((char *)buf, O_RDONLY, 0444)) == -1) {
			warn("Can't open `%s' for reading", (char *)buf);
			goto leave_ffs_write_file;
		}
	} else {
		p = buf;
	}

	chunk = 0;
	for (bufleft = din->di_size; bufleft > 0; bufleft -= chunk) {
		chunk = MIN(bufleft, fsopts->bsize);
		if (isfile) {
			if (read(ffd, fbuf, chunk) != chunk)
				err(1, "Reading `%s', %lld bytes to go",
				    (char *)buf, (long long)bufleft);
			p = fbuf;
		}
		offset = din->di_size - bufleft;
		if (debug & DEBUG_FS_WRITE_FILE_BLOCK)
			printf(
		"ffs_write_file: write %p offset %lld size %lld left %lld\n",
			    p, (long long)offset,
			    (long long)chunk, (long long)bufleft);
	/*
	 * XXX	if holey support is desired, do the check here
	 *
	 * XXX	might need to write out last bit in fragroundup
	 *	sized chunk. however, ffs_balloc() handles this for us
	 */
		errno = ffs_balloc(&in, offset, chunk, &bp);
 bad_ffs_write_file:
		if (errno != 0)
			err(1,
			    "Writing inode %d (%s), bytes %lld + %lld",
			    ino,
			    isfile ? (char *)buf :
			      inode_type(din->di_mode & S_IFMT),
			    (long long)offset, (long long)chunk);
		memcpy(bp->b_data, p, chunk);
		errno = bwrite(bp);
		if (errno != 0)
			goto bad_ffs_write_file;
		brelse(bp);
		if (!isfile)
			p += chunk;
	}
  
 write_inode_and_leave:
	ffs_write_inode(&in.i_din.ffs_din, in.i_number, fsopts);

 leave_ffs_write_file:
	if (fbuf)
		free(fbuf);
	if (ffd != -1)
		close(ffd);
}


static void
ffs_dump_dirbuf(dirbuf_t *dbuf, const char *dir, int needswap)
{
	doff_t		i;
	struct direct	*de;
	uint16_t	reclen;

	assert (dbuf != NULL);
	assert (dir != NULL);
	printf("ffs_dump_dirbuf: dir %s size %d cur %d\n",
	    dir, dbuf->size, dbuf->cur);

	for (i = 0; i < dbuf->size; ) {
		de = (struct direct *)(dbuf->buf + i);
		reclen = ufs_rw16(de->d_reclen, needswap);
		printf(
	    " inode %4d %7s offset %4d reclen %3d namlen %3d name %s\n",
		    ufs_rw32(de->d_ino, needswap),
		    inode_type(DTTOIF(de->d_type)), i, reclen,
		    de->d_namlen, de->d_name);
		i += reclen;
		assert(reclen > 0);
	}
}

static void
ffs_make_dirbuf(dirbuf_t *dbuf, const char *name, fsnode *node, int needswap)
{
	struct direct	de, *dp;
	uint16_t	llen, reclen;

	assert (dbuf != NULL);
	assert (name != NULL);
	assert (node != NULL);
					/* create direct entry */
	(void)memset(&de, 0, sizeof(de));
	de.d_ino = ufs_rw32(node->inode->ino, needswap);
	de.d_type = IFTODT(node->type);
	de.d_namlen = (uint8_t)strlen(name);
	strcpy(de.d_name, name);
	reclen = DIRSIZ(0, &de, needswap);
	de.d_reclen = ufs_rw16(reclen, needswap);

	dp = (struct direct *)(dbuf->buf + dbuf->cur);
	llen = 0;
	if (dp != NULL)
		llen = DIRSIZ(0, dp, needswap);

	if (debug & DEBUG_FS_MAKE_DIRBUF)
		printf(
		    "ffs_make_dirbuf: dbuf siz %d cur %d lastlen %d\n"
		    "  ino %d type %d reclen %d namlen %d name %.30s\n",
		    dbuf->size, dbuf->cur, llen,
		    ufs_rw32(de.d_ino, needswap), de.d_type, reclen,
		    de.d_namlen, de.d_name);

	if (reclen + dbuf->cur + llen > roundup(dbuf->size, DIRBLKSIZ)) {
		dbuf->size += DIRBLKSIZ;	/* need another chunk */
		if (debug & DEBUG_FS_MAKE_DIRBUF)
			printf("ffs_make_dirbuf: growing buf to %d\n",
			    dbuf->size);
		if ((dbuf->buf = realloc(dbuf->buf, dbuf->size)) == NULL)
			err(1, "Allocating memory for directory buffer");
		memset(dbuf->buf + dbuf->size - DIRBLKSIZ, 0, DIRBLKSIZ);
		dbuf->cur = dbuf->size - DIRBLKSIZ;
	} else {				/* shrink end of previous */
		dp->d_reclen = ufs_rw16(llen,needswap);
		dbuf->cur += llen;
	}
	dp = (struct direct *)(dbuf->buf + dbuf->cur);
	memcpy(dp, &de, reclen);
	dp->d_reclen = ufs_rw16(dbuf->size - dbuf->cur, needswap);
}

/*
 * cribbed from sys/ufs/ffs/ffs_alloc.c
 */
static void
ffs_write_inode(struct dinode *ip, uint32_t ino, const fsinfo_t *fsopts)
{
	struct dinode	ibuf[MAXINOPB];
	struct cg	*cgp;
	struct fs	*fs;
	int		cg, cgino;
	daddr_t		d;
	char		sbbuf[MAXBSIZE];

	assert (ip != NULL);
	assert (ino > 0);
	assert (fsopts != NULL);

	fs = (struct fs *)fsopts->superblock;
	cg = ino_to_cg(fs, ino);
	cgino = ino % fs->fs_ipg;
	if (debug & DEBUG_FS_WRITE_INODE)
		printf("ffs_write_inode: din %p ino %u cg %d cgino %d\n",
		    ip, ino, cg, cgino);

	ffs_rdfs(fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize, &sbbuf,
	    fsopts);
	cgp = (struct cg *)sbbuf;
	if (!cg_chkmagic(cgp, fsopts->needswap))
		errx(1, "ffs_write_inode: cg %d: bad magic number", cg);

	assert (isclr(cg_inosused(cgp, fsopts->needswap), cgino));

	if (fs->fs_cstotal.cs_nifree == 0)
		errx(1, "ffs_write_inode: fs out of inodes for ino %u",
		    ino);
	if (fs->fs_cs(fs, cg).cs_nifree == 0)
		errx(1,
		    "ffs_write_inode: cg %d out of inodes for ino %u",
		    cg, ino);
	setbit(cg_inosused(cgp, fsopts->needswap), cgino);
	ufs_add32(cgp->cg_cs.cs_nifree, -1, fsopts->needswap);
	fs->fs_cstotal.cs_nifree--;
	fs->fs_cs(fs, cg).cs_nifree--;
	if (S_ISDIR(ip->di_mode)) {
		ufs_add32(cgp->cg_cs.cs_ndir, 1, fsopts->needswap);
		fs->fs_cstotal.cs_ndir++;
		fs->fs_cs(fs, cg).cs_ndir++; 
	}
	ffs_wtfs(fsbtodb(fs, cgtod(fs, cg)), (int)fs->fs_cgsize, &sbbuf,
	    fsopts);

					/* now write inode */
	d = fsbtodb(fs, ino_to_fsba(fs, ino));
	ffs_rdfs(d, fs->fs_bsize, ibuf, fsopts);
	if (fsopts->needswap)
		ffs_dinode_swap(ip, &ibuf[ino_to_fsbo(fs, ino)]);
	else
		ibuf[ino_to_fsbo(fs, ino)] = *ip;
	ffs_wtfs(d, fs->fs_bsize, ibuf, fsopts);
}

void
panic(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vwarnx(fmt, ap);
	va_end(ap);
	exit(1);
}
