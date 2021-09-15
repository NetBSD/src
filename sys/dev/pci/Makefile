#	$NetBSD: Makefile,v 1.16 2021/09/15 17:33:08 thorpej Exp $

# use 'make -f Makefile.pcidevs' to make pcidevs.h and pcidevs_data.h

INCSDIR= /usr/include/dev/pci

# Only install includes which are used by userland
INCS=	amrreg.h amrio.h mlyio.h mlyreg.h \
	pcidevs.h pcidevs_data.h pciio.h pcireg.h \
	tgareg.h twereg.h tweio.h

pci_calls.h: ${.CURDIR}/pci_calls
	echo "${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/pci_calls > ${.CURDIR}/pci_calls.h"
	${TOOL_AWK} -f ${.CURDIR}/../../kern/gendevcalls.awk \
	    ${.CURDIR}/pci_calls > ${.CURDIR}/pci_calls.h

.include <bsd.kinc.mk>
