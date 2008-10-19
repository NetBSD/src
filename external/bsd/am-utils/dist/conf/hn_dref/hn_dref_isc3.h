/*	$NetBSD: hn_dref_isc3.h,v 1.1.1.1.4.2 2008/10/19 22:39:35 haad Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) xstrlcpy((dst), (src), MAXHOSTNAMELEN)
