/*	$NetBSD: hn_dref_isc3.h,v 1.1.1.4 2001/05/13 17:50:17 veego Exp $	*/

/* $srcdir/conf/hn_dref/hn_dref_isc3.h */
#define NFS_HN_DREF(dst, src) { \
		strncpy((dst), (src), MAXHOSTNAMELEN); \
		(dst)[MAXHOSTNAMELEN] = '\0'; \
	}
