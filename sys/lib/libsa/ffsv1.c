/* $NetBSD: ffsv1.c,v 1.1.2.2 2004/09/18 14:54:02 skrll Exp $ */

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
