/*	$NetBSD: sa_dref_linux.h,v 1.1.1.1 2008/09/19 20:07:19 christos Exp $	*/

/* $srcdir/conf/sa_dref/sa_dref_linux.h */
#define	NFS_SA_DREF(dst, src) memmove((char *)&dst->addr, (char *) src, sizeof(struct sockaddr_in))
/* #undef NFS_ARGS_T_ADDR_IS_POINTER */
