/*	$NetBSD: library.c,v 1.33 2003/02/10 21:17:53 fvdl Exp $	*/

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
__RCSID("$NetBSD: library.c,v 1.33 2003/02/10 21:17:53 fvdl Exp $");
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

void	 add_blocks(FS_INFO *, BLOCK_INFO_15 *, int *, SEGSUM *, caddr_t,
    daddr_t, daddr_t);
void	 add_inodes(FS_INFO *, BLOCK_INFO_15 *, int *, SEGSUM *, caddr_t,
    daddr_t);
int	 bi_compare(const void *, const void *);
int	 bi_toss(const void *, const void *, const void *);
void	 get_ifile(FS_INFO *, int);
int	 get_superblock(FS_INFO *, struct lfs *);
int	 pseg_valid(FS_INFO *, SEGSUM *, daddr_t);
int      pseg_size(daddr_t, FS_INFO *, SEGSUM *);

extern int debug;
extern u_long cksum(void *, size_t);	/* XXX */

static int ifile_fd = -1;
static int dev_fd = -1;

/*
 * This function will get information on a a filesystem which matches
 * the name and type given.  If a "name" is in a filesystem of the given
 * type, then buf is filled with that filesystem's info, and the
 * a non-zero value is returned.
 */
int
fs_getmntinfo(struct statfs **buf, char *name, const char *type)
{
	/* allocate space for the filesystem info */
	if ((*buf = malloc(sizeof(struct statfs))) == NULL)
		return 0;

	/* grab the filesystem info */
        if (ifile_fd == -1) {
		if (statfs(name, *buf) == -1)
			goto bad;
        } else if(fstatfs(ifile_fd, *buf) == -1)
		goto bad;

	/* check to see if it's the one we want */
	if (strncmp(type, (*buf)->f_fstypename, MFSNAMELEN) ||
	    strncmp(name, (*buf)->f_mntonname, MNAMELEN))
		goto bad;

	return 1;
bad:
	free(*buf);
	return 0;
}

/*
 * Get all the information available on an LFS file system.
 * Returns an pointer to an FS_INFO structure, NULL on error.
 */
FS_INFO *
get_fs_info(struct statfs *lstatfsp, int use_mmap)
{
	FS_INFO	*fsp;

	if ((fsp = malloc(sizeof(FS_INFO))) == NULL)
		return NULL;
	memset(fsp, 0, sizeof(FS_INFO));

	fsp->fi_statfsp = lstatfsp;
	if (get_superblock(fsp, &fsp->fi_lfs)) {
		syslog(LOG_ERR, "get_fs_info: get_superblock failed (%m)");
                exit(1);
        }
	get_ifile(fsp, use_mmap);
	return (fsp);
}

/*
 * If we are reading the ifile then we need to refresh it.  Even if
 * we are mmapping it, it might have grown.  Finally, we need to
 * refresh the file system information (statfs) info.
 */
void
reread_fs_info(FS_INFO *fsp, int use_mmap)
{
	if (ifile_fd != -1) {
		if (fstatfs(ifile_fd, fsp->fi_statfsp) == -1) {
			syslog(LOG_ERR, "reread_fs_info: fstatfs failed (%m)");
                	exit(1);
		}
	} else if (statfs(fsp->fi_statfsp->f_mntonname, fsp->fi_statfsp)) {
		syslog(LOG_ERR, "reread_fs_info: statfs `%s' failed (%m)",
		    fsp->fi_statfsp->f_mntonname);
                exit(1);
        }
	get_ifile(fsp, use_mmap);
}

static int
getdevfd(FS_INFO *fsp)
{
	char rdev[MNAMELEN];

	if (dev_fd != -1)
		return dev_fd;

	(void)snprintf(rdev, sizeof(rdev), "/dev/r%s",
	    fsp->fi_statfsp->f_mntfromname + 5);
	if ((dev_fd = open(rdev, O_RDONLY)) == -1) {
		syslog(LOG_ERR, "Cannot open `%s' (%m)", rdev);
		exit(1);
	}
	return dev_fd;
}


/*
 * Read a block from disk.
 */
int
get_rawblock(FS_INFO *fsp, char *buf, size_t size, daddr_t daddr)
{
	return pread(getdevfd(fsp), buf, size, fsbtob(&fsp->fi_lfs,
	    (off_t)daddr));
}

/*
 * Read an inode from disk.
 */
