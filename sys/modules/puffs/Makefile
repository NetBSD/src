# $NetBSD: Makefile,v 1.1.2.2 2008/06/02 13:24:21 mjf Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmodule.mk>
