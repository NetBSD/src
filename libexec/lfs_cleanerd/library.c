/*	$NetBSD: library.c,v 1.20 2001/01/10 01:13:55 lukem Exp $	*/

/*-
 * Copyright (c) 1992, 1993
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
#if 0
static char sccsid[] = "@(#)library.c	8.3 (Berkeley) 5/24/95";
#else
__RCSID("$NetBSD: library.c,v 1.20 2001/01/10 01:13:55 lukem Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/types.h>
#include <sys/mman.h>

#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>

#include "clean.h"

void	 add_blocks __P((FS_INFO *, BLOCK_INFO *, int *, SEGSUM *, caddr_t,
	     daddr_t, daddr_t));
void	 add_inodes __P((FS_INFO *, BLOCK_INFO *, int *, SEGSUM *, caddr_t,
	     daddr_t));
int	 bi_compare __P((const void *, const void *));
int	 bi_toss __P((const void *, const void *, const void *));
void	 get_ifile __P((FS_INFO *, int));
int	 get_superblock __P((FS_INFO *, struct lfs *));
int	 pseg_valid __P((FS_INFO *, SEGSUM *, daddr_t));
int      pseg_size __P((daddr_t, FS_INFO *, SEGSUM *));
void	 print_SEGSUM __P((struct lfs *, SEGSUM *, daddr_t));

extern int debug;
extern u_long cksum __P((void *, size_t));	/* XXX */

static int ifile_fd;
static int dev_fd;

/*
 * This function will get information on a a filesystem which matches
 * the name and type given.  If a "name" is in a filesystem of the given
 * type, then buf is filled with that filesystem's info, and the
 * a non-zero value is returned.
 */
int
fs_getmntinfo(buf, name, type)
	struct	statfs	**buf;
	char	*name;
	const	char	*type;
{
	/* allocate space for the filesystem info */
	*buf = (struct statfs *)malloc(sizeof(struct statfs));
	if (*buf == NULL)
		return 0;

	/* grab the filesystem info */
        if (ifile_fd <= 0) {
		if (statfs(name, *buf) < 0) {
			free(*buf);
			return 0;
		}
        } else if(fstatfs(ifile_fd, *buf) < 0) {
		free(*buf);
		return 0;
        }

	/* check to see if it's the one we want */
	if (strncmp(type, (*buf)->f_fstypename, MFSNAMELEN) ||
	    strncmp(name, (*buf)->f_mntonname, MNAMELEN)) {
		/* "this is not the filesystem you're looking for" */
		free(*buf);
		return 0;
	}

	return 1;
}

/*
 * Get all the information available on an LFS file system.
 * Returns an pointer to an FS_INFO structure, NULL on error.
 */
FS_INFO *
get_fs_info (lstatfsp, use_mmap)
	struct statfs *lstatfsp;	/* IN: pointer to statfs struct */
	int use_mmap;			/* IN: mmap or read */
{
	FS_INFO	*fsp;

	fsp = (FS_INFO *)malloc(sizeof(FS_INFO));
	if (fsp == NULL)
		return NULL;
	memset(fsp, 0, sizeof(FS_INFO));

	fsp->fi_statfsp = lstatfsp;
	if (get_superblock (fsp, &fsp->fi_lfs)) {
		syslog(LOG_ERR, "Exiting: get_fs_info: get_superblock failed: %m");
                exit(1);
        }
	fsp->fi_daddr_shift =
	     fsp->fi_lfs.lfs_bshift - fsp->fi_lfs.lfs_fsbtodb;
	get_ifile (fsp, use_mmap);
	return (fsp);
}

/*
 * If we are reading the ifile then we need to refresh it.  Even if
 * we are mmapping it, it might have grown.  Finally, we need to
 * refresh the file system information (statfs) info.
 */
