#	$NetBSD: Makefile,v 1.3 1996/08/27 21:44:32 explorer Exp $

.include "../Makefile.inc"

.PATH:	$S/isofs/cd9660 ${.CURDIR}/../..

NOMAN=	yep

KMOD=		cd9660

SRCS=	lkminit_vfs.c
SRCS+=	cd9660_bmap.c cd9660_lookup.c cd9660_rrip.c cd9660_vfsops.c
SRCS+=	cd9660_node.c cd9660_vnops.c cd9660_util.c

.include <bsd.kmod.mk>
