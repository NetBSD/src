#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:52 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/isofs/cd9660 ${.CURDIR}/../..

MKMAN=	no

KMOD=	cd9660

SRCS=	lkminit_vfs.c
SRCS+=	cd9660_bmap.c cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c
SRCS+=	cd9660_node.c cd9660_vnops.c cd9660_util.c

.include <bsd.kmod.mk>
