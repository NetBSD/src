#	$NetBSD: Makefile,v 1.3.114.1 2008/12/13 01:13:19 haad Exp $

# Makefile for news68k tags file

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TNEWS68K=	../news68k/tags
SNEWS68K=	../news68k/news68k/*.[ch] ../news68k/include/*.h \
		../news68k/dev/*.[ch]
ANEWS68K=	../news68k/news68k/*.S

# Directories in which to place tags links
DNEWS68K=	dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TNEWS68K} ${SNEWS68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ANEWS68K} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TNEWS68K}
	sort -o ${TNEWS68K} ${TNEWS68K}

links:
	-for i in ${DNEWS68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	compile include stand

.include <bsd.subdir.mk>
