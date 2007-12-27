/*	$NetBSD: resize_ffs.c,v 1.10.6.1 2007/12/27 00:47:04 mjf Exp $	*/
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
 * Resize a filesystem.  Is capable of both growing and shrinking.
 *
 * Usage: resize_ffs filesystem newsize
 *
 * Example: resize_ffs /dev/rsd1e 29574
 *
 * newsize is in DEV_BSIZE units (ie, disk sectors, usually 512 bytes
 *  each).
 *
 * Note: this currently requires gcc to build, since it is written
 *  depending on gcc-specific features, notably nested function
 *  definitions (which in at least a few cases depend on the lexical
 *  scoping gcc provides, so they can't be trivially moved outside).
 *
 * It will not do anything useful with filesystems in other than
 *  host-native byte order.  This really should be fixed (it's largely
 *  a historical accident; the original version of this program is
 *  older than bi-endian support in FFS).
 *
 * Many thanks go to John Kohl <jtk@NetBSD.org> for finding bugs: the
 *  one responsible for the "realloccgblk: can't find blk in cyl"
 *  problem and a more minor one which left fs_dsize wrong when
 *  shrinking.  (These actually indicate bugs in fsck too - it should
 *  have caught and fixed them.)
 *
 */

#include <sys/cdefs.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <err.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <sys/param.h>		/* MAXFRAG */
#include <ufs/ffs/fs.h>
#include <ufs/ufs/dir.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ufs/ufs_bswap.h>	/* ufs_rw32 */

/* Suppress warnings about unused arguments */
#if	defined(__GNUC__) &&				\
	( (__GNUC__ > 2) ||				\
	  ( (__GNUC__ == 2) &&				\
	    defined(__GNUC_MINOR__) &&			\
	    (__GNUC_MINOR__ >= 7) ) )
#define UNUSED_ARG(x) x __unused
#define INLINE inline
#else
#define UNUSED_ARG(x) x
#define INLINE			/**/
#endif

/* new size of filesystem, in sectors */
static int newsize;

/* fd open onto disk device */
static int fd;

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
static char sbbuf[2 * SBLOCKSIZE] __attribute__((__aligned__(__alignof__(struct fs))));

/* a cg's worth of brand new squeaky-clean inodes */
static struct ufs1_dinode *zinodes;

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
static struct ufs1_dinode *inodes;

/* per-inode flags, indexed by inumber */
static unsigned char *iflags;
#define IF_DIRTY  0x01		/* needs to be written to disk */
#define IF_BDIRTY 0x02		/* like DIRTY, but is set on first inode in a
				 * block of inodes, and applies to the whole
				 * block. */

/* Old FFS1 macros */
#define cg_blktot(cgp, ns) \
    (cg_chkmagic(cgp, ns) ? \
    ((int32_t *)((u_int8_t *)(cgp) + ufs_rw32((cgp)->cg_old_btotoff, (ns)))) \
    : (((struct ocg *)(cgp))->cg_btot))
#define cg_blks(fs, cgp, cylno, ns) \
    (cg_chkmagic(cgp, ns) ? \
    ((int16_t *)((u_int8_t *)(cgp) + ufs_rw32((cgp)->cg_old_boff, (ns))) + \
	(cylno) * (fs)->fs_old_nrpos) \
    : (((struct ocg *)(cgp))->cg_b[cylno]))
#define cbtocylno(fs, bno) \
   (fsbtodb(fs, bno) / (fs)->fs_old_spc) 
#define cbtorpos(fs, bno) \
    ((fs)->fs_old_nrpos <= 1 ? 0 : \
     (fsbtodb(fs, bno) % (fs)->fs_old_spc / \
      (fs)->fs_old_nsect * (fs)->fs_old_trackskew + \
      fsbtodb(fs, bno) % (fs)->fs_old_spc % \
      (fs)->fs_old_nsect * (fs)->fs_old_interleave) %\
    (fs)->fs_old_nsect * (fs)->fs_old_nrpos / (fs)->fs_old_npsect)
#define dblksize(fs, dip, lbn) \
    (((lbn) >= NDADDR || (dip)->di_size >= lblktosize(fs, (lbn) + 1)) \
    ? (fs)->fs_bsize \
    : (fragroundup(fs, blkoff(fs, (dip)->di_size))))


/*
 * Number of disk sectors per block/fragment; assumes DEV_BSIZE byte
 * sector size. 
 */ 
#define NSPB(fs)	((fs)->fs_old_nspf << (fs)->fs_fragshift)
#define NSPF(fs)	((fs)->fs_old_nspf)

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
/*
 * Read size bytes starting at blkno into buf.  blkno is in DEV_BSIZE
 *  units, ie, after fsbtodb(); size is in bytes.
 */
static void
readat(off_t blkno, void *buf, int size)
{
	/* Seek to the correct place. */
	if (lseek(fd, blkno * DEV_BSIZE, L_SET) < 0)
		err(1, "lseek failed");

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
				err(1, "read failed");
			if (rv != n)
				errx(1, "read: wanted %d, got %d", n, rv);
			bp += n;
			left -= n;
		}
	} else {
		int rv;
		rv = read(fd, buf, size);
		if (rv < 0)
			err(1, "read failed");
		if (rv != size)
			errx(1, "read: wanted %d, got %d", size, rv);
	}
}
/*
 * Write size bytes from buf starting at blkno.  blkno is in DEV_BSIZE
 *  units, ie, after fsbtodb(); size is in bytes.
 */
