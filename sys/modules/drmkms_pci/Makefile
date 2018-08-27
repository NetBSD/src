# $NetBSD: Makefile,v 1.6 2018/08/27 13:55:35 riastradh Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/pci
.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_pci

CPPFLAGS+=	-DCONFIG_AGP
CPPFLAGS+=	-DCONFIG_PCI

#SRCS+=	ati_pcigart.c		# XXX Restore for ATI support.
SRCS+=	drm_agpsupport.c
SRCS+=	drm_pci.c
SRCS+=	drm_pci_module.c

SRCS+=	drmfb_pci.c

.include <bsd.kmodule.mk>
