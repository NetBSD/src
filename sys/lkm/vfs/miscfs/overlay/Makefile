#	$NetBSD: Makefile,v 1.1 2000/01/20 19:06:15 wrstuden Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/overlay $S/miscfs/genfs ${.CURDIR}/../..

MKMAN=	no

KMOD=	overlay

SRCS=	lkminit_vfs.c
SRCS+=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmod.mk>
