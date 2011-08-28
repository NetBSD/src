# $NetBSD: Makefile,v 1.1 2011/08/28 15:56:21 jmcneill Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core

KMOD=   ati_pcigart

SRCS=	ati_pcigart.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/shared-core

.include <bsd.kmodule.mk>
