#	$NetBSD: Makefile,v 1.2.30.1 2008/09/28 10:41:02 mjf Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_subs.c nfs_syscalls.c nfs_vfsops.c nfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
