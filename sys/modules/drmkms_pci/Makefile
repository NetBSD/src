# $NetBSD: Makefile,v 1.10.4.1 2023/02/22 18:34:30 martin Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/linux
.PATH:	${S}/external/bsd/drm2/pci

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_pci

SRCS+=	drmfb_pci.c
SRCS+=	linux_pci.c

SRCS+=	drm_pci.c
SRCS+=	drm_pci_module.c

WARNS=	3

.include <bsd.kmodule.mk>
