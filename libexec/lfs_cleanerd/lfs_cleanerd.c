/* $NetBSD: lfs_cleanerd.c,v 1.54 2015/10/03 08:28:30 dholland Exp $	 */

/*-
 * Copyright (c) 2005 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Konrad E. Schroder <perseant@hhhh.org>.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
 * The cleaner daemon for the NetBSD Log-structured File System.
 * Only tested for use with version 2 LFSs.
 */

#include <sys/syslog.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <ufs/lfs/lfs.h>

#include <assert.h>
#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <util.h>

#include "bufcache.h"
#include "vnode.h"
#include "lfs_user.h"
#include "fdfs.h"
#include "cleaner.h"
#include "kernelops.h"
#include "mount_lfs.h"

/*
 * Global variables.
 */
/* XXX these top few should really be fs-specific */
int use_fs_idle;	/* Use fs idle rather than cpu idle time */
int use_bytes;		/* Use bytes written rather than segments cleaned */
double load_threshold;	/* How idle is idle (CPU idle) */
int atatime;		/* How many segments (bytes) to clean at a time */

int nfss;		/* Number of filesystems monitored by this cleanerd */
struct clfs **fsp;	/* Array of extended filesystem structures */
int segwait_timeout;	/* Time to wait in lfs_segwait() */
int do_quit;		/* Quit after one cleaning loop */
int do_coalesce;	/* Coalesce filesystem */
int do_small;		/* Use small writes through markv */
char *copylog_filename; /* File to use for fs debugging analysis */
int inval_segment;	/* Segment to invalidate */
int stat_report;	/* Report statistics for this period of cycles */
int debug;		/* Turn on debugging */
struct cleaner_stats {
	double	util_tot;
	double	util_sos;
	off_t	bytes_read;
	off_t	bytes_written;
	off_t	segs_cleaned;
	off_t	segs_empty;
	off_t	segs_error;
} cleaner_stats;

extern u_int32_t cksum(void *, size_t);
extern u_int32_t lfs_sb_cksum(struct dlfs *);
extern u_int32_t lfs_cksum_part(void *, size_t, u_int32_t);
extern int ulfs_getlbns(struct lfs *, struct uvnode *, daddr_t, struct indir *, int *);

/* Ugh */
#define FSMNT_SIZE MAX(sizeof(((struct dlfs *)0)->dlfs_fsmnt), \
			sizeof(((struct dlfs64 *)0)->dlfs_fsmnt))


/* Compat */
void pwarn(const char *unused, ...) { /* Does nothing */ };

/*
 * Log a message if debugging is turned on.
 */
void
dlog(const char *fmt, ...)
{
	va_list ap;

	if (debug == 0)
		return;

	va_start(ap, fmt);
	vsyslog(LOG_DEBUG, fmt, ap);
	va_end(ap);
}

/*
 * Remove the specified filesystem from the list, due to its having
 * become unmounted or other error condition.
 */
void
handle_error(struct clfs **cfsp, int n)
{
	syslog(LOG_NOTICE, "%s: detaching cleaner", lfs_sb_getfsmnt(cfsp[n]));
	free(cfsp[n]);
	if (n != nfss - 1)
		cfsp[n] = cfsp[nfss - 1];
	--nfss;
}

/*
 * Reinitialize a filesystem if, e.g., its size changed.
 */
int
reinit_fs(struct clfs *fs)
{
	char fsname[FSMNT_SIZE];

	memcpy(fsname, lfs_sb_getfsmnt(fs), sizeof(fsname));
	fsname[sizeof(fsname) - 1] = '\0';

	kops.ko_close(fs->clfs_ifilefd);
	kops.ko_close(fs->clfs_devfd);
	fd_reclaim(fs->clfs_devvp);
	fd_reclaim(fs->lfs_ivnode);
	free(fs->clfs_dev);
	free(fs->clfs_segtab);
	free(fs->clfs_segtabp);

	return init_fs(fs, fsname);
}

#ifdef REPAIR_ZERO_FINFO
/*
 * Use fsck's lfs routines to load the Ifile from an unmounted fs.
 * We interpret "fsname" as the name of the raw disk device.
 */
int
init_unmounted_fs(struct clfs *fs, char *fsname)
{
	struct lfs *disc_fs;
	int i;
	
	fs->clfs_dev = fsname;
	if ((fs->clfs_devfd = kops.ko_open(fs->clfs_dev, O_RDWR)) < 0) {
		syslog(LOG_ERR, "couldn't open device %s read/write",
		       fs->clfs_dev);
		return -1;
	}

	disc_fs = lfs_init(fs->clfs_devfd, 0, 0, 0, 0);

	fs->lfs_dlfs = disc_fs->lfs_dlfs; /* Structure copy */
	strncpy(fs->lfs_fsmnt, fsname, MNAMELEN);
	fs->lfs_ivnode = (struct uvnode *)disc_fs->lfs_ivnode;
	fs->clfs_devvp = fd_vget(fs->clfs_devfd, fs->lfs_fsize, fs->lfs_ssize,
				 atatime);

	/* Allocate and clear segtab */
	fs->clfs_segtab = (struct clfs_seguse *)malloc(lfs_sb_getnseg(fs) *
						sizeof(*fs->clfs_segtab));
	fs->clfs_segtabp = (struct clfs_seguse **)malloc(lfs_sb_getnseg(fs) *
						sizeof(*fs->clfs_segtabp));
	for (i = 0; i < lfs_sb_getnseg(fs); i++) {
		fs->clfs_segtabp[i] = &(fs->clfs_segtab[i]);
		fs->clfs_segtab[i].flags = 0x0;
	}
	syslog(LOG_NOTICE, "%s: unmounted cleaner starting", fsname);

	return 0;
}
#endif

/*
 * Set up the file descriptors, including the Ifile descriptor.
 * If we can't get the Ifile, this is not an LFS (or the kernel is
 * too old to support the fcntl).
 * XXX Merge this and init_unmounted_fs, switching on whether
 * XXX "fsname" is a dir or a char special device.  Should
 * XXX also be able to read unmounted devices out of fstab, the way
 * XXX fsck does.
 */
