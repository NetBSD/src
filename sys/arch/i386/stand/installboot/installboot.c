/* $NetBSD: installboot.c,v 1.18 2003/04/02 10:39:32 fvdl Exp $	 */

/*
 * Copyright (c) 1994 Paul Kranenburg
 * All rights reserved.
 * Copyright (c) 1996, 1997
 * 	Matthias Drochner.  All rights reserved.
 * Copyright (c) 1996, 1997
 * 	Perry E. Metzger.  All rights reserved.
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
 *      This product includes software developed by Paul Kranenburg.
 *	This product includes software developed for the NetBSD Project
 *	by Matthias Drochner.
 *	This product includes software developed for the NetBSD Project
 *	by Perry E. Metzger.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/param.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/dir.h>
#include <ufs/ffs/fs.h>
#include <err.h>
#include <a.out.h>
#include <errno.h>
#include <fcntl.h>
#include <nlist.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <md5.h>
#include <sys/ioctl.h>

#include "loadfile.h"
#include "installboot.h"

#include "bbinfo.h"

#define DEFBBLKNAME "boot"

char *loadprotoblocks __P((char *, size_t *));
static int devread __P((int, void *, daddr_t, size_t, char *));
static int add_fsblk __P((struct fs *, daddr_t, int));
int setup_ffs_blks __P((char *, ino_t));
int setup_contig_blocks __P((char *, int, char *, unsigned int));
int save_contig_sec_boot __P((char *, int, char *, unsigned int));
ino_t save_ffs __P((char *, char *, char *, unsigned int));
ino_t save_passthru __P((char *, char *, char *, unsigned int));
static void usage __P((void));
int main __P((int, char **));

struct ofraglist *ofraglist;
struct fraglist *fraglist;

struct nlist nl[] = {
#define X_fraglist	0
#define X_boottimeout	1
#define X_bootpasswd	2
#define X_fraglist64	3
#ifdef __ELF__
	{{"fraglist"}},
	{{"boottimeout"}},
	{{"bootpasswd"}},
	{{"fraglist64"}},
#else
	{{"_fraglist"}},
	{{"_boottimeout"}},
	{{"_bootpasswd"}},
	{{"_fraglist64"}},
#endif
	{{NULL}}
};

int verbose = 0;
int conblockmode, conblockstart;


char *
loadprotoblocks(fname, size)
	char *fname;
	size_t *size;
{
	int fd;
	u_long marks[MARK_MAX], bp, value;
	int maxentries;

	fd = -1;

	/* Locate block number array in proto file */
	if (nlist(fname, nl) < 0) {
		warn("nlist: %s", fname);
		return NULL;
	}

	if (nl[X_fraglist].n_value == 0 &&
	    nl[X_fraglist64].n_value == 0) {
		/* fraglist is mandatory, other stuff is optional */
		warnx("nlist: no fraglist");
		return NULL;
	}

	marks[MARK_START] = 0;
	if ((fd = loadfile(fname, marks, COUNT_TEXT|COUNT_DATA)) == -1)
		return NULL;
	(void)close(fd);

	*size = roundup(marks[MARK_END], DEV_BSIZE);
	bp = marks[MARK_START] = (u_long)malloc(*size);
	if ((fd = loadfile(fname, marks, LOAD_TEXT|LOAD_DATA)) == -1)
		return NULL;
	(void)close(fd);

	if (nl[X_fraglist64].n_value != 0) {
		/* NOSTRICT */
		value = nl[X_fraglist64].n_value;
		fraglist = (struct fraglist *)(bp + value);
		if (fraglist->magic != FRAGLISTMAGIC) {
			warnx("invalid bootblock version");
			goto bad;
		}
		maxentries = fraglist->maxentries;
	} else {
		/* NOSTRICT */
		value = nl[X_fraglist].n_value;
		ofraglist = (struct ofraglist *) (bp + value);
		if (ofraglist->magic != OFRAGLISTMAGIC) {
			warnx("invalid bootblock version");
			goto bad;
		}
		maxentries = ofraglist->maxentries;
	}

	if (verbose) {
		(void) fprintf(stderr, "%s: entry point %#lx\n", fname,
			       marks[MARK_ENTRY]);
		(void) fprintf(stderr, "proto bootblock size %ld\n",
			       (long)*size);
		(void) fprintf(stderr, "room for %d filesystem blocks"
			       " at %#lx\n", maxentries, value);
	}
	return (char *) bp;
