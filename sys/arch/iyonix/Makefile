#	$NetBSD: Makefile,v 1.4 2011/04/06 01:49:23 dyoung Exp $

# Makefile for iyonix tags file and boot blocks

# Find where ARM source files are for inclusion in tags
.include <../arm/Makefile.inc>

TIYONIX=	${SYSDIR}/arch/iyonix/tags
SIYONIX=	${SYSDIR}/arch/iyonix/iyonix/*.[ch]
SIYONIX+=	${SYSDIR}/arch/iyonix/include/*.h

# Directories in which to place tags links
DIYONIX=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TIYONIX}
	-echo ${SIYONIX} ${SARM} | xargs ctags -wadtf ${TIYONIX}
	-${FINDCOMM} | xargs ctags -wadtf ${TIYONIX}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AARM} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TIYONIX}
	sort -o ${TIYONIX} ${TIYONIX}

links:
	-for i in ${DIYONIX}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	compile include

.include <bsd.subdir.mk>
