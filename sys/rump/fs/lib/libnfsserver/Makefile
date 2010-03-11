#	$NetBSD: Makefile,v 1.1.10.3 2010/03/11 15:04:36 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfsserver

SRCS=	nfs_export.c nfs_serv.c nfs_srvcache.c nfs_srvsubs.c		\
	nfs_srvsocket.c nfs_syscalls.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
