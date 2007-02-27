#	$NetBSD: Makefile,v 1.8.56.1 2007/02/27 16:51:59 yamt Exp $

# Makefile for mac68k tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TMAC68K=${SYSDIR}/arch/mac68k/tags
SMAC68K=${SYSDIR}/arch/mac68k/mac68k/*.[ch] ${SYSDIR}/arch/mac68k/include/*.h \
	${SYSDIR}/arch/mac68k/nubus/*.[ch] ${SYSDIR}/arch/mac68k/obio/*.[ch] \
	${SYSDIR}/arch/mac68k/dev/*.[ch]
AMAC68K=${SYSDIR}/arch/mac68k/mac68k/*.s ${SYSDIR}/arch/mac68k/dev/*.s

# Directories in which to place tags links
DMAC68K=mac68k dev include nubus obio

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TMAC68K}
	-echo ${SMAC68K} ${SM68K} ${COMM} | xargs ctags -wdtf ${TMAC68K}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AMAC68K} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TMAC68K}
	sort -o ${TMAC68K} ${TMAC68K}

links:
	-for i in ${DMAC68K}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include

.include <bsd.subdir.mk>
