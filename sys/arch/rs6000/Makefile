#	$NetBSD: Makefile,v 1.1.6.2 2008/01/09 01:48:01 matt Exp $

# Makefile for tags file

TRS6000=	../rs6000/tags
SRS6000=	../rs6000/rs6000/*.[ch] ../rs6000/include/*.h \
	../rs6000/mca/*.[ch] ../rs6000/ioplanar/*.[ch]
ARS6000=	../rs6000/rs6000/*.[sS]

# Directories in which to place tags links
DRS6000=       include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TRS6000} ${SRS6000} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ARS6000} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TRS6000}
	sort -o ${TRS6000} ${TRS6000}

links:
	-for i in ${DRS6000}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
