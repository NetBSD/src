#	$NetBSD: Makefile,v 1.2.4.1 1997/10/14 10:19:59 thorpej Exp $

# Makefile for x68k tags file and boot blocks

# Find where m68k source files are for inclusion in tags
.include <../m68k/Makefile.inc>

TX68K=	../x68k/tags
SX68K=	../x68k/x68k/*.[ch] ../x68k/include/*.h \
	../x68k/dev/*.[ch]
AX68K=	../x68k/x68k/*.s

# Directories in which to place tags links
DX68K=	x68k dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TX68K} ${SX68K} ${SM68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AX68K} ${AM68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TX68K}
	sort -o ${TX68K} ${TX68K}

links:
	-for i in ${DX68K}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	usr.bin usr.sbin sdboot fdboot

.include <bsd.subdir.mk>
