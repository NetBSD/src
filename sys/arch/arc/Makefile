#	$NetBSD: Makefile,v 1.5 2000/01/23 20:07:59 soda Exp $

# Makefile for pica tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TPICA=	../pica/tags
SPICA=	../pica/pica/*.[ch] ../pica/include/*.[ch] \
	../pica/dev/*.[ch] ../pica/ultrix/*.[ch]
APICA=	../pica/pica/*.s

# Directories in which to place tags links
DPICA=	dev dist include pica

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TPICA} ${SPICA} ${SMIPS} ${COMM}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${APICA} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TPICA}
	sort -o ${TPICA} ${TPICA}

links:
	-for i in ${DPICA}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include

.include <bsd.subdir.mk>
