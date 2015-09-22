#	$NetBSD: Makefile,v 1.11.38.1 2015/09/22 12:06:14 skrll Exp $
#

.PATH:  ${.CURDIR}/../../../../nfs

LIB=	rumpfs_nfs

SRCS=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c		\
	nfs_socket.c nfs_clntsocket.c nfs_clntsubs.c nfs_subs.c		\
	nfs_iod.c nfs_vfsops.c nfs_vnops.c nfs_bootdhcp.c nfs_bootparam.c

CPPFLAGS+=	-DNFS_BOOT_DHCP -DNFS_BOOT_BOOTPARAM

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
