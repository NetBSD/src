#	$NetBSD: Makefile,v 1.12 1997/10/11 08:42:04 mycroft Exp $

NOOBJ=

# Makefile for amiga tags file and boot blocks

SUBDIR=	stand

TAMIGA=	$S/arch/amiga/tags
SAMIGA=	$S/arch/m68k/m68k/*.c $S/arch/m68k/include/*.h \
	$S/arch/amiga/amiga/*.[ch] $S/arch/amiga/include/*.h \
	$S/arch/amiga/dev/*.[ch]
AAMIGA=	$S/arch/m68k/m68k/*.s $S/arch/amiga/amiga/*.s

# Directories in which to place amiga tags links
DAMIGA=	amiga dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TAMIGA} ${SAMIGA} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMIGA} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TAMIGA}
	sort -o ${TAMIGA} ${TAMIGA}

links:
	-for i in ${DAMIGA}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

.include <bsd.subdir.mk>
