/*	$NetBSD: hn_dref_isc3.h,v 1.1.1.4.8.1 2005/08/16 13:02:14 tron Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) xstrlcpy((dst), (src), MAXHOSTNAMELEN)
