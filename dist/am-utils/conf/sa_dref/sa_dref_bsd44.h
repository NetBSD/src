/*	$NetBSD: sa_dref_bsd44.h,v 1.1.1.2 2000/11/19 23:43:12 wiz Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_bsd44.h */
#define	NFS_SA_DREF(dst, src) { \
		(dst)->addr = (struct sockaddr *) (src); \
		(dst)->addrlen = sizeof(*src); \
	}
