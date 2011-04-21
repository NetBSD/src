#	$NetBSD: Makefile,v 1.5.16.1 2011/04/21 01:40:55 rmind Exp $

# Makefile for cobalt tags file

# Find where mips source files are for inclusion in tags
.include <../mips/Makefile.inc>

TCOBALT=	${SYSDIR}/arch/cobalt/tags
SCOBALT=	${SYSDIR}/arch/cobalt/cobalt/*.[ch] \
		${SYSDIR}/arch/cobalt/include/*.h \
		${SYSDIR}/arch/cobalt/dev/*.[ch] \
		${SYSDIR}/arch/cobalt/pci/*.[ch]
ACOBALT=	${SYSDIR}/arch/cobalt/cobalt/*.S

# Directories in which to place tags links
DCOBALT=	pci include

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TCOBALT}
	-echo ${SCOBALT} ${SMIPS} | xargs ctags -wadtf ${TCOBALT}
	-${FINDCOMM} | xargs ctags -wadtf ${TCOBALT}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${ACOBALT} ${AMIPS} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TCOBALT}
	sort -o ${TCOBALT} ${TCOBALT}

links:
	-for i in ${DCOBALT}; do \
	    cd $$i && rm -f tags; ln -s ../tags tags; done

SUBDIR=	compile include stand

.include <bsd.subdir.mk>
