/* $NetBSD: ffsv1.c,v 1.6.40.1 2020/04/13 08:05:04 martin Exp $ */

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
#define indp_t		int32_t

#if 0
#define	FSMOD	"wapbl/ufs/ffs"
#endif

#include "ufs.c"
