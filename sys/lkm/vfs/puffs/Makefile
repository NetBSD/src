#	$NetBSD: Makefile,v 1.2.22.1 2007/10/02 18:29:12 joerg Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_transport.c	\
	puffs_vfsops.c puffs_vnops.c

.include <bsd.kmod.mk>
