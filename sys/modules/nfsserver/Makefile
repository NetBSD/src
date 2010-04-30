#	$NetBSD: Makefile,v 1.1.12.1 2010/04/30 14:44:17 uebayasi Exp $

.include "../Makefile.inc"

KMOD=	nfsserver

CPPFLAGS+=	-DNFSSERVER

.PATH:	${S}/nfs

SRCS+=	nfs_export.c nfs_serv.c nfs_srvcache.c
SRCS+=	nfs_srvsubs.c nfs_srvsocket.c nfs_syscalls.c

.include <bsd.kmodule.mk>
