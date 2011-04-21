#	$NetBSD: Makefile,v 1.3.16.1 2011/04/21 01:40:54 rmind Exp $

# Makefile for cats tags file and boot blocks

# Find where ARM source files are for inclusion in tags
.include <../arm/Makefile.inc>

TCATS=	${SYSDIR}/arch/cats/tags
SCATS=	${SYSDIR}/arch/cats/cats/*.[ch] ${SYSDIR}/arch/cats/include/*.h \
	${SYSDIR}/arch/cats/pci/*.[ch]

# Directories in which to place tags links
DCATS=	include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TCATS}
	-echo ${SCATS} ${SARM} | xargs ctags -wadtf ${TCATS}
	-${FINDCOMM} | xargs ctags -wadtf ${TCATS}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ACATS} ${AARM} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3	\1	/^\2(\3\4$$/;" \
	    >> ${TCATS}
	sort -o ${TCATS} ${TCATS}

links:
	-for i in ${DCATS}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done


SUBDIR=	compile include

.include <bsd.subdir.mk>