int
init_fs(struct clfs *fs, char *fsname)
{
	char mnttmp[FSMNT_SIZE];
	struct statvfs sf;
	int rootfd;
	int i;
	void *sbuf;
	char *bn;

	/*
	 * Get the raw device from the block device.
	 * XXX this is ugly.  Is there a way to discover the raw device
	 * XXX for a given mount point?
	 */
	if (kops.ko_statvfs(fsname, &sf, ST_WAIT) < 0)
		return -1;
	fs->clfs_dev = malloc(strlen(sf.f_mntfromname) + 2);
	if (fs->clfs_dev == NULL) {
		syslog(LOG_ERR, "couldn't malloc device name string: %m");
		return -1;
	}
	bn = strrchr(sf.f_mntfromname, '/');
	bn = bn ? bn+1 : sf.f_mntfromname;
	strlcpy(fs->clfs_dev, sf.f_mntfromname, bn - sf.f_mntfromname + 1);
	strcat(fs->clfs_dev, "r");
	strcat(fs->clfs_dev, bn);
	if ((fs->clfs_devfd = kops.ko_open(fs->clfs_dev, O_RDONLY, 0)) < 0) {
		syslog(LOG_ERR, "couldn't open device %s for reading",
			fs->clfs_dev);
		return -1;
	}

	/* Find the Ifile and open it */
	if ((rootfd = kops.ko_open(fsname, O_RDONLY, 0)) < 0)
		return -2;
	if (kops.ko_fcntl(rootfd, LFCNIFILEFH, &fs->clfs_ifilefh) < 0)
		return -3;
	if ((fs->clfs_ifilefd = kops.ko_fhopen(&fs->clfs_ifilefh,
	    sizeof(fs->clfs_ifilefh), O_RDONLY)) < 0)
		return -4;
	kops.ko_close(rootfd);

	sbuf = malloc(LFS_SBPAD);
	if (sbuf == NULL) {
		syslog(LOG_ERR, "couldn't malloc superblock buffer");
		return -1;
	}

	/* Load in the superblock */
	if (kops.ko_pread(fs->clfs_devfd, sbuf, LFS_SBPAD, LFS_LABELPAD) < 0) {
		free(sbuf);
		return -1;
	}

	__CTASSERT(sizeof(struct dlfs) == sizeof(struct dlfs64));
	memcpy(&fs->lfs_dlfs_u, sbuf, sizeof(struct dlfs));
	free(sbuf);

	/* If it is not LFS, complain and exit! */
	if (fs->lfs_dlfs_u.u_32.dlfs_magic != LFS_MAGIC) {
		syslog(LOG_ERR, "%s: not LFS", fsname);
		return -1;
	}
	fs->lfs_is64 = 0; /* XXX notyet */
	fs->lfs_dobyteswap = 0; /* XXX notyet */
	/* XXX: can this ever need to be set? does the cleaner even care? */
	fs->lfs_hasolddirfmt = 0;

	/* If this is not a version 2 filesystem, complain and exit */
	if (lfs_sb_getversion(fs) != 2) {
		syslog(LOG_ERR, "%s: not a version 2 LFS", fsname);
		return -1;
	}

	/* Assume fsname is the mounted name */
	strncpy(mnttmp, fsname, sizeof(mnttmp));
	mnttmp[sizeof(mnttmp) - 1] = '\0';
	lfs_sb_setfsmnt(fs, mnttmp);

	/* Set up vnodes for Ifile and raw device */
	fs->lfs_ivnode = fd_vget(fs->clfs_ifilefd, lfs_sb_getbsize(fs), 0, 0);
	fs->clfs_devvp = fd_vget(fs->clfs_devfd, lfs_sb_getfsize(fs), lfs_sb_getssize(fs),
				 atatime);

	/* Allocate and clear segtab */
	fs->clfs_segtab = (struct clfs_seguse *)malloc(lfs_sb_getnseg(fs) *
						sizeof(*fs->clfs_segtab));
	fs->clfs_segtabp = (struct clfs_seguse **)malloc(lfs_sb_getnseg(fs) *
						sizeof(*fs->clfs_segtabp));
	if (fs->clfs_segtab == NULL || fs->clfs_segtabp == NULL) {
		syslog(LOG_ERR, "%s: couldn't malloc segment table: %m",
			fs->clfs_dev);
		return -1;
	}

	for (i = 0; i < lfs_sb_getnseg(fs); i++) {
		fs->clfs_segtabp[i] = &(fs->clfs_segtab[i]);
		fs->clfs_segtab[i].flags = 0x0;
	}

	syslog(LOG_NOTICE, "%s: attaching cleaner", fsname);
	return 0;
}

/*
 * Invalidate all the currently held Ifile blocks so they will be
 * reread when we clean.  Check the size while we're at it, and
 * resize the buffer cache if necessary.
 */
void
reload_ifile(struct clfs *fs)
{
	struct ubuf *bp;
	struct stat st;
	int ohashmax;
	extern int hashmax;

	while ((bp = LIST_FIRST(&fs->lfs_ivnode->v_dirtyblkhd)) != NULL) {
		bremfree(bp);
		buf_destroy(bp);
	}
	while ((bp = LIST_FIRST(&fs->lfs_ivnode->v_cleanblkhd)) != NULL) {
		bremfree(bp);
		buf_destroy(bp);
	}

	/* If Ifile is larger than buffer cache, rehash */
	fstat(fs->clfs_ifilefd, &st);
	if (st.st_size / lfs_sb_getbsize(fs) > hashmax) {
		ohashmax = hashmax;
		bufrehash(st.st_size / lfs_sb_getbsize(fs));
		dlog("%s: resized buffer hash from %d to %d",
		     lfs_sb_getfsmnt(fs), ohashmax, hashmax);
	}
}

/*
 * Get IFILE entry for the given inode, store in ifpp.	The buffer
 * which contains that data is returned in bpp, and must be brelse()d
 * by the caller.
 *
 * XXX this is cutpaste of LFS_IENTRY from lfs.h; unify the two.
 */
void
lfs_ientry(IFILE **ifpp, struct clfs *fs, ino_t ino, struct ubuf **bpp)
{
	IFILE64 *ifp64;
	IFILE32 *ifp32;
	IFILE_V1 *ifp_v1;
	int error;

	error = bread(fs->lfs_ivnode,
		      ino / lfs_sb_getifpb(fs) + lfs_sb_getcleansz(fs) +
		      lfs_sb_getsegtabsz(fs), lfs_sb_getbsize(fs), 0, bpp);
	if (error)
		syslog(LOG_ERR, "%s: ientry failed for ino %d",
			lfs_sb_getfsmnt(fs), (int)ino);
	if (fs->lfs_is64) {
		ifp64 = (IFILE64 *)(*bpp)->b_data;
		ifp64 += ino % lfs_sb_getifpb(fs);
		*ifpp = (IFILE *)ifp64;
	} else if (lfs_sb_getversion(fs) > 1) {
		ifp32 = (IFILE32 *)(*bpp)->b_data;
		ifp32 += ino % lfs_sb_getifpb(fs);
		*ifpp = (IFILE *)ifp32;
	} else {
		ifp_v1 = (IFILE_V1 *)(*bpp)->b_data;
		ifp_v1 += ino % lfs_sb_getifpb(fs);
		*ifpp = (IFILE *)ifp_v1;
	}
	return;
}

#ifdef TEST_PATTERN
/*
 * Check ULFS_ROOTINO for file data.  The assumption is that we are running
 * the "twofiles" test with the rest of the filesystem empty.  Files
 * created by "twofiles" match the test pattern, but ULFS_ROOTINO and the
 * executable itself (assumed to be inode 3) should not match.
 */
static void
check_test_pattern(BLOCK_INFO *bip)
{
	int j;
	unsigned char *cp = bip->bi_bp;

	/* Check inode sanity */
	if (bip->bi_lbn == LFS_UNUSED_LBN) {
		assert(((struct ulfs1_dinode *)bip->bi_bp)->di_inumber ==
			bip->bi_inode);
	}

	/* These can have the test pattern and it's all good */
	if (bip->bi_inode > 3)
		return;

	for (j = 0; j < bip->bi_size; j++) {
		if (cp[j] != (j & 0xff))
			break;
	}
	assert(j < bip->bi_size);
}
#endif /* TEST_PATTERN */

/*
 * Parse the partial segment at daddr, adding its information to
 * bip.	 Return the address of the next partial segment to read.
 */
