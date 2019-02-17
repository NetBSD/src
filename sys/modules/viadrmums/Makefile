# $NetBSD: Makefile,v 1.3 2019/02/17 04:06:00 rin Exp $

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

WARNS=	3

.if defined(HAVE_GCC) && ${HAVE_GCC} == 7 && ${ACTIVE_CC} == "gcc"
COPTS.via_dmablit.c+=	-Wno-error=implicit-fallthrough
.endif

.include <bsd.kmodule.mk>
