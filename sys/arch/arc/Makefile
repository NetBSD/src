#	$NetBSD: Makefile,v 1.10.74.1 2009/01/17 13:27:50 mjf Exp $
#	$OpenBSD: Makefile,v 1.5 1997/05/19 10:34:53 pefo Exp $
#	from: @(#)Makefile	8.1 (Berkeley) 6/16/93

# Makefile for arc tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TARC=	../arc/tags
SARC=	../arc/arc/*.[ch] ../arc/dev/*.[ch] \
	../arc/dti/*.[ch] ../arc/include/*.h ../arc/isa/*.[ch] \
	../arc/pci/*.[ch] ../arc/jazz/*.[ch]
AARC=	../arc/arc/*.S

# Directories in which to place tags links
DARC=	arc dev dti include isa pci jazz

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TARC} ${SARC} ${SMIPS} ${COMM}
	egrep "^LEAF(.*)|^[AN]LEAF(.*)|^NON_LEAF(.*)" ${AARC} ${AMIPS} | \
	    ${TOOL_SED} "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TARC}
	sort -o ${TARC} ${TARC}

links:
	-for i in ${DARC}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
