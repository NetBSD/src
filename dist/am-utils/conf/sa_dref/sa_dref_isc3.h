/*	$NetBSD: sa_dref_isc3.h,v 1.1.1.1 2000/06/07 00:52:21 dogcow Exp $ */
/* $srcdir/conf/sa_dref/sa_dref_isc3.h */
#define	NFS_SA_DREF(dst, src) { \
		(dst)->raddr.buf = (char *) (src); \
		(dst)->raddr.len = sizeof(struct sockaddr_in); \
		(dst)->trans = 1; \
	}
