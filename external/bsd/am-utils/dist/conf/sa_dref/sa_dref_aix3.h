/*	$NetBSD: sa_dref_aix3.h,v 1.1.1.1.4.2 2008/10/19 22:39:38 haad Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_aix3.h */
#define	NFS_SA_DREF(dst, src) (dst)->addr = *(src)
/* #undef NFS_ARGS_T_ADDR_IS_POINTER */
