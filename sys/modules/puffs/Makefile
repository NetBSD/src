# $NetBSD: Makefile,v 1.1.10.2 2008/09/24 05:28:33 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmodule.mk>
