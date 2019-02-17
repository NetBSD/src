# $NetBSD: Makefile,v 1.4 2019/02/17 04:05:56 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
