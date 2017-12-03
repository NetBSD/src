# $NetBSD: Makefile,v 1.2.10.3 2017/12/03 11:38:52 jdolecek Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/pci
.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_pci

#SRCS+=	ati_pcigart.c		# XXX Restore for ATI support.
SRCS+=	drm_agpsupport.c
SRCS+=	drm_pci.c
SRCS+=	drm_pci_module.c

SRCS+=	drmfb_pci.c

.include <bsd.kmodule.mk>
