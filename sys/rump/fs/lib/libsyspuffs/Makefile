#	$NetBSD: Makefile,v 1.4.4.1 2010/07/03 01:20:01 rmind Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/puffs

LIB=	rumpfs_syspuffs

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c

SRCS+=	puffs_rumpglue.c
SRCS+=	component.c

CPPFLAGS+= -DPUFFSDEBUG -I${RUMPTOP}/librump/rumpvfs
CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
