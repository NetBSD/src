#	$NetBSD: Makefile,v 1.2 1998/06/12 23:22:41 cgd Exp $

# Makefile for newsmips tags file

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TNEWSMIPS=	../newsmips/tags
SNEWSMIPS=	../newsmips/newsmips/*.[ch] ../newsmips/include/*.h \
		../newsmips/dev/*.[ch]
ANEWSMIPS=	../newsmips/newsmips/*.S

# Directories in which to place tags links
DNEWSMIPS=	dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TNEWSMIPS} ${SNEWSMIPS} ${SMIPS} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ANEWSMIPS} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TNEWSMIPS}
	sort -o ${TNEWSMIPS} ${TNEWSMIPS}

links:
	-for i in ${DNEWSMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	include

.include <bsd.subdir.mk>
