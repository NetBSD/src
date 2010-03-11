#	$NetBSD: Makefile,v 1.12.58.1 2010/03/11 15:03:42 yamt Exp $

# use 'make -f Makefile.pcidevs' to make pcidevs.h and pcidevs_data.h

SUBDIR=	hdaudio

INCSDIR= /usr/include/dev/pci

# Only install includes which are used by userland
INCS=	amrreg.h amrio.h if_lmc.h mlyio.h mlyreg.h \
	pcidevs.h pcidevs_data.h pciio.h pcireg.h \
	tgareg.h twereg.h tweio.h

.include <bsd.kinc.mk>
