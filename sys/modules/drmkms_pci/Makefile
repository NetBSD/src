# $NetBSD: Makefile,v 1.9 2019/02/17 04:05:47 rin Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/linux
.PATH:	${S}/external/bsd/drm2/pci

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_pci

SRCS+=	drmfb_pci.c
SRCS+=	linux_pci.c

#SRCS+=	ati_pcigart.c		# XXX Restore for ATI support.
SRCS+=	drm_agpsupport.c
SRCS+=	drm_pci.c
SRCS+=	drm_pci_module.c

WARNS=	3

.include <bsd.kmodule.mk>
