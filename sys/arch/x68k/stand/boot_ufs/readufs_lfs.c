/*	$Id: readufs_lfs.c,v 1.1 2001/09/27 10:14:50 minoura Exp $	*/

/*
 * FS specific support for 4.4BSD Log-structured Filesystem
 *
 * Written by ITOH, Yasufumi (itohy@netbsd.org).
 * Public domain.
 *
 * Intended to be used for boot programs (first stage).
 * DON'T ADD ANY FANCY FEATURE.  THIS SHALL BE COMPACT.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/mount.h>
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>

#include "readufs.h"

static int get_lfs_inode __P((ino_t ino, struct dinode *dibuf));

static struct dinode	ifile_dinode;

#define fsi	(*ufsinfo)
#define fsi_lfs	fsi.fs_u.u_lfs

/*
 * Read and check superblock.
 * If it is an LFS, save information from the superblock.
 */
int
try_lfs()
{
	struct ufs_info *ufsinfo = &ufs_info;
	struct dlfs	sblk, sblk2;
	struct dlfs	*s = &sblk;

#ifdef DEBUG_WITH_STDIO
	printf("trying LFS\n");
#endif
	/* read primary superblock */
	RAW_READ(&sblk, btodb(LFS_LABELPAD), sizeof sblk);

#ifdef DEBUG_WITH_STDIO
	printf("LFS: sblk: magic: 0x%x, version: %d\n",
		sblk.dlfs_magic, sblk.dlfs_version);
#endif

	if (sblk.dlfs_magic != LFS_MAGIC)
		return 1;

#ifdef DEBUG_WITH_STDIO
	printf("sboff[1]: %d\n", sblk.dlfs_sboffs[1]);
#endif

	if (sblk.dlfs_sboffs[1] > 0) {
		/* read secondary superblock */
		RAW_READ(&sblk2,
			 (unsigned int) sblk.dlfs_sboffs[1], sizeof sblk2);

#ifdef DEBUG_WITH_STDIO
		printf("LFS: sblk2: magic: 0x%x, version: %d\n",
			sblk2.dlfs_magic, sblk2.dlfs_version);
#endif

		if (sblk2.dlfs_magic == LFS_MAGIC &&
		    sblk.dlfs_tstamp > sblk2.dlfs_tstamp)
			s = &sblk2;
	}

	/* This partition looks like an LFS. */
	fsi.get_inode = get_lfs_inode;
	/* Disk addr in inode is in disk sector --- no shifting. */
	fsi.iblkshift = 0;

	/* Get information from the superblock. */
	fsi.bsize = s->dlfs_bsize;
	fsi.fsbtodb = s->dlfs_fsbtodb;
	fsi.nindir = s->dlfs_nindir;

	fsi_lfs.idaddr = s->dlfs_idaddr;
	fsi_lfs.inopb = s->dlfs_inopb;
	fsi_lfs.ifpb = s->dlfs_ifpb;
	fsi_lfs.cleansz = s->dlfs_cleansz;
	fsi_lfs.segtabsz = s->dlfs_segtabsz;

	/* ifile is always used to look-up other inodes, so keep its inode. */
	if (get_lfs_inode(LFS_IFILE_INUM, &ifile_dinode))
		return 1;	/* OOPS, failed to find inode of ifile! */

	fsi.fstype = UFSTYPE_LFS;

	return 0;
}

/*
 * Get inode from disk.
 */
static int
get_lfs_inode(ino, dibuf)
	ino_t ino;
	struct dinode *dibuf;
{
	struct ufs_info *ufsinfo = &ufs_info;
	ufs_daddr_t daddr;
	char *buf = alloca(fsi.bsize);
	struct dinode *di;
	int cnt;

	/* Get fs block which contains the specified inode. */
	if (ino == LFS_IFILE_INUM)
		daddr = fsi_lfs.idaddr;
	else {
#ifdef DEBUG_WITH_STDIO
	printf("LFS: ino: %d\nifpb: %d, cleansz: %d, segtabsz: %d, bsize: %d\n",
	    ino, fsi_lfs.ifpb, fsi_lfs.cleansz, fsi_lfs.segtabsz, fsi.bsize);
#endif
		ufs_read(&ifile_dinode, buf, 
			 ino / fsi_lfs.ifpb + fsi_lfs.cleansz +
				fsi_lfs.segtabsz,
			 fsi.bsize);
		daddr = ((IFILE *) buf + ino % fsi_lfs.ifpb)->if_daddr;
	}
#ifdef DEBUG_WITH_STDIO
	printf("LFS(%d): daddr: %d\n", ino, daddr);
#endif

	if (daddr == LFS_UNUSED_DADDR)
		return 1;

	/* Read the inode block. */
	RAW_READ(buf, (unsigned int) daddr, fsi.bsize);

	/* Search for the inode. */
	cnt = fsi_lfs.inopb;
	di = (struct dinode *) buf + (cnt - 1);

	for ( ; cnt--; di--)
		if (di->di_inumber == ino)
			goto found;
	/* not found */
	return 1;

found:
#ifdef DEBUG_WITH_STDIO
	printf("LFS: dinode(%d): mode 0%o, nlink %d, inumber %d, size %d, uid %d, gid %d, db[0] %d\n",
		ino, di->di_mode, di->di_nlink, di->di_inumber,
		(int)di->di_size, di->di_uid, di->di_gid, di->di_db[0]);
#endif

	*dibuf = *di;

	return 0;
}