bad:
	if (bp)
		free((void *)bp);
	return NULL;
}

static int
devread(fd, buf, blk, size, msg)
	int fd;
	void *buf;
	daddr_t blk;
	size_t size;
	char *msg;
{
	if (lseek(fd, (off_t)dbtob(blk), SEEK_SET) != dbtob(blk)) {
		warn("%s: devread: lseek", msg);
		return (1);
	}
	if (read(fd, buf, size) != size) {
		warn("%s: devread: read", msg);
		return (1);
	}
	return (0);
}

/* add file system blocks to fraglist */
static int
add_fsblk(fs, blk, blcnt)
	struct fs *fs;
	daddr_t blk;
	int blcnt;
{
	int nblk;

	/* convert to disk blocks */
	blk = fsbtodb(fs, blk);
	nblk = fs->fs_bsize / DEV_BSIZE;
	if (nblk > blcnt)
		nblk = blcnt;

	if (verbose)
		(void) fprintf(stderr, "dblk: %lld, num: %lld\n",
		    (long long)blk, (long long)nblk);

	/*
	 * Can only handle 32 bits worth in the old format.
	 */
	if (fraglist == NULL && blk > 0xffffffff)
		errx(1, "block %lld out of range", (long long)blk);

	if (fraglist != NULL) {
		/* start new entry or append to previous? */
		if (!fraglist->numentries ||
		    (fraglist->entries[fraglist->numentries - 1].offset
		     + fraglist->entries[fraglist->numentries - 1].num != blk)){
	
			/* need new entry */
		        if (fraglist->numentries > fraglist->maxentries - 1) {
				errx(1, "not enough fragment space in bootcode");
				return (-1);
			}
	
			fraglist->entries[fraglist->numentries].offset = blk;
			fraglist->entries[fraglist->numentries++].num = 0;
		}
		fraglist->entries[fraglist->numentries - 1].num += nblk;
	} else {
		/* start new entry or append to previous? */
		if (!ofraglist->numentries ||
		   (ofraglist->entries[ofraglist->numentries - 1].offset
		    +ofraglist->entries[ofraglist->numentries - 1].num != blk)){
	
			/* need new entry */
		        if (ofraglist->numentries > ofraglist->maxentries - 1) {
				errx(1, "not enough fragment space in bootcode");
				return (-1);
			}
	
			ofraglist->entries[ofraglist->numentries].offset = blk;
			ofraglist->entries[ofraglist->numentries++].num = 0;
		}
		ofraglist->entries[ofraglist->numentries - 1].num += nblk;
	}

	return (blcnt - nblk);
}

static union {
	char c[SBLOCKSIZE];
	struct fs s;
} sblock;

const daddr_t sblock_try[] = SBLOCKSEARCH;

