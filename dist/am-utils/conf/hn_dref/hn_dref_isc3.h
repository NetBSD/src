/*	$NetBSD: hn_dref_isc3.h,v 1.1.1.2 2000/11/19 23:43:05 wiz Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) { \
		strncpy((dst), (src), MAXHOSTNAMELEN); \
		(dst)[MAXHOSTNAMELEN] = '\0'; \
	}
