#	$NetBSD: Makefile,v 1.11.114.1 2008/12/13 01:13:10 haad Exp $

# Makefile for hp300 tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

THP300=	../hp300/tags
SHP300=	../hp300/hp300/*.[ch] ../hp300/include/*.h \
	../hp300/dev/*.[ch]
AHP300=	../hp300/hp300/*.s

# Directories in which to place tags links
DHP300=	dev hp300 include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${THP300} ${SHP300} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AHP300} ${AM68K} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${THP300}
	sort -o ${THP300} ${THP300}

links:
	-for i in ${DHP300}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