void
reread_fs_info(fsp, use_mmap)
	FS_INFO *fsp;	/* IN: prointer fs_infos to reread */
	int use_mmap;
{
	if (ifile_fd <= 0) {
		if (fstatfs(ifile_fd, fsp->fi_statfsp)) {
			syslog(LOG_ERR, "Exiting: reread_fs_info: fstatfs failed: %m");
                	exit(1);
		}
	} else if (statfs(fsp->fi_statfsp->f_mntonname, fsp->fi_statfsp)) {
		syslog(LOG_ERR, "Exiting: reread_fs_info: statfs failed: %m");
                exit(1);
        }
	get_ifile (fsp, use_mmap);
}

/*
 * Gets the superblock from disk (possibly in face of errors)
 */
int
get_superblock (fsp, sbp)
	FS_INFO *fsp;		/* local file system info structure */
	struct lfs *sbp;
{
	char mntfromname[MNAMELEN+1];
	char buf[LFS_SBPAD];

	strcpy(mntfromname, "/dev/r");
	strcat(mntfromname, fsp->fi_statfsp->f_mntfromname+5);

	if(dev_fd <= 0) {
		if ((dev_fd = open(mntfromname, O_RDONLY, (mode_t)0)) < 0) {
			syslog(LOG_WARNING,"get_superblock: bad open: %m");
			return (-1);
		}
	} else
		lseek(dev_fd, 0, SEEK_SET);
		
	get(dev_fd, LFS_LABELPAD, buf, LFS_SBPAD);
	memcpy(&(sbp->lfs_dlfs), buf, sizeof(struct dlfs));
	/* close (fid); */

	return (0);
}

/*
 * This function will map the ifile into memory.  It causes a
 * fatal error on failure.
 */
void
get_ifile (fsp, use_mmap)
	FS_INFO	*fsp;
	int use_mmap;

{
	struct stat file_stat;
	struct statfs statfsbuf;
	caddr_t ifp;
	char *ifile_name;
	int count;

	ifp = NULL;
	ifile_name = malloc(strlen(fsp->fi_statfsp->f_mntonname) +
	    strlen(IFILE_NAME)+2);
	strcat(strcat(strcpy(ifile_name, fsp->fi_statfsp->f_mntonname), "/"),
	    IFILE_NAME);

	if(ifile_fd <= 0) {
		/* XXX KS - Do we ever *write* to the ifile? */
		if ((ifile_fd = open(ifile_name, O_RDONLY, (mode_t)0)) < 0) {
			syslog(LOG_ERR, "Exiting: get_ifile: bad open: %m");
			exit(1);
		}
	} else
		lseek(ifile_fd, 0, SEEK_SET);

	if (fstat (ifile_fd, &file_stat)) {
		/* If the fs was unmounted, don't complain */
		statfs(fsp->fi_statfsp->f_mntonname, &statfsbuf);
		if(memcmp(&statfsbuf.f_fsid,&fsp->fi_statfsp->f_fsid,
			  sizeof(statfsbuf.f_fsid))!=0)
		{
			/* Filesystem still mounted, this error is real */
			syslog(LOG_ERR, "Exiting: get_ifile: fstat failed: %m");
                	exit(1);
		}
		exit(0);
        }
	fsp->fi_fs_tstamp = file_stat.st_mtimespec.tv_sec;

	if (use_mmap && file_stat.st_size == fsp->fi_ifile_length) {
		/* (void) close(fid); */
                free(ifile_name);
		return;
	}

	/* get the ifile */
	if (use_mmap) {
		if (fsp->fi_cip)
                    munmap((caddr_t)fsp->fi_cip, fsp->fi_ifile_length);
                /* XXX KS - Do we ever *write* to the ifile? */
		ifp = mmap ((caddr_t)0, file_stat.st_size,
		    PROT_READ, MAP_FILE|MAP_PRIVATE, ifile_fd, (off_t)0);
		if (ifp ==  (caddr_t)(-1)) {
                    syslog(LOG_ERR, "Exiting: get_ifile: mmap failed: %m");
                    exit(1);
                }
	} else {
		if (fsp->fi_cip)
			free(fsp->fi_cip);
		if (!(ifp = malloc (file_stat.st_size))) {
			syslog(LOG_ERR, "Exiting: get_ifile: malloc failed: %m");
                        exit(1);
                }
redo_read:
		count = read (ifile_fd, ifp, (size_t) file_stat.st_size);

		if (count < 0) {
			syslog(LOG_ERR, "Exiting: get_ifile: bad ifile read: %m");
                        exit(1);
                }
		else if (count < file_stat.st_size) {
			syslog(LOG_WARNING, "get_ifile: %m");
			if (lseek(ifile_fd, 0, SEEK_SET) < 0) {
                                syslog(LOG_ERR, "Exiting: get_ifile: bad ifile lseek: %m");
                                exit(1);
                        }
			goto redo_read;
		}
	}
	fsp->fi_ifile_length = file_stat.st_size;
	/* close (fid); */

	fsp->fi_cip = (CLEANERINFO *)ifp;
	fsp->fi_segusep = (SEGUSE *)(ifp + CLEANSIZE(fsp));
	fsp->fi_ifilep  = (IFILE *)((caddr_t)fsp->fi_segusep + SEGTABSIZE(fsp));

	/*
	 * The number of ifile entries is equal to the number of
	 * blocks in the ifile minus the ones allocated to cleaner info
	 * and segment usage table multiplied by the number of ifile
	 * entries per page.
	 */
	fsp->fi_ifile_count = ((fsp->fi_ifile_length >> fsp->fi_lfs.lfs_bshift)
	    - fsp->fi_lfs.lfs_cleansz - fsp->fi_lfs.lfs_segtabsz) *
	    fsp->fi_lfs.lfs_ifpb;

	free (ifile_name);
}


