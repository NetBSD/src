#	$NetBSD: Makefile,v 1.1.8.1 2002/01/08 00:33:25 nathanw Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/overlay $S/miscfs/genfs ${.CURDIR}/../..

KMOD=	overlay

SRCS=	lkminit_vfs.c
SRCS+=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmod.mk>
