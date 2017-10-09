/*	$NetBSD: resize_ffs.c,v 1.53 2017/10/09 05:24:26 mlelstv Exp $	*/
/* From sources sent on February 17, 2003 */
/*-
 * As its sole author, I explicitly place this code in the public
 *  domain.  Anyone may use it for any purpose (though I would
 *  appreciate credit where it is due).
 *
 *					der Mouse
 *
 *			       mouse@rodents.montreal.qc.ca
 *		     7D C8 61 52 5D E7 2D 39  4E F1 31 3E E8 B3 27 4B
 */
/*
 * resize_ffs:
 *
 * Resize a file system.  Is capable of both growing and shrinking.
 *
 * Usage: resize_ffs [-s newsize] [-y] file_system
 *
 * Example: resize_ffs -s 29574 /dev/rsd1e
 *
 * newsize is in DEV_BSIZE units (ie, disk sectors, usually 512 bytes
 *  each).
 *
 * Note: this currently requires gcc to build, since it is written
 *  depending on gcc-specific features, notably nested function
 *  definitions (which in at least a few cases depend on the lexical
 *  scoping gcc provides, so they can't be trivially moved outside).
 *
 * Many thanks go to John Kohl <jtk@NetBSD.org> for finding bugs: the
 *  one responsible for the "realloccgblk: can't find blk in cyl"
 *  problem and a more minor one which left fs_dsize wrong when
 *  shrinking.  (These actually indicate bugs in fsck too - it should
 *  have caught and fixed them.)
 *
 */

#include <sys/cdefs.h>
__RCSID("$NetBSD: resize_ffs.c,v 1.53 2017/10/09 05:24:26 mlelstv Exp $");

#include <sys/disk.h>
#include <sys/disklabel.h>
#include <sys/dkio.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>		/* MAXFRAG */
#include <ufs/ffs/fs.h>
#include <ufs/ffs/ffs_extern.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>	/* ufs_rw32 */

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "progress.h"

/* new size of file system, in sectors */
static int64_t newsize;

/* fd open onto disk device or file */
static int fd;

/* disk device or file path */
char *special;

/* must we break up big I/O operations - see checksmallio() */
static int smallio;

/* size of a cg, in bytes, rounded up to a frag boundary */
static int cgblksz;

/* possible superblock localtions */
static int search[] = SBLOCKSEARCH;
/* location of the superblock */
static off_t where;

/* Superblocks. */
static struct fs *oldsb;	/* before we started */
static struct fs *newsb;	/* copy to work with */
/* Buffer to hold the above.  Make sure it's aligned correctly. */
static char sbbuf[2 * SBLOCKSIZE]
	__attribute__((__aligned__(__alignof__(struct fs))));

union dinode {
	struct ufs1_dinode dp1;
	struct ufs2_dinode dp2;
};
#define DIP(dp, field)							      \
	((is_ufs2) ?							      \
	    (dp)->dp2.field : (dp)->dp1.field)

#define DIP_ASSIGN(dp, field, value)					      \
	do {								      \
		if (is_ufs2)						      \
			(dp)->dp2.field = (value);			      \
		else							      \
			(dp)->dp1.field = (value);			      \
	} while (0)

/* a cg's worth of brand new squeaky-clean inodes */
static struct ufs1_dinode *zinodes1;
static struct ufs2_dinode *zinodes2;

/* pointers to the in-core cgs, read off disk and possibly modified */
static struct cg **cgs;

/* pointer to csum array - the stuff pointed to on-disk by fs_csaddr */
static struct csum *csums;

/* per-cg flags, indexed by cg number */
static unsigned char *cgflags;
#define CGF_DIRTY   0x01	/* needs to be written to disk */
#define CGF_BLKMAPS 0x02	/* block bitmaps need rebuilding */
#define CGF_INOMAPS 0x04	/* inode bitmaps need rebuilding */

/* when shrinking, these two arrays record how we want blocks to move.	 */
/*  if blkmove[i] is j, the frag that started out as frag #i should end	 */
/*  up as frag #j.  inomove[i]=j means, similarly, that the inode that	 */
/*  started out as inode i should end up as inode j.			 */
static unsigned int *blkmove;
static unsigned int *inomove;

/* in-core copies of all inodes in the fs, indexed by inumber */
union dinode *inodes;

void *ibuf;	/* ptr to fs block-sized buffer for reading/writing inodes */

/* byteswapped inodes */
union dinode *sinodes;

/* per-inode flags, indexed by inumber */
static unsigned char *iflags;
#define IF_DIRTY  0x01		/* needs to be written to disk */
#define IF_BDIRTY 0x02		/* like DIRTY, but is set on first inode in a
				 * block of inodes, and applies to the whole
				 * block. */

/* resize_ffs works directly on dinodes, adapt blksize() */
#define dblksize(fs, dip, lbn, filesize) \
	(((lbn) >= UFS_NDADDR || (uint64_t)(filesize) >= ffs_lblktosize(fs, (lbn) + 1)) \
	    ? (fs)->fs_bsize						       \
	    : (ffs_fragroundup(fs, ffs_blkoff(fs, (filesize)))))


/*
 * Number of disk sectors per block/fragment
 */
#define NSPB(fs)	(FFS_FSBTODB((fs),1) << (fs)->fs_fragshift)
#define NSPF(fs)	(FFS_FSBTODB((fs),1))

/* global flags */
int is_ufs2 = 0;
int needswap = 0;
int verbose = 0;
int progress = 0;

static void usage(void) __dead;

/*
 * See if we need to break up large I/O operations.  This should never
 *  be needed, but under at least one <version,platform> combination,
 *  large enough disk transfers to the raw device hang.  So if we're
 *  talking to a character special device, play it safe; in this case,
 *  readat() and writeat() break everything up into pieces no larger
 *  than 8K, doing multiple syscalls for larger operations.
 */
static void
checksmallio(void)
{
	struct stat stb;

	fstat(fd, &stb);
	smallio = ((stb.st_mode & S_IFMT) == S_IFCHR);
}

static int
isplainfile(void)
{
	struct stat stb;

	fstat(fd, &stb);
	return S_ISREG(stb.st_mode);
}
/*
 * Read size bytes starting at blkno into buf.  blkno is in DEV_BSIZE
 *  units, ie, after FFS_FSBTODB(); size is in bytes.
 */
static void
readat(off_t blkno, void *buf, int size)
{
	/* Seek to the correct place. */
	if (lseek(fd, blkno * DEV_BSIZE, L_SET) < 0)
		err(EXIT_FAILURE, "lseek failed");

	/* See if we have to break up the transfer... */
	if (smallio) {
		char *bp;	/* pointer into buf */
		int left;	/* bytes left to go */
		int n;		/* number to do this time around */
		int rv;		/* syscall return value */
		bp = buf;
		left = size;
		while (left > 0) {
			n = (left > 8192) ? 8192 : left;
			rv = read(fd, bp, n);
			if (rv < 0)
				err(EXIT_FAILURE, "read failed");
			if (rv != n)
				errx(EXIT_FAILURE,
				    "read: wanted %d, got %d", n, rv);
			bp += n;
			left -= n;
		}
	} else {
		int rv;
		rv = read(fd, buf, size);
		if (rv < 0)
			err(EXIT_FAILURE, "read failed");
		if (rv != size)
			errx(EXIT_FAILURE, "read: wanted %d, got %d",
			    size, rv);
	}
}
/*
 * Write size bytes from buf starting at blkno.  blkno is in DEV_BSIZE
 *  units, ie, after FFS_FSBTODB(); size is in bytes.
 */
static void
writeat(off_t blkno, const void *buf, int size)
{
	/* Seek to the correct place. */
	if (lseek(fd, blkno * DEV_BSIZE, L_SET) < 0)
		err(EXIT_FAILURE, "lseek failed");
	/* See if we have to break up the transfer... */
	if (smallio) {
		const char *bp;	/* pointer into buf */
		int left;	/* bytes left to go */
		int n;		/* number to do this time around */
		int rv;		/* syscall return value */
		bp = buf;
		left = size;
		while (left > 0) {
			n = (left > 8192) ? 8192 : left;
			rv = write(fd, bp, n);
			if (rv < 0)
				err(EXIT_FAILURE, "write failed");
			if (rv != n)
				errx(EXIT_FAILURE,
				    "write: wanted %d, got %d", n, rv);
			bp += n;
			left -= n;
		}
	} else {
		int rv;
		rv = write(fd, buf, size);
		if (rv < 0)
			err(EXIT_FAILURE, "write failed");
		if (rv != size)
			errx(EXIT_FAILURE,
			    "write: wanted %d, got %d", size, rv);
	}
}
/*
 * Never-fail versions of malloc() and realloc(), and an allocation
 *  routine (which also never fails) for allocating memory that will
 *  never be freed until exit.
 */

/*
 * Never-fail malloc.
 */
static void *
nfmalloc(size_t nb, const char *tag)
{
	void *rv;

	rv = malloc(nb);
	if (rv)
		return (rv);
	err(EXIT_FAILURE, "Can't allocate %lu bytes for %s",
	    (unsigned long int) nb, tag);
}
/*
 * Never-fail realloc.
 */
static void *
nfrealloc(void *blk, size_t nb, const char *tag)
{
	void *rv;

	rv = realloc(blk, nb);
	if (rv)
		return (rv);
	err(EXIT_FAILURE, "Can't re-allocate %lu bytes for %s",
	    (unsigned long int) nb, tag);
}
/*
 * Allocate memory that will never be freed or reallocated.  Arguably
 *  this routine should handle small allocations by chopping up pages,
 *  but that's not worth the bother; it's not called more than a
 *  handful of times per run, and if the allocations are that small the
 *  waste in giving each one its own page is ignorable.
 */
