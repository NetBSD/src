#	$NetBSD: Makefile,v 1.3 2001/12/12 12:06:48 lukem Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/overlay $S/miscfs/genfs ${.CURDIR}/../..

KMOD=	overlay

SRCS=	lkminit_vfs.c
SRCS+=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmod.mk>
