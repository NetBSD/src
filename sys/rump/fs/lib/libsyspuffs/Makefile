#	$NetBSD: Makefile,v 1.2.16.2 2010/03/11 15:04:36 yamt Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/puffs
.PATH:  ${.CURDIR}/../../../../dev/putter

LIB=	rumpfs_syspuffs

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	putter.c

SRCS+=	puffs_rumpglue.c

CPPFLAGS+= -DPUFFSDEBUG -DPUTTERDEBUG -I${RUMPTOP}/librump/rumpvfs

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
