#	$NetBSD: Makefile,v 1.2.62.1 2019/06/10 22:09:34 christos Exp $

.include "../Makefile.inc"

KMOD=	nfsserver

CPPFLAGS+=	-DNFSSERVER

.PATH:	${S}/nfs

SRCS+=	nfs_export.c nfs_serv.c nfs_srvcache.c
SRCS+=	nfs_srvsubs.c nfs_srvsocket.c nfs_syscalls.c

WARNS=	3

.include <bsd.kmodule.mk>
