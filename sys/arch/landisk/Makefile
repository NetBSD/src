#	$NetBSD: Makefile,v 1.1.72.1 2008/12/13 01:13:15 haad Exp $

# Makefile for landisk tags file and boot blocks

TLANDISK=	../landisk/tags
SLANDISK=	../landisk/landisk/*.[ch] ../landisk/include/*.h
ALANDISK=	../landisk/landisk/*.[sS]

# Directories in which to place tags links
DLANDISK=	landisk include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TLANDISK} ${SLANDISK} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ALANDISK} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TLANDISK}
	sort -o ${TLANDISK} ${TLANDISK}

links:
	-for i in ${DLANDISK}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done

SUBDIR=	compile include	stand

.include <bsd.subdir.mk>
