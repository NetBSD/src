/*	$NetBSD: sa_dref_isc3.h,v 1.1.1.2 2000/11/19 23:43:12 wiz Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_isc3.h */
#define	NFS_SA_DREF(dst, src) { \
		(dst)->raddr.buf = (char *) (src); \
		(dst)->raddr.len = sizeof(struct sockaddr_in); \
		(dst)->trans = 1; \
	}