/*
 * Return the size of the partial segment, in bytes.
 */
int
pseg_size(pseg_addr, fsp, sp)
	daddr_t pseg_addr;    /* base address of the segsum */
	FS_INFO *fsp;         /* Filesystem info */
	SEGSUM *sp;           /* the segsum */
{
	int i, ssize = 0;
	struct lfs *lfsp;
	FINFO *fp;

	lfsp = &fsp->fi_lfs;
	ssize = LFS_SUMMARY_SIZE
		+ howmany(sp->ss_ninos, INOPB(lfsp)) * lfsp->lfs_bsize;
	for (fp = (FINFO *)(sp + 1), i = 0; i < sp->ss_nfinfo; ++i) {
		ssize += (fp->fi_nblocks-1) * lfsp->lfs_bsize
			+ fp->fi_lastlength;
		fp = (FINFO *)(&fp->fi_blocks[fp->fi_nblocks]);
	}

	return ssize;
}

/*
 * This function will scan a segment and return a list of
 * <inode, blocknum> pairs which indicate which blocks were
 * contained as live data within the segment when the segment
 * summary was read (it may have "died" since then).  Any given
 * pair will be listed at most once.
 */
int
lfs_segmapv(fsp, seg, seg_buf, blocks, bcount)
	FS_INFO *fsp;		/* pointer to local file system information */
	int seg;		/* the segment number */
	caddr_t seg_buf;	/* the buffer containing the segment's data */
	BLOCK_INFO **blocks;	/* OUT: array of block_info for live blocks */
	int *bcount;		/* OUT: number of active blocks in segment */
{
	BLOCK_INFO *bip, *_bip;
	SEGSUM *sp;
	SEGUSE *sup;
	FINFO *fip;
	struct lfs *lfsp;
	caddr_t s;
	daddr_t pseg_addr, seg_addr;
	int nelem, nblocks, nsegs, sumsize, i, ssize;

	i = 0;
	bip = NULL;
	lfsp = &fsp->fi_lfs;
	nelem = 2 * lfsp->lfs_ssize;
	if (!(bip = malloc(nelem * sizeof(BLOCK_INFO))))
		goto err0;

	sup = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, seg);
	s = seg_buf + (sup->su_flags & SEGUSE_SUPERBLOCK ? LFS_SBPAD : 0);
	seg_addr = sntoda(lfsp, seg);
	pseg_addr = seg_addr + (sup->su_flags & SEGUSE_SUPERBLOCK ? btodb(LFS_SBPAD) : 0);

        if(debug > 1)
            syslog(LOG_DEBUG, "\tsegment buffer at: %p\tseg_addr 0x%x", s, seg_addr);


	*bcount = 0;
	for (nsegs = 0; nsegs < sup->su_nsums; nsegs++) {
		sp = (SEGSUM *)s;

		nblocks = pseg_valid(fsp, sp, pseg_addr);
		if (nblocks <= 0) {
                        syslog(LOG_DEBUG, "Warning: invalid segment summary at 0x%x",
			    pseg_addr);
			goto err0;
		}

#ifdef DIAGNOSTIC
		/* Verify size of summary block */
		sumsize = sizeof(SEGSUM) +
		    (sp->ss_ninos + INOPB(lfsp) - 1) / INOPB(lfsp);
		for (i = 0, fip = (FINFO *)(sp + 1); i < sp->ss_nfinfo; ++i) {
			sumsize += sizeof(FINFO) +
			    (fip->fi_nblocks - 1) * sizeof(daddr_t);
			fip = (FINFO *)(&fip->fi_blocks[fip->fi_nblocks]);
		}
		if (sumsize > LFS_SUMMARY_SIZE) {
                        syslog(LOG_ERR,
			    "Exiting: Segment %d summary block too big: %d\n",
			    seg, sumsize);
			exit(1);
		}
#endif

		if (*bcount + nblocks + sp->ss_ninos > nelem) {
			nelem = *bcount + nblocks + sp->ss_ninos;
			bip = realloc (bip, nelem * sizeof(BLOCK_INFO));
			if (!bip)
				goto err0;
		}
		add_blocks(fsp, bip, bcount, sp, seg_buf, seg_addr, pseg_addr);
		add_inodes(fsp, bip, bcount, sp, seg_buf, seg_addr);

		ssize = pseg_size(pseg_addr, fsp, sp);
		s += ssize;
		pseg_addr += btodb(ssize); /* XXX was bytetoda(fsp,ssize) */
	}
	if(nsegs < sup->su_nsums) {
		syslog(LOG_WARNING,"only %d segment summaries in seg %d (expected %d)",
		       nsegs, seg, sup->su_nsums);
		goto err0;
	}
	qsort(bip, *bcount, sizeof(BLOCK_INFO), bi_compare);
	toss(bip, bcount, sizeof(BLOCK_INFO), bi_toss, NULL);

        if(debug > 1) {
            syslog(LOG_DEBUG, "BLOCK INFOS");
            for (_bip = bip, i=0; i < *bcount; ++_bip, ++i)
                PRINT_BINFO(_bip);
        }

	*blocks = bip;
	return (0);

    err0:
	if (bip)
		free(bip);
	*bcount = 0;
	return (-1);

}

