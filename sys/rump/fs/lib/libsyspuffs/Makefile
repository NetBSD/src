#	$NetBSD: Makefile,v 1.8.6.2 2016/03/19 11:30:36 skrll Exp $
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
