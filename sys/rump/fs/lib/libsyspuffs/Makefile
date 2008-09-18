#	$NetBSD: Makefile,v 1.2.18.1 2008/09/18 04:37:03 wrstuden Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/puffs
.PATH:  ${.CURDIR}/../../../../dev/putter

LIB=	rumpfs_syspuffs

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	putter.c

SRCS+=	puffs_rumpglue.c

CPPFLAGS+= -DPUFFSDEBUG -DPUTTERDEBUG

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
