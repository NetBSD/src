# $NetBSD: Makefile,v 1.3 2019/02/04 12:08:03 pgoyette Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

.include <bsd.kmodule.mk>
