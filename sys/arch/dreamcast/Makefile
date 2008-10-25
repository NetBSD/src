#	$NetBSD: Makefile,v 1.3 2008/10/25 22:27:37 apb Exp $

# Makefile for dreamcast tags file and boot blocks

#.include "../sh3/Makefile.inc"

TDREAMCAST=	../dreamcast/tags
SDREAMCAST=	../dreamcast/dreamcast/*.[ch] ../dreamcast/include/*.h
ADREAMCAST=	../dreamcast/dreamcast/*.[sS]

# Directories in which to place tags links
DDREAMCAST=	dreamcast include

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${TDREAMCAST} ${SDREAMCAST} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ADREAMCAST} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TDREAMCAST}
	sort -o ${TDREAMCAST} ${TDREAMCAST}

links:
	-for i in ${DDREAMCAST}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include	# stand

.include <bsd.subdir.mk>
