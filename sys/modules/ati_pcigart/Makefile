# $NetBSD: Makefile,v 1.1.54.1 2019/06/10 22:09:11 christos Exp $

.include "../Makefile.inc"

.PATH:	${S}/external/bsd/drm/dist/bsd-core

KMOD=   ati_pcigart

SRCS=	ati_pcigart.c

CPPFLAGS+=	-I${S}/external/bsd/drm/dist/shared-core

WARNS=	3

.include <bsd.kmodule.mk>