static daddr_t
parse_pseg(struct clfs *fs, daddr_t daddr, BLOCK_INFO **bipp, int *bic)
{
	SEGSUM *ssp;
	IFILE *ifp;
	BLOCK_INFO *bip, *nbip;
	daddr_t idaddr, odaddr;
	FINFO *fip;
	IINFO *iip;
	struct ubuf *ifbp;
	union lfs_dinode *dip;
	u_int32_t ck, vers;
	int fic, inoc, obic;
	size_t sumstart;
	int i;
	char *cp;

	odaddr = daddr;
	obic = *bic;
	bip = *bipp;

	/*
	 * Retrieve the segment header, set up the SEGSUM pointer
	 * as well as the first FINFO and inode address pointer.
	 */
	cp = fd_ptrget(fs->clfs_devvp, daddr);
	ssp = (SEGSUM *)cp;
	iip = SEGSUM_IINFOSTART(fs, cp);
	fip = SEGSUM_FINFOBASE(fs, cp);

	/*
	 * Check segment header magic and checksum
	 */
	if (lfs_ss_getmagic(fs, ssp) != SS_MAGIC) {
		syslog(LOG_WARNING, "%s: sumsum magic number bad at 0x%jx:"
		       " read 0x%x, expected 0x%x", lfs_sb_getfsmnt(fs),
		       (intmax_t)daddr, lfs_ss_getmagic(fs, ssp), SS_MAGIC);
		return 0x0;
	}
	sumstart = lfs_ss_getsumstart(fs);
	ck = cksum((char *)ssp + sumstart, lfs_sb_getsumsize(fs) - sumstart);
	if (ck != lfs_ss_getsumsum(fs, ssp)) {
		syslog(LOG_WARNING, "%s: sumsum checksum mismatch at 0x%jx:"
		       " read 0x%x, computed 0x%x", lfs_sb_getfsmnt(fs),
		       (intmax_t)daddr, lfs_ss_getsumsum(fs, ssp), ck);
		return 0x0;
	}

	/* Initialize data sum */
	ck = 0;

	/* Point daddr at next block after segment summary */
	++daddr;

	/*
	 * Loop over file info and inode pointers.  We always move daddr
	 * forward here because we are also computing the data checksum
	 * as we go.
	 */
	fic = inoc = 0;
	while (fic < lfs_ss_getnfinfo(fs, ssp) || inoc < lfs_ss_getninos(fs, ssp)) {
		/*
		 * We must have either a file block or an inode block.
		 * If we don't have either one, it's an error.
		 */
		if (fic >= lfs_ss_getnfinfo(fs, ssp) && lfs_ii_getblock(fs, iip) != daddr) {
			syslog(LOG_WARNING, "%s: bad pseg at %jx (seg %d)",
			       lfs_sb_getfsmnt(fs), (intmax_t)odaddr, lfs_dtosn(fs, odaddr));
			*bipp = bip;
			return 0x0;
		}

		/*
		 * Note each inode from the inode blocks
		 */
		if (inoc < lfs_ss_getninos(fs, ssp) && lfs_ii_getblock(fs, iip) == daddr) {
			cp = fd_ptrget(fs->clfs_devvp, daddr);
			ck = lfs_cksum_part(cp, sizeof(u_int32_t), ck);
			for (i = 0; i < lfs_sb_getinopb(fs); i++) {
				dip = DINO_IN_BLOCK(fs, cp, i);
				if (lfs_dino_getinumber(fs, dip) == 0)
					break;

				/*
				 * Check currency before adding it
				 */
#ifndef REPAIR_ZERO_FINFO
				lfs_ientry(&ifp, fs, lfs_dino_getinumber(fs, dip), &ifbp);
				idaddr = lfs_if_getdaddr(fs, ifp);
				brelse(ifbp, 0);
				if (idaddr != daddr)
#endif
					continue;

				/*
				 * A current inode.  Add it.
				 */
				++*bic;
				nbip = (BLOCK_INFO *)realloc(bip, *bic *
							     sizeof(*bip));
				if (nbip)
					bip = nbip;
				else {
					--*bic;
					*bipp = bip;
					return 0x0;
				}
				bip[*bic - 1].bi_inode = lfs_dino_getinumber(fs, dip);
				bip[*bic - 1].bi_lbn = LFS_UNUSED_LBN;
				bip[*bic - 1].bi_daddr = daddr;
				bip[*bic - 1].bi_segcreate = lfs_ss_getcreate(fs, ssp);
				bip[*bic - 1].bi_version = lfs_dino_getgen(fs, dip);
				bip[*bic - 1].bi_bp = dip;
				bip[*bic - 1].bi_size = DINOSIZE(fs);
			}
			inoc += i;
			daddr += lfs_btofsb(fs, lfs_sb_getibsize(fs));
			iip = NEXTLOWER_IINFO(fs, iip);
			continue;
		}

		/*
		 * Note each file block from the finfo blocks
		 */
		if (fic >= lfs_ss_getnfinfo(fs, ssp))
			continue;

		/* Count this finfo, whether or not we use it */
		++fic;

		/*
		 * If this finfo has nblocks==0, it was written wrong.
		 * Kernels with this problem always wrote this zero-sized
		 * finfo last, so just ignore it.
		 */
		if (lfs_fi_getnblocks(fs, fip) == 0) {
#ifdef REPAIR_ZERO_FINFO
			struct ubuf *nbp;
			SEGSUM *nssp;

			syslog(LOG_WARNING, "fixing short FINFO at %jx (seg %d)",
			       (intmax_t)odaddr, lfs_dtosn(fs, odaddr));
			bread(fs->clfs_devvp, odaddr, lfs_sb_getfsize(fs),
			    0, &nbp);
			nssp = (SEGSUM *)nbp->b_data;
			--nssp->ss_nfinfo;
			nssp->ss_sumsum = cksum(&nssp->ss_datasum,
				lfs_sb_getsumsize(fs) - sizeof(nssp->ss_sumsum));
			bwrite(nbp);
#endif
			syslog(LOG_WARNING, "zero-length FINFO at %jx (seg %d)",
			       (intmax_t)odaddr, lfs_dtosn(fs, odaddr));
			continue;
		}

		/*
		 * Check currency before adding blocks
		 */
#ifdef REPAIR_ZERO_FINFO
		vers = -1;
#else
		lfs_ientry(&ifp, fs, lfs_fi_getino(fs, fip), &ifbp);
		vers = lfs_if_getversion(fs, ifp);
		brelse(ifbp, 0);
#endif
		if (vers != lfs_fi_getversion(fs, fip)) {
			size_t size;

			/* Read all the blocks from the data summary */
			for (i = 0; i < lfs_fi_getnblocks(fs, fip); i++) {
				size = (i == lfs_fi_getnblocks(fs, fip) - 1) ?
					lfs_fi_getlastlength(fs, fip) : lfs_sb_getbsize(fs);
				cp = fd_ptrget(fs->clfs_devvp, daddr);
				ck = lfs_cksum_part(cp, sizeof(u_int32_t), ck);
				daddr += lfs_btofsb(fs, size);
			}
			fip = NEXT_FINFO(fs, fip);
			continue;
		}

		/* Add all the blocks from the finfos (current or not) */
		nbip = (BLOCK_INFO *)realloc(bip, (*bic + lfs_fi_getnblocks(fs, fip)) *
					     sizeof(*bip));
		if (nbip)
			bip = nbip;
		else {
			*bipp = bip;
			return 0x0;
		}

		for (i = 0; i < lfs_fi_getnblocks(fs, fip); i++) {
			bip[*bic + i].bi_inode = lfs_fi_getino(fs, fip);
			bip[*bic + i].bi_lbn = lfs_fi_getblock(fs, fip, i);
			bip[*bic + i].bi_daddr = daddr;
			bip[*bic + i].bi_segcreate = lfs_ss_getcreate(fs, ssp);
			bip[*bic + i].bi_version = lfs_fi_getversion(fs, fip);
			bip[*bic + i].bi_size = (i == lfs_fi_getnblocks(fs, fip) - 1) ?
				lfs_fi_getlastlength(fs, fip) : lfs_sb_getbsize(fs);
			cp = fd_ptrget(fs->clfs_devvp, daddr);
			ck = lfs_cksum_part(cp, sizeof(u_int32_t), ck);
			bip[*bic + i].bi_bp = cp;
			daddr += lfs_btofsb(fs, bip[*bic + i].bi_size);

#ifdef TEST_PATTERN
			check_test_pattern(bip + *bic + i); /* XXXDEBUG */
#endif
		}
		*bic += lfs_fi_getnblocks(fs, fip);
		fip = NEXT_FINFO(fs, fip);
	}

#ifndef REPAIR_ZERO_FINFO
	if (lfs_ss_getdatasum(fs, ssp) != ck) {
		syslog(LOG_WARNING, "%s: data checksum bad at 0x%jx:"
		       " read 0x%x, computed 0x%x", lfs_sb_getfsmnt(fs),
		       (intmax_t)odaddr,
		       lfs_ss_getdatasum(fs, ssp), ck);
		*bic = obic;
		return 0x0;
	}
#endif

	*bipp = bip;
	return daddr;
}

