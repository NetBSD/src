#	$NetBSD: Makefile,v 1.1.6.1 2002/01/10 20:01:18 thorpej Exp $

.include "../Makefile.inc"

.PATH:	$S/filecorefs ${.CURDIR}/..

KMOD=	filecorefs

SRCS=	lkminit_vfs.c
SRCS+=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmod.mk>
