#	$NetBSD: Makefile,v 1.1.6.2 2000/11/20 18:09:41 bouyer Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/overlay $S/miscfs/genfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	overlay

SRCS=	lkminit_vfs.c
SRCS+=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmod.mk>