/*
 * This will parse a partial segment and fill in BLOCK_INFO structures
 * for each block described in the segment summary.  It will not include
 * blocks or inodes from files with new version numbers.
 */
void
add_blocks (fsp, bip, countp, sp, seg_buf, segaddr, psegaddr)
	FS_INFO *fsp;		/* pointer to super block */
	BLOCK_INFO *bip;	/* Block info array */
	int *countp;		/* IN/OUT: number of blocks in array */
	SEGSUM	*sp;		/* segment summmary pointer */
	caddr_t seg_buf;	/* buffer containing segment */
	daddr_t segaddr;	/* address of this segment */
	daddr_t psegaddr;	/* address of this partial segment */
{
	IFILE	*ifp;
	FINFO	*fip;
	caddr_t	bp;
	daddr_t	*dp, *iaddrp;
	int db_per_block, i, j;
	int db_frag;
	u_long page_size;

        if(debug > 1)
            syslog(LOG_DEBUG, "FILE INFOS");

	db_per_block = fsbtodb(&fsp->fi_lfs, 1);
	page_size = fsp->fi_lfs.lfs_bsize;
	bp = seg_buf + datobyte(fsp, psegaddr - segaddr) + LFS_SUMMARY_SIZE;
	bip += *countp;
	psegaddr += bytetoda(fsp, LFS_SUMMARY_SIZE);
	iaddrp = (daddr_t *)((caddr_t)sp + LFS_SUMMARY_SIZE);
	--iaddrp;
	for (fip = (FINFO *)(sp + 1), i = 0; i < sp->ss_nfinfo;
	    ++i, fip = (FINFO *)(&fip->fi_blocks[fip->fi_nblocks])) {

		ifp = IFILE_ENTRY(&fsp->fi_lfs, fsp->fi_ifilep, fip->fi_ino);
		PRINT_FINFO(fip, ifp);
		if (ifp->if_version > fip->fi_version)
			continue;
		dp = &(fip->fi_blocks[0]);
		for (j = 0; j < fip->fi_nblocks; j++, dp++) {
			/* Skip over intervening inode blocks */
			while (psegaddr == *iaddrp) {
				psegaddr += db_per_block;
				bp += page_size;
				--iaddrp;
			}
			bip->bi_inode = fip->fi_ino;
			bip->bi_lbn = *dp;
			bip->bi_daddr = psegaddr;
			bip->bi_segcreate = (time_t)(sp->ss_create);
			bip->bi_bp = bp;
			bip->bi_version = ifp->if_version;

			if (j < fip->fi_nblocks-1
			    || fip->fi_lastlength == page_size)
			{
				bip->bi_size = page_size;
				psegaddr += db_per_block;
				bp += page_size;
			} else {
				db_frag = fragstodb(&(fsp->fi_lfs),
				    numfrags(&(fsp->fi_lfs),
				    fip->fi_lastlength));

                                if(debug > 1) {
					syslog(LOG_DEBUG, "lastlength, frags: %d, %d",
					       fip->fi_lastlength, db_frag);
				}

				bip->bi_size = fip->fi_lastlength;
				bp += fip->fi_lastlength;
				psegaddr += db_frag;
			}
			++bip;
			++(*countp);
		}
	}
}

