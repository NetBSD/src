#	$NetBSD: Makefile,v 1.1 1999/09/13 10:30:47 itojun Exp $

# Makefile for mmeye tags file and boot blocks

#.include "../sh3/Makefile.inc"

TMMEYE=	../mmeye/tags
SMMEYE=	../mmeye/mmeye/*.[ch] ../mmeye/include/*.h
AMMEYE=	../mmeye/mmeye/*.[sS]

# Directories in which to place tags links
DMMEYE=	mmeye include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TMMEYE} ${SMMEYE} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AMMEYE} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TMMEYE}
	sort -o ${TMMEYE} ${TMMEYE}

links:
	-for i in ${DMMEYE}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	include stand

.include <bsd.subdir.mk>
