#	$NetBSD: Makefile,v 1.10 1998/06/12 23:22:43 cgd Exp $

# Makefile for pmax tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TPMAX=	../pmax/tags
SPMAX=	../pmax/pmax/*.[ch] ../pmax/include/*.h \
	../pmax/dev/*.[ch] ../pmax/tc/*.[ch]
APMAX=	../pmax/pmax/*.S

# Directories in which to place tags links
DPMAX=	dev tc include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TPMAX} ${SPMAX} ${SMIPS} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${APMAX} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TPMAX}
	sort -o ${TPMAX} ${TPMAX}

links:
	-for i in ${DPMAX}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include stand

.include <bsd.subdir.mk>
