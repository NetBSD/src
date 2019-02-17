#	$NetBSD: Makefile,v 1.2 2019/02/17 04:05:47 rin Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_agp

SRCS+=	drm_agpsupport.c

WARNS=	3

.include <bsd.kmodule.mk>
