/* $NetBSD: ffsv1.c,v 1.1.2.1 2004/08/03 10:53:53 skrll Exp $ */

#define LIBSA_FFSv1

#define ufs_open	ffsv1_open
#define ufs_close	ffsv1_close
#define ufs_read	ffsv1_read
#define ufs_write	ffsv1_write
#define ufs_seek	ffsv1_seek
#define ufs_stat	ffsv1_stat

#define ufs_dinode	ufs1_dinode
#define indp_t		int32_t

#include "ufs.c"
