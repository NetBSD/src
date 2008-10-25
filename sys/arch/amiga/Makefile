#	$NetBSD: Makefile,v 1.21 2008/10/25 22:27:36 apb Exp $

# Makefile for amiga tags file and boot blocks

TAMIGA=	${SYSDIR}/arch/amiga/tags
SAMIGA=	${SYSDIR}/arch/amiga/amiga/*.[ch] ${SYSDIR}/arch/amiga/include/*.h \
	${SYSDIR}/arch/amiga/dev/*.[ch]
AAMIGA=	${SYSDIR}/arch/amiga/amiga/*.s

# Directories in which to place tags links
DAMIGA=	amiga dev include

.include "../../kern/Make.tags.inc"

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

tags:
	-echo ${COMM} | xargs ctags -wdtf ${TAMIGA} ${SAMIGA} ${SM68K}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMIGA} ${AM68K} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TAMIGA}
	sort -o ${TAMIGA} ${TAMIGA}

links:
	-for i in ${DAMIGA}; do \
	    cd ${.CURDIR}/$$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
