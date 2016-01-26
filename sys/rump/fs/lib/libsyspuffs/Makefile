#	$NetBSD: Makefile,v 1.10 2016/01/26 23:12:17 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/puffs

LIB=	rumpfs_syspuffs
COMMENT=puffs in-kernel driver

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	puffs_compat.c

SRCS+=	puffs_rumpglue.c

SRCS+=	syspuffs_component.c

CPPFLAGS+= -DPUFFSDEBUG

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
