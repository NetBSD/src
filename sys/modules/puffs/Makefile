# $NetBSD: Makefile,v 1.1.26.1 2010/08/17 06:47:42 uebayasi Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	puffs_compat.c

.include <bsd.kmodule.mk>
