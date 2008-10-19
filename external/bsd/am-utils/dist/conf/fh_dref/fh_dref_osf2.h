/*	$NetBSD: fh_dref_osf2.h,v 1.1.1.1.4.2 2008/10/19 22:39:34 haad Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_osf2.h */
#define	NFS_FH_DREF(dst, src) (dst) = (nfsv2fh_t *) (src)