static void
log_segment_read(struct clfs *fs, int sn)
{
        FILE *fp;
	char *cp;
	
        /*
         * Write the segment read, and its contents, into a log file in
         * the current directory.  We don't need to log the location of
         * the segment, since that can be inferred from the segments up
	 * to this point (ss_nextseg field of the previously written segment).
	 *
	 * We can use this info later to reconstruct the filesystem at any
	 * given point in time for analysis, by replaying the log forward
	 * indexed by the segment serial numbers; but it is not suitable
	 * for everyday use since the copylog will be simply enormous.
         */
	cp = fd_ptrget(fs->clfs_devvp, lfs_sntod(fs, sn));

        fp = fopen(copylog_filename, "ab");
        if (fp != NULL) {
                if (fwrite(cp, (size_t)lfs_sb_getssize(fs), 1, fp) != 1) {
                        perror("writing segment to copy log");
                }
        }
        fclose(fp);
}

/*
 * Read a segment to populate the BLOCK_INFO structures.
 * Return the number of partial segments read and parsed.
 */
int
load_segment(struct clfs *fs, int sn, BLOCK_INFO **bipp, int *bic)
{
	daddr_t daddr;
	int i, npseg;

	daddr = lfs_sntod(fs, sn);
	if (daddr < lfs_btofsb(fs, LFS_LABELPAD))
		daddr = lfs_btofsb(fs, LFS_LABELPAD);
	for (i = 0; i < LFS_MAXNUMSB; i++) {
		if (lfs_sb_getsboff(fs, i) == daddr) {
			daddr += lfs_btofsb(fs, LFS_SBPAD);
			break;
		}
	}

	/* Preload the segment buffer */
	if (fd_preload(fs->clfs_devvp, lfs_sntod(fs, sn)) < 0)
		return -1;

	if (copylog_filename)
		log_segment_read(fs, sn);

	/* Note bytes read for stats */
	cleaner_stats.segs_cleaned++;
	cleaner_stats.bytes_read += lfs_sb_getssize(fs);
	++fs->clfs_nactive;

	npseg = 0;
	while(lfs_dtosn(fs, daddr) == sn &&
	      lfs_dtosn(fs, daddr + lfs_btofsb(fs, lfs_sb_getbsize(fs))) == sn) {
		daddr = parse_pseg(fs, daddr, bipp, bic);
		if (daddr == 0x0) {
			++cleaner_stats.segs_error;
			break;
		}
		++npseg;
	}

	return npseg;
}

void
calc_cb(struct clfs *fs, int sn, struct clfs_seguse *t)
{
	time_t now;
	int64_t age, benefit, cost;

	time(&now);
	age = (now < t->lastmod ? 0 : now - t->lastmod);

	/* Under no circumstances clean active or already-clean segments */
	if ((t->flags & SEGUSE_ACTIVE) || !(t->flags & SEGUSE_DIRTY)) {
		t->priority = 0;
		return;
	}

	/*
	 * If the segment is empty, there is no reason to clean it.
	 * Clear its error condition, if any, since we are never going to
	 * try to parse this one.
	 */
	if (t->nbytes == 0) {
		t->flags &= ~SEGUSE_ERROR; /* Strip error once empty */
		t->priority = 0;
		return;
	}

	if (t->flags & SEGUSE_ERROR) {	/* No good if not already empty */
		/* No benefit */
		t->priority = 0;
		return;
	}

	if (t->nbytes > lfs_sb_getssize(fs)) {
		/* Another type of error */
		syslog(LOG_WARNING, "segment %d: bad seguse count %d",
		       sn, t->nbytes);
		t->flags |= SEGUSE_ERROR;
		t->priority = 0;
		return;
	}

	/*
	 * The non-degenerate case.  Use Rosenblum's cost-benefit algorithm.
	 * Calculate the benefit from cleaning this segment (one segment,
	 * minus fragmentation, dirty blocks and a segment summary block)
	 * and weigh that against the cost (bytes read plus bytes written).
	 * We count the summary headers as "dirty" to avoid cleaning very
	 * old and very full segments.
	 */
	benefit = (int64_t)lfs_sb_getssize(fs) - t->nbytes -
		  (t->nsums + 1) * lfs_sb_getfsize(fs);
	if (lfs_sb_getbsize(fs) > lfs_sb_getfsize(fs)) /* fragmentation */
		benefit -= (lfs_sb_getbsize(fs) / 2);
	if (benefit <= 0) {
		t->priority = 0;
		return;
	}

	cost = lfs_sb_getssize(fs) + t->nbytes;
	t->priority = (256 * benefit * age) / cost;

	return;
}

/*
 * Comparator for BLOCK_INFO structures.  Anything not in one of the segments
 * we're looking at sorts higher; after that we sort first by inode number
 * and then by block number (unsigned, i.e., negative sorts higher) *but*
 * sort inodes before data blocks.
 */
static int
bi_comparator(const void *va, const void *vb)
{
	const BLOCK_INFO *a, *b;

	a = (const BLOCK_INFO *)va;
	b = (const BLOCK_INFO *)vb;

	/* Check for out-of-place block */
	if (a->bi_segcreate == a->bi_daddr &&
	    b->bi_segcreate != b->bi_daddr)
		return -1;
	if (a->bi_segcreate != a->bi_daddr &&
	    b->bi_segcreate == b->bi_daddr)
		return 1;
	if (a->bi_size <= 0 && b->bi_size > 0)
		return 1;
	if (b->bi_size <= 0 && a->bi_size > 0)
		return -1;

	/* Check inode number */
	if (a->bi_inode != b->bi_inode)
		return a->bi_inode - b->bi_inode;

	/* Check lbn */
	if (a->bi_lbn == LFS_UNUSED_LBN) /* Inodes sort lower than blocks */
		return -1;
	if (b->bi_lbn == LFS_UNUSED_LBN)
		return 1;
	if ((u_int64_t)a->bi_lbn > (u_int64_t)b->bi_lbn)
		return 1;
	else
		return -1;

	return 0;
}

/*
 * Comparator for sort_segments: cost-benefit equation.
 */
static int
cb_comparator(const void *va, const void *vb)
{
	const struct clfs_seguse *a, *b;

	a = *(const struct clfs_seguse * const *)va;
	b = *(const struct clfs_seguse * const *)vb;
	return a->priority > b->priority ? -1 : 1;
}