struct dinode *
get_dinode(FS_INFO *fsp, ino_t ino)
{
        static struct dinode dino;
        struct dinode *dip, *dib;
        struct lfs *lfsp;
	BLOCK_INFO_15 bi;

        lfsp = &fsp->fi_lfs;

	/*
	 * Locate the inode block and find the inode.
	 * Use this to know how large the file is.
	 */
	memset(&bi, 0, sizeof(bi));
	bi.bi_inode = ino;
	bi.bi_lbn = LFS_UNUSED_LBN; /* We want the inode */
	if (lfs_bmapv(&fsp->fi_statfsp->f_fsid, &bi, 1) < 0) {
		syslog(LOG_WARNING, "lfs_bmapv: %m");
		return NULL;
	}
	if (bi.bi_daddr <= 0)
		return NULL;

	lseek(getdevfd(fsp), (off_t)0, SEEK_SET);
	if ((dib = malloc(lfsp->lfs_ibsize)) == NULL) {
		syslog(LOG_WARNING, "lfs_bmapv: %m");
		return NULL;
	}

	pread(dev_fd, dib, lfsp->lfs_ibsize, fsbtob(lfsp, (off_t)bi.bi_daddr));
	for (dip = dib; dip != dib + lfsp->lfs_inopb; ++dip)
		if (dip->di_u.inumber == ino)
			break;
	if (dip == dib + lfsp->lfs_inopb) {
		free(dib);
		syslog(LOG_WARNING, "dinode %d not found at fsb 0x%x",
		    ino, bi.bi_daddr);
		return NULL;
	}
        dino = *dip; /* structure copy */

        free(dib);
        return &dino;
}

/*
 * Gets the superblock from disk (possibly in face of errors)
 */
int
get_superblock(FS_INFO *fsp, struct lfs *sbp)
{
	char buf[LFS_SBPAD];
	static off_t sboff = LFS_LABELPAD;

	lseek(getdevfd(fsp), 0, SEEK_SET);
		
	for (;;) {
		get(dev_fd, sboff, buf, LFS_SBPAD);
		memcpy(&(sbp->lfs_dlfs), buf, sizeof(struct dlfs));
		if (sboff == LFS_LABELPAD && fsbtob(sbp, 1) > LFS_LABELPAD)
			sboff = fsbtob(sbp, (off_t)sbp->lfs_sboffs[0]);
		else
			break;
	}
	
	/* Compatibility */
	if (sbp->lfs_version < 2) {
		sbp->lfs_sumsize = LFS_V1_SUMMARY_SIZE;
		sbp->lfs_ibsize = sbp->lfs_bsize;
		sbp->lfs_start = sbp->lfs_sboffs[0];
		sbp->lfs_tstamp = sbp->lfs_otstamp;
		sbp->lfs_fsbtodb = 0;
	}

	return (0);
}

/*
 * This function will map the ifile into memory.  It causes a
 * fatal error on failure.
 */
void
get_ifile(FS_INFO *fsp, int use_mmap)
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

	if(ifile_fd == -1) {
		/* XXX KS - Do we ever *write* to the ifile? */
		if ((ifile_fd = open(ifile_name, O_RDONLY)) == -1) {
			syslog(LOG_ERR, "get_ifile: cannot open `%s': %m",
			    ifile_name);
			exit(1);
		}
	} else
		lseek(ifile_fd, (off_t)0, SEEK_SET);

	if (fstat(ifile_fd, &file_stat) == -1) {
		/* If the fs was unmounted, don't complain */
		if (statfs(fsp->fi_statfsp->f_mntonname, &statfsbuf) != -1) {
			if(memcmp(&statfsbuf.f_fsid, &fsp->fi_statfsp->f_fsid,
			    sizeof(statfsbuf.f_fsid)) != 0) {
				/* Filesystem still mounted, 
				 * this error is real
				 */
				syslog(LOG_ERR, "get_ifile: fstat failed: %m");
				exit(1);
			}
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
		ifp = mmap((caddr_t)0, file_stat.st_size,
		    PROT_READ, MAP_FILE|MAP_PRIVATE, ifile_fd, (off_t)0);
		if (ifp == (caddr_t)(-1)) {
                    syslog(LOG_ERR, "get_ifile: mmap failed (%m)");
                    exit(1);
                }
	} else {
		if (fsp->fi_cip)
			free(fsp->fi_cip);
		if ((ifp = malloc(file_stat.st_size)) == NULL) {
			syslog(LOG_ERR, "get_ifile: malloc failed (%m)");
                        exit(1);
                }
redo_read:
		count = read(ifile_fd, ifp, (size_t)file_stat.st_size);

		if (count < 0) {
			syslog(LOG_ERR, "get_ifile: bad ifile read (%m)");
                        exit(1);
                }
		else if (count < file_stat.st_size) {
			syslog(LOG_WARNING, "get_ifile (%m)");
			if (lseek(ifile_fd, 0, SEEK_SET) < 0) {
                                syslog(LOG_ERR,
				    "get_ifile: bad ifile lseek (%m)");
                                exit(1);
                        }
			goto redo_read;
		}
	}
	fsp->fi_ifile_length = file_stat.st_size;

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

	free(ifile_name);
}


