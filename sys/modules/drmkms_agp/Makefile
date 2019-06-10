#	$NetBSD: Makefile,v 1.2.4.2 2019/06/10 22:09:21 christos Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_agp

SRCS+=	drm_agpsupport.c

WARNS=	3

.include <bsd.kmodule.mk>
