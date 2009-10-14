#	$NetBSD: Makefile,v 1.4 2009/10/14 17:29:19 pooka Exp $
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
