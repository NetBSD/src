#	$NetBSD: Makefile,v 1.3.12.1 1997/10/14 10:16:48 thorpej Exp $

# Makefile for mac68k tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TMAC68K=../mac68k/tags
SMAC68K=../m68k/m68k/*.c ../m68k/include/*.h \
	../mac68k/mac68k/*.[ch] ../mac68k/include/*.h \
	../mac68k/dev/*.[ch]
AMAC68K=../mac68k/mac68k/*.s

# Directories in which to place tags links
DMAC68K=mac68k dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TMAC68K} ${SMAC68K} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AMAC68K} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TMAC68K}
	sort -o ${TMAC68K} ${TMAC68K}

links:
	-for i in ${DMAC68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	

.include <bsd.subdir.mk>
