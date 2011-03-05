# $NetBSD: Makefile,v 1.1.28.1 2011/03/05 20:55:47 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	puffs_compat.c

.include <bsd.kmodule.mk>
