/*	$NetBSD: fh_dref_aix3.h,v 1.1.1.1 2000/06/07 00:52:20 dogcow Exp $ */
/* $srcdir/conf/fh_dref/fh_dref_aix3.h */
#define	NFS_FH_DREF(dst, src) memcpy((char *) &(dst.x), (char *) src, sizeof(struct nfs_fh))
