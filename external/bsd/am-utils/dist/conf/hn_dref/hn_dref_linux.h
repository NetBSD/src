/*	$NetBSD: hn_dref_linux.h,v 1.1.1.1.4.2 2008/10/19 22:39:35 haad Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_linux.h */
#define NFS_HN_DREF(dst, src) xstrlcpy((dst), (src), MAXHOSTNAMELEN)
