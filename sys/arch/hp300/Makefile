#	$NetBSD: Makefile,v 1.7 1997/10/11 08:42:06 mycroft Exp $

NOOBJ=

# Makefile for hp300 tags file and boot blocks

SUBDIR= stand

THP300=	../hp300/tags
SHP300=	../hp300/hp300/*.[ch] ../hp300/include/*.h \
	../hp300/dev/*.[ch]
AHP300=	../hp300/hp300/*.s

# Directories in which to place hp300 tags links
DHP300=	dev hp300 include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${THP300} ${SHP300} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AHP300} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${THP300}
	sort -o ${THP300} ${THP300}

links:
	-for i in ${DHP300}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.subdir.mk>
