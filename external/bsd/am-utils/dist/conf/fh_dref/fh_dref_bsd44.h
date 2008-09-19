/*	$NetBSD: fh_dref_bsd44.h,v 1.1.1.1 2008/09/19 20:07:17 christos Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_bsd44.h */
#define	NFS_FH_DREF(dst, src) (dst) = (nfsv2fh_t *) (src)
