#	$NetBSD: Makefile,v 1.9.4.1 2010/04/30 14:44:28 uebayasi Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_clntsocket.c nfs_clntsubs.c nfs_subs.c		\
	nfs_iod.c nfs_vfsops.c nfs_vnops.c nfs_bootdhcp.c nfs_bootparam.c

CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnet/opt
CPPFLAGS+=	-I${.CURDIR}/../../../net/lib/libnetinet/opt
CPPFLAGS+=	-DNFS_BOOT_DHCP -DNFS_BOOT_BOOTPARAM

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
