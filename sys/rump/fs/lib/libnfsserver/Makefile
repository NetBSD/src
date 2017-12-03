#	$NetBSD: Makefile,v 1.3.20.1 2017/12/03 11:39:12 jdolecek Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfsserver
COMMENT=NFS server

SRCS=	nfs_export.c nfs_serv.c nfs_srvcache.c nfs_srvsubs.c		\
	nfs_srvsocket.c nfs_syscalls.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
