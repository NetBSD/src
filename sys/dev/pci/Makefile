#	$NetBSD: Makefile,v 1.11.8.1 2006/08/11 15:44:25 yamt Exp $

# use 'make -f Makefile.pcidevs' to make pcidevs.h and pcidevs_data.h

INCSDIR= /usr/include/dev/pci

# Only install includes which are used by userland
INCS=	amrreg.h amrio.h if_lmc.h mlyio.h mlyreg.h \
	pcidevs.h pcidevs_data.h pciio.h pcireg.h \
	tgareg.h twereg.h tweio.h

.include <bsd.kinc.mk>
