#	$NetBSD: Makefile,v 1.1 2000/02/29 15:21:53 nonaka Exp $

# Makefile for tags file

TPREP=	../prep/tags
SPREP=	../prep/prep/*.[ch] ../prep/include/*.h ../prep/isa/*.[ch] \
	../prep/pci/*.[ch]
APREP=	../prep/prep/*.[sS]

# Directories in which to place tags links
DPREP=       include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TPREP} ${SPREP} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${APREP} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TPREP}
	sort -o ${TPREP} ${TPREP}

links:
	-for i in ${DPREP}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	include stand

.include <bsd.subdir.mk>
