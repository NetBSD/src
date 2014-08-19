#	$NetBSD: Makefile,v 1.2.10.2 2014/08/20 00:04:32 tls Exp $

# XXX This module is not fit for use yet; too much of the PCI/AGP
# support in upstream is compile-time-conditional.

.include "../Makefile.inc"
.include "../drm2/Makefile.inc"

.PATH:	${S}/external/bsd/drm2/pci

KMOD=	drm2pci

SRCS+=	drm_agpsupport.c
SRCS+=	drm_pci.c
SRCS+=	drm_pci_module.c

.include <bsd.kmodule.mk>
