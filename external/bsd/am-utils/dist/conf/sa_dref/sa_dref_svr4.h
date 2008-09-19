/*	$NetBSD: sa_dref_svr4.h,v 1.1.1.1 2008/09/19 20:07:19 christos Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_svr4.h */
#define	NFS_SA_DREF(dst, src) { \
		(dst)->addr->buf = (char *) (src); \
		(dst)->addr->len = sizeof(struct sockaddr_in); \
		(dst)->addr->maxlen = sizeof(struct sockaddr_in); \
	}
#define NFS_ARGS_T_ADDR_IS_POINTER 1
