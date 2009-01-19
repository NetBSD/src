#	$NetBSD: Makefile,v 1.1.6.2 2009/01/19 13:20:21 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfsserver

SRCS=	nfs_export.c nfs_serv.c nfs_srvcache.c nfs_srvsubs.c		\
	nfs_srvsocket.c nfs_syscalls.c

CPPFLAGS+=	-DNFS
CFLAGS+=	-Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
