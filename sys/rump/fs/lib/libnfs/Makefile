#	$NetBSD: Makefile,v 1.8 2008/11/19 18:36:10 ad Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_subs.c nfs_iod.c nfs_vfsops.c nfs_vnops.c

SRCS+=	stubs.c

CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnet/opt
CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnetinet/opt
CPPFLAGS+=	-DNFS

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
