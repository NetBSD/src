/* $NetBSD: ffsv2.c,v 1.3.80.1 2008/12/13 01:15:18 haad Exp $ */

#define LIBSA_FFSv2

#define ufs_open	ffsv2_open
#define ufs_close	ffsv2_close
#define ufs_read	ffsv2_read
#define ufs_write	ffsv2_write
#define ufs_seek	ffsv2_seek
#define ufs_stat	ffsv2_stat

#define ufs_dinode	ufs2_dinode
#define indp_t		int64_t

#define	FSMOD		"ffs"

#include "ufs.c"
