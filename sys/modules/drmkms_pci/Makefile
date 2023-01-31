# $NetBSD: Makefile,v 1.11 2023/01/31 13:09:54 riastradh Exp $

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
