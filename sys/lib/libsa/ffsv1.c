/* $NetBSD: ffsv1.c,v 1.3.84.1 2009/01/19 13:19:57 skrll Exp $ */

#define LIBSA_FFSv1

#define ufs_open	ffsv1_open
#define ufs_close	ffsv1_close
#define ufs_read	ffsv1_read
#define ufs_write	ffsv1_write
#define ufs_seek	ffsv1_seek
#define ufs_stat	ffsv1_stat

#define ufs_dinode	ufs1_dinode
#define indp_t		int32_t

#define	FSMOD		"ffs"

#include "ufs.c"
