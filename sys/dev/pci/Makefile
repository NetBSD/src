#	$NetBSD: Makefile,v 1.7.2.2 2002/01/10 19:56:24 thorpej Exp $

# use 'make -f Makefile.pcidevs' to make pcidevs.h and pcidevs_data.h

INCSDIR= /usr/include/dev/pci

# Only install includes which are used by userland
INCS=	if_lmc_types.h if_lmcioctl.h mlyio.h mlyreg.h \
	pcidevs.h pcidevs_data.h pciio.h pcireg.h \
	tgareg.h

.include <bsd.kinc.mk>
