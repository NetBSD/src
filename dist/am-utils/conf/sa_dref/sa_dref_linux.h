/*	$NetBSD: sa_dref_linux.h,v 1.1.1.4 2001/05/13 17:50:23 veego Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_linux.h */
#define	NFS_SA_DREF(dst, src) memmove((char *)&dst->addr, (char *) src, sizeof(struct sockaddr_in))