/*
 * For a particular segment summary, reads the inode blocks and adds
 * INODE_INFO structures to the array.  Returns the number of inodes
 * actually added.
 */
void
add_inodes (fsp, bip, countp, sp, seg_buf, seg_addr)
	FS_INFO *fsp;		/* pointer to super block */
	BLOCK_INFO *bip;	/* block info array */
	int *countp;		/* pointer to current number of inodes */
	SEGSUM *sp;		/* segsum pointer */
	caddr_t	seg_buf;	/* the buffer containing the segment's data */
	daddr_t	seg_addr;	/* disk address of seg_buf */
{
	struct dinode *di = NULL;	/* XXX gcc */
	struct lfs *lfsp;
	IFILE *ifp;
	BLOCK_INFO *bp;
	daddr_t	*daddrp;
	ino_t inum;
	int i;

	if (sp->ss_ninos <= 0)
		return;

	bp = bip + *countp;
	lfsp = &fsp->fi_lfs;

        if(debug > 1)
            syslog(LOG_DEBUG, "INODES:");

	daddrp = (daddr_t *)((caddr_t)sp + LFS_SUMMARY_SIZE);
	for (i = 0; i < sp->ss_ninos; ++i) {
		if (i % INOPB(lfsp) == 0) {
			--daddrp;
			di = (struct dinode *)(seg_buf +
			    ((*daddrp - seg_addr) << fsp->fi_daddr_shift));
		} else
			++di;

		inum = di->di_inumber;
		bp->bi_lbn = LFS_UNUSED_LBN;
		bp->bi_inode = inum;
		bp->bi_daddr = *daddrp;
		bp->bi_bp = di;
		bp->bi_segcreate = sp->ss_create;
		bp->bi_size = i; /* XXX KS - kludge */

		if (inum == LFS_IFILE_INUM) {
			bp->bi_version = 1;	/* Ifile version should be 1 */
			bp++;
			++(*countp);
			PRINT_INODE(1, bp);
		} else {
			ifp = IFILE_ENTRY(lfsp, fsp->fi_ifilep, inum);
			PRINT_INODE(ifp->if_daddr == *daddrp, bp);
			bp->bi_version = ifp->if_version;
			if (ifp->if_daddr == *daddrp) {
				bp++;
				++(*countp);
			}
		}
	}
}

