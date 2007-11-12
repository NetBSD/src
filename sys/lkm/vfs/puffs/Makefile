#	$NetBSD: Makefile,v 1.4 2007/11/12 12:31:15 pooka Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmod.mk>
