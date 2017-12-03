#	$NetBSD: Makefile,v 1.7.18.2 2017/12/03 11:39:13 jdolecek Exp $
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