int
setup_ffs_blks(diskdev, inode)
	char *diskdev;
	ino_t inode;
{
	int devfd = -1;
	struct fs *fs;
	char *buf = 0;
	daddr_t blk;
	int32_t *ap32;
	int64_t *ap64;
	struct ufs1_dinode *ip1;
	struct ufs2_dinode *ip2;
	int i, ndb;
	int allok = 0, is_ufs2 = 0;

	fs = NULL;
	ip1 = NULL;
	ip2 = NULL;

	devfd = open(diskdev, O_RDONLY, 0);
	if (devfd < 0) {
		warn("open raw partition");
		return (1);
	}
	/* Read superblock */
	for (i = 0; sblock_try[i] != -1; i++) {
		if (devread(devfd, &sblock, sblock_try[i] / DEV_BSIZE,
		    SBLOCKSIZE, "superblock"))
			continue;
		fs = &sblock.s;
		if (fs->fs_magic == FS_UFS1_MAGIC)
			break;
		if (fs->fs_magic == FS_UFS2_MAGIC) {
			is_ufs2 = 1;
			break;
		}
	}

	if (sblock_try[i] == -1) {
		warnx("invalid super block");
		goto out;
	}

	/* Read inode */
	if ((buf = malloc((size_t)fs->fs_bsize)) == NULL) {
		warnx("No memory for filesystem block");
		goto out;
	}
	blk = fsbtodb(fs, ino_to_fsba(fs, inode));
	if (devread(devfd, buf, blk, (size_t)fs->fs_bsize, "inode"))
		goto out;
	if (is_ufs2) {
		ip2 = (struct ufs2_dinode *)buf + ino_to_fsbo(fs, inode);
		ndb = (int)(ip2->di_size / DEV_BSIZE);	/* size is rounded! */
	} else {
		ip1 = (struct ufs1_dinode *)buf + ino_to_fsbo(fs, inode);
		ndb = (int)(ip1->di_size / DEV_BSIZE);	/* size is rounded! */
	}

	/*
	 * Have the inode.  Figure out how many blocks we need.
	 */

	if (verbose)
		(void) fprintf(stderr, "Will load %d blocks.\n", ndb);

	if (is_ufs2) {
		/*
		 * Get the block numbers, first direct blocks
		 */
		ap64 = ip2->di_db;
		for (i = 0; i < NDADDR && *ap64 && ndb > 0; i++, ap64++)
			ndb = add_fsblk(fs, *ap64, ndb);
	
		if (ndb > 0) {
			/*
		         * Just one level of indirections; there isn't much room
		         * for more in the 1st-level bootblocks anyway.
		         */
			blk = fsbtodb(fs, ip2->di_ib[0]);
			if (devread(devfd, buf, blk, (size_t)fs->fs_bsize,
				    "indirect block"))
				goto out;
			ap64 = (int64_t *) buf;
			for (; i < NINDIR(fs) && *ap64 && ndb > 0; i++, ap64++){
				ndb = add_fsblk(fs, *ap64, ndb);
			}
		}
	} else {
		/*
		 * Get the block numbers, first direct blocks
		 */
		ap32 = ip1->di_db;
		for (i = 0; i < NDADDR && *ap32 && ndb > 0; i++, ap32++)
			ndb = add_fsblk(fs, *ap32, ndb);
	
		if (ndb > 0) {
			/*
		         * Just one level of indirections; there isn't much room
		         * for more in the 1st-level bootblocks anyway.
		         */
			blk = fsbtodb(fs, ip1->di_ib[0]);
			if (devread(devfd, buf, blk, (size_t)fs->fs_bsize,
				    "indirect block"))
				goto out;
			ap32 = (int32_t *) buf;
			for (; i < NINDIR(fs) && *ap32 && ndb > 0; i++, ap32++){
				ndb = add_fsblk(fs, *ap32, ndb);
			}
		}
	}

	if (!ndb)
	    allok = 1;
	else {
	    if (ndb > 0)
		warnx("too many fs blocks");
	    /* else, ie ndb < 0, add_fsblk returned error */
	    goto out;
	}

out:
	if (buf)
		free(buf);
	if (devfd >= 0)
		(void) close(devfd);
	return (!allok);
}


ino_t
save_ffs(diskdev, bootblkname, bp, size)
	char *diskdev, *bootblkname, *bp;
	unsigned int size;
{
	ino_t inode = -2;
	int loadsz;

	loadsz = fraglist != NULL ? fraglist->loadsz : ofraglist->loadsz;

	/* do we need the fraglist? */
	if (size > loadsz * DEV_BSIZE) {

		inode = createfileondev(diskdev, bootblkname, bp, size);
		if (inode == (ino_t)-1)
			return inode;

		/* paranoia */
		sync();
		(void) sleep(3);

		if (setup_ffs_blks(diskdev, inode))
			return (ino_t)(-1);
	}
	return inode;
}

