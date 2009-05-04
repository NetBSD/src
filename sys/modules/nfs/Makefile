#	$NetBSD: Makefile,v 1.1.10.2 2009/05/04 08:14:11 yamt Exp $

.include "../Makefile.inc"

KMOD=	nfs

CPPFLAGS+=	-DNFS -DNFS_BOOT_DHCP -DNFS_BOOT_BOOTPARAM -DARP=1

.PATH:	${S}/nfs

SRCS+=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c
SRCS+=	nfs_socket.c nfs_subs.c nfs_iod.c nfs_vnops.c
SRCS+=	nfs_vfsops.c nfs_bootdhcp.c nfs_bootparam.c 

.include <bsd.kmodule.mk>
