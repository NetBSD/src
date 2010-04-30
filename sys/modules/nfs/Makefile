#	$NetBSD: Makefile,v 1.1.12.1 2010/04/30 14:44:17 uebayasi Exp $

.include "../Makefile.inc"

KMOD=	nfs

CPPFLAGS+=	-DNFS_BOOT_DHCP -DNFS_BOOT_BOOTPARAM -DARP=1

.PATH:	${S}/nfs

SRCS+=	krpc_subr.c nfs_bio.c nfs_boot.c nfs_kq.c nfs_node.c
SRCS+=	nfs_socket.c nfs_subs.c nfs_iod.c nfs_vnops.c
SRCS+=	nfs_vfsops.c nfs_bootdhcp.c nfs_bootparam.c 
SRCS+=	nfs_clntsocket.c nfs_clntsubs.c

.include <bsd.kmodule.mk>