void
toss_old_blocks(struct clfs *fs, BLOCK_INFO **bipp, blkcnt_t *bic, int *sizep)
{
	blkcnt_t i;
	int r;
	BLOCK_INFO *bip = *bipp;
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;

	if (bic == 0 || bip == NULL)
		return;

	/*
	 * Kludge: Store the disk address in segcreate so we know which
	 * ones to toss.
	 */
	for (i = 0; i < *bic; i++)
		bip[i].bi_segcreate = bip[i].bi_daddr;

	/*
	 * XXX: blkcnt_t is 64 bits, so *bic might overflow size_t
	 * (the argument type of heapsort's number argument) on a
	 * 32-bit platform. However, if so we won't have got this far
	 * because we'll have failed trying to allocate the array. So
	 * while *bic here might cause a 64->32 truncation, it's safe.
	 */
	/* Sort the blocks */
	heapsort(bip, *bic, sizeof(BLOCK_INFO), bi_comparator);

	/* Use bmapv to locate the blocks */
	lim.blkiov = bip;
	lim.blkcnt = *bic;
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNBMAPV, &lim)) < 0) {
		syslog(LOG_WARNING, "%s: bmapv returned %d (%m)",
		       lfs_sb_getfsmnt(fs), r);
		return;
	}

	/* Toss blocks not in this segment */
	heapsort(bip, *bic, sizeof(BLOCK_INFO), bi_comparator);

	/* Get rid of stale blocks */
	if (sizep)
		*sizep = 0;
	for (i = 0; i < *bic; i++) {
		if (bip[i].bi_segcreate != bip[i].bi_daddr)
			break;
		if (sizep)
			*sizep += bip[i].bi_size;
	}
	*bic = i; /* XXX should we shrink bip? */
	*bipp = bip;

	return;
}

/*
 * Clean a segment and mark it invalid.
 */
int
invalidate_segment(struct clfs *fs, int sn)
{
	BLOCK_INFO *bip;
	int i, r, bic;
	blkcnt_t widebic;
	off_t nb;
	double util;
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;

	dlog("%s: inval seg %d", lfs_sb_getfsmnt(fs), sn);

	bip = NULL;
	bic = 0;
	fs->clfs_nactive = 0;
	if (load_segment(fs, sn, &bip, &bic) <= 0)
		return -1;
	widebic = bic;
	toss_old_blocks(fs, &bip, &widebic, NULL);
	bic = widebic;

	/* Record statistics */
	for (i = nb = 0; i < bic; i++)
		nb += bip[i].bi_size;
	util = ((double)nb) / (fs->clfs_nactive * lfs_sb_getssize(fs));
	cleaner_stats.util_tot += util;
	cleaner_stats.util_sos += util * util;
	cleaner_stats.bytes_written += nb;

	/*
	 * Use markv to move the blocks.
	 */
	lim.blkiov = bip;
	lim.blkcnt = bic;
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNMARKV, &lim)) < 0) {
		syslog(LOG_WARNING, "%s: markv returned %d (%m) "
		       "for seg %d", lfs_sb_getfsmnt(fs), r, sn);
		return r;
	}

	/*
	 * Finally call invalidate to invalidate the segment.
	 */
	if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNINVAL, &sn)) < 0) {
		syslog(LOG_WARNING, "%s: inval returned %d (%m) "
		       "for seg %d", lfs_sb_getfsmnt(fs), r, sn);
		return r;
	}

	return 0;
}

/*
 * Check to see if the given ino/lbn pair is represented in the BLOCK_INFO
 * array we are sending to the kernel, or if the kernel will have to add it.
 * The kernel will only add each such pair once, though, so keep track of
 * previous requests in a separate "extra" BLOCK_INFO array.  Returns 1
 * if the block needs to be added, 0 if it is already represented.
 */
static int
check_or_add(ino_t ino, daddr_t lbn, BLOCK_INFO *bip, int bic, BLOCK_INFO **ebipp, int *ebicp)
{
	BLOCK_INFO *t, *ebip = *ebipp;
	int ebic = *ebicp;
	int k;

	for (k = 0; k < bic; k++) {
		if (bip[k].bi_inode != ino)
			break;
		if (bip[k].bi_lbn == lbn) {
			return 0;
		}
	}

	/* Look on the list of extra blocks, too */
	for (k = 0; k < ebic; k++) {
		if (ebip[k].bi_inode == ino && ebip[k].bi_lbn == lbn) {
			return 0;
		}
	}

	++ebic;
	t = realloc(ebip, ebic * sizeof(BLOCK_INFO));
	if (t == NULL)
		return 1; /* Note *ebicp is unchanged */

	ebip = t;
	ebip[ebic - 1].bi_inode = ino;
	ebip[ebic - 1].bi_lbn = lbn;

	*ebipp = ebip;
	*ebicp = ebic;
	return 1;
}

/*
 * Look for indirect blocks we will have to write which are not
 * contained in this collection of blocks.  This constitutes
 * a hidden cleaning cost, since we are unaware of it until we
 * have already read the segments.  Return the total cost, and fill
 * in *ifc with the part of that cost due to rewriting the Ifile.
 */
static off_t
check_hidden_cost(struct clfs *fs, BLOCK_INFO *bip, int bic, off_t *ifc)
{
	int start;
	struct indir in[ULFS_NIADDR + 1];
	int num;
	int i, j, ebic;
	BLOCK_INFO *ebip;
	daddr_t lbn;

	start = 0;
	ebip = NULL;
	ebic = 0;
	for (i = 0; i < bic; i++) {
		if (i == 0 || bip[i].bi_inode != bip[start].bi_inode) {
			start = i;
			/*
			 * Look for IFILE blocks, unless this is the Ifile.
			 */
			if (bip[i].bi_inode != LFS_IFILE_INUM) {
				lbn = lfs_sb_getcleansz(fs) + bip[i].bi_inode /
							lfs_sb_getifpb(fs);
				*ifc += check_or_add(LFS_IFILE_INUM, lbn,
						     bip, bic, &ebip, &ebic);
			}
		}
		if (bip[i].bi_lbn == LFS_UNUSED_LBN)
			continue;
		if (bip[i].bi_lbn < ULFS_NDADDR)
			continue;

		/* XXX the struct lfs cast is completely wrong/unsafe */
		ulfs_getlbns((struct lfs *)fs, NULL, (daddr_t)bip[i].bi_lbn, in, &num);
		for (j = 0; j < num; j++) {
			check_or_add(bip[i].bi_inode, in[j].in_lbn,
				     bip + start, bic - start, &ebip, &ebic);
		}
	}
	return ebic;
}

/*
 * Select segments to clean, add blocks from these segments to a cleaning
 * list, and send this list through lfs_markv() to move them to new
 * locations on disk.
 */
