/*	$NetBSD: sa_dref_aix3.h,v 1.1.1.1 2008/09/19 20:07:19 christos Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_aix3.h */
#define	NFS_SA_DREF(dst, src) (dst)->addr = *(src)
/* #undef NFS_ARGS_T_ADDR_IS_POINTER */
