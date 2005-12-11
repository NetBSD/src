#	$NetBSD: Makefile,v 1.9.18.4 2005/12/11 10:28:57 christos Exp $

# use 'make -f Makefile.pcidevs' to make pcidevs.h and pcidevs_data.h

INCSDIR= /usr/include/dev/pci

# Only install includes which are used by userland
INCS=	if_lmc.h mlyio.h mlyreg.h \
	pcidevs.h pcidevs_data.h pciio.h pcireg.h \
	tgareg.h twereg.h tweio.h

.include <bsd.kinc.mk>
