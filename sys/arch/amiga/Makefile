#	$NetBSD: Makefile,v 1.15 1998/06/12 23:22:32 cgd Exp $

# Makefile for amiga tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TAMIGA=	../amiga/tags
SAMIGA=	../amiga/amiga/*.[ch] ../amiga/include/*.h \
	../amiga/dev/*.[ch]
AAMIGA=	../amiga/amiga/*.s

# Directories in which to place tags links
DAMIGA=	amiga dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TAMIGA} ${SAMIGA} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMIGA} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TAMIGA}
	sort -o ${TAMIGA} ${TAMIGA}

links:
	-for i in ${DAMIGA}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include stand

.include <bsd.subdir.mk>
