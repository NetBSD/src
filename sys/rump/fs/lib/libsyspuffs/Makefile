#	$NetBSD: Makefile,v 1.7.18.1 2014/08/20 00:04:39 tls Exp $
#

.PATH:  ${.CURDIR}/../../../../fs/puffs

LIB=	rumpfs_syspuffs

SRCS=	puffs_msgif.c puffs_node.c puffs_subr.c puffs_vfsops.c puffs_vnops.c
SRCS+=	puffs_compat.c

SRCS+=	puffs_rumpglue.c

SRCS+=	syspuffs_component.c

CPPFLAGS+= -DPUFFSDEBUG -I${RUMPTOP}/librump/rumpvfs
CPPFLAGS+= -I${RUMPTOP}/librump/rumpkern

.include <bsd.lib.mk>
.include <bsd.klinks.mk>
