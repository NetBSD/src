#	$NetBSD: Makefile,v 1.2 2010/02/16 20:42:46 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfsserver

SRCS=	nfs_export.c nfs_serv.c nfs_srvcache.c nfs_srvsubs.c		\
	nfs_srvsocket.c nfs_syscalls.c

CPPFLAGS+=	-DNFS

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
