/* $NetBSD: lfsv1.c,v 1.4.2.1 2012/06/03 21:42:52 jdc Exp $ */

#define	LIBSA_LFS
#define	REQUIRED_LFS_VERSION	1

#define	ufs_open		lfsv1_open
#define	ufs_close		lfsv1_close
#define	ufs_read		lfsv1_read
#define	ufs_write		lfsv1_write
#define	ufs_seek		lfsv1_seek
#define	ufs_stat		lfsv1_stat
#if defined(LIBSA_ENABLE_LS_OP)
#define	ufs_ls			lfsv1_ls
#endif

#define	fs_bsize		lfs_ibsize
#define	IFILE_Vx		IFILE_V1

#define	FSBTODB(fs, daddr)	(daddr)		/* LFSv1 uses sectors for addresses */
#define	INOPBx(fs) INOPB(fs)

#define	FSMOD			"lfs"

#include "lib/libsa/ufs.c"
