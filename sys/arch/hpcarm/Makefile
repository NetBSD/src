#	$NetBSD: Makefile,v 1.3 2008/10/25 22:27:37 apb Exp $

# Makefile for hpcarm tags file and boot blocks

THPCARM=	../hpcarm/tags
SHPCARM=	../hpcarm/hpcarm/*.[ch] ../hpcarm/include/*.h \
		../hpcarm/dev/*.[ch] ../hpcarm/mainbus/*.[ch] \
		../hpcarm/sa/*.[ch]
AHPCARM=	../hpcarm/hpcarm/*.s ../hpcarm/sa/*.s

# Directories in which to place tags links
DHPCARM=	dev mainbus include sa

.include "../../kern/Make.tags.inc"

tags:
	-ctags -wdtf ${THPCARM} ${SHPCARM} ${COMM}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AHPCARM} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${THPCARM}
	sort -o ${THPCARM} ${THPCARM}

links:
	-for i in ${DHPCARM}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
