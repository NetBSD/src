# $NetBSD: Makefile,v 1.2 2019/02/17 04:05:43 rin Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core

KMOD=   ati_pcigart

SRCS=	ati_pcigart.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

.include <bsd.kmodule.mk>
