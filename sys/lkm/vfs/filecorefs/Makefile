#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:16 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/filecorefs ${.CURDIR}/..

NOMAN=	# defined

KMOD=	filecorefs

SRCS=	lkminit_vfs.c
SRCS+=	filecore_bmap.c filecore_lookup.c filecore_node.c filecore_utils.c \
	filecore_vfsops.c filecore_vnops.c

.include <bsd.kmod.mk>
