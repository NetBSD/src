#	$NetBSD: Makefile,v 1.1.20.2 2009/05/04 08:14:12 yamt Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/overlay ${S}/miscfs/genfs

KMOD=	overlay
SRCS=	overlay_vfsops.c overlay_vnops.c

.include <bsd.kmodule.mk>