static void *
alloconce(size_t nb, const char *tag)
{
	void *rv;

	rv = mmap(0, nb, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (rv != MAP_FAILED)
		return (rv);
	err(EXIT_FAILURE, "Can't map %lu bytes for %s",
	    (unsigned long int) nb, tag);
}
/*
 * Load the cgs and csums off disk.  Also allocates the space to load
 *  them into and initializes the per-cg flags.
 */
static void
loadcgs(void)
{
	int cg;
	char *cgp;

	cgblksz = roundup(oldsb->fs_cgsize, oldsb->fs_fsize);
	cgs = nfmalloc(oldsb->fs_ncg * sizeof(*cgs), "cg pointers");
	cgp = alloconce(oldsb->fs_ncg * cgblksz, "cgs");
	cgflags = nfmalloc(oldsb->fs_ncg, "cg flags");
	csums = nfmalloc(oldsb->fs_cssize, "cg summary");
	for (cg = 0; cg < oldsb->fs_ncg; cg++) {
		cgs[cg] = (struct cg *) cgp;
		readat(FFS_FSBTODB(oldsb, cgtod(oldsb, cg)), cgp, cgblksz);
		if (needswap)
			ffs_cg_swap(cgs[cg],cgs[cg],oldsb);
		cgflags[cg] = 0;
		cgp += cgblksz;
	}
	readat(FFS_FSBTODB(oldsb, oldsb->fs_csaddr), csums, oldsb->fs_cssize);
	if (needswap)
		ffs_csum_swap(csums,csums,oldsb->fs_cssize);
}
/*
 * Set n bits, starting with bit #base, in the bitmap pointed to by
 *  bitvec (which is assumed to be large enough to include bits base
 *  through base+n-1).
 */
static void
set_bits(unsigned char *bitvec, unsigned int base, unsigned int n)
{
	if (n < 1)
		return;		/* nothing to do */
	if (base & 7) {		/* partial byte at beginning */
		if (n <= 8 - (base & 7)) {	/* entirely within one byte */
			bitvec[base >> 3] |= (~((~0U) << n)) << (base & 7);
			return;
		}
		bitvec[base >> 3] |= (~0U) << (base & 7);
		n -= 8 - (base & 7);
		base = (base & ~7) + 8;
	}
	if (n >= 8) {		/* do full bytes */
		memset(bitvec + (base >> 3), 0xff, n >> 3);
		base += n & ~7;
		n &= 7;
	}
	if (n) {		/* partial byte at end */
		bitvec[base >> 3] |= ~((~0U) << n);
	}
}
/*
 * Clear n bits, starting with bit #base, in the bitmap pointed to by
 *  bitvec (which is assumed to be large enough to include bits base
 *  through base+n-1).  Code parallels set_bits().
 */
static void
clr_bits(unsigned char *bitvec, int base, int n)
{
	if (n < 1)
		return;
	if (base & 7) {
		if (n <= 8 - (base & 7)) {
			bitvec[base >> 3] &= ~((~((~0U) << n)) << (base & 7));
			return;
		}
		bitvec[base >> 3] &= ~((~0U) << (base & 7));
		n -= 8 - (base & 7);
		base = (base & ~7) + 8;
	}
	if (n >= 8) {
		memset(bitvec + (base >> 3), 0, n >> 3);
		base += n & ~7;
		n &= 7;
	}
	if (n) {
		bitvec[base >> 3] &= (~0U) << n;
	}
}
/*
 * Test whether bit #bit is set in the bitmap pointed to by bitvec.
 */
static int
bit_is_set(unsigned char *bitvec, int bit)
{
	return (bitvec[bit >> 3] & (1 << (bit & 7)));
}
/*
 * Test whether bit #bit is clear in the bitmap pointed to by bitvec.
 */
static int
bit_is_clr(unsigned char *bitvec, int bit)
{
	return (!bit_is_set(bitvec, bit));
}
/*
 * Test whether a whole block of bits is set in a bitmap.  This is
 *  designed for testing (aligned) disk blocks in a bit-per-frag
 *  bitmap; it has assumptions wired into it based on that, essentially
 *  that the entire block fits into a single byte.  This returns true
 *  iff _all_ the bits are set; it is not just the complement of
 *  blk_is_clr on the same arguments (unless blkfrags==1).
 */
static int
blk_is_set(unsigned char *bitvec, int blkbase, int blkfrags)
{
	unsigned int mask;

	mask = (~((~0U) << blkfrags)) << (blkbase & 7);
	return ((bitvec[blkbase >> 3] & mask) == mask);
}
/*
 * Test whether a whole block of bits is clear in a bitmap.  See
 *  blk_is_set (above) for assumptions.  This returns true iff _all_
 *  the bits are clear; it is not just the complement of blk_is_set on
 *  the same arguments (unless blkfrags==1).
 */
static int
blk_is_clr(unsigned char *bitvec, int blkbase, int blkfrags)
{
	unsigned int mask;

	mask = (~((~0U) << blkfrags)) << (blkbase & 7);
	return ((bitvec[blkbase >> 3] & mask) == 0);
}
/*
 * Initialize a new cg.  Called when growing.  Assumes memory has been
 *  allocated but not otherwise set up.  This code sets the fields of
 *  the cg, initializes the bitmaps (and cluster summaries, if
 *  applicable), updates both per-cylinder summary info and the global
 *  summary info in newsb; it also writes out new inodes for the cg.
 *
 * This code knows it can never be called for cg 0, which makes it a
 *  bit simpler than it would otherwise be.
 */
static void
initcg(int cgn)
{
	struct cg *cg;		/* The in-core cg, of course */
	int64_t base;		/* Disk address of cg base */
	int64_t dlow;		/* Size of pre-cg data area */
	int64_t dhigh;		/* Offset of post-inode data area, from base */
	int64_t dmax;		/* Offset of end of post-inode data area */
	int i;			/* Generic loop index */
	int n;			/* Generic count */
	int start;		/* start of cg maps */

	cg = cgs[cgn];
	/* Place the data areas */
	base = cgbase(newsb, cgn);
	dlow = cgsblock(newsb, cgn) - base;
	dhigh = cgdmin(newsb, cgn) - base;
	dmax = newsb->fs_size - base;
	if (dmax > newsb->fs_fpg)
		dmax = newsb->fs_fpg;
	start = (unsigned char *)&cg->cg_space[0] - (unsigned char *) cg;
	/*
         * Clear out the cg - assumes all-0-bytes is the correct way
         * to initialize fields we don't otherwise touch, which is
         * perhaps not the right thing to do, but it's what fsck and
         * mkfs do.
         */
	memset(cg, 0, newsb->fs_cgsize);
	if (newsb->fs_old_flags & FS_FLAGS_UPDATED)
		cg->cg_time = newsb->fs_time;
	cg->cg_magic = CG_MAGIC;
	cg->cg_cgx = cgn;
	cg->cg_niblk = newsb->fs_ipg;
	cg->cg_ndblk = dmax;

	if (is_ufs2) {
		cg->cg_time = newsb->fs_time;
		cg->cg_initediblk = newsb->fs_ipg < 2 * FFS_INOPB(newsb) ?
		    newsb->fs_ipg : 2 * FFS_INOPB(newsb);
		cg->cg_iusedoff = start;
	} else {
		cg->cg_old_time = newsb->fs_time;
		cg->cg_old_niblk = cg->cg_niblk;
		cg->cg_niblk = 0;
		cg->cg_initediblk = 0;


		cg->cg_old_ncyl = newsb->fs_old_cpg;
		/* Update the cg_old_ncyl value for the last cylinder. */
		if (cgn == newsb->fs_ncg - 1) {
			if ((newsb->fs_old_flags & FS_FLAGS_UPDATED) == 0)
				cg->cg_old_ncyl = newsb->fs_old_ncyl %
				    newsb->fs_old_cpg;
		}

		/* Set up the bitmap pointers.  We have to be careful
		 * to lay out the cg _exactly_ the way mkfs and fsck
		 * do it, since fsck compares the _entire_ cg against
		 * a recomputed cg, and whines if there is any
		 * mismatch, including the bitmap offsets. */
		/* XXX update this comment when fsck is fixed */
		cg->cg_old_btotoff = start;
		cg->cg_old_boff = cg->cg_old_btotoff
		    + (newsb->fs_old_cpg * sizeof(int32_t));
		cg->cg_iusedoff = cg->cg_old_boff +
		    (newsb->fs_old_cpg * newsb->fs_old_nrpos * sizeof(int16_t));
	}
	cg->cg_freeoff = cg->cg_iusedoff + howmany(newsb->fs_ipg, NBBY);
	if (newsb->fs_contigsumsize > 0) {
		cg->cg_nclusterblks = cg->cg_ndblk / newsb->fs_frag;
		cg->cg_clustersumoff = cg->cg_freeoff +
		    howmany(newsb->fs_fpg, NBBY) - sizeof(int32_t);
		cg->cg_clustersumoff =
		    roundup(cg->cg_clustersumoff, sizeof(int32_t));
		cg->cg_clusteroff = cg->cg_clustersumoff +
		    ((newsb->fs_contigsumsize + 1) * sizeof(int32_t));
		cg->cg_nextfreeoff = cg->cg_clusteroff +
		    howmany(ffs_fragstoblks(newsb,newsb->fs_fpg), NBBY);
		n = dlow / newsb->fs_frag;
		if (n > 0) {
			set_bits(cg_clustersfree(cg, 0), 0, n);
			cg_clustersum(cg, 0)[(n > newsb->fs_contigsumsize) ?
			    newsb->fs_contigsumsize : n]++;
		}
	} else {
		cg->cg_nextfreeoff = cg->cg_freeoff +
		    howmany(newsb->fs_fpg, NBBY);
	}
	/* Mark the data areas as free; everything else is marked busy by the
	 * memset() up at the top. */
	set_bits(cg_blksfree(cg, 0), 0, dlow);
	set_bits(cg_blksfree(cg, 0), dhigh, dmax - dhigh);
	/* Initialize summary info */
	cg->cg_cs.cs_ndir = 0;
	cg->cg_cs.cs_nifree = newsb->fs_ipg;
	cg->cg_cs.cs_nbfree = dlow / newsb->fs_frag;
	cg->cg_cs.cs_nffree = 0;

	/* This is the simplest way of doing this; we perhaps could
	 * compute the correct cg_blktot()[] and cg_blks()[] values
	 * other ways, but it would be complicated and hardly seems
	 * worth the effort.  (The reason there isn't
	 * frag-at-beginning and frag-at-end code here, like the code
	 * below for the post-inode data area, is that the pre-sb data
	 * area always starts at 0, and thus is block-aligned, and
	 * always ends at the sb, which is block-aligned.) */
	if ((newsb->fs_old_flags & FS_FLAGS_UPDATED) == 0) {
		int64_t di;

		for (di = 0; di < dlow; di += newsb->fs_frag) {
			old_cg_blktot(cg, 0)[old_cbtocylno(newsb, di)]++;
			old_cg_blks(newsb, cg,
			    old_cbtocylno(newsb, di),
			    0)[old_cbtorpos(newsb, di)]++;
		}
	}

	/* Deal with a partial block at the beginning of the post-inode area.
	 * I'm not convinced this can happen - I think the inodes are always
	 * block-aligned and always an integral number of blocks - but it's
	 * cheap to do the right thing just in case. */
	if (dhigh % newsb->fs_frag) {
		n = newsb->fs_frag - (dhigh % newsb->fs_frag);
		cg->cg_frsum[n]++;
		cg->cg_cs.cs_nffree += n;
		dhigh += n;
	}
	n = (dmax - dhigh) / newsb->fs_frag;
	/* We have n full-size blocks in the post-inode data area. */
	if (n > 0) {
		cg->cg_cs.cs_nbfree += n;
		if (newsb->fs_contigsumsize > 0) {
			i = dhigh / newsb->fs_frag;
			set_bits(cg_clustersfree(cg, 0), i, n);
			cg_clustersum(cg, 0)[(n > newsb->fs_contigsumsize) ?
			    newsb->fs_contigsumsize : n]++;
		}
		for (i = n; i > 0; i--) {
			if (is_ufs2 == 0) {
				old_cg_blktot(cg, 0)[old_cbtocylno(newsb,
					    dhigh)]++;
				old_cg_blks(newsb, cg,
				    old_cbtocylno(newsb, dhigh),
				    0)[old_cbtorpos(newsb,
					    dhigh)]++;
			}
			dhigh += newsb->fs_frag;
		}
	}
	/* Deal with any leftover frag at the end of the cg. */
	i = dmax - dhigh;
	if (i) {
		cg->cg_frsum[i]++;
		cg->cg_cs.cs_nffree += i;
	}
	/* Update the csum info. */
	csums[cgn] = cg->cg_cs;
	newsb->fs_cstotal.cs_nffree += cg->cg_cs.cs_nffree;
	newsb->fs_cstotal.cs_nbfree += cg->cg_cs.cs_nbfree;
	newsb->fs_cstotal.cs_nifree += cg->cg_cs.cs_nifree;
	if (is_ufs2) {
		/* Write out the cleared inodes. */
		writeat(FFS_FSBTODB(newsb, cgimin(newsb, cgn)), zinodes2,
		    cg->cg_initediblk * sizeof(*zinodes2));
	} else {
		/* Write out the cleared inodes. */
		writeat(FFS_FSBTODB(newsb, cgimin(newsb, cgn)), zinodes1,
		    newsb->fs_ipg * sizeof(*zinodes1));
	}
	/* Dirty the cg. */
	cgflags[cgn] |= CGF_DIRTY;
}
/*
 * Find free space, at least nfrags consecutive frags of it.  Pays no
 *  attention to block boundaries, but refuses to straddle cg
 *  boundaries, even if the disk blocks involved are in fact
 *  consecutive.  Return value is the frag number of the first frag of
 *  the block, or -1 if no space was found.  Uses newsb for sb values,
 *  and assumes the cgs[] structures correctly describe the area to be
 *  searched.
 *
 * XXX is there a bug lurking in the ignoring of block boundaries by
 *  the routine used by fragmove() in evict_data()?  Can an end-of-file
 *  frag legally straddle a block boundary?  If not, this should be
 *  cloned and fixed to stop at block boundaries for that use.  The
 *  current one may still be needed for csum info motion, in case that
 *  takes up more than a whole block (is the csum info allowed to begin
 *  partway through a block and continue into the following block?).
 *
 * If we wrap off the end of the file system back to the beginning, we
 *  can end up searching the end of the file system twice.  I ignore
 *  this inefficiency, since if that happens we're going to croak with
 *  a no-space error anyway, so it happens at most once.
 */
static int
find_freespace(unsigned int nfrags)
{
	static int hand = 0;	/* hand rotates through all frags in the fs */
	int cgsize;		/* size of the cg hand currently points into */
	int cgn;		/* number of cg hand currently points into */
	int fwc;		/* frag-within-cg number of frag hand points
				 * to */
	unsigned int run;	/* length of run of free frags seen so far */
	int secondpass;		/* have we wrapped from end of fs to
				 * beginning? */
	unsigned char *bits;	/* cg_blksfree()[] for cg hand points into */

	cgn = dtog(newsb, hand);
	fwc = dtogd(newsb, hand);
	secondpass = (hand == 0);
	run = 0;
	bits = cg_blksfree(cgs[cgn], 0);
	cgsize = cgs[cgn]->cg_ndblk;
	while (1) {
		if (bit_is_set(bits, fwc)) {
			run++;
			if (run >= nfrags)
				return (hand + 1 - run);
		} else {
			run = 0;
		}
		hand++;
		fwc++;
		if (fwc >= cgsize) {
			fwc = 0;
			cgn++;
			if (cgn >= newsb->fs_ncg) {
				hand = 0;
				if (secondpass)
					return (-1);
				secondpass = 1;
				cgn = 0;
			}
			bits = cg_blksfree(cgs[cgn], 0);
			cgsize = cgs[cgn]->cg_ndblk;
			run = 0;
		}
	}
}
/*
 * Find a free block of disk space.  Finds an entire block of frags,
 *  all of which are free.  Return value is the frag number of the
 *  first frag of the block, or -1 if no space was found.  Uses newsb
 *  for sb values, and assumes the cgs[] structures correctly describe
 *  the area to be searched.
 *
 * See find_freespace(), above, for remarks about hand wrapping around.
 */
static int
find_freeblock(void)
{
	static int hand = 0;	/* hand rotates through all frags in fs */
	int cgn;		/* cg number of cg hand points into */
	int fwc;		/* frag-within-cg number of frag hand points
				 * to */
	int cgsize;		/* size of cg hand points into */
	int secondpass;		/* have we wrapped from end to beginning? */
	unsigned char *bits;	/* cg_blksfree()[] for cg hand points into */

	cgn = dtog(newsb, hand);
	fwc = dtogd(newsb, hand);
	secondpass = (hand == 0);
	bits = cg_blksfree(cgs[cgn], 0);
	cgsize = ffs_blknum(newsb, cgs[cgn]->cg_ndblk);
	while (1) {
		if (blk_is_set(bits, fwc, newsb->fs_frag))
			return (hand);
		fwc += newsb->fs_frag;
		hand += newsb->fs_frag;
		if (fwc >= cgsize) {
			fwc = 0;
			cgn++;
			if (cgn >= newsb->fs_ncg) {
				hand = 0;
				if (secondpass)
					return (-1);
				secondpass = 1;
				cgn = 0;
			}
			bits = cg_blksfree(cgs[cgn], 0);
			cgsize = ffs_blknum(newsb, cgs[cgn]->cg_ndblk);
		}
	}
}
/*
 * Find a free inode, returning its inumber or -1 if none was found.
 *  Uses newsb for sb values, and assumes the cgs[] structures
 *  correctly describe the area to be searched.
 *
 * See find_freespace(), above, for remarks about hand wrapping around.
 */
static int
find_freeinode(void)
{
	static int hand = 0;	/* hand rotates through all inodes in fs */
	int cgn;		/* cg number of cg hand points into */
	int iwc;		/* inode-within-cg number of inode hand points
				 * to */
	int secondpass;		/* have we wrapped from end to beginning? */
	unsigned char *bits;	/* cg_inosused()[] for cg hand points into */

	cgn = hand / newsb->fs_ipg;
	iwc = hand % newsb->fs_ipg;
	secondpass = (hand == 0);
	bits = cg_inosused(cgs[cgn], 0);
	while (1) {
		if (bit_is_clr(bits, iwc))
			return (hand);
		hand++;
		iwc++;
		if (iwc >= newsb->fs_ipg) {
			iwc = 0;
			cgn++;
			if (cgn >= newsb->fs_ncg) {
				hand = 0;
				if (secondpass)
					return (-1);
				secondpass = 1;
				cgn = 0;
			}
			bits = cg_inosused(cgs[cgn], 0);
		}
	}
}
/*
 * Mark a frag as free.  Sets the frag's bit in the cg_blksfree bitmap
 *  for the appropriate cg, and marks the cg as dirty.
 */
static void
free_frag(int fno)
{
	int cgn;

	cgn = dtog(newsb, fno);
	set_bits(cg_blksfree(cgs[cgn], 0), dtogd(newsb, fno), 1);
	cgflags[cgn] |= CGF_DIRTY | CGF_BLKMAPS;
}
/*
 * Allocate a frag.  Clears the frag's bit in the cg_blksfree bitmap
 *  for the appropriate cg, and marks the cg as dirty.
 */
static void
alloc_frag(int fno)
{
	int cgn;

	cgn = dtog(newsb, fno);
	clr_bits(cg_blksfree(cgs[cgn], 0), dtogd(newsb, fno), 1);
	cgflags[cgn] |= CGF_DIRTY | CGF_BLKMAPS;
}
/*
 * Fix up the csum array.  If shrinking, this involves freeing zero or
 *  more frags; if growing, it involves allocating them, or if the
 *  frags being grown into aren't free, finding space elsewhere for the
 *  csum info.  (If the number of occupied frags doesn't change,
 *  nothing happens here.)
 */
static void
csum_fixup(void)
{
	int nold;		/* # frags in old csum info */
	int ntot;		/* # frags in new csum info */
	int nnew;		/* ntot-nold */
	int newloc;		/* new location for csum info, if necessary */
	int i;			/* generic loop index */
	int j;			/* generic loop index */
	int f;			/* "from" frag number, if moving */
	int t;			/* "to" frag number, if moving */
	int cgn;		/* cg number, used when shrinking */

	ntot = howmany(newsb->fs_cssize, newsb->fs_fsize);
	nold = howmany(oldsb->fs_cssize, newsb->fs_fsize);
	nnew = ntot - nold;
	/* First, if there's no change in frag counts, it's easy. */
	if (nnew == 0)
		return;
	/* Next, if we're shrinking, it's almost as easy.  Just free up any
	 * frags in the old area we no longer need. */
	if (nnew < 0) {
		for ((i = newsb->fs_csaddr + ntot - 1), (j = nnew);
		    j < 0;
		    i--, j++) {
			free_frag(i);
		}
		return;
	}
	/* We must be growing.  Check to see that the new csum area fits
	 * within the file system.  I think this can never happen, since for
	 * the csum area to grow, we must be adding at least one cg, so the
	 * old csum area can't be this close to the end of the new file system.
	 * But it's a cheap check. */
	/* XXX what if csum info is at end of cg and grows into next cg, what
	 * if it spills over onto the next cg's backup superblock?  Can this
	 * happen? */
	if (newsb->fs_csaddr + ntot <= newsb->fs_size) {
		/* Okay, it fits - now,  see if the space we want is free. */
		for ((i = newsb->fs_csaddr + nold), (j = nnew);
		    j > 0;
		    i++, j--) {
			cgn = dtog(newsb, i);
			if (bit_is_clr(cg_blksfree(cgs[cgn], 0),
				dtogd(newsb, i)))
				break;
		}
		if (j <= 0) {
			/* Win win - all the frags we want are free. Allocate
			 * 'em and we're all done.  */
			for ((i = newsb->fs_csaddr + ntot - nnew),
				 (j = nnew); j > 0; i++, j--) {
				alloc_frag(i);
			}
			return;
		}
	}
	/* We have to move the csum info, sigh.  Look for new space, free old
	 * space, and allocate new.  Update fs_csaddr.  We don't copy anything
	 * on disk at this point; the csum info will be written to the
	 * then-current fs_csaddr as part of the final flush. */
	newloc = find_freespace(ntot);
	if (newloc < 0)
		errx(EXIT_FAILURE, "Sorry, no space available for new csums");
	for (i = 0, f = newsb->fs_csaddr, t = newloc; i < ntot; i++, f++, t++) {
		if (i < nold) {
			free_frag(f);
		}
		alloc_frag(t);
	}
	newsb->fs_csaddr = newloc;
}
/*
 * Recompute newsb->fs_dsize.  Just scans all cgs, adding the number of
 *  data blocks in that cg to the total.
 */
static void
recompute_fs_dsize(void)
{
	int i;

	newsb->fs_dsize = 0;
	for (i = 0; i < newsb->fs_ncg; i++) {
		int64_t dlow;	/* size of before-sb data area */
		int64_t dhigh;	/* offset of post-inode data area */
		int64_t dmax;	/* total size of cg */
		int64_t base;	/* base of cg, since cgsblock() etc add it in */
		base = cgbase(newsb, i);
		dlow = cgsblock(newsb, i) - base;
		dhigh = cgdmin(newsb, i) - base;
		dmax = newsb->fs_size - base;
		if (dmax > newsb->fs_fpg)
			dmax = newsb->fs_fpg;
		newsb->fs_dsize += dlow + dmax - dhigh;
	}
	/* Space in cg 0 before cgsblock is boot area, not free space! */
	newsb->fs_dsize -= cgsblock(newsb, 0) - cgbase(newsb, 0);
	/* And of course the csum info takes up space. */
	newsb->fs_dsize -= howmany(newsb->fs_cssize, newsb->fs_fsize);
}
/*
 * Return the current time.  We call this and assign, rather than
 *  calling time() directly, as insulation against OSes where fs_time
 *  is not a time_t.
 */
static time_t
timestamp(void)
{
	time_t t;

	time(&t);
	return (t);
}

/*
 * Calculate new filesystem geometry
 *  return 0 if geometry actually changed
 */
static int
makegeometry(int chatter)
{

	/* Update the size. */
	newsb->fs_size = FFS_DBTOFSB(newsb, newsize);
	if (is_ufs2)
		newsb->fs_ncg = howmany(newsb->fs_size, newsb->fs_fpg);
	else {
		/* Update fs_old_ncyl and fs_ncg. */
		newsb->fs_old_ncyl = howmany(newsb->fs_size * NSPF(newsb),
		    newsb->fs_old_spc);
		newsb->fs_ncg = howmany(newsb->fs_old_ncyl, newsb->fs_old_cpg);
	}

	/* Does the last cg end before the end of its inode area? There is no
	 * reason why this couldn't be handled, but it would complicate a lot
	 * of code (in all file system code - fsck, kernel, etc) because of the
	 * potential partial inode area, and the gain in space would be
	 * minimal, at most the pre-sb data area. */
	if (cgdmin(newsb, newsb->fs_ncg - 1) > newsb->fs_size) {
		newsb->fs_ncg--;
		if (is_ufs2)
			newsb->fs_size = newsb->fs_ncg * newsb->fs_fpg;
		else {
			newsb->fs_old_ncyl = newsb->fs_ncg * newsb->fs_old_cpg;
			newsb->fs_size = (newsb->fs_old_ncyl *
				newsb->fs_old_spc) / NSPF(newsb);
		}
		if (chatter || verbose) {
			printf("Warning: last cylinder group is too small;\n");
			printf("    dropping it.  New size = %lu.\n",
			(unsigned long int) FFS_FSBTODB(newsb, newsb->fs_size));
		}
	}

	/* Did we actually not grow?  (This can happen if newsize is less than
	 * a frag larger than the old size - unlikely, but no excuse to
	 * misbehave if it happens.) */
	if (newsb->fs_size == oldsb->fs_size)
		return 1;

	return 0;
}


/*
 * Grow the file system.
 */
static void
grow(void)
{
	int i;

	if (makegeometry(1)) {
		printf("New fs size %"PRIu64" = old fs size %"PRIu64
		    ", not growing.\n", newsb->fs_size, oldsb->fs_size);
		return;
	}

	if (verbose) {
		printf("Growing fs from %"PRIu64" blocks to %"PRIu64
		    " blocks.\n", oldsb->fs_size, newsb->fs_size);
	}

	/* Update the timestamp. */
	newsb->fs_time = timestamp();
	/* Allocate and clear the new-inode area, in case we add any cgs. */
	if (is_ufs2) {
		zinodes2 = alloconce(newsb->fs_ipg * sizeof(*zinodes2),
			"zeroed inodes");
		memset(zinodes2, 0, newsb->fs_ipg * sizeof(*zinodes2));
	} else {
		zinodes1 = alloconce(newsb->fs_ipg * sizeof(*zinodes1),
			"zeroed inodes");
		memset(zinodes1, 0, newsb->fs_ipg * sizeof(*zinodes1));
	}
	
	/* Check that the new last sector (frag, actually) is writable.  Since
	 * it's at least one frag larger than it used to be, we know we aren't
	 * overwriting anything important by this.  (The choice of sbbuf as
	 * what to write is irrelevant; it's just something handy that's known
	 * to be at least one frag in size.) */
	writeat(FFS_FSBTODB(newsb,newsb->fs_size - 1), &sbbuf, newsb->fs_fsize);

	/* Find out how big the csum area is, and realloc csums if bigger. */
	newsb->fs_cssize = ffs_fragroundup(newsb,
	    newsb->fs_ncg * sizeof(struct csum));
	if (newsb->fs_cssize > oldsb->fs_cssize)
		csums = nfrealloc(csums, newsb->fs_cssize, "new cg summary");
	/* If we're adding any cgs, realloc structures and set up the new
	   cgs. */
	if (newsb->fs_ncg > oldsb->fs_ncg) {
		char *cgp;
		cgs = nfrealloc(cgs, newsb->fs_ncg * sizeof(*cgs),
                                "cg pointers");
		cgflags = nfrealloc(cgflags, newsb->fs_ncg, "cg flags");
		memset(cgflags + oldsb->fs_ncg, 0,
		    newsb->fs_ncg - oldsb->fs_ncg);
		cgp = alloconce((newsb->fs_ncg - oldsb->fs_ncg) * cgblksz,
                                "cgs");
		for (i = oldsb->fs_ncg; i < newsb->fs_ncg; i++) {
			cgs[i] = (struct cg *) cgp;
			progress_bar(special, "grow cg",
			    i - oldsb->fs_ncg, newsb->fs_ncg - oldsb->fs_ncg);
			initcg(i);
			cgp += cgblksz;
		}
		cgs[oldsb->fs_ncg - 1]->cg_old_ncyl = oldsb->fs_old_cpg;
		cgflags[oldsb->fs_ncg - 1] |= CGF_DIRTY;
	}
	/* If the old fs ended partway through a cg, we have to update the old
	 * last cg (though possibly not to a full cg!). */
	if (oldsb->fs_size % oldsb->fs_fpg) {
		struct cg *cg;
		int64_t newcgsize;
		int64_t prevcgtop;
		int64_t oldcgsize;
		cg = cgs[oldsb->fs_ncg - 1];
		cgflags[oldsb->fs_ncg - 1] |= CGF_DIRTY | CGF_BLKMAPS;
		prevcgtop = oldsb->fs_fpg * (oldsb->fs_ncg - 1);
		newcgsize = newsb->fs_size - prevcgtop;
		if (newcgsize > newsb->fs_fpg)
			newcgsize = newsb->fs_fpg;
		oldcgsize = oldsb->fs_size % oldsb->fs_fpg;
		set_bits(cg_blksfree(cg, 0), oldcgsize, newcgsize - oldcgsize);
		cg->cg_old_ncyl = oldsb->fs_old_cpg;
		cg->cg_ndblk = newcgsize;
	}
	/* Fix up the csum info, if necessary. */
	csum_fixup();
	/* Make fs_dsize match the new reality. */
	recompute_fs_dsize();

	progress_done();
}
/*
 * Call (*fn)() for each inode, passing the inode and its inumber.  The
 *  number of cylinder groups is pased in, so this can be used to map
 *  over either the old or the new file system's set of inodes.
 */
static void
map_inodes(void (*fn) (union dinode * di, unsigned int, void *arg),
	   int ncg, void *cbarg) {
	int i;
	int ni;

	ni = oldsb->fs_ipg * ncg;
	for (i = 0; i < ni; i++)
		(*fn) (inodes + i, i, cbarg);
}
/* Values for the third argument to the map function for
 * map_inode_data_blocks.  MDB_DATA indicates the block is contains
 * file data; MDB_INDIR_PRE and MDB_INDIR_POST indicate that it's an
 * indirect block.  The MDB_INDIR_PRE call is made before the indirect
 * block pointers are followed and the pointed-to blocks scanned,
 * MDB_INDIR_POST after.
 */
#define MDB_DATA       1
#define MDB_INDIR_PRE  2
#define MDB_INDIR_POST 3

typedef void (*mark_callback_t) (off_t blocknum, unsigned int nfrags,
				 unsigned int blksize, int opcode);

/* Helper function - handles a data block.  Calls the callback
 * function and returns number of bytes occupied in file (actually,
 * rounded up to a frag boundary).  The name is historical.  */
static int
markblk(mark_callback_t fn, union dinode * di, off_t bn, off_t o)
{
	int sz;
	int nb;
	off_t filesize;

	filesize = DIP(di,di_size);
	if (o >= filesize)
		return (0);
	sz = dblksize(newsb, di, ffs_lblkno(newsb, o), filesize);
	nb = (sz > filesize - o) ? filesize - o : sz;
	if (bn)
		(*fn) (bn, ffs_numfrags(newsb, sz), nb, MDB_DATA);
	return (sz);
}
/* Helper function - handles an indirect block.  Makes the
 * MDB_INDIR_PRE callback for the indirect block, loops over the
 * pointers and recurses, and makes the MDB_INDIR_POST callback.
 * Returns the number of bytes occupied in file, as does markblk().
 * For the sake of update_for_data_move(), we read the indirect block
 * _after_ making the _PRE callback.  The name is historical.  */
static off_t
markiblk(mark_callback_t fn, union dinode * di, off_t bn, off_t o, int lev)
{
	int i;
	unsigned k;
	off_t j, tot;
	static int32_t indirblk1[howmany(MAXBSIZE, sizeof(int32_t))];
	static int32_t indirblk2[howmany(MAXBSIZE, sizeof(int32_t))];
	static int32_t indirblk3[howmany(MAXBSIZE, sizeof(int32_t))];
	static int32_t *indirblks[3] = {
		&indirblk1[0], &indirblk2[0], &indirblk3[0]
	};

	if (lev < 0)
		return (markblk(fn, di, bn, o));
	if (bn == 0) {
		for (j = newsb->fs_bsize;
		    lev >= 0;
		    j *= FFS_NINDIR(newsb), lev--);
		return (j);
	}
	(*fn) (bn, newsb->fs_frag, newsb->fs_bsize, MDB_INDIR_PRE);
	readat(FFS_FSBTODB(newsb, bn), indirblks[lev], newsb->fs_bsize);
	if (needswap)
		for (k = 0; k < howmany(MAXBSIZE, sizeof(int32_t)); k++)
			indirblks[lev][k] = bswap32(indirblks[lev][k]);
	tot = 0;
	for (i = 0; i < FFS_NINDIR(newsb); i++) {
		j = markiblk(fn, di, indirblks[lev][i], o, lev - 1);
		if (j == 0)
			break;
		o += j;
		tot += j;
	}
	(*fn) (bn, newsb->fs_frag, newsb->fs_bsize, MDB_INDIR_POST);
	return (tot);
}


/*
 * Call (*fn)() for each data block for an inode.  This routine assumes
 *  the inode is known to be of a type that has data blocks (file,
 *  directory, or non-fast symlink).  The called function is:
 *
 * (*fn)(unsigned int blkno, unsigned int nf, unsigned int nb, int op)
 *
 *  where blkno is the frag number, nf is the number of frags starting
 *  at blkno (always <= fs_frag), nb is the number of bytes that belong
 *  to the file (usually nf*fs_frag, often less for the last block/frag
 *  of a file).
 */
static void
map_inode_data_blocks(union dinode * di, mark_callback_t fn)
{
	off_t o;		/* offset within  inode */
	off_t inc;		/* increment for o */
	int b;			/* index within di_db[] and di_ib[] arrays */

	/* Scan the direct blocks... */
	o = 0;
	for (b = 0; b < UFS_NDADDR; b++) {
		inc = markblk(fn, di, DIP(di,di_db[b]), o);
		if (inc == 0)
			break;
		o += inc;
	}
	/* ...and the indirect blocks. */
	if (inc) {
		for (b = 0; b < UFS_NIADDR; b++) {
			inc = markiblk(fn, di, DIP(di,di_ib[b]), o, b);
			if (inc == 0)
				return;
			o += inc;
		}
	}
}

static void
dblk_callback(union dinode * di, unsigned int inum, void *arg)
{
	mark_callback_t fn;
	off_t filesize;

	filesize = DIP(di,di_size);
	fn = (mark_callback_t) arg;
	switch (DIP(di,di_mode) & IFMT) {
	case IFLNK:
		if (filesize <= newsb->fs_maxsymlinklen) {
			break;
		}
		/* FALLTHROUGH */
	case IFDIR:
	case IFREG:
		map_inode_data_blocks(di, fn);
		break;
	}
}
/*
 * Make a callback call, a la map_inode_data_blocks, for all data
 *  blocks in the entire fs.  This is used only once, in
 *  update_for_data_move, but it's out at top level because the complex
 *  downward-funarg nesting that would otherwise result seems to give
 *  gcc gastric distress.
 */
static void
map_data_blocks(mark_callback_t fn, int ncg)
{
	map_inodes(&dblk_callback, ncg, (void *) fn);
}
/*
 * Initialize the blkmove array.
 */
static void
blkmove_init(void)
{
	int i;

	blkmove = alloconce(oldsb->fs_size * sizeof(*blkmove), "blkmove");
	for (i = 0; i < oldsb->fs_size; i++)
		blkmove[i] = i;
}
/*
 * Load the inodes off disk.  Allocates the structures and initializes
 *  them - the inodes from disk, the flags to zero.
 */
static void
loadinodes(void)
{
	int imax, ino, i, j;
	struct ufs1_dinode *dp1 = NULL;
	struct ufs2_dinode *dp2 = NULL;

	/* read inodes one fs block at a time and copy them */

	inodes = alloconce(oldsb->fs_ncg * oldsb->fs_ipg *
	    sizeof(union dinode), "inodes");
	iflags = alloconce(oldsb->fs_ncg * oldsb->fs_ipg, "inode flags");
	memset(iflags, 0, oldsb->fs_ncg * oldsb->fs_ipg);

	ibuf = nfmalloc(oldsb->fs_bsize,"inode block buf");
	if (is_ufs2)
		dp2 = (struct ufs2_dinode *)ibuf;
	else
		dp1 = (struct ufs1_dinode *)ibuf;

	for (ino = 0,imax = oldsb->fs_ipg * oldsb->fs_ncg; ino < imax; ) {
		readat(FFS_FSBTODB(oldsb, ino_to_fsba(oldsb, ino)), ibuf,
		    oldsb->fs_bsize);

		for (i = 0; i < oldsb->fs_inopb; i++) {
			if (is_ufs2) {
				if (needswap) {
					ffs_dinode2_swap(&(dp2[i]), &(dp2[i]));
					for (j = 0; j < UFS_NDADDR; j++)
						dp2[i].di_db[j] =
						    bswap32(dp2[i].di_db[j]);
					for (j = 0; j < UFS_NIADDR; j++)
						dp2[i].di_ib[j] =
						    bswap32(dp2[i].di_ib[j]);
				}
				memcpy(&inodes[ino].dp2, &dp2[i],
				    sizeof(inodes[ino].dp2));
			} else {
				if (needswap) {
					ffs_dinode1_swap(&(dp1[i]), &(dp1[i]));
					for (j = 0; j < UFS_NDADDR; j++)
						dp1[i].di_db[j] =
						    bswap32(dp1[i].di_db[j]);
					for (j = 0; j < UFS_NIADDR; j++)
						dp1[i].di_ib[j] =
						    bswap32(dp1[i].di_ib[j]);
				}
				memcpy(&inodes[ino].dp1, &dp1[i],
				    sizeof(inodes[ino].dp1));
			}
			    if (++ino > imax)
				    errx(EXIT_FAILURE,
					"Exceeded number of inodes");
		}

	}
}
/*
 * Report a file-system-too-full problem.
 */
__dead static void
toofull(void)
{
	errx(EXIT_FAILURE, "Sorry, would run out of data blocks");
}
/*
 * Record a desire to move "n" frags from "from" to "to".
 */
static void
mark_move(unsigned int from, unsigned int to, unsigned int n)
{
	for (; n > 0; n--)
		blkmove[from++] = to++;
}
/* Helper function - evict n frags, starting with start (cg-relative).
 * The free bitmap is scanned, unallocated frags are ignored, and
 * each block of consecutive allocated frags is moved as a unit.
 */
static void
fragmove(struct cg * cg, int64_t base, unsigned int start, unsigned int n)
{
	unsigned int i;
	int run;

	run = 0;
	for (i = 0; i <= n; i++) {
		if ((i < n) && bit_is_clr(cg_blksfree(cg, 0), start + i)) {
			run++;
		} else {
			if (run > 0) {
				int off;
				off = find_freespace(run);
				if (off < 0)
					toofull();
				mark_move(base + start + i - run, off, run);
				set_bits(cg_blksfree(cg, 0), start + i - run,
				    run);
				clr_bits(cg_blksfree(cgs[dtog(oldsb, off)], 0),
				    dtogd(oldsb, off), run);
			}
			run = 0;
		}
	}
}
/*
 * Evict all data blocks from the given cg, starting at minfrag (based
 *  at the beginning of the cg), for length nfrag.  The eviction is
 *  assumed to be entirely data-area; this should not be called with a
 *  range overlapping the metadata structures in the cg.  It also
 *  assumes minfrag points into the given cg; it will misbehave if this
 *  is not true.
 *
 * See the comment header on find_freespace() for one possible bug
 *  lurking here.
 */
static void
evict_data(struct cg * cg, unsigned int minfrag, int nfrag)
{
	int64_t base;	/* base of cg (in frags from beginning of fs) */

	base = cgbase(oldsb, cg->cg_cgx);
	/* Does the boundary fall in the middle of a block?  To avoid
	 * breaking between frags allocated as consecutive, we always
	 * evict the whole block in this case, though one could argue
	 * we should check to see if the frag before or after the
	 * break is unallocated. */
	if (minfrag % oldsb->fs_frag) {
		int n;
		n = minfrag % oldsb->fs_frag;
		minfrag -= n;
		nfrag += n;
	}
	/* Do whole blocks.  If a block is wholly free, skip it; if
	 * wholly allocated, move it in toto.  If neither, call
	 * fragmove() to move the frags to new locations. */
	while (nfrag >= oldsb->fs_frag) {
		if (!blk_is_set(cg_blksfree(cg, 0), minfrag, oldsb->fs_frag)) {
			if (blk_is_clr(cg_blksfree(cg, 0), minfrag,
				oldsb->fs_frag)) {
				int off;
				off = find_freeblock();
				if (off < 0)
					toofull();
				mark_move(base + minfrag, off, oldsb->fs_frag);
				set_bits(cg_blksfree(cg, 0), minfrag,
				    oldsb->fs_frag);
				clr_bits(cg_blksfree(cgs[dtog(oldsb, off)], 0),
				    dtogd(oldsb, off), oldsb->fs_frag);
			} else {
				fragmove(cg, base, minfrag, oldsb->fs_frag);
			}
		}
		minfrag += oldsb->fs_frag;
		nfrag -= oldsb->fs_frag;
	}
	/* Clean up any sub-block amount left over. */
	if (nfrag) {
		fragmove(cg, base, minfrag, nfrag);
	}
}
/*
 * Move all data blocks according to blkmove.  We have to be careful,
 *  because we may be updating indirect blocks that will themselves be
 *  getting moved, or inode int32_t arrays that point to indirect
 *  blocks that will be moved.  We call this before
 *  update_for_data_move, and update_for_data_move does inodes first,
 *  then indirect blocks in preorder, so as to make sure that the
 *  file system is self-consistent at all points, for better crash
 *  tolerance.  (We can get away with this only because all the writes
 *  done by perform_data_move() are writing into space that's not used
 *  by the old file system.)  If we crash, some things may point to the
 *  old data and some to the new, but both copies are the same.  The
 *  only wrong things should be csum info and free bitmaps, which fsck
 *  is entirely capable of cleaning up.
 *
 * Since blkmove_init() initializes all blocks to move to their current
 *  locations, we can have two blocks marked as wanting to move to the
 *  same location, but only two and only when one of them is the one
 *  that was already there.  So if blkmove[i]==i, we ignore that entry
 *  entirely - for unallocated blocks, we don't want it (and may be
 *  putting something else there), and for allocated blocks, we don't
 *  want to copy it anywhere.
 */
static void
perform_data_move(void)
{
	int i;
	int run;
	int maxrun;
	char buf[65536];

	maxrun = sizeof(buf) / newsb->fs_fsize;
	run = 0;
	for (i = 0; i < oldsb->fs_size; i++) {
		if ((blkmove[i] == (unsigned)i /*XXX cast*/) ||
		    (run >= maxrun) ||
		    ((run > 0) &&
			(blkmove[i] != blkmove[i - 1] + 1))) {
			if (run > 0) {
				readat(FFS_FSBTODB(oldsb, i - run), &buf[0],
				    run << oldsb->fs_fshift);
				writeat(FFS_FSBTODB(oldsb, blkmove[i - run]),
				    &buf[0], run << oldsb->fs_fshift);
			}
			run = 0;
		}
		if (blkmove[i] != (unsigned)i /*XXX cast*/)
			run++;
	}
	if (run > 0) {
		readat(FFS_FSBTODB(oldsb, i - run), &buf[0],
		    run << oldsb->fs_fshift);
		writeat(FFS_FSBTODB(oldsb, blkmove[i - run]), &buf[0],
		    run << oldsb->fs_fshift);
	}
}
/*
 * This modifies an array of int32_t, according to blkmove.  This is
 *  used to update inode block arrays and indirect blocks to point to
 *  the new locations of data blocks.
 *
 * Return value is the number of int32_ts that needed updating; in
 *  particular, the return value is zero iff nothing was modified.
 */
static int
movemap_blocks(int32_t * vec, int n)
{
	int rv;

	rv = 0;
	for (; n > 0; n--, vec++) {
		if (blkmove[*vec] != (unsigned)*vec /*XXX cast*/) {
			*vec = blkmove[*vec];
			rv++;
		}
	}
	return (rv);
}
static void
moveblocks_callback(union dinode * di, unsigned int inum, void *arg)
{
	int32_t *dblkptr, *iblkptr;

	switch (DIP(di,di_mode) & IFMT) {
	case IFLNK:
		if ((off_t)DIP(di,di_size) <= oldsb->fs_maxsymlinklen) {
			break;
		}
		/* FALLTHROUGH */
	case IFDIR:
	case IFREG:
		if (is_ufs2) {
			/* XXX these are not int32_t and this is WRONG! */
			dblkptr = (void *) &(di->dp2.di_db[0]);
			iblkptr = (void *) &(di->dp2.di_ib[0]);
		} else {
			dblkptr = &(di->dp1.di_db[0]);
			iblkptr = &(di->dp1.di_ib[0]);
		}
		/*
		 * Don't || these two calls; we need their
		 * side-effects.
		 */
		if (movemap_blocks(dblkptr, UFS_NDADDR)) {
			iflags[inum] |= IF_DIRTY;
		}
		if (movemap_blocks(iblkptr, UFS_NIADDR)) {
			iflags[inum] |= IF_DIRTY;
		}
		break;
	}
}

static void
moveindir_callback(off_t off, unsigned int nfrag, unsigned int nbytes,
		   int kind)
{
	unsigned int i;

	if (kind == MDB_INDIR_PRE) {
		int32_t blk[howmany(MAXBSIZE, sizeof(int32_t))];
		readat(FFS_FSBTODB(oldsb, off), &blk[0], oldsb->fs_bsize);
		if (needswap)
			for (i = 0; i < howmany(MAXBSIZE, sizeof(int32_t)); i++)
				blk[i] = bswap32(blk[i]);
		if (movemap_blocks(&blk[0], FFS_NINDIR(oldsb))) {
			if (needswap)
				for (i = 0; i < howmany(MAXBSIZE,
					sizeof(int32_t)); i++)
					blk[i] = bswap32(blk[i]);
			writeat(FFS_FSBTODB(oldsb, off), &blk[0], oldsb->fs_bsize);
		}
	}
}
/*
 * Update all inode data arrays and indirect blocks to point to the new
 *  locations of data blocks.  See the comment header on
 *  perform_data_move for some ordering considerations.
 */
static void
update_for_data_move(void)
{
	map_inodes(&moveblocks_callback, oldsb->fs_ncg, NULL);
	map_data_blocks(&moveindir_callback, oldsb->fs_ncg);
}
/*
 * Initialize the inomove array.
 */
static void
inomove_init(void)
{
	int i;

	inomove = alloconce(oldsb->fs_ipg * oldsb->fs_ncg * sizeof(*inomove),
                            "inomove");
	for (i = (oldsb->fs_ipg * oldsb->fs_ncg) - 1; i >= 0; i--)
		inomove[i] = i;
}
/*
 * Flush all dirtied inodes to disk.  Scans the inode flags array; for
 *  each dirty inode, it sets the BDIRTY bit on the first inode in the
 *  block containing the dirty inode.  Then it scans by blocks, and for
 *  each marked block, writes it.
 */
static void
flush_inodes(void)
{
	int i, j, k, ni, m;
	struct ufs1_dinode *dp1 = NULL;
	struct ufs2_dinode *dp2 = NULL;

	ni = newsb->fs_ipg * newsb->fs_ncg;
	m = FFS_INOPB(newsb) - 1;
	for (i = 0; i < ni; i++) {
		if (iflags[i] & IF_DIRTY) {
			iflags[i & ~m] |= IF_BDIRTY;
		}
	}
	m++;

	if (is_ufs2)
		dp2 = (struct ufs2_dinode *)ibuf;
	else
		dp1 = (struct ufs1_dinode *)ibuf;

	for (i = 0; i < ni; i += m) {
		if ((iflags[i] & IF_BDIRTY) == 0)
			continue;
		if (is_ufs2)
			for (j = 0; j < m; j++) {
				dp2[j] = inodes[i + j].dp2;
				if (needswap) {
					for (k = 0; k < UFS_NDADDR; k++)
						dp2[j].di_db[k] =
						    bswap32(dp2[j].di_db[k]);
					for (k = 0; k < UFS_NIADDR; k++)
						dp2[j].di_ib[k] =
						    bswap32(dp2[j].di_ib[k]);
					ffs_dinode2_swap(&dp2[j],
					    &dp2[j]);
				}
			}
		else
			for (j = 0; j < m; j++) {
				dp1[j] = inodes[i + j].dp1;
				if (needswap) {
					for (k = 0; k < UFS_NDADDR; k++)
						dp1[j].di_db[k]=
						    bswap32(dp1[j].di_db[k]);
					for (k = 0; k < UFS_NIADDR; k++)
						dp1[j].di_ib[k]=
						    bswap32(dp1[j].di_ib[k]);
					ffs_dinode1_swap(&dp1[j],
					    &dp1[j]);
				}
			}

		writeat(FFS_FSBTODB(newsb, ino_to_fsba(newsb, i)),
		    ibuf, newsb->fs_bsize);
	}
}
/*
 * Evict all inodes from the specified cg.  shrink() already checked
 *  that there were enough free inodes, so the no-free-inodes check is
 *  a can't-happen.  If it does trip, the file system should be in good
 *  enough shape for fsck to fix; see the comment on perform_data_move
 *  for the considerations in question.
 */
static void
evict_inodes(struct cg * cg)
{
	int inum;
	int i;
	int fi;

	inum = newsb->fs_ipg * cg->cg_cgx;
	for (i = 0; i < newsb->fs_ipg; i++, inum++) {
		if (DIP(inodes + inum,di_mode) != 0) {
			fi = find_freeinode();
			if (fi < 0)
				errx(EXIT_FAILURE, "Sorry, inodes evaporated - "
				    "file system probably needs fsck");
			inomove[inum] = fi;
			clr_bits(cg_inosused(cg, 0), i, 1);
			set_bits(cg_inosused(cgs[ino_to_cg(newsb, fi)], 0),
			    fi % newsb->fs_ipg, 1);
		}
	}
}
/*
 * Move inodes from old locations to new.  Does not actually write
 *  anything to disk; just copies in-core and sets dirty bits.
 *
 * We have to be careful here for reasons similar to those mentioned in
 *  the comment header on perform_data_move, above: for the sake of
 *  crash tolerance, we want to make sure everything is present at both
 *  old and new locations before we update pointers.  So we call this
 *  first, then flush_inodes() to get them out on disk, then update
 *  directories to match.
 */
static void
perform_inode_move(void)
{
	unsigned int i;
	unsigned int ni;

	ni = oldsb->fs_ipg * oldsb->fs_ncg;
	for (i = 0; i < ni; i++) {
		if (inomove[i] != i) {
			inodes[inomove[i]] = inodes[i];
			iflags[inomove[i]] = iflags[i] | IF_DIRTY;
		}
	}
}
/*
 * Update the directory contained in the nb bytes at buf, to point to
 *  inodes' new locations.
 */
static int
update_dirents(char *buf, int nb)
{
	int rv;
#define d ((struct direct *)buf)
#define s32(x) (needswap?bswap32((x)):(x))
#define s16(x) (needswap?bswap16((x)):(x))

	rv = 0;
	while (nb > 0) {
		if (inomove[s32(d->d_ino)] != s32(d->d_ino)) {
			rv++;
			d->d_ino = s32(inomove[s32(d->d_ino)]);
		}
		nb -= s16(d->d_reclen);
		buf += s16(d->d_reclen);
	}
	return (rv);
#undef d
#undef s32
#undef s16
}
/*
 * Callback function for map_inode_data_blocks, for updating a
 *  directory to point to new inode locations.
 */
static void
update_dir_data(off_t bn, unsigned int size, unsigned int nb, int kind)
{
	if (kind == MDB_DATA) {
		union {
			struct direct d;
			char ch[MAXBSIZE];
		}     buf;
		readat(FFS_FSBTODB(oldsb, bn), &buf, size << oldsb->fs_fshift);
		if (update_dirents((char *) &buf, nb)) {
			writeat(FFS_FSBTODB(oldsb, bn), &buf,
			    size << oldsb->fs_fshift);
		}
	}
}
static void
dirmove_callback(union dinode * di, unsigned int inum, void *arg)
{
	switch (DIP(di,di_mode) & IFMT) {
	case IFDIR:
		map_inode_data_blocks(di, &update_dir_data);
		break;
	}
}
/*
 * Update directory entries to point to new inode locations.
 */
static void
update_for_inode_move(void)
{
	map_inodes(&dirmove_callback, newsb->fs_ncg, NULL);
}
/*
 * Shrink the file system.
 */
static void
shrink(void)
{
	int i;

	if (makegeometry(1)) {
		printf("New fs size %"PRIu64" = old fs size %"PRIu64
		    ", not shrinking.\n", newsb->fs_size, oldsb->fs_size);
		return;
	}

	/* Let's make sure we're not being shrunk into oblivion. */
	if (newsb->fs_ncg < 1)
		errx(EXIT_FAILURE, "Size too small - file system would "
		    "have no cylinders");

	if (verbose) {
		printf("Shrinking fs from %"PRIu64" blocks to %"PRIu64
		    " blocks.\n", oldsb->fs_size, newsb->fs_size);
	}

	/* Load the inodes off disk - we'll need 'em. */
	loadinodes();

	/* Update the timestamp. */
	newsb->fs_time = timestamp();

	/* Initialize for block motion. */
	blkmove_init();
	/* Update csum size, then fix up for the new size */
	newsb->fs_cssize = ffs_fragroundup(newsb,
	    newsb->fs_ncg * sizeof(struct csum));
	csum_fixup();
	/* Evict data from any cgs being wholly eliminated */
	for (i = newsb->fs_ncg; i < oldsb->fs_ncg; i++) {
		int64_t base;
		int64_t dlow;
		int64_t dhigh;
		int64_t dmax;
		base = cgbase(oldsb, i);
		dlow = cgsblock(oldsb, i) - base;
		dhigh = cgdmin(oldsb, i) - base;
		dmax = oldsb->fs_size - base;
		if (dmax > cgs[i]->cg_ndblk)
			dmax = cgs[i]->cg_ndblk;
		evict_data(cgs[i], 0, dlow);
		evict_data(cgs[i], dhigh, dmax - dhigh);
		newsb->fs_cstotal.cs_ndir -= cgs[i]->cg_cs.cs_ndir;
		newsb->fs_cstotal.cs_nifree -= cgs[i]->cg_cs.cs_nifree;
		newsb->fs_cstotal.cs_nffree -= cgs[i]->cg_cs.cs_nffree;
		newsb->fs_cstotal.cs_nbfree -= cgs[i]->cg_cs.cs_nbfree;
	}
	/* Update the new last cg. */
	cgs[newsb->fs_ncg - 1]->cg_ndblk = newsb->fs_size -
	    ((newsb->fs_ncg - 1) * newsb->fs_fpg);
	/* Is the new last cg partial?  If so, evict any data from the part
	 * being shrunken away. */
	if (newsb->fs_size % newsb->fs_fpg) {
		struct cg *cg;
		int oldcgsize;
		int newcgsize;
		cg = cgs[newsb->fs_ncg - 1];
		newcgsize = newsb->fs_size % newsb->fs_fpg;
		oldcgsize = oldsb->fs_size - ((newsb->fs_ncg - 1) &
		    oldsb->fs_fpg);
		if (oldcgsize > oldsb->fs_fpg)
			oldcgsize = oldsb->fs_fpg;
		evict_data(cg, newcgsize, oldcgsize - newcgsize);
		clr_bits(cg_blksfree(cg, 0), newcgsize, oldcgsize - newcgsize);
	}
	/* Find out whether we would run out of inodes.  (Note we
	 * haven't actually done anything to the file system yet; all
	 * those evict_data calls just update blkmove.) */
	{
		int slop;
		slop = 0;
		for (i = 0; i < newsb->fs_ncg; i++)
			slop += cgs[i]->cg_cs.cs_nifree;
		for (; i < oldsb->fs_ncg; i++)
			slop -= oldsb->fs_ipg - cgs[i]->cg_cs.cs_nifree;
		if (slop < 0)
			errx(EXIT_FAILURE, "Sorry, would run out of inodes");
	}
	/* Copy data, then update pointers to data.  See the comment
	 * header on perform_data_move for ordering considerations. */
	perform_data_move();
	update_for_data_move();
	/* Now do inodes.  Initialize, evict, move, update - see the
	 * comment header on perform_inode_move. */
	inomove_init();
	for (i = newsb->fs_ncg; i < oldsb->fs_ncg; i++)
		evict_inodes(cgs[i]);
	perform_inode_move();
	flush_inodes();
	update_for_inode_move();
	/* Recompute all the bitmaps; most of them probably need it anyway,
	 * the rest are just paranoia and not wanting to have to bother
	 * keeping track of exactly which ones require it. */
	for (i = 0; i < newsb->fs_ncg; i++)
		cgflags[i] |= CGF_DIRTY | CGF_BLKMAPS | CGF_INOMAPS;
	/* Update the cg_old_ncyl value for the last cylinder. */
	if ((newsb->fs_old_flags & FS_FLAGS_UPDATED) == 0)
		cgs[newsb->fs_ncg - 1]->cg_old_ncyl =
		    newsb->fs_old_ncyl % newsb->fs_old_cpg;
	/* Make fs_dsize match the new reality. */
	recompute_fs_dsize();
}
/*
 * Recompute the block totals, block cluster summaries, and rotational
 *  position summaries, for a given cg (specified by number), based on
 *  its free-frag bitmap (cg_blksfree()[]).
 */
static void
rescan_blkmaps(int cgn)
{
	struct cg *cg;
	int f;
	int b;
	int blkfree;
	int blkrun;
	int fragrun;
	int fwb;

	cg = cgs[cgn];
	/* Subtract off the current totals from the sb's summary info */
	newsb->fs_cstotal.cs_nffree -= cg->cg_cs.cs_nffree;
	newsb->fs_cstotal.cs_nbfree -= cg->cg_cs.cs_nbfree;
	/* Clear counters and bitmaps. */
	cg->cg_cs.cs_nffree = 0;
	cg->cg_cs.cs_nbfree = 0;
	memset(&cg->cg_frsum[0], 0, MAXFRAG * sizeof(cg->cg_frsum[0]));
	memset(&old_cg_blktot(cg, 0)[0], 0,
	    newsb->fs_old_cpg * sizeof(old_cg_blktot(cg, 0)[0]));
	memset(&old_cg_blks(newsb, cg, 0, 0)[0], 0,
	    newsb->fs_old_cpg * newsb->fs_old_nrpos *
	    sizeof(old_cg_blks(newsb, cg, 0, 0)[0]));
	if (newsb->fs_contigsumsize > 0) {
		cg->cg_nclusterblks = cg->cg_ndblk / newsb->fs_frag;
		memset(&cg_clustersum(cg, 0)[1], 0,
		    newsb->fs_contigsumsize *
		    sizeof(cg_clustersum(cg, 0)[1]));
		if (is_ufs2)
			memset(&cg_clustersfree(cg, 0)[0], 0,
			    howmany(newsb->fs_fpg / NSPB(newsb), NBBY));
		else
			memset(&cg_clustersfree(cg, 0)[0], 0,
			    howmany((newsb->fs_old_cpg * newsb->fs_old_spc) /
				NSPB(newsb), NBBY));
	}
	/* Scan the free-frag bitmap.  Runs of free frags are kept
	 * track of with fragrun, and recorded into cg_frsum[] and
	 * cg_cs.cs_nffree; on each block boundary, entire free blocks
	 * are recorded as well. */
	blkfree = 1;
	blkrun = 0;
	fragrun = 0;
	f = 0;
	b = 0;
	fwb = 0;
	while (f < cg->cg_ndblk) {
		if (bit_is_set(cg_blksfree(cg, 0), f)) {
			fragrun++;
		} else {
			blkfree = 0;
			if (fragrun > 0) {
				cg->cg_frsum[fragrun]++;
				cg->cg_cs.cs_nffree += fragrun;
			}
			fragrun = 0;
		}
		f++;
		fwb++;
		if (fwb >= newsb->fs_frag) {
			if (blkfree) {
				cg->cg_cs.cs_nbfree++;
				if (newsb->fs_contigsumsize > 0)
					set_bits(cg_clustersfree(cg, 0), b, 1);
				if (is_ufs2 == 0) {
					old_cg_blktot(cg, 0)[
						old_cbtocylno(newsb,
						    f - newsb->fs_frag)]++;
					old_cg_blks(newsb, cg,
					    old_cbtocylno(newsb,
						f - newsb->fs_frag),
					    0)[old_cbtorpos(newsb,
						    f - newsb->fs_frag)]++;
				}
				blkrun++;
			} else {
				if (fragrun > 0) {
					cg->cg_frsum[fragrun]++;
					cg->cg_cs.cs_nffree += fragrun;
				}
				if (newsb->fs_contigsumsize > 0) {
					if (blkrun > 0) {
						cg_clustersum(cg, 0)[(blkrun
						    > newsb->fs_contigsumsize)
						    ? newsb->fs_contigsumsize
						    : blkrun]++;
					}
				}
				blkrun = 0;
			}
			fwb = 0;
			b++;
			blkfree = 1;
			fragrun = 0;
		}
	}
	if (fragrun > 0) {
		cg->cg_frsum[fragrun]++;
		cg->cg_cs.cs_nffree += fragrun;
	}
	if ((blkrun > 0) && (newsb->fs_contigsumsize > 0)) {
		cg_clustersum(cg, 0)[(blkrun > newsb->fs_contigsumsize) ?
		    newsb->fs_contigsumsize : blkrun]++;
	}
	/*
         * Put the updated summary info back into csums, and add it
         * back into the sb's summary info.  Then mark the cg dirty.
         */
	csums[cgn] = cg->cg_cs;
	newsb->fs_cstotal.cs_nffree += cg->cg_cs.cs_nffree;
	newsb->fs_cstotal.cs_nbfree += cg->cg_cs.cs_nbfree;
	cgflags[cgn] |= CGF_DIRTY;
}
/*
 * Recompute the cg_inosused()[] bitmap, and the cs_nifree and cs_ndir
 *  values, for a cg, based on the in-core inodes for that cg.
 */
static void
rescan_inomaps(int cgn)
{
	struct cg *cg;
	int inum;
	int iwc;

	cg = cgs[cgn];
	newsb->fs_cstotal.cs_ndir -= cg->cg_cs.cs_ndir;
	newsb->fs_cstotal.cs_nifree -= cg->cg_cs.cs_nifree;
	cg->cg_cs.cs_ndir = 0;
	cg->cg_cs.cs_nifree = 0;
	memset(&cg_inosused(cg, 0)[0], 0, howmany(newsb->fs_ipg, NBBY));
	inum = cgn * newsb->fs_ipg;
	if (cgn == 0) {
		set_bits(cg_inosused(cg, 0), 0, 2);
		iwc = 2;
		inum += 2;
	} else {
		iwc = 0;
	}
	for (; iwc < newsb->fs_ipg; iwc++, inum++) {
		switch (DIP(inodes + inum, di_mode) & IFMT) {
		case 0:
			cg->cg_cs.cs_nifree++;
			break;
		case IFDIR:
			cg->cg_cs.cs_ndir++;
			/* FALLTHROUGH */
		default:
			set_bits(cg_inosused(cg, 0), iwc, 1);
			break;
		}
	}
	csums[cgn] = cg->cg_cs;
	newsb->fs_cstotal.cs_ndir += cg->cg_cs.cs_ndir;
	newsb->fs_cstotal.cs_nifree += cg->cg_cs.cs_nifree;
	cgflags[cgn] |= CGF_DIRTY;
}
/*
 * Flush cgs to disk, recomputing anything they're marked as needing.
 */
static void
flush_cgs(void)
{
	int i;

	for (i = 0; i < newsb->fs_ncg; i++) {
		progress_bar(special, "flush cg",
		    i, newsb->fs_ncg - 1);
		if (cgflags[i] & CGF_BLKMAPS) {
			rescan_blkmaps(i);
		}
		if (cgflags[i] & CGF_INOMAPS) {
			rescan_inomaps(i);
		}
		if (cgflags[i] & CGF_DIRTY) {
			cgs[i]->cg_rotor = 0;
			cgs[i]->cg_frotor = 0;
			cgs[i]->cg_irotor = 0;
			if (needswap)
				ffs_cg_swap(cgs[i],cgs[i],newsb);
			writeat(FFS_FSBTODB(newsb, cgtod(newsb, i)), cgs[i],
			    cgblksz);
		}
	}
	if (needswap)
		ffs_csum_swap(csums,csums,newsb->fs_cssize);
	writeat(FFS_FSBTODB(newsb, newsb->fs_csaddr), csums, newsb->fs_cssize);

	progress_done();
}
/*
 * Write the superblock, both to the main superblock and to each cg's
 *  alternative superblock.
 */
static void
write_sbs(void)
{
	int i;

	if (newsb->fs_magic == FS_UFS1_MAGIC &&
	    (newsb->fs_old_flags & FS_FLAGS_UPDATED) == 0) {
		newsb->fs_old_time = newsb->fs_time;
	    	newsb->fs_old_size = newsb->fs_size;
	    	/* we don't update fs_csaddr */
	    	newsb->fs_old_dsize = newsb->fs_dsize;
		newsb->fs_old_cstotal.cs_ndir = newsb->fs_cstotal.cs_ndir;
		newsb->fs_old_cstotal.cs_nbfree = newsb->fs_cstotal.cs_nbfree;
		newsb->fs_old_cstotal.cs_nifree = newsb->fs_cstotal.cs_nifree;
		newsb->fs_old_cstotal.cs_nffree = newsb->fs_cstotal.cs_nffree;
		/* fill fs_old_postbl_start with 256 bytes of 0xff? */
	}
	/* copy newsb back to oldsb, so we can use it for offsets if
	   newsb has been swapped for writing to disk */
	memcpy(oldsb, newsb, SBLOCKSIZE);
	if (needswap)
		ffs_sb_swap(newsb,newsb);
	writeat(where /  DEV_BSIZE, newsb, SBLOCKSIZE);
	for (i = 0; i < oldsb->fs_ncg; i++) {
		progress_bar(special, "write sb",
		    i, oldsb->fs_ncg - 1);
		writeat(FFS_FSBTODB(oldsb, cgsblock(oldsb, i)), newsb, SBLOCKSIZE);
	}

	progress_done();
}

/*
 * Check to see wether new size changes the filesystem
 *  return exit code
 */
static int
checkonly(void)
{
	if (makegeometry(0)) {
		if (verbose) {
			printf("Wouldn't change: already %" PRId64
			    " blocks\n", (int64_t)oldsb->fs_size);
		}
		return 1;
	}

	if (verbose) {
		printf("Would change: newsize: %" PRId64 " oldsize: %"
		    PRId64 " fsdb: %" PRId64 "\n", FFS_DBTOFSB(oldsb, newsize),
		    (int64_t)oldsb->fs_size,
		    (int64_t)oldsb->fs_fsbtodb);
	}
	return 0;
}

static off_t
get_dev_size(char *dev_name)
{
	struct dkwedge_info dkw;
	struct partition *pp;
	struct disklabel lp;
	struct stat st;
	size_t ptn;

	/* Get info about partition/wedge */
	if (ioctl(fd, DIOCGWEDGEINFO, &dkw) != -1)
		return dkw.dkw_size;
	if (ioctl(fd, DIOCGDINFO, &lp) != -1) {
		ptn = strchr(dev_name, '\0')[-1] - 'a';
		if (ptn >= lp.d_npartitions)
			return 0;
		pp = &lp.d_partitions[ptn];
		return pp->p_size;
	}
	if (fstat(fd, &st) != -1 && S_ISREG(st.st_mode))
		return st.st_size / DEV_BSIZE;

	return 0;
}

/*
 * main().
 */
int
main(int argc, char **argv)
{
	int ch;
	int CheckOnlyFlag;
	int ExpertFlag;
	int SFlag;
	size_t i;

	char reply[5];

	newsize = 0;
	ExpertFlag = 0;
	SFlag = 0;
        CheckOnlyFlag = 0;

	while ((ch = getopt(argc, argv, "cps:vy")) != -1) {
		switch (ch) {
                case 'c':
			CheckOnlyFlag = 1;
			break;
		case 'p':
			progress = 1;
			break;
		case 's':
			SFlag = 1;
			newsize = strtoll(optarg, NULL, 10);
			if(newsize < 1) {
				usage();
			}
			break;
		case 'v':
			verbose = 1;
			break;
		case 'y':
			ExpertFlag = 1;
			break;
		case '?':
			/* FALLTHROUGH */
		default:
			usage();
		}
	}
	argc -= optind;
	argv += optind;

	if (argc != 1) {
		usage();
	}

	special = *argv;

	if (ExpertFlag == 0 && CheckOnlyFlag == 0) {
		printf("It's required to manually run fsck on file system "
		    "before you can resize it\n\n"
		    " Did you run fsck on your disk (Yes/No) ? ");
		fgets(reply, (int)sizeof(reply), stdin);
		if (strcasecmp(reply, "Yes\n")) {
			printf("\n Nothing done \n");
			exit(EXIT_SUCCESS);
		}
	}

	fd = open(special, O_RDWR, 0);
	if (fd < 0)
		err(EXIT_FAILURE, "Can't open `%s'", special);
	checksmallio();

	if (SFlag == 0) {
		newsize = get_dev_size(special);
		if (newsize == 0)
			err(EXIT_FAILURE,
			    "Can't resize file system, newsize not known.");
	}

	oldsb = (struct fs *) & sbbuf;
	newsb = (struct fs *) (SBLOCKSIZE + (char *) &sbbuf);
	for (where = search[i = 0]; search[i] != -1; where = search[++i]) {
		readat(where / DEV_BSIZE, oldsb, SBLOCKSIZE);
		switch (oldsb->fs_magic) {
		case FS_UFS2_MAGIC:
			is_ufs2 = 1;
			/* FALLTHROUGH */
		case FS_UFS1_MAGIC:
			needswap = 0;
			break;
		case FS_UFS2_MAGIC_SWAPPED:
 			is_ufs2 = 1;
			/* FALLTHROUGH */
		case FS_UFS1_MAGIC_SWAPPED:
			needswap = 1;
			break;
		default:
			continue;
		}
		if (!is_ufs2 && where == SBLOCK_UFS2)
			continue;
		break;
	}
	if (where == (off_t)-1)
		errx(EXIT_FAILURE, "Bad magic number");
	if (needswap)
		ffs_sb_swap(oldsb,oldsb);
	if (oldsb->fs_magic == FS_UFS1_MAGIC &&
	    (oldsb->fs_old_flags & FS_FLAGS_UPDATED) == 0) {
		oldsb->fs_csaddr = oldsb->fs_old_csaddr;
		oldsb->fs_size = oldsb->fs_old_size;
		oldsb->fs_dsize = oldsb->fs_old_dsize;
		oldsb->fs_cstotal.cs_ndir = oldsb->fs_old_cstotal.cs_ndir;
		oldsb->fs_cstotal.cs_nbfree = oldsb->fs_old_cstotal.cs_nbfree;
		oldsb->fs_cstotal.cs_nifree = oldsb->fs_old_cstotal.cs_nifree;
		oldsb->fs_cstotal.cs_nffree = oldsb->fs_old_cstotal.cs_nffree;
		/* any others? */
		printf("Resizing with ffsv1 superblock\n");
	}

	oldsb->fs_qbmask = ~(int64_t) oldsb->fs_bmask;
	oldsb->fs_qfmask = ~(int64_t) oldsb->fs_fmask;
	if (oldsb->fs_ipg % FFS_INOPB(oldsb))
		errx(EXIT_FAILURE, "ipg[%d] %% FFS_INOPB[%d] != 0",
		    (int) oldsb->fs_ipg, (int) FFS_INOPB(oldsb));
	/* The superblock is bigger than struct fs (there are trailing
	 * tables, of non-fixed size); make sure we copy the whole
	 * thing.  SBLOCKSIZE may be an over-estimate, but we do this
	 * just once, so being generous is cheap. */
	memcpy(newsb, oldsb, SBLOCKSIZE);

	if (progress) {
		progress_ttywidth(0);
		signal(SIGWINCH, progress_ttywidth);
	}

	loadcgs();

	if (progress && !CheckOnlyFlag) {
		progress_switch(progress);
		progress_init();
	}

	if (newsize > FFS_FSBTODB(oldsb, oldsb->fs_size)) {
		if (CheckOnlyFlag)
			exit(checkonly());
		grow();
	} else if (newsize < FFS_FSBTODB(oldsb, oldsb->fs_size)) {
		if (is_ufs2)
			errx(EXIT_FAILURE,"shrinking not supported for ufs2");
		if (CheckOnlyFlag)
			exit(checkonly());
		shrink();
	} else {
		if (CheckOnlyFlag)
			exit(checkonly());
		if (verbose)
			printf("No change requested: already %" PRId64
			    " blocks\n", (int64_t)oldsb->fs_size);
	}

	flush_cgs();
	write_sbs();
	if (isplainfile())
		ftruncate(fd,newsize * DEV_BSIZE);
	return 0;
}

static void
usage(void)
{

	(void)fprintf(stderr, "usage: %s [-cpvy] [-s size] special\n",
	    getprogname());
	exit(EXIT_FAILURE);
}
