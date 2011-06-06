#	$NetBSD: Makefile,v 1.1.8.2 2011/06/06 09:05:15 jruoho Exp $

# Makefile for emips tags file and boot blocks

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TEMIPS=	../emips/tags
SEMIPS=	../emips/emips/*.[ch] ../emips/include/*.h \
	../emips/dev/*.[ch] ../emips/tc/*.[ch]
AEMIPS=	../emips/emips/*.S

# Directories in which to place tags links
DEMIPS=	dev tc include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TEMIPS} ${SEMIPS} ${SMIPS} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEMIPS} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TEMIPS}
	sort -o ${TEMIPS} ${TEMIPS}

links:
	-for i in ${DEMIPS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
