#	$NetBSD: Makefile,v 1.4 2002/12/23 17:30:38 jdolecek Exp $

.include "../Makefile.inc"

.PATH:	$S/fs/filecorefs ${.CURDIR}/..

KMOD=	filecorefs

SRCS=	lkminit_vfs.c
SRCS+=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmod.mk>
