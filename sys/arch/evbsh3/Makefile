#	$NetBSD: Makefile,v 1.3 2008/10/25 22:27:37 apb Exp $

# Makefile for evbsh3 tags file and boot blocks

#.include "../sh3/Makefile.inc"

TEVBSH3=	../evbsh3/tags
SEVBSH3=	../evbsh3/evbsh3/*.[ch] ../evbsh3/include/*.h
AEVBSH3=	../evbsh3/evbsh3/*.[sS]

# Directories in which to place tags links
DEVBSH3=	evbsh3 include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TEVBSH3} ${SEVBSH3} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AEVBSH3} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TEVBSH3}
	sort -o ${TEVBSH3} ${TEVBSH3}

links:
	-for i in ${DEVBSH3}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include # stand

.include <bsd.subdir.mk>
