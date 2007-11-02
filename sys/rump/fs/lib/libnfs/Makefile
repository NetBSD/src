#	$NetBSD: Makefile,v 1.2.14.2 2007/11/02 12:43:39 joerg Exp $
#

.include <bsd.own.mk>

LIB=	nfs

.PATH:  ${NETBSDSRCDIR}/sys/nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_subs.c nfs_syscalls.c nfs_vfsops.c nfs_vnops.c

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
