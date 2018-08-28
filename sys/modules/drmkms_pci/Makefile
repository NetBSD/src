# $NetBSD: Makefile,v 1.7 2018/08/28 03:41:40 riastradh Exp $

.include "../Makefile.inc"
.include "../drmkms/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/linux
.PATH:	${S}/external/bsd/drm2/pci

.PATH:	${S}/external/bsd/drm2/dist/drm

KMOD=	drmkms_pci

CPPFLAGS+=	-I.

SRCS+=	drmfb_pci.c
SRCS+=	linux_pci.c

#SRCS+=	ati_pcigart.c		# XXX Restore for ATI support.
SRCS+=	drm_agpsupport.c
SRCS+=	drm_pci.c
SRCS+=	drm_pci_module.c

.include <bsd.kmodule.mk>
