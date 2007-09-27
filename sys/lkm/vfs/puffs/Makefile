#	$NetBSD: Makefile,v 1.3 2007/09/27 23:21:07 pooka Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/puffs ${.CURDIR}/..

KMOD=	puffs

SRCS=	lkminit_vfs.c
SRCS+=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_transport.c	\
	puffs_vfsops.c puffs_vnops.c

.include <bsd.kmod.mk>
