# $NetBSD: Makefile,v 1.1.2.2 2015/06/06 14:40:25 skrll Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

KMOD=	viadrmums
IOCONF=	viadrmums.ioconf

CPPFLAGS+=	-I${S}/external/bsd/drm2/dist/drm/via
CPPFLAGS+=	-I${S}/external/bsd/drm2/via

.PATH:	${S}/external/bsd/drm2/via
.PATH:	${S}/external/bsd/drm2/dist/drm/via

COPTS.via_verifier.c+=	-Wno-shadow
COPTS.via_pci.c+=	-Wno-missing-field-initializers

SRCS+=	via_dma.c
SRCS+=	via_dmablit.c
SRCS+=	via_drv.c
SRCS+=	via_irq.c
SRCS+=	via_map.c
SRCS+=	via_mm.c
SRCS+=	via_verifier.c
SRCS+=	via_video.c

SRCS+=	via_module.c
SRCS+=	via_pci.c

.include <bsd.kmodule.mk>
