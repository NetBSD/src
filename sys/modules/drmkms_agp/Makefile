#	$NetBSD: Makefile,v 1.1.2.2 2018/09/06 06:56:43 pgoyette Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_agp

SRCS+=	drm_agpsupport.c

.include <bsd.kmodule.mk>
