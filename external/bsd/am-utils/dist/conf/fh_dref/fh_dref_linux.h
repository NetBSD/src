/*	$NetBSD: fh_dref_linux.h,v 1.1.1.1.4.2 2008/10/19 22:39:34 haad Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_linux.h */
#define	NFS_FH_DREF(dst, src) memcpy((char *) &(dst.data), (char *) src, sizeof(struct nfs_fh))
