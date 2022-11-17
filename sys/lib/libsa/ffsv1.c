/* $NetBSD: ffsv1.c,v 1.10 2022/11/17 06:40:39 chs Exp $ */

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

#include "ufs.c"