static int
clean_fs(struct clfs *fs, const CLEANERINFO64 *cip)
{
	int i, j, ngood, sn, bic, r, npos;
	blkcnt_t widebic;
	int bytes, totbytes;
	struct ubuf *bp;
	SEGUSE *sup;
	static BLOCK_INFO *bip;
	struct lfs_fcntl_markv /* {
		BLOCK_INFO *blkiov;
		int blkcnt;
	} */ lim;
	int mc;
	BLOCK_INFO *mbip;
	int inc;
	off_t nb;
	off_t goal;
	off_t extra, if_extra;
	double util;

	/* Read the segment table into our private structure */
	npos = 0;
	for (i = 0; i < lfs_sb_getnseg(fs); i+= lfs_sb_getsepb(fs)) {
		bread(fs->lfs_ivnode,
		      lfs_sb_getcleansz(fs) + i / lfs_sb_getsepb(fs),
		      lfs_sb_getbsize(fs), 0, &bp);
		for (j = 0; j < lfs_sb_getsepb(fs) && i + j < lfs_sb_getnseg(fs); j++) {
			sup = ((SEGUSE *)bp->b_data) + j;
			fs->clfs_segtab[i + j].nbytes  = sup->su_nbytes;
			fs->clfs_segtab[i + j].nsums = sup->su_nsums;
			fs->clfs_segtab[i + j].lastmod = sup->su_lastmod;
			/* Keep error status but renew other flags */
			fs->clfs_segtab[i + j].flags  &= SEGUSE_ERROR;
			fs->clfs_segtab[i + j].flags  |= sup->su_flags;

			/* Compute cost-benefit coefficient */
			calc_cb(fs, i + j, fs->clfs_segtab + i + j);
			if (fs->clfs_segtab[i + j].priority > 0)
				++npos;
		}
		brelse(bp, 0);
	}

	/* Sort segments based on cleanliness, fulness, and condition */
	heapsort(fs->clfs_segtabp, lfs_sb_getnseg(fs), sizeof(struct clfs_seguse *),
		 cb_comparator);

	/* If no segment is cleanable, just return */
	if (fs->clfs_segtabp[0]->priority == 0) {
		dlog("%s: no segment cleanable", lfs_sb_getfsmnt(fs));
		return 0;
	}

	/* Load some segments' blocks into bip */
	bic = 0;
	fs->clfs_nactive = 0;
	ngood = 0;
	if (use_bytes) {
		/* Set attainable goal */
		goal = lfs_sb_getssize(fs) * atatime;
		if (goal > (cip->clean - 1) * lfs_sb_getssize(fs) / 2)
			goal = MAX((cip->clean - 1) * lfs_sb_getssize(fs),
				   lfs_sb_getssize(fs)) / 2;

		dlog("%s: cleaning with goal %" PRId64
		     " bytes (%d segs clean, %d cleanable)",
		     lfs_sb_getfsmnt(fs), goal, cip->clean, npos);
		syslog(LOG_INFO, "%s: cleaning with goal %" PRId64
		       " bytes (%d segs clean, %d cleanable)",
		       lfs_sb_getfsmnt(fs), goal, cip->clean, npos);
		totbytes = 0;
		for (i = 0; i < lfs_sb_getnseg(fs) && totbytes < goal; i++) {
			if (fs->clfs_segtabp[i]->priority == 0)
				break;
			/* Upper bound on number of segments at once */
			if (ngood * lfs_sb_getssize(fs) > 4 * goal)
				break;
			sn = (fs->clfs_segtabp[i] - fs->clfs_segtab);
			dlog("%s: add seg %d prio %" PRIu64
			     " containing %ld bytes",
			     lfs_sb_getfsmnt(fs), sn, fs->clfs_segtabp[i]->priority,
			     fs->clfs_segtabp[i]->nbytes);
			if ((r = load_segment(fs, sn, &bip, &bic)) > 0) {
				++ngood;
				widebic = bic;
				toss_old_blocks(fs, &bip, &widebic, &bytes);
				bic = widebic;
				totbytes += bytes;
			} else if (r == 0)
				fd_release(fs->clfs_devvp);
			else
				break;
		}
	} else {
		/* Set attainable goal */
		goal = atatime;
		if (goal > cip->clean - 1)
			goal = MAX(cip->clean - 1, 1);

		dlog("%s: cleaning with goal %d segments (%d clean, %d cleanable)",
		       lfs_sb_getfsmnt(fs), (int)goal, cip->clean, npos);
		for (i = 0; i < lfs_sb_getnseg(fs) && ngood < goal; i++) {
			if (fs->clfs_segtabp[i]->priority == 0)
				break;
			sn = (fs->clfs_segtabp[i] - fs->clfs_segtab);
			dlog("%s: add seg %d prio %" PRIu64,
			     lfs_sb_getfsmnt(fs), sn, fs->clfs_segtabp[i]->priority);
			if ((r = load_segment(fs, sn, &bip, &bic)) > 0)
				++ngood;
			else if (r == 0)
				fd_release(fs->clfs_devvp);
			else
				break;
		}
		widebic = bic;
		toss_old_blocks(fs, &bip, &widebic, NULL);
		bic = widebic;
	}

	/* If there is nothing to do, try again later. */
	if (bic == 0) {
		dlog("%s: no blocks to clean in %d cleanable segments",
		       lfs_sb_getfsmnt(fs), (int)ngood);
		fd_release_all(fs->clfs_devvp);
		return 0;
	}

	/* Record statistics */
	for (i = nb = 0; i < bic; i++)
		nb += bip[i].bi_size;
	util = ((double)nb) / (fs->clfs_nactive * lfs_sb_getssize(fs));
	cleaner_stats.util_tot += util;
	cleaner_stats.util_sos += util * util;
	cleaner_stats.bytes_written += nb;

	/*
	 * Check out our blocks to see if there are hidden cleaning costs.
	 * If there are, we might be cleaning ourselves deeper into a hole
	 * rather than doing anything useful.
	 * XXX do something about this.
	 */
	if_extra = 0;
	extra = lfs_sb_getbsize(fs) * (off_t)check_hidden_cost(fs, bip, bic, &if_extra);
	if_extra *= lfs_sb_getbsize(fs);

	/*
	 * Use markv to move the blocks.
	 */
	if (do_small) 
		inc = MAXPHYS / lfs_sb_getbsize(fs) - 1;
	else
		inc = LFS_MARKV_MAXBLKCNT / 2;
	for (mc = 0, mbip = bip; mc < bic; mc += inc, mbip += inc) {
		lim.blkiov = mbip;
		lim.blkcnt = (bic - mc > inc ? inc : bic - mc);
#ifdef TEST_PATTERN
		dlog("checking blocks %d-%d", mc, mc + lim.blkcnt - 1);
		for (i = 0; i < lim.blkcnt; i++) {
			check_test_pattern(mbip + i);
		}
#endif /* TEST_PATTERN */
		dlog("sending blocks %d-%d", mc, mc + lim.blkcnt - 1);
		if ((r = kops.ko_fcntl(fs->clfs_ifilefd, LFCNMARKV, &lim))<0) {
			int oerrno = errno;
			syslog(LOG_WARNING, "%s: markv returned %d (errno %d, %m)",
			       lfs_sb_getfsmnt(fs), r, errno);
			if (oerrno != EAGAIN && oerrno != ESHUTDOWN) {
				syslog(LOG_DEBUG, "%s: errno %d, returning",
				       lfs_sb_getfsmnt(fs), oerrno);
				fd_release_all(fs->clfs_devvp);
				return r;
			}
			if (oerrno == ESHUTDOWN) {
				syslog(LOG_NOTICE, "%s: filesystem unmounted",
				       lfs_sb_getfsmnt(fs));
				fd_release_all(fs->clfs_devvp);
				return r;
			}
		}
	}

	/*
	 * Report progress (or lack thereof)
	 */
	syslog(LOG_INFO, "%s: wrote %" PRId64 " dirty + %"
	       PRId64 " supporting indirect + %"
	       PRId64 " supporting Ifile = %"
	       PRId64 " bytes to clean %d segs (%" PRId64 "%% recovery)",
	       lfs_sb_getfsmnt(fs), (int64_t)nb, (int64_t)(extra - if_extra),
	       (int64_t)if_extra, (int64_t)(nb + extra), ngood,
	       (ngood ? (int64_t)(100 - (100 * (nb + extra)) /
					 (ngood * lfs_sb_getssize(fs))) :
		(int64_t)0));
	if (nb + extra >= ngood * lfs_sb_getssize(fs))
		syslog(LOG_WARNING, "%s: cleaner not making forward progress",
		       lfs_sb_getfsmnt(fs));

	/*
	 * Finally call reclaim to prompt cleaning of the segments.
	 */
	kops.ko_fcntl(fs->clfs_ifilefd, LFCNRECLAIM, NULL);

	fd_release_all(fs->clfs_devvp);
	return 0;
}

