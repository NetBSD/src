#	$NetBSD: Makefile,v 1.6 2008/10/16 19:06:40 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_subs.c nfs_syscalls.c nfs_vfsops.c nfs_vnops.c

SRCS+=	stubs.c

CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnet/opt
CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnetinet/opt

.include <bsd.lib.mk>
.include <bsd.klinks.mk>

CFLAGS+=	-Wno-stack-protector
