#	$NetBSD: Makefile,v 1.7.2.1 2009/01/19 13:20:20 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_subs.c nfs_iod.c nfs_vfsops.c nfs_vnops.c	\
	nfs_bootdhcp.c nfs_bootparam.c

CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnet/opt
CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnetinet/opt
CPPFLAGS+=	-DNFS -DNFS_BOOT_DHCP -DNFS_BOOT_BOOTPARAM
CFLAGS+=        -Wno-pointer-sign

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
