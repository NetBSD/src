/*	from Id: readufs_ffs.c,v 1.5 2002/01/26 16:26:44 itohy Exp 	*/

/*
 * FS specific support for 4.2BSD Fast Filesystem
 *
 * Written by ITOH, Yasufumi (itohy@netbsd.org).
 * Public domain.
 *
 * Intended to be used for boot programs (first stage).
 * DON'T ADD ANY FANCY FEATURE.  THIS SHALL BE COMPACT.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <ufs/ufs/dinode.h>
#include <ufs/ffs/fs.h>

#include "readufs.h"

static int get_ffs_inode __P((ino_t ino, struct dinode *dibuf));

#define fsi	(*ufsinfo)
#define fsi_ffs	fsi.fs_u.u_ffs

/*
 * Read and check superblock.
 * If it is an FFS, save information from the superblock.
 */
int
try_ffs()
{
	union {
		struct fs	sblk;
		unsigned char	pad[SBSIZE];
	} buf;
	struct ufs_info *ufsinfo = &ufs_info;

#ifdef DEBUG_WITH_STDIO
	printf("trying FFS\n");
#endif
	/* read FFS superblock */
	RAW_READ(&buf, SBLOCK, SBSIZE);

#ifdef DEBUG_WITH_STDIO
	printf("FFS: sblk: magic: 0x%x\n", buf.sblk.fs_magic);
#endif

	if (buf.sblk.fs_magic != FS_MAGIC)
		return 1;

	/* This partition looks like an FFS. */
	fsi.fstype = UFSTYPE_FFS;
	fsi.get_inode = get_ffs_inode;

	/* Get information from the superblock. */
	fsi.bsize = buf.sblk.fs_bsize;
	fsi.fsbtodb = buf.sblk.fs_fsbtodb;
	fsi.nindir = buf.sblk.fs_nindir;

	fsi_ffs.iblkno = buf.sblk.fs_iblkno;
	fsi_ffs.cgoffset = buf.sblk.fs_cgoffset;
	fsi_ffs.cgmask = buf.sblk.fs_cgmask;
	fsi_ffs.fragshift = buf.sblk.fs_fragshift;
	fsi_ffs.inopb = buf.sblk.fs_inopb;
	fsi_ffs.ipg = buf.sblk.fs_ipg;
	fsi_ffs.fpg = buf.sblk.fs_fpg;

	return 0;
}

/* for inode macros */
#define fs_ipg		fs_u.u_ffs.ipg
#define fs_iblkno	fs_u.u_ffs.iblkno
#define fs_cgoffset	fs_u.u_ffs.cgoffset
#define fs_cgmask	fs_u.u_ffs.cgmask
#define fs_fpg		fs_u.u_ffs.fpg
#define fs_inopb	fs_u.u_ffs.inopb
#define fs_fragshift	fs_u.u_ffs.fragshift
#define fs_fsbtodb	fsbtodb

/*
 * Get inode from disk.
 */
static int
get_ffs_inode(ino, dibuf)
	ino_t ino;
	struct dinode *dibuf;
{
	struct ufs_info *ufsinfo = &ufs_info;
	struct dinode *buf = alloca((size_t) fsi.bsize);
	struct dinode *di;

	RAW_READ(buf, fsbtodb(&fsi, ino_to_fsba(&fsi, ino)),
			(size_t) fsi.bsize);

	di = &buf[ino_to_fsbo(&fsi, ino)];
#ifdef DEBUG_WITH_STDIO
	printf("FFS: dinode(%d): mode 0%o, nlink %d, size %d, uid %d, gid %d, db[0] %d\n",
		ino, di->di_mode, di->di_nlink, (int)di->di_size,
		di->di_uid, di->di_gid, di->di_db[0]);
#endif

	if (di->di_mode == 0)
		return 1;	/* unused inode (file is not found) */

	*dibuf = *di;

	return 0;
}

