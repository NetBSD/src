/*	$NetBSD: fh_dref_irix.h,v 1.1.1.1 2000/06/07 00:52:20 dogcow Exp $ */
/* $srcdir/conf/fh_dref/fh_dref_irix.h */
#define	NFS_FH_DREF(dst, src) (dst) = (fhandle_t *) (src)
