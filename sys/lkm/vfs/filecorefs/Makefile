#	$NetBSD: Makefile,v 1.1 2001/01/22 13:41:51 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/filecorefs ${.CURDIR}/..

MKMAN=	no

KMOD=	filecorefs

SRCS=	lkminit_vfs.c
SRCS+=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmod.mk>
