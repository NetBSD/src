# $NetBSD: Makefile,v 1.2 2010/07/06 16:13:57 pooka Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	puffs_compat.c

.include <bsd.kmodule.mk>
