/*	from Id: readufs.h,v 1.7 2002/01/26 15:55:51 itohy Exp 	*/

/*
 * Written by ITOH, Yasufumi (itohy@netbsd.org)
 * Public domain.
 */

#include <ufs/ufs/dir.h>

/*
 * filesystem information
 */
struct ufs_info {
	enum ufs_fstype {
		UFSTYPE_UNKNOWN
#ifdef USE_FFS
		, UFSTYPE_FFS
#endif
#ifdef USE_LFS
		, UFSTYPE_LFS
#endif
	} fstype;
	int (*get_inode) __P((ino_t ino, struct dinode *dibuf));

	/* superblock information */
	u_int32_t bsize;	/* fs block size */
	u_int32_t nindir;	/* # indirect per block */
	u_int32_t fsbtodb;	/* block -> sector shift count */
	union {
#ifdef USE_FFS
		struct {
			ufs_daddr_t iblkno;	/* inode-block offset */
			int32_t cgoffset;	/* cylinder group offset */
			int32_t cgmask;		/* cylinder group mask */
			int32_t fragshift;	/* block to fragmentation */
			int32_t inopb;		/* # inodes per block */
			int32_t ipg;		/* # inodes per group */
			int32_t fpg;		/* # inodes per group * frag */
		} u_ffs;
#endif
#ifdef USE_LFS
		struct {
			u_int32_t version;	/* LFS version # */
			ufs_daddr_t idaddr;	/* ifile inode disk address */
			u_int32_t inopb;	/* inodes per block (v1) */
						/* inodes per frag (v2) */
			u_int32_t ifpb;		/* inode addrs / ifile block */
			u_int32_t ioffset;	/* start of inode in ifile */
						/* (in sector) */
			u_int32_t ibsize;	/* size of inode block */
		} u_lfs;
#endif
	} fs_u;
};

extern struct ufs_info	ufs_info;
#define ufs_get_inode(ino, di)	((*ufs_info.get_inode)((ino), (di)))

void RAW_READ __P((void *buf, ufs_daddr_t blkpos, size_t bytelen));

size_t ufs_read __P((struct dinode *di, void *buf, unsigned off, size_t count));
ino_t ufs_lookup __P((ino_t dirino, const char *fn));
ino_t ufs_lookup_path __P((const char *path));
size_t ufs_load_file __P((void *buf, ino_t dirino, const char *fn));
int ufs_init __P((void));

#ifdef USE_FFS
int try_ffs __P((void));
#endif

#ifdef USE_LFS
int try_lfs __P((void));
#endif

#ifdef DEBUG_WITH_STDIO
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#endif
