#	$NetBSD: Makefile,v 1.2 2001/12/12 01:50:17 tv Exp $

.include "../Makefile.inc"

.PATH:	$S/miscfs/overlay $S/miscfs/genfs ${.CURDIR}/../..

NOMAN=	# defined

KMOD=	overlay

SRCS=	lkminit_vfs.c
SRCS+=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmod.mk>
