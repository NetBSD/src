#	$NetBSD: Makefile,v 1.1.26.1 2011/05/31 03:05:06 rmind Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/overlay ${S}/miscfs/genfs

KMOD=	overlay
SRCS=	overlay_vfsops.c overlay_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
