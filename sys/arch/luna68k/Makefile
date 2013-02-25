#	$NetBSD: Makefile,v 1.3.38.1 2013/02/25 00:28:47 tls Exp $

# Makefile for luna68k tags file

# Find where mips source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TLUNA68K=	../luna68k/tags
SLUNA68K=	../luna68k/luna68k/*.[ch] ../luna68k/include/*.h \
		../luna68k/dev/*.[ch]
ALUNA68K=	../luna68k/luna68k/*.s

# Directories in which to place tags links
DLUNA68K=	dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TLUNA68K} ${SLUNA68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ALUNA68K} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TLUNA68K}
	sort -o ${TLUNA68K} ${TLUNA68K}

links:
	-for i in ${DLUNA68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	compile include stand

.include <bsd.subdir.mk>
