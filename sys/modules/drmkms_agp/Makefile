#	$NetBSD: Makefile,v 1.1 2018/08/28 03:41:40 riastradh Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_agp

SRCS+=	drm_agpsupport.c

.include <bsd.kmodule.mk>
