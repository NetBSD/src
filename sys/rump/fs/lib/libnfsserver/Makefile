#	$NetBSD: Makefile,v 1.1 2008/11/27 14:14:30 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfsserver

SRCS=	nfs_export.c nfs_serv.c nfs_srvcache.c nfs_srvsubs.c		\
	nfs_srvsocket.c nfs_syscalls.c

CPPFLAGS+=	-DNFS
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
