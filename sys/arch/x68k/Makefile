#	$NetBSD: Makefile,v 1.4 1997/10/11 08:44:50 mycroft Exp $

# Makefile for x68k tags file and boot blocks

SUBDIR=	usr.bin usr.sbin sdboot fdboot

TX68K=	../x68k/tags
SX68K=	../m68k/m68k/*.c ../m68k/include/*.h \
	../x68k/x68k/*.[ch] ../x68k/include/*.h \
	../x68k/dev/*.[ch]
AX68K=	../m68k/m68k/*.s ../x68k/x68k/*.s

# Directories in which to place x68k tags links
DX68K=	x68k dev include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TX68K} ${SX68K} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AX68K} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TX68K}
	sort -o ${TX68K} ${TX68K}

links:
	-for i in ${DX68K}; do \
	    cd ../$$i && rm -f tags TAGS; ln -s ../tags ../TAGS .; done

.include <bsd.subdir.mk>
