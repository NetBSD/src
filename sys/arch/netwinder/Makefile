#	$NetBSD: Makefile,v 1.1 2001/06/11 17:46:23 matt Exp $

# Makefile for netwinder tags file and boot blocks

TNETWINDER=	../netwinder/tags
SNETWINDER=	../netwinder/netwinder/*.[ch] ../netwinder/include/*.h \
		../netwinder/isa/*.[ch] ../netwinder/pci/*.[ch]
ANETWINDER=	../netwinder/netwinder/*.s

# Directories in which to place tags links
DNETWINDER=	isa pci include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TNETWINDER} ${SNETWINDER} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ANETWINDER} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TNETWINDER}
	sort -o ${TNETWINDER} ${TNETWINDER}

links:
	-for i in ${DNETWINDER}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include

.include <bsd.subdir.mk>