int
setup_contig_blocks(diskdev, blkno, bp, size) 
	char *diskdev, *bp;
	int blkno;
	unsigned int size;
{
	int i, ndb, rdb, db;
	int tableblksize = 8;
	int maxentries;

	ndb = howmany(size, tableblksize * DEV_BSIZE);
	rdb = howmany(size, DEV_BSIZE);

	maxentries = fraglist != NULL ? fraglist->maxentries :
	    ofraglist->maxentries;
	
	if (verbose)
		printf("%s: block number %d, size %u table blocks: %d/%d\n",
		       diskdev, blkno, size, ndb, maxentries);
	if (ndb > maxentries) {
		errx(1, "not enough fragment space in bootcode");
		return (-1);
	}

	if (verbose)
		printf("%s: block numbers:", diskdev);
	for (i = 0; i < ndb; i++) {
		db = tableblksize;
		if (rdb < tableblksize)
			db = rdb;
		rdb -= tableblksize;

		if (fraglist != NULL) {
			fraglist->numentries = i+1;
			fraglist->entries[i].offset = blkno;
			fraglist->entries[i].num = db;
		} else {
			ofraglist->numentries = i+1;
			ofraglist->entries[i].offset = blkno;
			ofraglist->entries[i].num = db;
		}
		if (verbose)
			printf(" %d", blkno);

		blkno += tableblksize;
	}
	if (verbose)
		printf("\n");

	return 0;
}

int
save_contig_sec_boot(diskdev, off, bp, size)
	char *diskdev, *bp;
	unsigned int size;
	int off;
{
	int fd;

	fd = open(diskdev, O_RDWR, 0444);
	if (fd < 0) {
		warn("open %s", diskdev);
		return (-1);
	}
	if (lseek(fd, (off_t)off, SEEK_SET) == (off_t)-1) {
		warn("lseek %s", diskdev);
		return (-1);
	}
	if (write(fd, bp, size) < 0) {
		warn("write %s", diskdev);
		return (-1);
	}
	if (fsync(fd) != 0) {
		warn("fsync: %s", diskdev);
		return -1;
	}
	(void) close(fd);

	return 0;
}

ino_t
save_passthru(diskdev, bootblkname, bp, size)
	char *diskdev, *bootblkname, *bp;
	unsigned int size;
{

	if (save_contig_sec_boot(diskdev,
				 conblockstart * DEV_BSIZE,
				 bp, size) != 0)
		return (ino_t)(-1);

	if (setup_contig_blocks(diskdev, conblockstart, bp, size) != 0)
		return (ino_t)(-1);

	return (ino_t)(-2);
}


