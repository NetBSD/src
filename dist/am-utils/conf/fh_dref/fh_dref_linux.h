/*	$NetBSD: fh_dref_linux.h,v 1.1.1.4 2001/05/13 17:50:17 veego Exp $	*/

/* $srcdir/conf/fh_dref/fh_dref_linux.h */
#define	NFS_FH_DREF(dst, src) memcpy((char *) &(dst.data), (char *) src, sizeof(struct nfs_fh))
