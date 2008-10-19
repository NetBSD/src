/*	$NetBSD: sa_dref_linux.h,v 1.1.1.1.4.2 2008/10/19 22:39:38 haad Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_linux.h */
#define	NFS_SA_DREF(dst, src) memmove((char *)&dst->addr, (char *) src, sizeof(struct sockaddr_in))
/* #undef NFS_ARGS_T_ADDR_IS_POINTER */
