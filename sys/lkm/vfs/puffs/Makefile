#	$NetBSD: Makefile,v 1.2.24.1 2007/11/06 23:33:14 matt Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_transport.c	\
	puffs_vfsops.c puffs_vnops.c

.include <bsd.kmod.mk>
