#	$NetBSD: Makefile,v 1.7 1997/10/11 08:42:09 mycroft Exp $

NOOBJ=

# Makefile for pmax tags file and boot blocks

SUBDIR=	stand

# Find where mips source fils are for inclusion in tags
.include <../mips/Makefile.inc>

TPMAX=	../pmax/tags
SPMAX=	../pmax/pmax/*.[ch] ../pmax/include/*.h \
	../pmax/dev/*.[ch] ../pmax/tc/*.[ch]
APMAX=	../pmax/pmax/*.S

# Directories in which to place pmax (mips) tags links
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

.include <bsd.subdir.mk>
