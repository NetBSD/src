/*	$NetBSD: readufs_ffs.c,v 1.13.14.1 2014/08/20 00:03:28 tls Exp $	*/
/*	from Id: readufs_ffs.c,v 1.6 2003/04/08 09:19:32 itohy Exp 	*/

/*
 * FS specific support for 4.2BSD Fast Filesystem
 *
 * Written in 1999, 2002, 2003 by ITOH Yasufumi.
 * Public domain.
 *
 * Intended to be used for boot programs (first stage).
 * DON'T ADD ANY FANCY FEATURE.  THIS SHALL BE COMPACT.
 */

#include "readufs.h"

#include <ufs/ffs/fs.h>

static int get_ffs_inode(ino32_t ino, union ufs_dinode *dibuf);

#define fsi	(*ufsinfo)
#define fsi_ffs	fsi.fs_u.u_ffs

/*
 * Read and check superblock.
 * If it is an FFS, save information from the superblock.
 */
int
try_ffs(void)
{
	union {
		struct fs	sblk;
		unsigned char	pad[SBLOCKSIZE];
	} buf;
	struct ufs_info *ufsinfo = &ufs_info;
	static const int sblocs[] = SBLOCKSEARCH;
	const int *sbl;
	int magic;

#ifdef DEBUG_WITH_STDIO
	printf("trying FFS\n");
#endif
	/* read FFS superblock */
	for (sbl = sblocs; ; sbl++) {
		if (*sbl == -1)
			return 1;

		RAW_READ(&buf, (daddr_t) btodb(*sbl), SBLOCKSIZE);

		magic = buf.sblk.fs_magic;
#ifdef DEBUG_WITH_STDIO
		printf("FFS: sblk: pos %d magic 0x%x\n", btodb(*sbl), magic);
#endif
#ifdef USE_UFS1
		if (magic == FS_UFS1_MAGIC
		    && !(buf.sblk.fs_old_flags & FS_FLAGS_UPDATED)) {
			if (*sbl == SBLOCK_UFS2)
				/* might be an alternate suberblock */
				continue;
			break;
		}
#endif
		if (*sbl != buf.sblk.fs_sblockloc)
			/* must be an alternate suberblock */
			continue;

#ifdef USE_UFS1
		if (magic == FS_UFS1_MAGIC) {
			break;
		}
#endif
#ifdef USE_UFS2
		if (magic == FS_UFS2_MAGIC) {
#ifdef USE_UFS1
			fsi.ufstype = UFSTYPE_UFS2;
#endif
			break;
		}
#endif
	}

	/*
	 * XXX <ufs/ffs/fs.h> always uses fs_magic
	 * (UFS1 only or UFS2 only is impossible)
	 */
	fsi_ffs.magic = magic;
#ifdef DEBUG_WITH_STDIO
	printf("FFS: detected UFS%d format\n", (magic == FS_UFS2_MAGIC) + 1);
#endif

	/* This partition looks like an FFS. */
	fsi.fstype = UFSTYPE_FFS;
	fsi.get_inode = get_ffs_inode;

	/* Get information from the superblock. */
	fsi.bsize = buf.sblk.fs_bsize;
	fsi.fsbtodb = buf.sblk.fs_fsbtodb;
	fsi.nindir = buf.sblk.fs_nindir;

	fsi_ffs.iblkno = buf.sblk.fs_iblkno;
	fsi_ffs.old_cgoffset = buf.sblk.fs_old_cgoffset;
	fsi_ffs.old_cgmask = buf.sblk.fs_old_cgmask;
	fsi_ffs.fragshift = buf.sblk.fs_fragshift;
	fsi_ffs.inopb = buf.sblk.fs_inopb;
	fsi_ffs.ipg = buf.sblk.fs_ipg;
	fsi_ffs.fpg = buf.sblk.fs_fpg;

	return 0;
}

/* for inode macros */
#define fs_ipg		fs_u.u_ffs.ipg
#define fs_iblkno	fs_u.u_ffs.iblkno
#define fs_old_cgoffset	fs_u.u_ffs.old_cgoffset
#define fs_old_cgmask	fs_u.u_ffs.old_cgmask
#define fs_fpg		fs_u.u_ffs.fpg
#define fs_magic	fs_u.u_ffs.magic
#define fs_inopb	fs_u.u_ffs.inopb
#define fs_fragshift	fs_u.u_ffs.fragshift
#define fs_fsbtodb	fsbtodb

/*
 * Get inode from disk.
 */
static int
get_ffs_inode(ino32_t ino, union ufs_dinode *dibuf)
{
	struct ufs_info *ufsinfo = &ufs_info;
	union ufs_dinode *buf = alloca((size_t) fsi.bsize);
	union ufs_dinode *di;
	unsigned ioff;

	RAW_READ(buf, FFS_FSBTODB(&fsi, ino_to_fsba(&fsi, ino)),
			(size_t) fsi.bsize);

	ioff = ino_to_fsbo(&fsi, ino);

#if defined(USE_UFS1) && defined(USE_UFS2)
	if (ufsinfo->ufstype == UFSTYPE_UFS1)
		di = (void *) &(&buf->di1)[ioff];
	else {
		di = (void *) &(&buf->di2)[ioff];

		/* XXX for DI_SIZE() macro */
		di->di1.di_size = di->di2.di_size;
	}
#else
	di = &buf[ioff];
#endif

#ifdef DEBUG_WITH_STDIO
	printf("FFS: dinode(%d): mode 0%o, nlink %d, size %u\n",
		ino, di->di_common.di_mode, di->di_common.di_nlink,
		(unsigned) DI_SIZE(di));
#endif

	if (di->di_common.di_mode == 0)
		return 1;	/* unused inode (file is not found) */

	*dibuf = *di;

	return 0;
}
