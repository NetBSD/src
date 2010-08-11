# $NetBSD: Makefile,v 1.1.22.3 2010/08/11 22:54:52 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	puffs_compat.c

.include <bsd.kmodule.mk>
