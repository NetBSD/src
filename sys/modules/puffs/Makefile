# $NetBSD: Makefile,v 1.1.4.2 2008/06/04 02:05:47 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmodule.mk>
