/* $NetBSD: ffsv2.c,v 1.1.2.2 2004/09/18 14:54:02 skrll Exp $ */

#define LIBSA_FFSv2

#define ufs_open	ffsv2_open
#define ufs_close	ffsv2_close
#define ufs_read	ffsv2_read
#define ufs_write	ffsv2_write
#define ufs_seek	ffsv2_seek
#define ufs_stat	ffsv2_stat

#define ufs_dinode	ufs2_dinode
#define indp_t		int64_t

#include "ufs.c"
