#	$NetBSD: Makefile,v 1.1.2.1 1999/12/27 18:32:54 wrstuden Exp $

# Makefile for news68k tags file

# Find where mips source files are for inclusion in tags
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
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TNEWS68K}
	sort -o ${TNEWS68K} ${TNEWS68K}

links:
	-for i in ${DNEWS68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	include stand

.include <bsd.subdir.mk>
