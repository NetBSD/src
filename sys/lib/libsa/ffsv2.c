/* $NetBSD: ffsv2.c,v 1.7.16.1 2021/05/31 22:15:21 cjep Exp $ */

#define LIBSA_FFSv2

#define ufs_open	ffsv2_open
#define ufs_close	ffsv2_close
#define ufs_read	ffsv2_read
#define ufs_write	ffsv2_write
#define ufs_seek	ffsv2_seek
#define ufs_stat	ffsv2_stat
#if defined(LIBSA_ENABLE_LS_OP)
#define ufs_ls		ffsv2_ls
#endif

#define ufs_dinode	ufs2_dinode
#define ufs_dinode_swap	ffs_dinode2_swap
#define ufs_indp_swap	bswap64
#define indp_t		int64_t

#define FS_MAGIC FS_UFS2_MAGIC

#if 0
#define	FSMOD	"wapbl/ufs/ffs"
#endif

#include "ufs.c"
