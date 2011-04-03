# $NetBSD: Makefile,v 1.14 2011/04/03 23:07:19 dyoung Exp $

# Makefile for alpha tags file and boot blocks

TALPHA=	${SYSDIR}/arch/alpha/tags
SALPHA=	${SYSDIR}/arch/alpha/alpha/*.[ch] ${SYSDIR}/arch/alpha/common/*.[ch] \
	${SYSDIR}/arch/alpha/eisa/*.[ch] ${SYSDIR}/arch/alpha/include/*.h \
	${SYSDIR}/arch/alpha/isa/*.[ch] \
	${SYSDIR}/arch/alpha/jensenio/*.[ch] ${SYSDIR}/arch/alpha/mcbus/*.[ch] \
	${SYSDIR}/arch/alpha/pci/*.[ch] ${SYSDIR}/arch/alpha/sableio/*.[ch] \
	${SYSDIR}/arch/alpha/tc/*.[ch] \
	${SYSDIR}/arch/alpha/tlsb/*.[ch]
AALPHA=	${SYSDIR}/arch/alpha/alpha/*.s ${SYSDIR}/arch/alpha/pci/*.s

# Directories in which to place tags links
DALPHA=	a12 common eisa include isa jensenio mcbus pci sableio tc tlsb

.include "../../kern/Make.tags.inc"

tags:
	-rm -f ${TALPHA}
	-echo ${SALPHA} | xargs ctags -wadtf ${TALPHA}
	-${FINDCOMM} | xargs ctags -wadtf ${TALPHA}
	egrep "^ENTRY(.*)|^ALTENTRY(.*)" ${AALPHA} | \
	    ${TOOL_SED} "s;\([^:]*\):\([^(]*\)(\([^, )]*\)\(.*\);\3 \1 /^\2(\3\4$$/;" \
	    >> ${TALPHA}
	sort -o ${TALPHA} ${TALPHA}

links:
	-for i in ${DALPHA}; do \
	    (cd $$i && rm -f tags; ln -s ../tags tags); done


SUBDIR=	compile include stand

.include <bsd.subdir.mk>
