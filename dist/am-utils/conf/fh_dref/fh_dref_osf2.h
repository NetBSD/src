/*	$NetBSD: fh_dref_osf2.h,v 1.1.1.1.4.2 2000/06/07 00:52:21 dogcow Exp $ */
/* $srcdir/conf/fh_dref/fh_dref_osf2.h */
#define	NFS_FH_DREF(dst, src) (dst) = (nfsv2fh_t *) (src)
