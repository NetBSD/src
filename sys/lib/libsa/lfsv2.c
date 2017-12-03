/* $NetBSD: lfsv2.c,v 1.5.2.3 2017/12/03 11:38:46 jdolecek Exp $ */

#define	LIBSA_LFS
#define	REQUIRED_LFS_VERSION	2

#define	ufs_open		lfsv2_open
#define	ufs_close		lfsv2_close
#define	ufs_read		lfsv2_read
#define	ufs_write		lfsv2_write
#define	ufs_seek		lfsv2_seek
#define	ufs_stat		lfsv2_stat
#if defined(LIBSA_ENABLE_LS_OP)
#define	ufs_ls			lfsv2_ls
#endif

#define ufs_dinode		lfs32_dinode

#define	fs_bsize		lfs_dlfs_u.u_32.dlfs_bsize

#ifdef LFS_IFILE_FRAG_ADDRESSING	/* XXX see sys/ufs/lfs/ -- not tested */
#define	INOPBx(fs) LFS_INOPF(fs)
#else
#define	INOPBx(fs) LFS_INOPB(fs)
#endif

#define UFS_NINDIR		LFS_NINDIR
#define ufs_blkoff(a, b)	lfs_blkoff((a), (b))
#define ufs_lblkno(a, b)	lfs_lblkno((a), (b))
#define dblksize(a, b, c)	lfs_dblksize((a), (b), (c))
#define FSBTODB(a, b)		LFS_FSBTODB((a), (b))

#define	FSMOD			"lfs"

#include "lib/libsa/ufs.c"
