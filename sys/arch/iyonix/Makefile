#	$NetBSD: Makefile,v 1.2.78.1 2009/05/04 08:11:22 yamt Exp $

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
#	    ${TOOL_SED} -e \
#		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
#	    >> ${TIYONIX}
	sort -o ${TIYONIX} ${TIYONIX}

links:
	-for i in ${DIYONIX}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