static void
writeat(off_t blkno, const void *buf, int size)
{
	/* Seek to the correct place. */
	if (lseek(fd, blkno * DEV_BSIZE, L_SET) < 0)
		err(1, "lseek failed");
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
				err(1, "write failed");
			if (rv != n)
				errx(1, "write: wanted %d, got %d", n, rv);
			bp += n;
			left -= n;
		}
	} else {
		int rv;
		rv = write(fd, buf, size);
		if (rv < 0)
			err(1, "write failed");
		if (rv != size)
			errx(1, "write: wanted %d, got %d", size, rv);
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
	err(1, "Can't allocate %lu bytes for %s",
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
	err(1, "Can't re-allocate %lu bytes for %s",
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
	err(1, "Can't map %lu bytes for %s",
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
	cgs = nfmalloc(oldsb->fs_ncg * sizeof(struct cg *), "cg pointers");
	cgp = alloconce(oldsb->fs_ncg * cgblksz, "cgs");
	cgflags = nfmalloc(oldsb->fs_ncg, "cg flags");
	csums = nfmalloc(oldsb->fs_cssize, "cg summary");
	for (cg = 0; cg < oldsb->fs_ncg; cg++) {
		cgs[cg] = (struct cg *) cgp;
		readat(fsbtodb(oldsb, cgtod(oldsb, cg)), cgp, cgblksz);
		cgflags[cg] = 0;
		cgp += cgblksz;
	}
	readat(fsbtodb(oldsb, oldsb->fs_csaddr), csums, oldsb->fs_cssize);
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
		bzero(bitvec + (base >> 3), n >> 3);
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
INLINE static int
bit_is_set(unsigned char *bitvec, int bit)
{
	return (bitvec[bit >> 3] & (1 << (bit & 7)));
}
/*
 * Test whether bit #bit is clear in the bitmap pointed to by bitvec.
 */
INLINE static int
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
INLINE static int
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
INLINE static int
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
	int base;		/* Disk address of cg base */
	int dlow;		/* Size of pre-cg data area */
	int dhigh;		/* Offset of post-inode data area, from base */
	int dmax;		/* Offset of end of post-inode data area */
	int i;			/* Generic loop index */
	int n;			/* Generic count */

	cg = cgs[cgn];
	/* Place the data areas */
	base = cgbase(newsb, cgn);
	dlow = cgsblock(newsb, cgn) - base;
	dhigh = cgdmin(newsb, cgn) - base;
	dmax = newsb->fs_size - base;
	if (dmax > newsb->fs_fpg)
		dmax = newsb->fs_fpg;
	/*
         * Clear out the cg - assumes all-0-bytes is the correct way
         * to initialize fields we don't otherwise touch, which is
         * perhaps not the right thing to do, but it's what fsck and
         * mkfs do.
         */
	bzero(cg, newsb->fs_cgsize);
	cg->cg_time = newsb->fs_time;
	cg->cg_magic = CG_MAGIC;
	cg->cg_cgx = cgn;
	cg->cg_old_ncyl = newsb->fs_old_cpg;
	/* fsck whines if the cg->cg_old_ncyl value in the last cg is fs_old_cpg
	 * instead of zero, when fs_old_cpg is the correct value. */
	/* XXX fix once fsck is fixed */
	if ((cgn == newsb->fs_ncg - 1) /* && (newsb->fs_old_ncyl % newsb->fs_old_cpg) */ ) {
		cg->cg_old_ncyl = newsb->fs_old_ncyl % newsb->fs_old_cpg;
	}
	cg->cg_niblk = newsb->fs_ipg;
	cg->cg_ndblk = dmax;
	/* Set up the bitmap pointers.  We have to be careful to lay out the
	 * cg _exactly_ the way mkfs and fsck do it, since fsck compares the
	 * _entire_ cg against a recomputed cg, and whines if there is any
	 * mismatch, including the bitmap offsets. */
	/* XXX update this comment when fsck is fixed */
	cg->cg_old_btotoff = &cg->cg_space[0] - (unsigned char *) cg;
	cg->cg_old_boff = cg->cg_old_btotoff
	    + (newsb->fs_old_cpg * sizeof(int32_t));
	cg->cg_iusedoff = cg->cg_old_boff +
	    (newsb->fs_old_cpg * newsb->fs_old_nrpos * sizeof(int16_t));
	cg->cg_freeoff = cg->cg_iusedoff + howmany(newsb->fs_ipg, NBBY);
	if (newsb->fs_contigsumsize > 0) {
		cg->cg_nclusterblks = cg->cg_ndblk / newsb->fs_frag;
		cg->cg_clustersumoff = cg->cg_freeoff +
		    howmany(newsb->fs_old_cpg * newsb->fs_old_spc / NSPF(newsb),
		    NBBY) - sizeof(int32_t);
		cg->cg_clustersumoff =
		    roundup(cg->cg_clustersumoff, sizeof(int32_t));
		cg->cg_clusteroff = cg->cg_clustersumoff +
		    ((newsb->fs_contigsumsize + 1) * sizeof(int32_t));
		cg->cg_nextfreeoff = cg->cg_clusteroff +
		    howmany(newsb->fs_old_cpg * newsb->fs_old_spc / NSPB(newsb),
		    NBBY);
		n = dlow / newsb->fs_frag;
		if (n > 0) {
			set_bits(cg_clustersfree(cg, 0), 0, n);
			cg_clustersum(cg, 0)[(n > newsb->fs_contigsumsize) ?
			    newsb->fs_contigsumsize : n]++;
		}
	} else {
		cg->cg_nextfreeoff = cg->cg_freeoff +
		    howmany(newsb->fs_old_cpg * newsb->fs_old_spc / NSPF(newsb),
		    NBBY);
	}
	/* Mark the data areas as free; everything else is marked busy by the
	 * bzero up at the top. */
	set_bits(cg_blksfree(cg, 0), 0, dlow);
	set_bits(cg_blksfree(cg, 0), dhigh, dmax - dhigh);
	/* Initialize summary info */
	cg->cg_cs.cs_ndir = 0;
	cg->cg_cs.cs_nifree = newsb->fs_ipg;
	cg->cg_cs.cs_nbfree = dlow / newsb->fs_frag;
	cg->cg_cs.cs_nffree = 0;

	/* This is the simplest way of doing this; we perhaps could compute
	 * the correct cg_blktot()[] and cg_blks()[] values other ways, but it
	 * would be complicated and hardly seems worth the effort.  (The
	 * reason there isn't frag-at-beginning and frag-at-end code here,
	 * like the code below for the post-inode data area, is that the
	 * pre-sb data area always starts at 0, and thus is block-aligned, and
	 * always ends at the sb, which is block-aligned.) */
	for (i = 0; i < dlow; i += newsb->fs_frag) {
		cg_blktot(cg, 0)[cbtocylno(newsb, i)]++;
		cg_blks(newsb, cg, cbtocylno(newsb, i), 0)[cbtorpos(newsb, i)]++;
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
			cg_blktot(cg, 0)[cbtocylno(newsb, dhigh)]++;
			cg_blks(newsb, cg,
			    cbtocylno(newsb, dhigh), 0)[cbtorpos(newsb,
				dhigh)]++;
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
	/* Write out the cleared inodes. */
	writeat(fsbtodb(newsb, cgimin(newsb, cgn)), zinodes,
	    newsb->fs_ipg * sizeof(struct ufs1_dinode));
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
 * If we wrap off the end of the filesystem back to the beginning, we
 *  can end up searching the end of the filesystem twice.  I ignore
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
	int run;		/* length of run of free frags seen so far */
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
	cgsize = blknum(newsb, cgs[cgn]->cg_ndblk);
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
			cgsize = blknum(newsb, cgs[cgn]->cg_ndblk);
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
	 * within the filesystem.  I think this can never happen, since for
	 * the csum area to grow, we must be adding at least one cg, so the
	 * old csum area can't be this close to the end of the new filesystem.
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
			for ((i = newsb->fs_csaddr + ntot - nnew), (j = nnew); j > 0; i++, j--) {
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
	if (newloc < 0) {
		printf("Sorry, no space available for new csums\n");
		exit(1);
	}
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
		int dlow;	/* size of before-sb data area */
		int dhigh;	/* offset of post-inode data area */
		int dmax;	/* total size of cg */
		int base;	/* base of cg, since cgsblock() etc add it in */
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
 * Grow the filesystem.
 */
static void
grow(void)
{
	int i;

	/* Update the timestamp. */
	newsb->fs_time = timestamp();
	/* Allocate and clear the new-inode area, in case we add any cgs. */
	zinodes = alloconce(newsb->fs_ipg * sizeof(struct ufs1_dinode),
                            "zeroed inodes");
	bzero(zinodes, newsb->fs_ipg * sizeof(struct ufs1_dinode));
	/* Update the size. */
	newsb->fs_size = dbtofsb(newsb, newsize);
	/* Did we actually not grow?  (This can happen if newsize is less than
	 * a frag larger than the old size - unlikely, but no excuse to
	 * misbehave if it happens.) */
	if (newsb->fs_size == oldsb->fs_size)
		return;
	/* Check that the new last sector (frag, actually) is writable.  Since
	 * it's at least one frag larger than it used to be, we know we aren't
	 * overwriting anything important by this.  (The choice of sbbuf as
	 * what to write is irrelevant; it's just something handy that's known
	 * to be at least one frag in size.) */
	writeat(newsb->fs_size - 1, &sbbuf, newsb->fs_fsize);
	/* Update fs_old_ncyl and fs_ncg. */
	newsb->fs_old_ncyl = (newsb->fs_size * NSPF(newsb)) / newsb->fs_old_spc;
	newsb->fs_ncg = howmany(newsb->fs_old_ncyl, newsb->fs_old_cpg);
	/* Does the last cg end before the end of its inode area? There is no
	 * reason why this couldn't be handled, but it would complicate a lot
	 * of code (in all filesystem code - fsck, kernel, etc) because of the
	 * potential partial inode area, and the gain in space would be
	 * minimal, at most the pre-sb data area. */
	if (cgdmin(newsb, newsb->fs_ncg - 1) > newsb->fs_size) {
		newsb->fs_ncg--;
		newsb->fs_old_ncyl = newsb->fs_ncg * newsb->fs_old_cpg;
		newsb->fs_size = (newsb->fs_old_ncyl * newsb->fs_old_spc) / NSPF(newsb);
		printf("Warning: last cylinder group is too small;\n");
		printf("    dropping it.  New size = %lu.\n",
		    (unsigned long int) fsbtodb(newsb, newsb->fs_size));
	}
	/* Find out how big the csum area is, and realloc csums if bigger. */
	newsb->fs_cssize = fragroundup(newsb,
	    newsb->fs_ncg * sizeof(struct csum));
	if (newsb->fs_cssize > oldsb->fs_cssize)
		csums = nfrealloc(csums, newsb->fs_cssize, "new cg summary");
	/* If we're adding any cgs, realloc structures and set up the new cgs. */
	if (newsb->fs_ncg > oldsb->fs_ncg) {
		char *cgp;
		cgs = nfrealloc(cgs, newsb->fs_ncg * sizeof(struct cg *),
                                "cg pointers");
		cgflags = nfrealloc(cgflags, newsb->fs_ncg, "cg flags");
		bzero(cgflags + oldsb->fs_ncg, newsb->fs_ncg - oldsb->fs_ncg);
		cgp = alloconce((newsb->fs_ncg - oldsb->fs_ncg) * cgblksz,
                                "cgs");
		for (i = oldsb->fs_ncg; i < newsb->fs_ncg; i++) {
			cgs[i] = (struct cg *) cgp;
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
		int newcgsize;
		int prevcgtop;
		int oldcgsize;
		cg = cgs[oldsb->fs_ncg - 1];
		cgflags[oldsb->fs_ncg - 1] |= CGF_DIRTY | CGF_BLKMAPS;
		prevcgtop = oldsb->fs_fpg * (oldsb->fs_ncg - 1);
		newcgsize = newsb->fs_size - prevcgtop;
		if (newcgsize > newsb->fs_fpg)
			newcgsize = newsb->fs_fpg;
		oldcgsize = oldsb->fs_size % oldsb->fs_fpg;
		set_bits(cg_blksfree(cg, 0), oldcgsize, newcgsize - oldcgsize);
		cg->cg_old_ncyl = howmany(newcgsize * NSPF(newsb), newsb->fs_old_spc);
		cg->cg_ndblk = newcgsize;
	}
	/* Fix up the csum info, if necessary. */
	csum_fixup();
	/* Make fs_dsize match the new reality. */
	recompute_fs_dsize();
}
/*
 * Call (*fn)() for each inode, passing the inode and its inumber.  The
 *  number of cylinder groups is pased in, so this can be used to map
 *  over either the old or the new filesystem's set of inodes.
 */
static void
     map_inodes(void (*fn) (struct ufs1_dinode * di, unsigned int, void *arg), int ncg, void *cbarg) {
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

typedef void (*mark_callback_t) (unsigned int blocknum, unsigned int nfrags, unsigned int blksize, int opcode);

/* Helper function - handles a data block.  Calls the callback
 * function and returns number of bytes occupied in file (actually,
 * rounded up to a frag boundary).  The name is historical.  */
static int
markblk(mark_callback_t fn, struct ufs1_dinode * di, int bn, off_t o)
{
	int sz;
	int nb;
	if (o >= di->di_size)
		return (0);
	sz = dblksize(newsb, di, lblkno(newsb, o));
	nb = (sz > di->di_size - o) ? di->di_size - o : sz;
	if (bn)
		(*fn) (bn, numfrags(newsb, sz), nb, MDB_DATA);
	return (sz);
}
/* Helper function - handles an indirect block.  Makes the
 * MDB_INDIR_PRE callback for the indirect block, loops over the
 * pointers and recurses, and makes the MDB_INDIR_POST callback.
 * Returns the number of bytes occupied in file, as does markblk().
 * For the sake of update_for_data_move(), we read the indirect block
 * _after_ making the _PRE callback.  The name is historical.  */
static int
markiblk(mark_callback_t fn, struct ufs1_dinode * di, int bn, off_t o, int lev)
{
	int i;
	int j;
	int tot;
	static int32_t indirblk1[howmany(MAXBSIZE, sizeof(int32_t))];
	static int32_t indirblk2[howmany(MAXBSIZE, sizeof(int32_t))];
	static int32_t indirblk3[howmany(MAXBSIZE, sizeof(int32_t))];
	static int32_t *indirblks[3] = {
		&indirblk1[0], &indirblk2[0], &indirblk3[0]
	};
	if (lev < 0)
		return (markblk(fn, di, bn, o));
	if (bn == 0) {
		for (i = newsb->fs_bsize;
		    lev >= 0;
		    i *= NINDIR(newsb), lev--);
		return (i);
	}
	(*fn) (bn, newsb->fs_frag, newsb->fs_bsize, MDB_INDIR_PRE);
	readat(fsbtodb(newsb, bn), indirblks[lev], newsb->fs_bsize);
	tot = 0;
	for (i = 0; i < NINDIR(newsb); i++) {
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
map_inode_data_blocks(struct ufs1_dinode * di, mark_callback_t fn)
{
	off_t o;		/* offset within  inode */
	int inc;		/* increment for o - maybe should be off_t? */
	int b;			/* index within di_db[] and di_ib[] arrays */

	/* Scan the direct blocks... */
	o = 0;
	for (b = 0; b < NDADDR; b++) {
		inc = markblk(fn, di, di->di_db[b], o);
		if (inc == 0)
			break;
		o += inc;
	}
	/* ...and the indirect blocks. */
	if (inc) {
		for (b = 0; b < NIADDR; b++) {
			inc = markiblk(fn, di, di->di_ib[b], o, b);
			if (inc == 0)
				return;
			o += inc;
		}
	}
}

static void
dblk_callback(struct ufs1_dinode * di, unsigned int inum, void *arg)
{
	mark_callback_t fn;
	fn = (mark_callback_t) arg;
	switch (di->di_mode & IFMT) {
	case IFLNK:
		if (di->di_size > newsb->fs_maxsymlinklen) {
	case IFDIR:
	case IFREG:
			map_inode_data_blocks(di, fn);
		}
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
	int cg;
	struct ufs1_dinode *iptr;

	inodes = alloconce(oldsb->fs_ncg * oldsb->fs_ipg * sizeof(struct ufs1_dinode), "inodes");
	iflags = alloconce(oldsb->fs_ncg * oldsb->fs_ipg, "inode flags");
	bzero(iflags, oldsb->fs_ncg * oldsb->fs_ipg);
	iptr = inodes;
	for (cg = 0; cg < oldsb->fs_ncg; cg++) {
		readat(fsbtodb(oldsb, cgimin(oldsb, cg)), iptr,
		    oldsb->fs_ipg * sizeof(struct ufs1_dinode));
		iptr += oldsb->fs_ipg;
	}
}
/*
 * Report a filesystem-too-full problem.
 */
static void
toofull(void)
{
	printf("Sorry, would run out of data blocks\n");
	exit(1);
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
fragmove(struct cg * cg, int base, unsigned int start, unsigned int n)
{
	int i;
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
evict_data(struct cg * cg, unsigned int minfrag, unsigned int nfrag)
{
	int base;		/* base of cg (in frags from beginning of fs) */


	base = cgbase(oldsb, cg->cg_cgx);
	/* Does the boundary fall in the middle of a block?  To avoid breaking
	 * between frags allocated as consecutive, we always evict the whole
	 * block in this case, though one could argue we should check to see
	 * if the frag before or after the break is unallocated. */
	if (minfrag % oldsb->fs_frag) {
		int n;
		n = minfrag % oldsb->fs_frag;
		minfrag -= n;
		nfrag += n;
	}
	/* Do whole blocks.  If a block is wholly free, skip it; if wholly
	 * allocated, move it in toto.  If neither, call fragmove() to move
	 * the frags to new locations. */
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
 *  filesystem is self-consistent at all points, for better crash
 *  tolerance.  (We can get away with this only because all the writes
 *  done by perform_data_move() are writing into space that's not used
 *  by the old filesystem.)  If we crash, some things may point to the
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
		if ((blkmove[i] == i) ||
		    (run >= maxrun) ||
		    ((run > 0) &&
			(blkmove[i] != blkmove[i - 1] + 1))) {
			if (run > 0) {
				readat(fsbtodb(oldsb, i - run), &buf[0],
				    run << oldsb->fs_fshift);
				writeat(fsbtodb(oldsb, blkmove[i - run]),
				    &buf[0], run << oldsb->fs_fshift);
			}
			run = 0;
		}
		if (blkmove[i] != i)
			run++;
	}
	if (run > 0) {
		readat(fsbtodb(oldsb, i - run), &buf[0],
		    run << oldsb->fs_fshift);
		writeat(fsbtodb(oldsb, blkmove[i - run]), &buf[0],
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
		if (blkmove[*vec] != *vec) {
			*vec = blkmove[*vec];
			rv++;
		}
	}
	return (rv);
}
static void
moveblocks_callback(struct ufs1_dinode * di, unsigned int inum, void *arg)
{
	switch (di->di_mode & IFMT) {
	case IFLNK:
		if (di->di_size > oldsb->fs_maxsymlinklen) {
	case IFDIR:
	case IFREG:
			/* don't || these two calls; we need their
			 * side-effects */
			if (movemap_blocks(&di->di_db[0], NDADDR)) {
				iflags[inum] |= IF_DIRTY;
			}
			if (movemap_blocks(&di->di_ib[0], NIADDR)) {
				iflags[inum] |= IF_DIRTY;
			}
		}
		break;
	}
}

static void
moveindir_callback(unsigned int off, unsigned int nfrag, unsigned int nbytes, int kind)
{
	if (kind == MDB_INDIR_PRE) {
		int32_t blk[howmany(MAXBSIZE, sizeof(int32_t))];
		readat(fsbtodb(oldsb, off), &blk[0], oldsb->fs_bsize);
		if (movemap_blocks(&blk[0], NINDIR(oldsb))) {
			writeat(fsbtodb(oldsb, off), &blk[0], oldsb->fs_bsize);
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
	int i;
	int ni;
	int m;

	ni = newsb->fs_ipg * newsb->fs_ncg;
	m = INOPB(newsb) - 1;
	for (i = 0; i < ni; i++) {
		if (iflags[i] & IF_DIRTY) {
			iflags[i & ~m] |= IF_BDIRTY;
		}
	}
	m++;
	for (i = 0; i < ni; i += m) {
		if (iflags[i] & IF_BDIRTY) {
			writeat(fsbtodb(newsb, ino_to_fsba(newsb, i)),
			    inodes + i, newsb->fs_bsize);
		}
	}
}
/*
 * Evict all inodes from the specified cg.  shrink() already checked
 *  that there were enough free inodes, so the no-free-inodes check is
 *  a can't-happen.  If it does trip, the filesystem should be in good
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
		if (inodes[inum].di_mode != 0) {
			fi = find_freeinode();
			if (fi < 0) {
				printf("Sorry, inodes evaporated - "
				    "filesystem probably needs fsck\n");
				exit(1);
			}
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
	int i;
	int ni;

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

	rv = 0;
	while (nb > 0) {
		if (inomove[d->d_ino] != d->d_ino) {
			rv++;
			d->d_ino = inomove[d->d_ino];
		}
		nb -= d->d_reclen;
		buf += d->d_reclen;
	}
	return (rv);
#undef d
}
/*
 * Callback function for map_inode_data_blocks, for updating a
 *  directory to point to new inode locations.
 */
static void
update_dir_data(unsigned int bn, unsigned int size, unsigned int nb, int kind)
{
	if (kind == MDB_DATA) {
		union {
			struct direct d;
			char ch[MAXBSIZE];
		}     buf;
		readat(fsbtodb(oldsb, bn), &buf, size << oldsb->fs_fshift);
		if (update_dirents((char *) &buf, nb)) {
			writeat(fsbtodb(oldsb, bn), &buf,
			    size << oldsb->fs_fshift);
		}
	}
}
static void
dirmove_callback(struct ufs1_dinode * di, unsigned int inum, void *arg)
{
	switch (di->di_mode & IFMT) {
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
 * Shrink the filesystem.
 */
static void
shrink(void)
{
	int i;

	/* Load the inodes off disk - we'll need 'em. */
	loadinodes();
	/* Update the timestamp. */
	newsb->fs_time = timestamp();
	/* Update the size figures. */
	newsb->fs_size = dbtofsb(newsb, newsize);
	newsb->fs_old_ncyl = (newsb->fs_size * NSPF(newsb)) / newsb->fs_old_spc;
	newsb->fs_ncg = howmany(newsb->fs_old_ncyl, newsb->fs_old_cpg);
	/* Does the (new) last cg end before the end of its inode area?  See
	 * the similar code in grow() for more on this. */
	if (cgdmin(newsb, newsb->fs_ncg - 1) > newsb->fs_size) {
		newsb->fs_ncg--;
		newsb->fs_old_ncyl = newsb->fs_ncg * newsb->fs_old_cpg;
		newsb->fs_size = (newsb->fs_old_ncyl * newsb->fs_old_spc) / NSPF(newsb);
		printf("Warning: last cylinder group is too small;\n");
		printf("    dropping it.  New size = %lu.\n",
		    (unsigned long int) fsbtodb(newsb, newsb->fs_size));
	}
	/* Let's make sure we're not being shrunk into oblivion. */
	if (newsb->fs_ncg < 1) {
		printf("Size too small - filesystem would have no cylinders\n");
		exit(1);
	}
	/* Initialize for block motion. */
	blkmove_init();
	/* Update csum size, then fix up for the new size */
	newsb->fs_cssize = fragroundup(newsb,
	    newsb->fs_ncg * sizeof(struct csum));
	csum_fixup();
	/* Evict data from any cgs being wholly eliminated */
	for (i = newsb->fs_ncg; i < oldsb->fs_ncg; i++) {
		int base;
		int dlow;
		int dhigh;
		int dmax;
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
		oldcgsize = oldsb->fs_size - ((newsb->fs_ncg - 1) & oldsb->fs_fpg);
		if (oldcgsize > oldsb->fs_fpg)
			oldcgsize = oldsb->fs_fpg;
		evict_data(cg, newcgsize, oldcgsize - newcgsize);
		clr_bits(cg_blksfree(cg, 0), newcgsize, oldcgsize - newcgsize);
	}
	/* Find out whether we would run out of inodes.  (Note we haven't
	 * actually done anything to the filesystem yet; all those evict_data
	 * calls just update blkmove.) */
	{
		int slop;
		slop = 0;
		for (i = 0; i < newsb->fs_ncg; i++)
			slop += cgs[i]->cg_cs.cs_nifree;
		for (; i < oldsb->fs_ncg; i++)
			slop -= oldsb->fs_ipg - cgs[i]->cg_cs.cs_nifree;
		if (slop < 0) {
			printf("Sorry, would run out of inodes\n");
			exit(1);
		}
	}
	/* Copy data, then update pointers to data.  See the comment header on
	 * perform_data_move for ordering considerations. */
	perform_data_move();
	update_for_data_move();
	/* Now do inodes.  Initialize, evict, move, update - see the comment
	 * header on perform_inode_move. */
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
	/* Update the cg_old_ncyl value for the last cylinder.  The condition is
	 * commented out because fsck whines if not - see the similar
	 * condition in grow() for more. */
	/* XXX fix once fsck is fixed */
	/* if (newsb->fs_old_ncyl % newsb->fs_old_cpg) XXX */
/*XXXJTK*/
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
	bzero(&cg->cg_frsum[0], MAXFRAG * sizeof(cg->cg_frsum[0]));
	bzero(&cg_blktot(cg, 0)[0],
	    newsb->fs_old_cpg * sizeof(cg_blktot(cg, 0)[0]));
	bzero(&cg_blks(newsb, cg, 0, 0)[0],
	    newsb->fs_old_cpg * newsb->fs_old_nrpos *
	    sizeof(cg_blks(newsb, cg, 0, 0)[0]));
	if (newsb->fs_contigsumsize > 0) {
		cg->cg_nclusterblks = cg->cg_ndblk / newsb->fs_frag;
		bzero(&cg_clustersum(cg, 0)[1],
		    newsb->fs_contigsumsize *
		    sizeof(cg_clustersum(cg, 0)[1]));
		bzero(&cg_clustersfree(cg, 0)[0],
		    howmany((newsb->fs_old_cpg * newsb->fs_old_spc) / NSPB(newsb),
			NBBY));
	}
	/* Scan the free-frag bitmap.  Runs of free frags are kept track of
	 * with fragrun, and recorded into cg_frsum[] and cg_cs.cs_nffree; on
	 * each block boundary, entire free blocks are recorded as well. */
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
				cg_blktot(cg, 0)[cbtocylno(newsb, f - newsb->fs_frag)]++;
				cg_blks(newsb, cg,
				    cbtocylno(newsb, f - newsb->fs_frag),
				    0)[cbtorpos(newsb, f - newsb->fs_frag)]++;
				blkrun++;
			} else {
				if (fragrun > 0) {
					cg->cg_frsum[fragrun]++;
					cg->cg_cs.cs_nffree += fragrun;
				}
				if (newsb->fs_contigsumsize > 0) {
					if (blkrun > 0) {
						cg_clustersum(cg, 0)[(blkrun > newsb->fs_contigsumsize) ? newsb->fs_contigsumsize : blkrun]++;
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
	bzero(&cg_inosused(cg, 0)[0], howmany(newsb->fs_ipg, NBBY));
	inum = cgn * newsb->fs_ipg;
	if (cgn == 0) {
		set_bits(cg_inosused(cg, 0), 0, 2);
		iwc = 2;
		inum += 2;
	} else {
		iwc = 0;
	}
	for (; iwc < newsb->fs_ipg; iwc++, inum++) {
		switch (inodes[inum].di_mode & IFMT) {
		case 0:
			cg->cg_cs.cs_nifree++;
			break;
		case IFDIR:
			cg->cg_cs.cs_ndir++;
			/* fall through */
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
			writeat(fsbtodb(newsb, cgtod(newsb, i)), cgs[i],
			    cgblksz);
		}
	}
	writeat(fsbtodb(newsb, newsb->fs_csaddr), csums, newsb->fs_cssize);
}
/*
 * Write the superblock, both to the main superblock and to each cg's
 *  alternative superblock.
 */
static void
write_sbs(void)
{
	int i;

	writeat(where /  DEV_BSIZE, newsb, SBLOCKSIZE);
	for (i = 0; i < newsb->fs_ncg; i++) {
		writeat(fsbtodb(newsb, cgsblock(newsb, i)), newsb, SBLOCKSIZE);
	}
}
/*
 * main().
 */
int main(int, char **);
int
main(int ac, char **av)
{
	size_t i;
	if (ac != 3) {
		fprintf(stderr, "usage: %s filesystem new-size\n",
		    getprogname());
		exit(1);
	}
	fd = open(av[1], O_RDWR, 0);
	if (fd < 0)
		err(1, "Cannot open `%s'", av[1]);
	checksmallio();
	newsize = atoi(av[2]);
	oldsb = (struct fs *) & sbbuf;
	newsb = (struct fs *) (SBLOCKSIZE + (char *) &sbbuf);
	for (where = search[i = 0]; search[i] != -1; where = search[++i]) {
		readat(where / DEV_BSIZE, oldsb, SBLOCKSIZE);
		if (oldsb->fs_magic == FS_UFS1_MAGIC)
			break;
		if (where == SBLOCK_UFS2)
			continue;
		if (oldsb->fs_old_flags & FS_FLAGS_UPDATED)
			err(1, "Cannot resize ffsv2 format suberblock!");
	}
	if (where == (off_t)-1)
		errx(1, "Bad magic number");
	oldsb->fs_qbmask = ~(int64_t) oldsb->fs_bmask;
	oldsb->fs_qfmask = ~(int64_t) oldsb->fs_fmask;
	if (oldsb->fs_ipg % INOPB(oldsb)) {
		printf("ipg[%d] %% INOPB[%d] != 0\n", (int) oldsb->fs_ipg,
		    (int) INOPB(oldsb));
		exit(1);
	}
	/* The superblock is bigger than struct fs (there are trailing tables,
	 * of non-fixed size); make sure we copy the whole thing.  SBLOCKSIZE may
	 * be an over-estimate, but we do this just once, so being generous is
	 * cheap. */
	bcopy(oldsb, newsb, SBLOCKSIZE);
	loadcgs();
	if (newsize > fsbtodb(oldsb, oldsb->fs_size)) {
		grow();
	} else if (newsize < fsbtodb(oldsb, oldsb->fs_size)) {
		shrink();
	}
	flush_cgs();
	write_sbs();
	exit(0);
}
