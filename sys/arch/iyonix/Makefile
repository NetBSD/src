#	$NetBSD: Makefile,v 1.1.2.2 2004/10/19 15:56:36 skrll Exp $

# Makefile for iyonix tags file and boot blocks

TIYONIX=	../iyonix/tags
SIYONIX=	../iyonix/include/*.h \
		../iyonix/dev/*.[ch] \
#AIYONIX=

# Directories in which to place tags links
DIYONIX=	include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TIYONIX} ${SIYONIX} ${COMM}
#	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AIYONIX} | \
#	    sed "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
#	    >> ${TIYONIX}
	sort -o ${TIYONIX} ${TIYONIX}

links:
	-for i in ${DIYONIX}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
