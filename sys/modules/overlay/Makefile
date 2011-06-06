#	$NetBSD: Makefile,v 1.1.32.1 2011/06/06 09:09:49 jruoho Exp $

.include "../Makefile.inc"

.PATH:	${S}/miscfs/overlay ${S}/miscfs/genfs

KMOD=	overlay
SRCS=	overlay_vfsops.c overlay_vnops.c

CWARNFLAGS.clang=	-Wno-conversion

.include <bsd.kmodule.mk>