static void
usage()
{
	(void) fprintf(stderr,
	       "usage: installboot [-b bno] [-n] [-v] [-f] <boot> <device>\n");
	exit(1);
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
	int c;
	char *bp = 0;
	size_t size;
	ino_t inode = (ino_t) -1;
	int devfd = -1;
	struct disklabel dl;
	int bsdoffs;
	int i, res;
	int forceifnolabel = 0;
	char *bootblkname = DEFBBLKNAME;
	int nowrite = 0;
	int allok = 0;
	int timeout = -1;
	char *bootpasswd = 0;
	ino_t (*save_func) __P((char *, char *, char *, unsigned int));
	int loadsz;

	while ((c = getopt(argc, argv, "b:vnft:p:")) != -1) {
		switch (c) {
		case 'b':
			/* generic override, supply starting block # */
			conblockmode = 1;
			conblockstart = atoi(optarg);
			break;
		case 't':
			/* boot timeout */
			timeout = atoi(optarg);
			break;
		case 'p':
			/* boot password */
			bootpasswd = optarg;
			break;
		case 'n':
			/* Do not actually write the bootblock to disk */
			nowrite = 1;
			break;
		case 'v':
			/* Chat */
			verbose = 1;
			break;
		case 'f':
			/* assume zero offset if no disklabel */
			forceifnolabel = 1;
			break;
		default:
			usage();
		}
	}

	if (argc - optind != 2) {
		usage();
	}

	bp = loadprotoblocks(argv[optind], &size);
	if (!bp)
		errx(1, "error reading bootblocks");

	if (timeout >= 0) {
		if (nl[X_boottimeout].n_value != 0)
			*((int *)(bp + nl[X_boottimeout].n_value)) = timeout;
		else {
			warnx("no timeout support in bootblock");
			goto out;
		}
	}

	if (bootpasswd) {
		if (nl[X_bootpasswd].n_value != 0) {
			MD5_CTX md5ctx;

			MD5Init(&md5ctx);
			MD5Update(&md5ctx, bootpasswd, strlen(bootpasswd));
			MD5Final(bp + nl[X_bootpasswd].n_value, &md5ctx);
		} else {
			warnx("no password support in bootblock");
			goto out;
		}
	}

	if (fraglist != NULL) {
		fraglist->numentries = 0;
		loadsz = fraglist->loadsz;
	} else {
		ofraglist->numentries = 0;
		loadsz = ofraglist->loadsz;
	}

	if (conblockmode)
		save_func = save_passthru;
	else
		save_func = save_ffs;

	if ((inode = (*save_func)(argv[optind + 1],
				  bootblkname,
				  bp + loadsz * DEV_BSIZE,
				  size - loadsz * DEV_BSIZE))
	    == (ino_t)-1)
		goto out;

	size = loadsz * DEV_BSIZE;
	/* size to be written to bootsect */

	devfd = open(argv[optind + 1], O_RDWR, 0);
	if (devfd < 0) {
		warn("open raw partition RW");
		goto out;
	}
	if (ioctl(devfd, DIOCGDINFO, &dl) < 0) {
		if ((errno == EINVAL) || (errno == ENOTTY)) {
			if (forceifnolabel)
				bsdoffs = 0;
			else {
				warnx("no disklabel, use -f to install anyway");
				goto out;
			}
		} else {
			warn("get disklabel");
			goto out;
		}
	} else {
		char p = argv[optind + 1][strlen(argv[optind + 1]) - 1];
#define isvalidpart(c) ((c) >= 'a' && (c) <= 'z')
		if (!isvalidpart(p) || (p - 'a') >= dl.d_npartitions) {
			warnx("invalid partition");
			goto out;
		}
		bsdoffs = dl.d_partitions[p - 'a'].p_offset;
	}
	if (verbose)
		(void) fprintf(stderr, "BSD partition starts at sector %d\n",
			       bsdoffs);

	/*
         * add offset of BSD partition to fraglist entries
         */
	if (fraglist != NULL) {
		for (i = 0; i < fraglist->numentries; i++)
			fraglist->entries[i].offset += bsdoffs;
	} else {
		for (i = 0; i < ofraglist->numentries; i++)
			ofraglist->entries[i].offset += bsdoffs;
	}

	if (!nowrite) {
		/*
	         * write first blocks (max loadsz) to start of BSD partition,
	         * skip disklabel (in second disk block)
	         */
		(void) lseek(devfd, (off_t)0, SEEK_SET);
		res = write(devfd, bp, DEV_BSIZE);
		if (res < 0) {
			warn("final write1");
			goto out;
		}
		(void) lseek(devfd, (off_t)(2 * DEV_BSIZE), SEEK_SET);
		res = write(devfd, bp + 2 * DEV_BSIZE, size - 2 * DEV_BSIZE);
		if (res < 0) {
			warn("final write2");
			goto out;
		}
	}
	allok = 1;

out:
	if (devfd >= 0)
		(void) close(devfd);
	if (bp)
		free(bp);
	if (inode != (ino_t)-1 && /* failed? */
	    inode != (ino_t)-2 && /* small boot, no FS blocks? */
	    !conblockmode) {	/* contiguous blocks? */
		cleanupfileondev(argv[optind + 1], bootblkname, !allok || nowrite);
	}
	return (!allok);
}
