#	$NetBSD: Makefile,v 1.3 2000/09/26 11:45:06 wdk Exp $

# Makefile for mipsco tags file

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TMIPSCO=	../mipsco/tags
SMIPSCO=	../mipsco/mipsco/*.[ch] ../mipsco/include/*.h \
		../mipsco/obio/*.[ch]
AMIPSCO=	../mipsco/mipsco/*.S

# Directories in which to place tags links
DMIPSCO=	obio include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TMIPSCO} ${SMIPSCO} ${SMIPS} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AMIPSCO} ${AMIPS} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TMIPSCO}
	sort -o ${TMIPSCO} ${TMIPSCO}

links:
	-for i in ${DMIPSCO}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	include stand

.include <bsd.subdir.mk>
