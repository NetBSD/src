/*	$NetBSD: fh_dref_osf2.h,v 1.1.1.4 2001/05/13 17:50:17 veego Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_osf2.h */
#define	NFS_FH_DREF(dst, src) (dst) = (nfsv2fh_t *) (src)