/*
 * Read the cleanerinfo block and apply cleaning policy to determine whether
 * the given filesystem needs to be cleaned.  Returns 1 if it does, 0 if it
 * does not, or -1 on error.
 */
static int
needs_cleaning(struct clfs *fs, CLEANERINFO64 *cip)
{
	CLEANERINFO *cipu;
	struct ubuf *bp;
	struct stat st;
	daddr_t fsb_per_seg, max_free_segs;
	time_t now;
	double loadavg;

	/* If this fs is "on hold", don't clean it. */
	if (fs->clfs_onhold)
		return 0;

	/*
	 * Read the cleanerinfo block from the Ifile.  We don't want
	 * the cached information, so invalidate the buffer before
	 * handing it back.
	 */
	if (bread(fs->lfs_ivnode, 0, lfs_sb_getbsize(fs), 0, &bp)) {
		syslog(LOG_ERR, "%s: can't read inode", lfs_sb_getfsmnt(fs));
		return -1;
	}
	cipu = (CLEANERINFO *)bp->b_data;
	if (fs->lfs_is64) {
		/* Structure copy */
		*cip = cipu->u_64;
	} else {
		/* Copy the fields and promote to 64 bit */
		cip->clean = cipu->u_32.clean;
		cip->dirty = cipu->u_32.dirty;
		cip->bfree = cipu->u_32.bfree;
		cip->avail = cipu->u_32.avail;
		cip->free_head = cipu->u_32.free_head;
		cip->free_tail = cipu->u_32.free_tail;
		cip->flags = cipu->u_32.flags;
	}
	brelse(bp, B_INVAL);
	cleaner_stats.bytes_read += lfs_sb_getbsize(fs);

	/*
	 * If the number of segments changed under us, reinit.
	 * We don't have to start over from scratch, however,
	 * since we don't hold any buffers.
	 */
	if (lfs_sb_getnseg(fs) != cip->clean + cip->dirty) {
		if (reinit_fs(fs) < 0) {
			/* The normal case for unmount */
			syslog(LOG_NOTICE, "%s: filesystem unmounted", lfs_sb_getfsmnt(fs));
			return -1;
		}
		syslog(LOG_NOTICE, "%s: nsegs changed", lfs_sb_getfsmnt(fs));
	}

	/* Compute theoretical "free segments" maximum based on usage */
	fsb_per_seg = lfs_segtod(fs, 1);
	max_free_segs = MAX(cip->bfree, 0) / fsb_per_seg + lfs_sb_getminfreeseg(fs);

	dlog("%s: bfree = %d, avail = %d, clean = %d/%d",
	     lfs_sb_getfsmnt(fs), cip->bfree, cip->avail, cip->clean,
	     lfs_sb_getnseg(fs));

	/* If the writer is waiting on us, clean it */
	if (cip->clean <= lfs_sb_getminfreeseg(fs) ||
	    (cip->flags & LFS_CLEANER_MUST_CLEAN))
		return 1;

	/* If there are enough segments, don't clean it */
	if (cip->bfree - cip->avail <= fsb_per_seg &&
	    cip->avail > fsb_per_seg)
		return 0;

	/* If we are in dire straits, clean it */
	if (cip->bfree - cip->avail > fsb_per_seg &&
	    cip->avail <= fsb_per_seg)
		return 1;

	/* If under busy threshold, clean regardless of load */
	if (cip->clean < max_free_segs * BUSY_LIM)
		return 1;

	/* Check busy status; clean if idle and under idle limit */
	if (use_fs_idle) {
		/* Filesystem idle */
		time(&now);
		if (fstat(fs->clfs_ifilefd, &st) < 0) {
			syslog(LOG_ERR, "%s: failed to stat ifile",
			       lfs_sb_getfsmnt(fs));
			return -1;
		}
		if (now - st.st_mtime > segwait_timeout &&
		    cip->clean < max_free_segs * IDLE_LIM)
			return 1;
	} else {
		/* CPU idle - use one-minute load avg */
		if (getloadavg(&loadavg, 1) == -1) {
			syslog(LOG_ERR, "%s: failed to get load avg",
			       lfs_sb_getfsmnt(fs));
			return -1;
		}
		if (loadavg < load_threshold &&
		    cip->clean < max_free_segs * IDLE_LIM)
			return 1;
	}

	return 0;
}

/*
 * Report statistics.  If the signal was SIGUSR2, clear the statistics too.
 * If the signal was SIGINT, exit.
 */
static void
sig_report(int sig)
{
	double avg = 0.0, stddev;

	avg = cleaner_stats.util_tot / MAX(cleaner_stats.segs_cleaned, 1.0);
	stddev = cleaner_stats.util_sos / MAX(cleaner_stats.segs_cleaned -
					      avg * avg, 1.0);
	syslog(LOG_INFO, "bytes read:	     %" PRId64, cleaner_stats.bytes_read);
	syslog(LOG_INFO, "bytes written:     %" PRId64, cleaner_stats.bytes_written);
	syslog(LOG_INFO, "segments cleaned:  %" PRId64, cleaner_stats.segs_cleaned);
#if 0
	/* "Empty segments" is meaningless, since the kernel handles those */
	syslog(LOG_INFO, "empty segments:    %" PRId64, cleaner_stats.segs_empty);
#endif
	syslog(LOG_INFO, "error segments:    %" PRId64, cleaner_stats.segs_error);
	syslog(LOG_INFO, "utilization total: %g", cleaner_stats.util_tot);
	syslog(LOG_INFO, "utilization sos:   %g", cleaner_stats.util_sos);
	syslog(LOG_INFO, "utilization avg:   %4.2f", avg);
	syslog(LOG_INFO, "utilization sdev:  %9.6f", stddev);

	if (debug)
		bufstats();

	if (sig == SIGUSR2)
		memset(&cleaner_stats, 0, sizeof(cleaner_stats));
	if (sig == SIGINT)
		exit(0);
}

static void
sig_exit(int sig)
{
	exit(0);
}

static void
usage(void)
{
	errx(1, "usage: lfs_cleanerd [-bcdfmqs] [-i segnum] [-l load] "
	     "[-n nsegs] [-r report_freq] [-t timeout] fs_name ...");
}

#ifndef LFS_CLEANER_AS_LIB
/*
 * Main.
 */
int
main(int argc, char **argv)
{

	return lfs_cleaner_main(argc, argv);
}
#endif

