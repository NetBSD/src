#	$NetBSD: Makefile,v 1.3.6.1 2009/01/17 13:27:48 mjf Exp $

# Makefile for amd64 tags file and boot blocks

TAMD64=		${SYSDIR}/arch/amd64/tags
SAMD64=		${SYSDIR}/arch/amd64/amd64/*.[ch] \
		${SYSDIR}/arch/amd64/include/*.h \
		${SYSDIR}/arch/amd64/isa/*.[ch] \
		${SYSDIR}/arch/amd64/pci/*.[ch]
AAMD64=		${SYSDIR}/arch/amd64/amd64/*.s ${SYSDIR}/arch/amd64/isa/*.s

# Directories in which to place tags links
DAMD64=		amd64 isa include pci

.include "../../kern/Make.tags.inc"

tags:
	rm -f ${TAMD64}
	-echo ${SAMD64} ${COMM} | xargs ctags -wadtf ${TAMD64}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AAMD64} | \
	    ${TOOL_SED} -e \
		"s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
		>> ${TAMD64}
	sort -o ${TAMD64} ${TAMD64}

links:
	-for i in ${DAMD64}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include ../x86/include ../xen/include \
	../i386/include ../i386/stand

.include <bsd.subdir.mk>
