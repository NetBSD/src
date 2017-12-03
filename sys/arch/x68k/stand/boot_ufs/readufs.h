/*	$NetBSD: readufs.h,v 1.10.14.2 2017/12/03 11:36:49 jdolecek Exp $	*/
/*	from Id: readufs.h,v 1.9 2003/10/15 14:16:58 itohy Exp 	*/

/*
 * Written in 1999, 2002, 2003 by ITOH Yasufumi.
 * Public domain.
 */

#include <sys/types.h>
#include <sys/param.h>
#ifdef USE_LFS
#include <ufs/lfs/lfs.h>
#endif
#include <ufs/ufs/dinode.h>
#include <ufs/lfs/lfs.h>
#include <ufs/ufs/dir.h>

/*
 * UFS1 / UFS2
 */
union ufs_dinode {
#ifdef USE_UFS1
	struct ufs1_dinode di1;
#endif
#ifdef USE_UFS2
	struct ufs2_dinode di2;
#endif
#ifdef USE_LFS
	struct lfs32_dinode dil32;
#endif
};

/* For more compact code and independence on 64-bit types and ops */
typedef uint32_t	ino32_t;

/* short-cut for common fields (di_mode, di_nlink) */
#ifdef USE_UFS1
# define di_common	di1
#elif defined USE_UFS2
# define di_common	di2
#endif

/* for fields of same names and different locations */
#if !(defined(USE_UFS1) && defined(USE_UFS2))
# ifdef USE_UFS1
#  define di_thisver	di1
# endif
# ifdef USE_UFS2
#  define di_thisver	di2
# endif
#endif

/* this is a size hack */
#if defined(USE_UFS1) && defined(USE_UFS2)
# define DI_SIZE(di)	((di)->di1.di_size)
#else
# define DI_SIZE(di)	((di)->di_thisver.di_size)
#endif
/* and may break following fields on UFS2 */
#define di_gid		di_gid__is_not_available
#define di_blksize	di_blksize__is_not_available

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
#if defined(USE_UFS1) && defined(USE_UFS2)
	enum ufs_ufstype {
		UFSTYPE_UFS1, UFSTYPE_UFS2
	} ufstype;
#endif
	int (*get_inode)(ino32_t ino, union ufs_dinode *dibuf);

	/* superblock information */
	u_int32_t bsize;	/* fs block size */
	u_int32_t nindir;	/* # indirect per block */
	u_int32_t fsbtodb;	/* block -> sector shift count */
	union {
#ifdef USE_FFS
		struct {
			daddr_t iblkno;		/* inode-block offset */
			int32_t old_cgoffset;	/* cylinder group offset */
			int32_t old_cgmask;	/* cylinder group mask */
			int32_t fragshift;	/* block to fragmentation */
			int32_t inopb;		/* # inodes per block */
			int32_t ipg;		/* # inodes per group */
			int32_t fpg;		/* # inodes per group * frag */
			int32_t magic;		/* FS_UFSx_MAGIC */
		} u_ffs;
#endif
#ifdef USE_LFS
		struct {
			u_int32_t version;	/* LFS version # */
			daddr_t idaddr;		/* ifile inode disk address */
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

void RAW_READ(void *buf, daddr_t blkpos, size_t bytelen);

size_t ufs_read(union ufs_dinode *di, void *buf, unsigned off,
    size_t count);
ino32_t ufs_lookup(ino32_t dirino, const char *fn);
ino32_t ufs_lookup_path(const char *path);
size_t ufs_load_file(void *buf, ino32_t dirino, const char *fn);
int ufs_init(void);

#ifdef USE_FFS
int try_ffs(void);
#endif

#ifdef USE_LFS
int try_lfs(void);
#endif

#ifdef DEBUG_WITH_STDIO
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#endif

#ifdef __GNUC__
# ifndef alloca
#  define alloca(n)	__builtin_alloca(n)
# endif
# ifndef strcmp
#  define strcmp(p, q)	__builtin_strcmp(p, q)
# endif
#endif
