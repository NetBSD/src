/*	$NetBSD: fh_dref_linux.h,v 1.1.1.1 2008/09/19 20:07:17 christos Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_linux.h */
#define	NFS_FH_DREF(dst, src) memcpy((char *) &(dst.data), (char *) src, sizeof(struct nfs_fh))
