/*	$NetBSD: fh_dref_osf2.h,v 1.1.1.2 2000/11/19 23:43:05 wiz Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_osf2.h */
#define	NFS_FH_DREF(dst, src) (dst) = (nfsv2fh_t *) (src)
