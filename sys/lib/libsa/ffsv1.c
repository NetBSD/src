/* $NetBSD: ffsv1.c,v 1.9 2022/04/24 06:52:59 mlelstv Exp $ */

#define LIBSA_FFSv1

#define ufs_open	ffsv1_open
#define ufs_close	ffsv1_close
#define ufs_read	ffsv1_read
#define ufs_write	ffsv1_write
#define ufs_seek	ffsv1_seek
#define ufs_stat	ffsv1_stat
#if defined(LIBSA_ENABLE_LS_OP)
#define ufs_ls		ffsv1_ls
#endif

#define ufs_dinode	ufs1_dinode
#define ufs_dinode_swap	ffs_dinode1_swap
#define ufs_indp_swap	bswap32
#define indp_t		int32_t

#define FS_MAGIC FS_UFS1_MAGIC

/* #define	FSMOD	"wapbl/ufs/ffs" */
#define	FSMOD	NULL

#include "ufs.c"
