/*	$NetBSD: fh_dref_sunos4.h,v 1.1.1.4 2001/05/13 17:50:17 veego Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_sunos4.h */
#define	NFS_FH_DREF(dst, src) (dst) = (caddr_t) (src)
