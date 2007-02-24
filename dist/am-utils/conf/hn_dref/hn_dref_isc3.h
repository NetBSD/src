/*	$NetBSD: hn_dref_isc3.h,v 1.1.1.4.8.2 2007/02/24 12:17:10 bouyer Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) xstrlcpy((dst), (src), MAXHOSTNAMELEN)
