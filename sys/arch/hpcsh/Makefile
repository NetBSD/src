#	$NetBSD: Makefile,v 1.1.2.2 2001/01/17 05:21:49 itojun Exp $

# Makefile for hpcsh tags file and boot blocks

#.include "../sh3/Makefile.inc"

TEVBSH3=	../hpcsh/tags
SEVBSH3=	../hpcsh/hpcsh/*.[ch] ../hpcsh/include/*.h
AEVBSH3=	../hpcsh/hpcsh/*.[sS]

# Directories in which to place tags links
DEVBSH3=	hpcsh include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TEVBSH3} ${SEVBSH3} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEVBSH3} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TEVBSH3}
	sort -o ${TEVBSH3} ${TEVBSH3}

links:
	-for i in ${DEVBSH3}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	include # stand

.include <bsd.subdir.mk>
