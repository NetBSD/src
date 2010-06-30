#	$NetBSD: Makefile,v 1.5 2010/06/30 15:50:20 pooka Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/puffs
.PATH:  ${.CURDIR}/../../../../dev/putter

LIB=	rumpfs_syspuffs

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	putter.c

SRCS+=	puffs_rumpglue.c
SRCS+=	component.c

CPPFLAGS+= -DPUFFSDEBUG -DPUTTERDEBUG -I${RUMPTOP}/librump/rumpvfs
CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
