#	$NetBSD: Makefile,v 1.4 1997/10/11 09:10:25 mycroft Exp $

# Makefile for arm32 tags file and boot blocks

TARM32=	../arm32/tags
SARM32=	../arm32/arm32/*.[ch] ../arm32/include/*.h \
	../arm32/dev/*.[ch] ../arm32/podulebus/*.[ch] ../arm32/mainbus/*.[ch]
AARM32=	../arm32/arm32/*.s

# Directories in which to place tags links
DARM32=	dev mainbus podulebus include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TARM32} ${SARM32} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AARM32} | \
	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TARM32}
	sort -o ${TARM32} ${TARM32}

links:
	-for i in ${DARM32}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	

.include <bsd.subdir.mk>
