#	$NetBSD: Makefile,v 1.1 2001/10/06 22:04:11 bjh21 Exp $

# Makefile for acorn32 tags file and boot blocks

TACORN32=	../acorn32/tags
SACORN32=	../acorn32/acorn32/*.[ch] ../acorn32/include/*.h \
	../acorn32/dev/*.[ch] ../acorn32/podulebus/*.[ch] ../acorn32/mainbus/*.[ch]
AACORN32=	../acorn32/acorn32/*.s

# Directories in which to place tags links
DACORN32=	dev mainbus podulebus include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TACORN32} ${SACORN32} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AACORN32} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TACORN32}
	sort -o ${TACORN32} ${TACORN32}

links:
	-for i in ${DACORN32}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	include

.include <bsd.subdir.mk>