/*
 * Return the size of the partial segment, in bytes.
 */
int
pseg_size(daddr_t pseg_addr, FS_INFO *fsp, SEGSUM *sp)
{
	int i, ssize = 0;
	struct lfs *lfsp;
	FINFO *fp;

	lfsp = &fsp->fi_lfs;
	ssize = lfsp->lfs_sumsize
		+ howmany(sp->ss_ninos, INOPB(lfsp)) * lfsp->lfs_ibsize;

	if (lfsp->lfs_version == 1)
		fp = (FINFO *)(((char *)sp) + sizeof(SEGSUM_V1));
	else
		fp = (FINFO *)(sp + 1);
	for (i = 0; i < sp->ss_nfinfo; ++i) {
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
lfs_segmapv(FS_INFO *fsp, int seg, caddr_t seg_buf, BLOCK_INFO_15 **blocks, int *bcount)
{
	BLOCK_INFO_15 *bip, *_bip;
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
	nelem = 2 * segtod(lfsp, 1);
	if (!(bip = malloc(nelem * sizeof(BLOCK_INFO_15)))) {
		syslog(LOG_DEBUG, "couldn't allocate %ld bytes in lfs_segmapv",
			(long)(nelem * sizeof(BLOCK_INFO_15)));
		goto err0;
	}

	sup = SEGUSE_ENTRY(lfsp, fsp->fi_segusep, seg);
	s = seg_buf + (sup->su_flags & SEGUSE_SUPERBLOCK ? LFS_SBPAD : 0);
	seg_addr = sntod(lfsp, seg);
	pseg_addr = seg_addr + (sup->su_flags & SEGUSE_SUPERBLOCK ? 
		btofsb(lfsp, LFS_SBPAD) : 0);
	if (seg == 0 && lfsp->lfs_version > 1) {
		if (fsbtob(lfsp, pseg_addr) < LFS_LABELPAD + LFS_SBPAD) {
			pseg_addr = btofsb(lfsp, LFS_LABELPAD + LFS_SBPAD);
			s = seg_buf + LFS_LABELPAD + LFS_SBPAD;
			syslog(LOG_DEBUG, "adj segment 0 offset to 0x%llx\n",
			       (long long)pseg_addr);
		}
	}

        if(debug > 1)
            syslog(LOG_DEBUG, "\tsegment buffer at: %p\tseg_addr 0x%llx", s,
		(long long)seg_addr);


	*bcount = 0;
	for (nsegs = 0; nsegs < sup->su_nsums; nsegs++) {
		sp = (SEGSUM *)s;

		nblocks = pseg_valid(fsp, sp, pseg_addr);
		if (nblocks <= 0) {
                        syslog(LOG_DEBUG, "Warning: invalid segment summary at 0x%llx",
			    (long long)pseg_addr);
			goto err0;
		}

#ifdef DIAGNOSTIC
		/* Verify size of summary block */
		sumsize = (lfsp->lfs_version == 1 ? sizeof(SEGSUM_V1) :
							sizeof(SEGSUM)) +
		    (sp->ss_ninos + INOPB(lfsp) - 1) / INOPB(lfsp);
		if (lfsp->lfs_version == 1)
			fip = (FINFO *)(((char *)sp) + sizeof(SEGSUM_V1));
		else
			fip = (FINFO *)(sp + 1);
		for (i = 0; i < sp->ss_nfinfo; ++i) {
			sumsize += sizeof(FINFO) +
			    (fip->fi_nblocks - 1) * sizeof(int32_t);
			fip = (FINFO *)(&fip->fi_blocks[fip->fi_nblocks]);
		}
		if (sumsize > lfsp->lfs_sumsize) {
                        syslog(LOG_ERR,
			    "Segment %d summary block too big: %d",
			    seg, sumsize);
			exit(1);
		}
#endif

		if (*bcount + nblocks + sp->ss_ninos > nelem) {
			nelem = *bcount + nblocks + sp->ss_ninos;
			bip = realloc(bip, nelem * sizeof(BLOCK_INFO_15));
			if (!bip)
				goto err0;
		}
		add_blocks(fsp, bip, bcount, sp, seg_buf, seg_addr, pseg_addr);
		add_inodes(fsp, bip, bcount, sp, seg_buf, seg_addr);

		ssize = pseg_size(pseg_addr, fsp, sp);
		s += ssize;
		pseg_addr += btofsb(lfsp, ssize); 
	}
	if(nsegs < sup->su_nsums) {
		syslog(LOG_WARNING,"only %d segment summaries in seg %d (expected %d)",
		       nsegs, seg, sup->su_nsums);
		goto err0;
	}
	qsort(bip, *bcount, sizeof(BLOCK_INFO_15), bi_compare);
	toss(bip, bcount, sizeof(BLOCK_INFO_15), bi_toss, NULL);

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
 * This will parse a partial segment and fill in BLOCK_INFO_15 structures
 * for each block described in the segment summary.  It will not include
 * blocks or inodes from files with new version numbers.
 */
void
add_blocks(FS_INFO *fsp, BLOCK_INFO_15 *bip, int *countp, SEGSUM *sp,
    caddr_t seg_buf, daddr_t segaddr, daddr_t psegaddr)
{
	IFILE	*ifp;
	FINFO	*fip;
	caddr_t	bp;
	/* XXX ondisk32 */
	int32_t	*dp, *iaddrp;
	int fsb_per_block, fsb_per_iblock, i, j;
	int fsb_frag, iblks_seen;
	u_long iblk_size, blk_size;
	struct lfs *lfsp;

        if(debug > 1)
            syslog(LOG_DEBUG, "FILE INFOS");

	lfsp = &fsp->fi_lfs;
	fsb_per_block = fragstofsb(lfsp, lfsp->lfs_frag);
	fsb_per_iblock = btofsb(lfsp, lfsp->lfs_ibsize);
	blk_size = fsp->fi_lfs.lfs_bsize;
	iblk_size = fsp->fi_lfs.lfs_ibsize;
	bp = seg_buf + fsbtob(lfsp, psegaddr - segaddr) + lfsp->lfs_sumsize;
	bip += *countp;
	psegaddr += btofsb(lfsp, lfsp->lfs_sumsize);
	/* XXX ondisk32 */
	iaddrp = (int32_t *)((caddr_t)sp + lfsp->lfs_sumsize);
	--iaddrp;
	if (lfsp->lfs_version == 1)
		fip = (FINFO *)(((char *)sp) + sizeof(SEGSUM_V1));
	else
		fip = (FINFO *)(sp + 1);
	iblks_seen = 0;
	for (i = 0; i < sp->ss_nfinfo;
	    ++i, fip = (FINFO *)(&fip->fi_blocks[fip->fi_nblocks])) {

		ifp = IFILE_ENTRY(&fsp->fi_lfs, fsp->fi_ifilep, fip->fi_ino);
		PRINT_FINFO(fip, ifp);
		dp = &(fip->fi_blocks[0]);
		for (j = 0; j < fip->fi_nblocks; j++, dp++) {
			/* Skip over intervening inode blocks */
			while (psegaddr == *iaddrp) {
				/* syslog(LOG_DEBUG, "skipping ino block at 0x%x",
				       psegaddr); */
				psegaddr += fsb_per_iblock;
				bp += iblk_size;
				--iaddrp;
			}
			bip->bi_inode = fip->fi_ino;
			bip->bi_lbn = *dp;
			bip->bi_daddr = psegaddr;
			if (lfsp->lfs_version == 1) 
				bip->bi_segcreate = (time_t)(sp->ss_ident);
			else
				bip->bi_segcreate = (time_t)(sp->ss_create);
			/* syslog(LOG_DEBUG, "ino %d lbn %d 0x%x %p",
				bip->bi_inode, bip->bi_lbn, bip->bi_daddr,
				bp); */
			bip->bi_bp = bp;
			bip->bi_version = ifp->if_version;

			if (j < fip->fi_nblocks-1
			    || fip->fi_lastlength == blk_size)
			{
				bip->bi_size = blk_size;
				psegaddr += fsb_per_block;
				bp += blk_size;
			} else {
				fsb_frag = fragstofsb(&(fsp->fi_lfs),
				    numfrags(&(fsp->fi_lfs),
				    fip->fi_lastlength));

                                if(debug > 1) {
					syslog(LOG_DEBUG, "lastlength, frags: %d, %d",
					       fip->fi_lastlength, fsb_frag);
				}

				bip->bi_size = fip->fi_lastlength;
				bp += fip->fi_lastlength;
				psegaddr += fsb_frag;
			}
			if (ifp->if_version == fip->fi_version) {
				++bip;
				++(*countp);
			}
		}
	}
}

/*
 * For a particular segment summary, reads the inode blocks and adds
 * INODE_INFO structures to the array.  Returns the number of inodes
 * actually added.
 */
void
add_inodes(FS_INFO *fsp, BLOCK_INFO_15 *bip, int *countp, SEGSUM *sp,
	    caddr_t seg_buf, daddr_t seg_addr)
{
	struct dinode *di = NULL;	/* XXX gcc */
	struct lfs *lfsp;
	IFILE *ifp;
	BLOCK_INFO_15 *bp;
	int32_t	*daddrp;
	ino_t inum;
	int i;

	if (sp->ss_ninos <= 0)
		return;

	bp = bip + *countp;
	lfsp = &fsp->fi_lfs;

        if(debug > 1)
            syslog(LOG_DEBUG, "INODES:");

	daddrp = (int32_t *)((caddr_t)sp + lfsp->lfs_sumsize);
	for (i = 0; i < sp->ss_ninos; ++i) {
		if (i % INOPB(lfsp) == 0) {
			--daddrp;
			di = (struct dinode *)(seg_buf + fsbtob(lfsp, 
				*daddrp - seg_addr));
		} else
			++di;

		inum = di->di_inumber;
		bp->bi_lbn = LFS_UNUSED_LBN;
		bp->bi_inode = inum;
		bp->bi_daddr = *daddrp;
		bp->bi_bp = di;
		if (lfsp->lfs_version == 1) 
			bp->bi_segcreate = sp->ss_ident;
		else
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
pseg_valid(FS_INFO *fsp, SEGSUM *ssp, daddr_t addr)
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
	    nblocks > (fsp->fi_lfs.lfs_ssize / fsp->fi_lfs.lfs_fsize) - 1)
		return(0);

#if 0
	/* check data/inode block(s) checksum too */
	datap = (u_long *)malloc(nblocks * sizeof(u_long));
	p = (caddr_t)ssp + lfsp->lfs_sumsize;
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
mmap_segment(FS_INFO *fsp, int segment, caddr_t *segbuf, int use_mmap)
{
	struct lfs *lfsp;
	daddr_t seg_daddr;	/* base disk address of segment */
	off_t seg_byte;
	size_t ssize;

	lfsp = &fsp->fi_lfs;

	/* get the disk address of the beginning of the segment */
	seg_daddr = sntod(lfsp, segment);
	seg_byte = fsbtob(lfsp, (off_t)seg_daddr);
	ssize = seg_size(lfsp);

	lseek(getdevfd(fsp), 0, SEEK_SET);

	if (use_mmap) {
		*segbuf = mmap((caddr_t)0, seg_size(lfsp), PROT_READ,
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
		if (lseek(dev_fd, seg_byte, SEEK_SET) != seg_byte) {
			syslog(LOG_WARNING,"mmap_segment: bad lseek: %m");
			free(*segbuf);
			return (-1);
		}
		if (read(dev_fd, *segbuf, ssize) != ssize) {
			syslog(LOG_WARNING,"mmap_segment: bad read: %m");
			free(*segbuf);
			return (-1);
		}
	}
	/* close (fid); */

	return (0);
}

void
munmap_segment(FS_INFO *fsp, caddr_t seg_buf, int use_mmap)
{
	if (use_mmap)
		munmap(seg_buf, seg_size(&fsp->fi_lfs));
	else
		free(seg_buf);
}

/*
 * USEFUL DEBUGGING TOOLS:
 */
void
print_SEGSUM(struct lfs *lfsp, SEGSUM *p, daddr_t addr)
{
	if (p)
		(void) dump_summary(lfsp, p, DUMP_ALL, NULL, addr);
	else
                syslog(LOG_DEBUG, "0x0");
}

int
bi_compare(const void *a, const void *b)
{
	const BLOCK_INFO_15 *ba, *bb;
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
bi_toss(const void *dummy, const void *a, const void *b)
{
	const BLOCK_INFO_15 *ba, *bb;

	ba = a;
	bb = b;

	return(ba->bi_inode == bb->bi_inode && ba->bi_lbn == bb->bi_lbn);
}

void
toss(void *p, int *nump, size_t size, int (*dotoss)(const void *, const void *, const void *), void *client)
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
