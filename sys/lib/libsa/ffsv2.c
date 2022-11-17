/* $NetBSD: ffsv2.c,v 1.10 2022/11/17 06:40:39 chs Exp $ */

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

#include "ufs.c"
