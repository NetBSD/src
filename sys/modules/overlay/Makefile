#	$NetBSD: Makefile,v 1.2 2011/05/26 12:56:31 joerg Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/overlay ${S}/miscfs/genfs

KMOD=	overlay
SRCS=	overlay_vfsops.c overlay_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
