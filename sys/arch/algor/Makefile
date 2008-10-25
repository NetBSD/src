#	$NetBSD: Makefile,v 1.3 2008/10/25 22:27:36 apb Exp $

# Makefile for algor tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TALGOR= ../algor/tags
SALGOR= ../algor/algor/*.[ch] ../algor/include/*.h ../algor/isa/*.[ch] \
	../algor/pci/*.[ch]
AALGOR= ../algor/algor/*.S

# Directories in which to place tags links
DALGOR= algor include isa pci

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TALGOR} ${SALGOR} ${SMIPS} ${COMM}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${AALGOR} ${AMIPS} | \
	    ${TOOL_SED} "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TARC}
	sort -o ${TALGOR} ${TALGOR}

links:
	-for i in ${DALGOR}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
