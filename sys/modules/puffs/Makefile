# $NetBSD: Makefile,v 1.1 2008/05/19 17:08:57 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmodule.mk>
