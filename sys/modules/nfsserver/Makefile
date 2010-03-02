#	$NetBSD: Makefile,v 1.2 2010/03/02 23:37:16 pooka Exp $

.include "../Makefile.inc"

KMOD=	nfsserver

CPPFLAGS+=	-DNFSSERVER

.PATH:	${S}/nfs

SRCS+=	nfs_export.c nfs_serv.c nfs_srvcache.c
SRCS+=	nfs_srvsubs.c nfs_srvsocket.c nfs_syscalls.c

.include <bsd.kmodule.mk>
