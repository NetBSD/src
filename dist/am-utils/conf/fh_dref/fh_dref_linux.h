/*	$NetBSD: fh_dref_linux.h,v 1.1.1.2 2000/11/19 23:43:05 wiz Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_linux.h */
#define	NFS_FH_DREF(dst, src) memcpy((char *) &(dst.data), (char *) src, sizeof(struct nfs_fh))
