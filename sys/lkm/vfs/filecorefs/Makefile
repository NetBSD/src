#	$NetBSD: Makefile,v 1.3 2001/12/12 12:06:47 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/filecorefs ${.CURDIR}/..

KMOD=	filecorefs

SRCS=	lkminit_vfs.c
SRCS+=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmod.mk>
