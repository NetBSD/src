#	$NetBSD: Makefile,v 1.6.12.1 2001/04/21 17:53:56 bouyer Exp $

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
	-ctags -wdtf ${TMAC68K} ${SMAC68K} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AMAC68K} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TMAC68K}
	sort -o ${TMAC68K} ${TMAC68K}

links:
	-for i in ${DMAC68K}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	include

.include <bsd.subdir.mk>
