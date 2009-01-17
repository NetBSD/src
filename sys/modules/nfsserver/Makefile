#	$NetBSD: Makefile,v 1.1.4.2 2009/01/17 13:29:30 mjf Exp $

.include "../Makefile.inc"

KMOD=	nfsserver

CPPFLAGS+=	-DNFS -DNFSSERVER

.PATH:	${S}/nfs

SRCS+=	nfs_export.c nfs_serv.c nfs_srvcache.c
SRCS+=	nfs_srvsubs.c nfs_srvsocket.c nfs_syscalls.c

.include <bsd.kmodule.mk>
