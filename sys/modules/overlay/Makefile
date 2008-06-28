#	$NetBSD: Makefile,v 1.1 2008/06/28 16:11:36 rumble Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/overlay ${S}/miscfs/genfs

KMOD=	overlay
SRCS=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmodule.mk>