int
lfs_cleaner_main(int argc, char **argv)
{
	int i, opt, error, r, loopcount, nodetach;
	struct timeval tv;
#ifdef LFS_CLEANER_AS_LIB
	sem_t *semaddr = NULL;
#endif
	CLEANERINFO64 ci;
#ifndef USE_CLIENT_SERVER
	char *cp, *pidname;
#endif

#if defined(__GNUC__) && __GNUC__ >= 4 && __GNUC_MINOR__ == 8 && \
    defined(__OPTIMIZE_SIZE__)
	/*
	 * XXX: Work around apparent bug with gcc 4.8 and -Os: it
	 * claims that ci.clean is uninitialized in clean_fs (at one
	 * of the several uses of it, which is neither the first nor
	 * last use) -- this doesn't happen with plain -O2.
	 *
	 * Hopefully in the future further rearrangements will allow
	 * removing this hack.
	 */
	ci.clean = 0;
#endif

	/*
	 * Set up defaults
	 */
	atatime	 = 1;
	segwait_timeout = 300; /* Five minutes */
	load_threshold	= 0.2;
	stat_report	= 0;
	inval_segment	= -1;
	copylog_filename = NULL;
	nodetach        = 0;

	/*
	 * Parse command-line arguments
	 */
	while ((opt = getopt(argc, argv, "bC:cdDfi:l:mn:qr:sS:t:")) != -1) {
		switch (opt) {
		    case 'b':	/* Use bytes written, not segments read */
			    use_bytes = 1;
			    break;
		    case 'C':	/* copy log */
			    copylog_filename = optarg;
			    break;
		    case 'c':	/* Coalesce files */
			    do_coalesce++;
			    break;
		    case 'd':	/* Debug mode. */
			    nodetach++;
			    debug++;
			    break;
		    case 'D':	/* stay-on-foreground */
			    nodetach++;
			    break;
		    case 'f':	/* Use fs idle time rather than cpu idle */
			    use_fs_idle = 1;
			    break;
		    case 'i':	/* Invalidate this segment */
			    inval_segment = atoi(optarg);
			    break;
		    case 'l':	/* Load below which to clean */
			    load_threshold = atof(optarg);
			    break;
		    case 'm':	/* [compat only] */
			    break;
		    case 'n':	/* How many segs to clean at once */
			    atatime = atoi(optarg);
			    break;
		    case 'q':	/* Quit after one run */
			    do_quit = 1;
			    break;
		    case 'r':	/* Report every stat_report segments */
			    stat_report = atoi(optarg);
			    break;
		    case 's':	/* Small writes */
			    do_small = 1;
			    break;
#ifdef LFS_CLEANER_AS_LIB
		    case 'S':	/* semaphore */
			    semaddr = (void*)(uintptr_t)strtoull(optarg,NULL,0);
			    break;
#endif
		    case 't':	/* timeout */
			    segwait_timeout = atoi(optarg);
			    break;
		    default:
			    usage();
			    /* NOTREACHED */
		}
	}
	argc -= optind;
	argv += optind;

	if (argc < 1)
		usage();
	if (inval_segment >= 0 && argc != 1) {
		errx(1, "lfs_cleanerd: may only specify one filesystem when "
		     "using -i flag");
	}

	if (do_coalesce) {
		errx(1, "lfs_cleanerd: -c disabled due to reports of file "
		     "corruption; you may re-enable it by rebuilding the "
		     "cleaner");
	}

	/*
	 * Set up daemon mode or foreground mode
	 */
	if (nodetach) {
		openlog("lfs_cleanerd", LOG_NDELAY | LOG_PID | LOG_PERROR,
			LOG_DAEMON);
		signal(SIGINT, sig_report);
	} else {
		if (daemon(0, 0) == -1)
			err(1, "lfs_cleanerd: couldn't become a daemon!");
		openlog("lfs_cleanerd", LOG_NDELAY | LOG_PID, LOG_DAEMON);
		signal(SIGINT, sig_exit);
	}

	/*
	 * Look for an already-running master daemon.  If there is one,
	 * send it our filesystems to add to its list and exit.
	 * If there is none, become the master.
	 */
#ifdef USE_CLIENT_SERVER
	try_to_become_master(argc, argv);
#else
	/* XXX think about this */
	asprintf(&pidname, "lfs_cleanerd:m:%s", argv[0]);
	if (pidname == NULL) {
		syslog(LOG_ERR, "malloc failed: %m");
		exit(1);
	}
	for (cp = pidname; cp != NULL; cp = strchr(cp, '/'))
		*cp = '|';
	pidfile(pidname);
#endif

	/*
	 * Signals mean daemon should report its statistics
	 */
	memset(&cleaner_stats, 0, sizeof(cleaner_stats));
	signal(SIGUSR1, sig_report);
	signal(SIGUSR2, sig_report);

	/*
	 * Start up buffer cache.  We only use this for the Ifile,
	 * and we will resize it if necessary, so it can start small.
	 */
	bufinit(4);

#ifdef REPAIR_ZERO_FINFO
	{
		BLOCK_INFO *bip = NULL;
		int bic = 0;

		nfss = 1;
		fsp = (struct clfs **)malloc(sizeof(*fsp));
		fsp[0] = (struct clfs *)calloc(1, sizeof(**fsp));

		if (init_unmounted_fs(fsp[0], argv[0]) < 0) {
			err(1, "init_unmounted_fs");
		}
		dlog("Filesystem has %d segments", fsp[0]->lfs_nseg);
		for (i = 0; i < fsp[0]->lfs_nseg; i++) {
			load_segment(fsp[0], i, &bip, &bic);
			bic = 0;
		}
		exit(0);
	}
#endif

	/*
	 * Initialize cleaning structures, open devices, etc.
	 */
	nfss = argc;
	fsp = (struct clfs **)malloc(nfss * sizeof(*fsp));
	if (fsp == NULL) {
		syslog(LOG_ERR, "couldn't allocate fs table: %m");
		exit(1);
	}
	for (i = 0; i < nfss; i++) {
		fsp[i] = (struct clfs *)calloc(1, sizeof(**fsp));
		if ((r = init_fs(fsp[i], argv[i])) < 0) {
			syslog(LOG_ERR, "%s: couldn't init: error code %d",
			       argv[i], r);
			handle_error(fsp, i);
			--i; /* Do the new #i over again */
		}
	}

	/*
	 * If asked to coalesce, do so and exit.
	 */
	if (do_coalesce) {
		for (i = 0; i < nfss; i++)
			clean_all_inodes(fsp[i]);
		exit(0);
	}

	/*
	 * If asked to invalidate a segment, do that and exit.
	 */
	if (inval_segment >= 0) {
		invalidate_segment(fsp[0], inval_segment);
		exit(0);
	}

	/*
	 * Main cleaning loop.
	 */
	loopcount = 0;
#ifdef LFS_CLEANER_AS_LIB
	if (semaddr)
		sem_post(semaddr);
#endif
	error = 0;
	while (nfss > 0) {
		int cleaned_one;
		do {
#ifdef USE_CLIENT_SERVER
			check_control_socket();
#endif
			cleaned_one = 0;
			for (i = 0; i < nfss; i++) {
				if ((error = needs_cleaning(fsp[i], &ci)) < 0) {
					syslog(LOG_DEBUG, "%s: needs_cleaning returned %d",
					       getprogname(), error);
					handle_error(fsp, i);
					continue;
				}
				if (error == 0) /* No need to clean */
					continue;
				
				reload_ifile(fsp[i]);
				if ((error = clean_fs(fsp[i], &ci)) < 0) {
					syslog(LOG_DEBUG, "%s: clean_fs returned %d",
					       getprogname(), error);
					handle_error(fsp, i);
					continue;
				}
				++cleaned_one;
			}
			++loopcount;
			if (stat_report && loopcount % stat_report == 0)
				sig_report(0);
			if (do_quit)
				exit(0);
		} while(cleaned_one);
		tv.tv_sec = segwait_timeout;
		tv.tv_usec = 0;
		/* XXX: why couldn't others work if fsp socket is shutdown? */
		error = kops.ko_fcntl(fsp[0]->clfs_ifilefd,LFCNSEGWAITALL,&tv);
		if (error) {
			if (errno == ESHUTDOWN) {
				for (i = 0; i < nfss; i++) {
					syslog(LOG_INFO, "%s: shutdown",
					       getprogname());
					handle_error(fsp, i);
					assert(nfss == 0);
				}
			} else {
#ifdef LFS_CLEANER_AS_LIB
				error = ESHUTDOWN;
				break;
#else
				err(1, "LFCNSEGWAITALL");
#endif
			}
		}
	}

	/* NOTREACHED */
	return error;
}