/*
 * Checks the summary checksum and the data checksum to determine if the
 * segment is valid or not.  Returns the size of the partial segment if it
 * is valid, and 0 otherwise.  Use dump_summary to figure out size of the
 * the partial as well as whether or not the checksum is valid.
 */
int
pseg_valid (fsp, ssp, addr)
	FS_INFO *fsp;   /* pointer to file system info */
	SEGSUM *ssp;	/* pointer to segment summary block */
	daddr_t addr;   /* address of the summary block on disk */
{
	int nblocks;
#if 0
	caddr_t	p;
	int i;
	u_long *datap;
#endif

	if (ssp->ss_magic != SS_MAGIC) {
                syslog(LOG_WARNING, "Bad magic number: 0x%x instead of 0x%x", ssp->ss_magic, SS_MAGIC);
		return(0);
        }

	if ((nblocks = dump_summary(&fsp->fi_lfs, ssp, 0, NULL, addr)) <= 0 ||
	    nblocks > fsp->fi_lfs.lfs_ssize - 1)
		return(0);

#if 0
	/* check data/inode block(s) checksum too */
	datap = (u_long *)malloc(nblocks * sizeof(u_long));
	p = (caddr_t)ssp + LFS_SUMMARY_SIZE;
	for (i = 0; i < nblocks; ++i) {
		datap[i] = *((u_long *)p);
		p += fsp->fi_lfs.lfs_bsize;
	}
	if (cksum ((void *)datap, nblocks * sizeof(u_long)) != ssp->ss_datasum) {
                syslog(LOG_WARNING, "Bad data checksum");
		free(datap);
		return 0;
        }
#endif
	return (nblocks);
}


/* #define MMAP_SEGMENT */
/*
 * read a segment into a memory buffer
 */
int
mmap_segment (fsp, segment, segbuf, use_mmap)
	FS_INFO *fsp;		/* file system information */
	int segment;		/* segment number */
	caddr_t *segbuf;	/* pointer to buffer area */
	int use_mmap;		/* mmap instead of read */
{
	struct lfs *lfsp;
	daddr_t seg_daddr;	/* base disk address of segment */
	off_t seg_byte;
	size_t ssize;
	char mntfromname[MNAMELEN+2];

	lfsp = &fsp->fi_lfs;

	/* get the disk address of the beginning of the segment */
	seg_daddr = sntoda(lfsp, segment);
	seg_byte = datobyte(fsp, seg_daddr);
	ssize = seg_size(lfsp);

	strcpy(mntfromname, "/dev/r");
	strcat(mntfromname, fsp->fi_statfsp->f_mntfromname+5);

	if(dev_fd <= 0) {
		if ((dev_fd = open(mntfromname, O_RDONLY, (mode_t)0)) < 0) {
			syslog(LOG_WARNING,"mmap_segment: bad open: %m");
			return (-1);
		}
	} else
		lseek(dev_fd, 0, SEEK_SET);

	if (use_mmap) {
		*segbuf = mmap ((caddr_t)0, seg_size(lfsp), PROT_READ,
		    MAP_FILE|MAP_SHARED, dev_fd, seg_byte);
		if (*(long *)segbuf < 0) {
                        syslog(LOG_WARNING,"mmap_segment: mmap failed: %m");
			return (0);
		}
	} else {
                if(debug > 1)
                        syslog(LOG_DEBUG, "mmap_segment\tseg_daddr: %lu\tseg_size: %lu\tseg_offset: %llu",
                               (u_long)seg_daddr, (u_long)ssize, (long long)seg_byte);
            
		/* malloc the space for the buffer */
		*segbuf = malloc(ssize);
		if (!*segbuf) {
			syslog(LOG_WARNING,"mmap_segment: malloc failed: %m");
			return (0);
		}

		/* read the segment data into the buffer */
		if (lseek (dev_fd, seg_byte, SEEK_SET) != seg_byte) {
			syslog(LOG_WARNING,"mmap_segment: bad lseek: %m");
			free(*segbuf);
			return (-1);
		}
		if (read (dev_fd, *segbuf, ssize) != ssize) {
			syslog(LOG_WARNING,"mmap_segment: bad read: %m");
			free(*segbuf);
			return (-1);
		}
	}
	/* close (fid); */

	return (0);
}

