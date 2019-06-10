# $NetBSD: Makefile,v 1.2.60.1 2019/06/10 22:09:37 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/fs/puffs

KMOD=	puffs
SRCS=	puffs_node.c puffs_msgif.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

WARNS=	3

.include <bsd.kmodule.mk>