void
munmap_segment (fsp, seg_buf, use_mmap)
	FS_INFO *fsp;		/* file system information */
	caddr_t seg_buf;	/* pointer to buffer area */
	int use_mmap;		/* mmap instead of read/write */
{
	if (use_mmap)
		munmap (seg_buf, seg_size(&fsp->fi_lfs));
	else
		free (seg_buf);
}

/*
 * USEFUL DEBUGGING TOOLS:
 */
void
print_SEGSUM (lfsp, p, addr)
	struct lfs *lfsp;
	SEGSUM	*p;
	daddr_t addr;
{
	if (p)
		(void) dump_summary(lfsp, p, DUMP_ALL, NULL, addr);
	else
                syslog(LOG_DEBUG, "0x0");
}

int
bi_compare(a, b)
	const void *a;
	const void *b;
{
	const BLOCK_INFO *ba, *bb;
	int diff;

	ba = a;
	bb = b;

	if ((diff = (int)(ba->bi_inode - bb->bi_inode)))
		return (diff);
	if ((diff = (int)(ba->bi_lbn - bb->bi_lbn))) {
		if (ba->bi_lbn == LFS_UNUSED_LBN)
			return(-1);
		else if (bb->bi_lbn == LFS_UNUSED_LBN)
			return(1);
		else if (ba->bi_lbn < 0 && bb->bi_lbn >= 0)
			return(1);
		else if (bb->bi_lbn < 0 && ba->bi_lbn >= 0)
			return(-1);
		else
			return (diff);
	}
	if ((diff = (int)(ba->bi_daddr - bb->bi_daddr)))
		return (diff);
	if(ba->bi_inode != LFS_IFILE_INUM && debug)
		syslog(LOG_DEBUG,"bi_compare: using kludge on ino %d!", ba->bi_inode);
	diff = ba->bi_size - bb->bi_size;
	return diff;
}

int
bi_toss(dummy, a, b)
	const void *dummy;
	const void *a;
	const void *b;
{
	const BLOCK_INFO *ba, *bb;

	ba = a;
	bb = b;

	return(ba->bi_inode == bb->bi_inode && ba->bi_lbn == bb->bi_lbn);
}

void
toss(p, nump, size, dotoss, client)
	void *p;
	int *nump;
	size_t size;
	int (*dotoss) __P((const void *, const void *, const void *));
	void *client;
{
	int i;
	char *p0, *p1;

	if (*nump == 0)
		return;

	p0 = p;
	for (i = *nump; --i > 0;) {
		p1 = p0 + size;
		if (dotoss(client, p0, p1)) {
			memmove(p0, p1, i * size);
			--(*nump);
		} else
			p0 += size;
	}
}
